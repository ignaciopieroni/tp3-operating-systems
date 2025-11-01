/*
 * Ejercicio 5/6 del TP III - Cliente con modo transaccional por lock/unlock
 * (mantiene el menú original; opción 9 ahora bloquea/unbloquea automáticamente)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>

typedef struct msg {
    long mtype;
    char mtext[128];
} msg;

static int msgid = -1;
static pid_t mypid = 0;
/* para desbloquear si el usuario cierra con Ctrl+C teniendo un lock */
static int txn_in_progress = 0;
static int txn_reg = -1;

/* helpers */
void ingresar(char *to,int n) {
    memset(to,0,n);
    if (fgets(to,n,stdin)==NULL) { to[0]='\0'; return; }
    size_t L=strlen(to);
    if (L>0 && to[L-1]=='\n') to[L-1]='\0';
}

static void send_and_recv(msg *mi, msg *mo) {
    msgsnd(msgid, mi, 128, 0);
    msgrcv(msgid, mo, 128, mo->mtype, 0);
}

/* si hay una transacción con lock tomado, liberar antes de salir */
static void on_sigint(int signo) {
    (void)signo;
    if (txn_in_progress && txn_reg >= 0) {
        msg mi, mo;
        mi.mtype = 1L;
        mo.mtype = mypid;
        memset(mi.mtext,0,128);
        snprintf(mi.mtext,128,"%d,%d,unlock",mypid,txn_reg);
        /* intento único (no bloqueante adicional) */
        msgsnd(msgid,&mi,128,0);
    }
    _exit(130); /* salida por SIGINT */
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    /* abrir cola existente */
    msgid = msgget(0xA,0);
    if ( msgid < 0 ) {
        printf("main(): Error, cola de mensajes invalida!\n");
        return 1;
    }
    mypid = getpid();

    /* instalar handler para liberar lock si hay Ctrl+C en medio de la tx */
    struct sigaction sa;
    memset(&sa,0,sizeof(sa));
    sa.sa_handler = on_sigint;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT,&sa,NULL);

    msg mi,mo;
    mi.mtype = 1L;
    mo.mtype = mypid;

    int opcion=0,nreg=0,i=0,n=0,r=0;
    char linea[100],linea2[100];

    do {
        memset(mi.mtext,0,128);
        memset(mo.mtext,0,128);
        printf("\n\t\t\tMENU  (pid=%d)\n",mypid);
        printf("\t 1. Agregar   Registro\n");
        printf("\t 2. Borrar    Registro\n");
        printf("\t 3. Consultar Registro\n");
        printf("\t 4. Modificar Registro\n");
        printf("\t 5. Listar    Registros\n");
        printf("\t 6. DesBorrar Registro\n");
        printf("\t 7. lock\n");
        printf("\t 8. unlock\n");
        printf("\t 9. Ejecutar Transaccion (lock->operaciones->unlock)\n");
        printf("\t10. Salir\n");
        printf("Opcion:");
        ingresar(linea,100);
        opcion=atoi(linea);

        switch(opcion) {
        case 1: // agregar (-1,descr)
            printf("Descripcion:"); ingresar(linea,100);
            snprintf(mi.mtext,128,"%d,%d,%s",mypid,-1,linea);
            send_and_recv(&mi,&mo);
            printf("ret,nreg,descr\n%s\n",mo.mtext);
            printf("Intro para continuar..."); ingresar(linea,2);
            break;

        case 2: // borrar
            printf("Registro:"); ingresar(linea,10);
            nreg=atoi(linea);
            if ( nreg >= 0 ) {
                snprintf(mi.mtext,128,"%d,%d,borrar",mypid,nreg);
                send_and_recv(&mi,&mo);
                printf("ret,nreg,descr\n%s\n",mo.mtext);
            } else printf("%d es un registro invalido!\n",nreg);
            printf("Intro para continuar..."); ingresar(linea,2);
            break;

        case 3: // leer
            printf("Registro:"); ingresar(linea,10);
            nreg=atoi(linea);
            if ( nreg >= 0 ) {
                snprintf(mi.mtext,128,"%d,%d,leer",mypid,nreg);
                send_and_recv(&mi,&mo);
                printf("ret,nreg,descr\n%s\n",mo.mtext);
            } else printf("%d es un registro invalido!\n",nreg);
            printf("Intro para continuar..."); ingresar(linea,2);
            break;

        case 4: // modificar
            printf("Registro:"); ingresar(linea,10);
            nreg=atoi(linea);
            if ( nreg >= 0 ) {
                printf("Descripcion:"); ingresar(linea,100);
                snprintf(mi.mtext,128,"%d,%d,%s",mypid,nreg,linea);
                send_and_recv(&mi,&mo);
                printf("ret,nreg,descr\n%s\n",mo.mtext);
            } else printf("%d es un registro invalido!\n",nreg);
            printf("Intro para continuar..."); ingresar(linea,2);
            break;

        case 5: // listar (igual que antes)
            nreg=0;
            printf("Reg#\tLock\tDescripcion\n");
            do {
                memset(mi.mtext,0,128);
                memset(mo.mtext,0,128);
                snprintf(mi.mtext,128,"%d,%d,leer",mypid,nreg);
                send_and_recv(&mi,&mo);
                memset(mi.mtext,0,128);
                n = sscanf(mo.mtext,"%d,%d,%99c",&r,&i,mi.mtext);
                if ( n == 3 ) {
                    if ( r == 1 ) printf("%4d\t\t%s\n",i,mi.mtext);
                    if ( r == 0 ) if ( strstr(mi.mtext,"lock") ) printf("%4d\tX\t%s\n",nreg,mo.mtext);
                } else {
                    printf("%4d\t\tError en retorno\n",nreg);
                }
                nreg++;
            } while(strstr(mo.mtext," invalido") == NULL);
            break;

        case 6: // desborrar
            printf("Registro:"); ingresar(linea,10);
            nreg=atoi(linea);
            if ( nreg >= 0 ) {
                snprintf(mi.mtext,128,"%d,%d,desborrar",mypid,nreg);
                send_and_recv(&mi,&mo);
                printf("ret,nreg,descr\n%s\n",mo.mtext);
            } else printf("%d es un registro invalido!\n",nreg);
            printf("Intro para continuar..."); ingresar(linea,2);
            break;

        case 7: // lock
            printf("Registro:"); ingresar(linea,10);
            nreg=atoi(linea);
            if ( nreg >= 0 ) {
                snprintf(mi.mtext,128,"%d,%d,lock",mypid,nreg);
                send_and_recv(&mi,&mo);
                printf("ret,nreg,descr\n%s\n",mo.mtext);
            } else printf("%d es un registro invalido!\n",nreg);
            printf("Intro para continuar..."); ingresar(linea,2);
            break;

        case 8: // unlock
            printf("Registro:"); ingresar(linea,10);
            nreg=atoi(linea);
            if ( nreg >= 0 ) {
                snprintf(mi.mtext,128,"%d,%d,unlock",mypid,nreg);
                send_and_recv(&mi,&mo);
                printf("ret,nreg,descr\n%s\n",mo.mtext);
            } else printf("%d es un registro invalido!\n",nreg);
            printf("Intro para continuar..."); ingresar(linea,2);
            break;

        case 9: { // TRANSACCIÓN: lock -> {leer|modificar|borrar|desborrar}* -> unlock
            printf("Registro a transaccionar: "); ingresar(linea,10);
            int regtxn = atoi(linea);
            if ( regtxn < 0 ) { printf("Registro invalido!\n"); break; }

            /* intento lock */
            snprintf(mi.mtext,128,"%d,%d,lock",mypid,regtxn);
            send_and_recv(&mi,&mo);
            if (mo.mtext[0] != '1') {
                printf("No se pudo lockear: %s\n",mo.mtext);
                break;
            }
            txn_in_progress = 1;
            txn_reg = regtxn;
            printf("** TXN iniciada sobre reg %d (lock tomado). Comandos: leer | set | borrar | desborrar | fin\n", regtxn);

            for(;;) {
                printf("txn> "); ingresar(linea,100);
                if (strncmp(linea,"fin",3)==0) break;

                if (strncmp(linea,"leer",4)==0) {
                    snprintf(mi.mtext,128,"%d,%d,leer",mypid,regtxn);
                    send_and_recv(&mi,&mo);
                    printf("%s\n",mo.mtext);
                } else if (strncmp(linea,"set",3)==0) {
                    /* set <descripcion...> */
                    char *p = linea+3;
                    while(*p==' ') p++;
                    if (*p=='\0') { printf("Uso: set <descripcion>\n"); continue; }
                    snprintf(mi.mtext,128,"%d,%d,%s",mypid,regtxn,p);
                    send_and_recv(&mi,&mo);
                    printf("%s\n",mo.mtext);
                } else if (strncmp(linea,"borrar",6)==0) {
                    snprintf(mi.mtext,128,"%d,%d,borrar",mypid,regtxn);
                    send_and_recv(&mi,&mo);
                    printf("%s\n",mo.mtext);
                } else if (strncmp(linea,"desborrar",9)==0) {
                    snprintf(mi.mtext,128,"%d,%d,desborrar",mypid,regtxn);
                    send_and_recv(&mi,&mo);
                    printf("%s\n",mo.mtext);
                } else {
                    printf("Comandos válidos: leer | set <texto> | borrar | desborrar | fin\n");
                }
            }

            /* siempre unlock al salir de la txn */
            snprintf(mi.mtext,128,"%d,%d,unlock",mypid,regtxn);
            send_and_recv(&mi,&mo);
            printf("Unlock: %s\n",mo.mtext);
            txn_in_progress = 0;
            txn_reg = -1;
            } break;

        case 10:
            /* si el usuario sale con un lock activo (muy raro aquí), lo libero por las dudas */
            if (txn_in_progress && txn_reg >= 0) {
                snprintf(mi.mtext,128,"%d,%d,unlock",mypid,txn_reg);
                send_and_recv(&mi,&mo);
            }
            break;

        default:
            printf("Opcion %d invalida!\n",opcion);
        }

    } while(opcion != 10);

    return 0;
}
