/*
 * Ejercicio 5 del TP III - Programa servidor (versión integrada sin diff)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>

typedef struct msg {
    long mtype;       /* message type, must be > 0 */
    char mtext[128];  /* message data */
} msg;

typedef struct cmd {
    int pid;
    int nreg;
    char descr[100];
    int ret;
} cmd;

typedef struct reg {
    unsigned char estado; /* 0=vacío, 1=ocupado, 2=borrado */
    char descr[100];
} reg;

FILE *f = NULL;
char archivo[256];
int msgid = -1;
int nlocks = 0;
pid_t *locks = NULL;

/* FIN ORDENADA: bandera de parada ante Ctrl+C */
static volatile sig_atomic_t stop_requested = 0;
static void on_sigint(int signo) {
    (void)signo;
    stop_requested = 1;
}

/* Prototipos */
void salir(int);
int parsing(char *,cmd *);
int parsing2(char *,cmd *);
int proceso(cmd *);

int main(int argc, char **argv) {
    if ( argc != 2 ) {
        printf("Forma de Uso:\n./mpserver <archivo>\n");
        return 1;
    }

    /* abre/crea la cola 0xA */
    msgid = msgget(0xA,0);
    if ( msgid == -1 ) msgid = msgget(0xA,IPC_CREAT|IPC_EXCL|0600);
    printf("msgid = %d\n",msgid);

    /* instalar handler que NO sale de inmediato: marca bandera */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_sigint;
    sigemptyset(&sa.sa_mask);
    /* sin SA_RESTART: si msgrcv está bloqueado, vuelve con EINTR */
    sigaction(SIGINT, &sa, NULL);

    /* abrir/crear archivo de acceso directo */
    strcpy(archivo,argv[1]);
    f = fopen(archivo,"r+b");
    if ( f == NULL ) {
        reg r;
        memset(&r,0,sizeof(reg));
        f = fopen(archivo,"wb");
        if ( f != NULL ) {
            for(int i=0;i<1000;i++) fwrite(&r,sizeof(reg),1,f);
            printf("Archivo [%s] creado!\n",archivo);
            fclose(f);
            f = fopen(archivo,"r+b");
        } else {
            printf("Error!, imposible crear archivo [%s]\n",archivo);
            return 2;
        }
    }

    /* inicializo vector de locks según tamaño actual del archivo */
    fseek(f,0L,SEEK_END);
    nlocks = ftell(f) / (int)sizeof(reg);
    locks = (pid_t *) malloc(sizeof(pid_t)*nlocks);
    if (!locks) { perror("malloc locks"); salir(1); }
    memset(locks,0,sizeof(pid_t)*nlocks);
    printf("Hay %d registros!\n",nlocks);

    /* bucle del servidor */
    msg mi,mo;
    mi.mtype = 1L;
    cmd c;

    do {
        memset(mi.mtext,0,128);
        memset(&c,0,sizeof(cmd));

        ssize_t rc = msgrcv(msgid,&mi,128,1L,0);
        if (rc < 0) {
            if (errno == EINTR && stop_requested) {
                /* nos pidieron parar y no hay mensaje en curso */
                break;
            }
            /* otros errores: seguir intentando */
            continue;
        }

        printf("Recibi [%s]\n",mi.mtext);

        if ( parsing(mi.mtext,&c) ) {
            printf("Parsing: pid: %d reg: %d descr: [%s]\n",c.pid,c.nreg,c.descr);
            proceso(&c);
            printf("Proceso: pid: %d reg: %d descr: [%s] ret: %d\n",c.pid,c.nreg,c.descr,c.ret);

            /* responder al cliente por mtype = pid */
            mo.mtype = (long) c.pid;
            snprintf(mo.mtext,128,"%d,%d,%s",c.ret,c.nreg,c.descr);
            msgsnd(msgid,&mo,128,0);

            /* si llegó SIGINT durante el procesamiento, salimos tras responder */
            if (stop_requested) break;

        } else {
            printf("Error! en parsing cmd [%s]\n",mi.mtext);
            if ( c.pid > 1 ) {
                mo.mtype = (long) c.pid;
                snprintf(mo.mtext,128,"0,-1,Error parsing");
                msgsnd(msgid,&mo,128,0);
            }
        }
    } while(strncmp(mi.mtext,"0,0,fin",7) != 0 && !stop_requested);

    salir(0);
    return 0;
}

/* salida ordenada */
void salir(int signo) {
    (void)signo;
    printf("Saliendo...\n");
    if ( f ) fclose(f);
    if ( msgid != -1 ) {
        printf("Borro cola de mensajes 0xA...\n");
        msgctl(msgid,IPC_RMID,NULL);
    }
    if ( locks ) free(locks);
    exit(0);
}

/* parsing con sscanf */
int parsing(char *m,cmd *c) {
    int n = sscanf(m,"%d,%d,%99c",&c->pid,&c->nreg,c->descr);
    if ( n == 3 ) {
        if ( c->pid <= 1 ) return 0;
        if ( c->nreg < -1 ) return 0;
        if ( strlen(c->descr) <= 0 ) return 0;
        return 1;
    } else {
        return 0;
    }
}

/* parsing manual (no usado) */
int parsing2(char *m,cmd *c) {
    char tmp[128];
    int i=0,w=0;
    while(i<128 && m[i] && m[i] != ',' && m[i] >= '0' && m[i] <= '9' ) { tmp[w]=m[i];i++;w++; }
    tmp[w]='\0';
    c->pid = atoi(tmp);
    if ( c->pid <= 0 ) return 0;
    w=0;
    if ( m[i] != ',' ) return 0;
    i++;
    while(i<128 && m[i] && m[i] != ',' && m[i] >= '0' && m[i] <= '9' ) { tmp[w]=m[i];i++;w++; }
    tmp[w]='\0';
    c->nreg = atoi(tmp);
    if ( m[i] != ',' ) return 0;
    w=0;
    while(i<128 && m[i] ) { tmp[w]=m[i];i++;w++; }
    tmp[w]='\0';
    strcpy(c->descr,tmp);
    return 1;
}

/* ejecutar operación sobre el archivo */
int proceso(cmd *c) {
    reg r;
    memset(&r,0,sizeof(reg));
    c->ret = 0;

    fseek(f,0L,SEEK_END);
    int n = ftell(f) / (int)sizeof(reg);

    if ( c->nreg == -1 ) {
        /* alta al final */
        fseek(f,0L,SEEK_END);
        c->nreg = ftell(f) / (int)sizeof(reg);
        r.estado = 1;
        strncpy(r.descr,c->descr,100);
        fwrite(&r,sizeof(reg),1,f);
        fflush(f);
        /* ampliar vector de locks */
        pid_t *tmp = (pid_t *)realloc(locks,sizeof(pid_t)*(nlocks+1));
        if (tmp) {
            locks = tmp;
            memset(locks+nlocks,0,sizeof(pid_t));
            nlocks++;
        }
        c->ret = 1;

    } else if ( strncmp(c->descr,"borrar",6) == 0 ) {
        /* baja lógica */
        if ( c->nreg >= 0 && c->nreg < n ) {
            if ( *(locks+c->nreg) && *(locks+c->nreg) != c->pid ) {
                c->ret = 0;
                snprintf(c->descr,100,"Error,reg %d lockeado por PID %d",c->nreg,*(locks+c->nreg));
            } else {
                fseek(f,c->nreg*sizeof(reg),SEEK_SET);
                fread(&r,sizeof(reg),1,f);
                switch(r.estado) {
                    case 0:
                        c->ret = 0;
                        snprintf(c->descr,100,"Error,reg %d esta vacio",c->nreg);
                        break;
                    case 1:
                        fseek(f,c->nreg*sizeof(reg),SEEK_SET);
                        r.estado = 2;
                        fwrite(&r,sizeof(reg),1,f);
                        fflush(f);
                        c->ret = 1;
                        break;
                    case 2:
                        c->ret = 0;
                        snprintf(c->descr,100,"Error,reg %d esta borrado",c->nreg);
                        break;
                    default:
                        c->ret = 0;
                        snprintf(c->descr,100,"Error,reg %d estado %d erroneo",c->nreg,r.estado);
                }
            }
        } else {
            c->ret = 0;
            snprintf(c->descr,100,"Error,reg %d invalido (%d-%d)",c->nreg,0,n);
        }

    } else if ( strncmp(c->descr,"leer",4) == 0 ) {
        /* lectura */
        if ( c->nreg >= 0 && c->nreg < n ) {
            if ( *(locks+c->nreg) && *(locks+c->nreg) != c->pid ) {
                c->ret = 0;
                snprintf(c->descr,100,"Error,reg %d lockeado por PID %d",c->nreg,*(locks+c->nreg));
            } else {
                fseek(f,c->nreg*sizeof(reg),SEEK_SET);
                fread(&r,sizeof(reg),1,f);
                switch(r.estado) {
                    case 0:
                        c->ret = 0;
                        snprintf(c->descr,100,"Error,reg %d esta vacio",c->nreg);
                        break;
                    case 1:
                        strncpy(c->descr,r.descr,100);
                        c->ret = 1;
                        break;
                    case 2:
                        c->ret = 0;
                        snprintf(c->descr,100,"Error,reg %d esta borrado",c->nreg);
                        break;
                    default:
                        c->ret = 0;
                        snprintf(c->descr,100,"Error,reg %d estado %d erroneo",c->nreg,r.estado);
                }
            }
        } else {
            c->ret = 0;
            snprintf(c->descr,100,"Error,reg %d invalido (%d-%d)",c->nreg,0,n);
        }

    } else if (strncmp(c->descr,"desborrar",9) == 0) {
        /* reactivar */
        if ( c->nreg >= 0 && c->nreg < n ) {
            if ( *(locks+c->nreg) && *(locks+c->nreg) != c->pid ) {
                c->ret = 0;
                snprintf(c->descr,100,"Error,reg %d lockeado por PID %d",c->nreg,*(locks+c->nreg));
            } else {
                fseek(f,c->nreg*sizeof(reg),SEEK_SET);
                fread(&r,sizeof(reg),1,f);
                switch(r.estado) {
                    case 0:
                        c->ret = 0;
                        snprintf(c->descr,100,"Error,reg %d esta vacio",c->nreg);
                        break;
                    case 1:
                        c->ret = 0;
                        snprintf(c->descr,100,"Error,reg %d esta ocupado",c->nreg);
                        break;
                    case 2:
                        fseek(f,c->nreg*sizeof(reg),SEEK_SET);
                        r.estado = 1;
                        fwrite(&r,sizeof(reg),1,f);
                        fflush(f);
                        c->ret = 1;
                        strncpy(c->descr,r.descr,100);
                        break;
                    default:
                        c->ret = 0;
                        snprintf(c->descr,100,"Error,reg %d estado %d erroneo",c->nreg,r.estado);
                }
            }
        } else {
            c->ret = 0;
            snprintf(c->descr,100,"Error,reg %d invalido (%d-%d)",c->nreg,0,n);
        }

    } else if (strncmp(c->descr,"lock",4) == 0) {
        /* lock de registro ocupado */
        if ( c->nreg >= 0 && c->nreg < n ) {
            if ( *(locks+c->nreg) ) {
                c->ret = 0;
                snprintf(c->descr,100,"Error,reg %d ya esta locked por %d",c->nreg,*(locks+c->nreg));
            } else {
                fseek(f,c->nreg*sizeof(reg),SEEK_SET);
                fread(&r,sizeof(reg),1,f);
                switch(r.estado) {
                    case 0:
                        c->ret = 0;
                        snprintf(c->descr,100,"Error,reg %d esta vacio",c->nreg);
                        break;
                    case 1:
                        strncpy(c->descr,r.descr,100);
                        c->ret = 1;
                        *(locks+c->nreg) = c->pid;
                        break;
                    case 2:
                        c->ret = 0;
                        snprintf(c->descr,100,"Error,reg %d esta borrado",c->nreg);
                        break;
                    default:
                        c->ret = 0;
                        snprintf(c->descr,100,"Error,reg %d estado %d erroneo",c->nreg,r.estado);
                }
            }
        } else {
            c->ret = 0;
            snprintf(c->descr,100,"Error,reg %d invalido (%d-%d)",c->nreg,0,n-1);
        }

    } else if (strncmp(c->descr,"unlock",6) == 0) {
        /* unlock si lo puse yo */
        if ( c->nreg >= 0 && c->nreg < n ) {
            if ( *(locks+c->nreg) ) {
                if ( c->pid == *(locks+c->nreg) ) {
                    fseek(f,c->nreg*sizeof(reg),SEEK_SET);
                    fread(&r,sizeof(reg),1,f);
                    switch(r.estado) {
                        case 0:
                            c->ret = 0;
                            snprintf(c->descr,100,"Error,reg %d esta vacio",c->nreg);
                            break;
                        case 1:
                            strncpy(c->descr,r.descr,100);
                            c->ret = 1;
                            memset(locks+c->nreg,0,sizeof(pid_t));
                            break;
                        case 2:
                            c->ret = 0;
                            snprintf(c->descr,100,"Error,reg %d esta borrado",c->nreg);
                            break;
                        default:
                            c->ret = 0;
                            snprintf(c->descr,100,"Error,reg %d estado %d erroneo",c->nreg,r.estado);
                    }
                } else {
                    c->ret = 0;
                    snprintf(c->descr,100,"Error,reg %d esta lockeado por %d",c->nreg,*(locks+c->nreg));
                }
            } else {
                c->ret = 0;
                snprintf(c->descr,100,"Error,reg %d no esta lockeado por nadie!",c->nreg);
            }
        } else {
            c->ret = 0;
            snprintf(c->descr,100,"Error,reg %d invalido (%d-%d)",c->nreg,0,n-1);
        }

    } else {
        /* alta/modificación directa de descripción */
        if ( c->nreg >= 0 && c->nreg < n ) {
            if ( *(locks+c->nreg) && *(locks+c->nreg) != c->pid ) {
                c->ret = 0;
                snprintf(c->descr,100,"Error,reg %d lockeado por PID %d",c->nreg,*(locks+c->nreg));
            } else {
                fseek(f,c->nreg*sizeof(reg),SEEK_SET);
                fread(&r,sizeof(reg),1,f);
                switch(r.estado) {
                    case 0:
                    case 2:
                        r.estado = 1;
                        /* fallthrough */
                    case 1:
                        strncpy(r.descr,c->descr,100);
                        fseek(f,c->nreg*sizeof(reg),SEEK_SET);
                        fwrite(&r,sizeof(reg),1,f);
                        fflush(f);
                        c->ret = 1;
                        break;
                    default:
                        c->ret = 0;
                        snprintf(c->descr,100,"Error,reg %d estado %d erroneo",c->nreg,r.estado);
                }
            }
        } else {
            c->ret = 0;
            snprintf(c->descr,100,"Error,reg %d invalido (%d-%d)",c->nreg,0,n);
        }
    }
    return c->ret;
}
