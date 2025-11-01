 /*
 * Ejercicio 5 del TP III 
 * Programa cliente, observe que es independiente del archivo, no
 * utiliza ninguna funcion de archivo ni estructura de registro de
 * archivo.
 * Solo utiliza estructura para comunicacion a traves de cola de mensaje
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

typedef struct msg {
	long mtype;       /* message type, must be > 0 */
	char mtext[128];  /* message data */
} msg;

// ingreso por teclado
void ingresar(char *to,int n);

int main(int argc, char **argv) {
	int msgid = msgget(0xA,0);
	printf("main(): msgid = %d\n",msgid);
	if ( msgid < 0 ) {
		printf("main(): Error, cola de mensajes invalida!\n");
		return 1;
	}
	msg mi,mo;
	int opcion=0,nreg=0,i=0,n=0,r=0;
	char linea[100],linea2[100];
	
	// proceso lector
	pid_t pid = getpid();
	mi.mtype = 1L;
	mo.mtype = pid;
	do {
		memset(mi.mtext,0,128);
		memset(mo.mtext,0,128);
		printf("\t\t\tMENU  (pid=%d)\n",pid);
		printf("\t 1. Agregar   Registro\n");
		printf("\t 2. Borrar    Registro\n");
		printf("\t 3. Consultar Registro\n");
		printf("\t 4. Modificar Registro\n");
		printf("\t 5. Listar    Registros\n");
		printf("\t 6. DesBorrar Registro\n");
		printf("\t 7. lock\n");
		printf("\t 8. unlock\n");
		printf("\t 9. Ejecutar Transaccion\n");
		printf("\t10. Salir\n");
		printf("Opcion:");
		ingresar(linea,100);
		opcion=atoi(linea);
		switch(opcion) {
			case 1: // agregar
				printf("Descripcion:");ingresar(linea,100);
				snprintf(mi.mtext,128,"%d,%d,%s",pid,-1,linea);
				msgsnd(msgid,&mi,128,0);
				msgrcv(msgid,&mo,128,mo.mtype,0);
				printf("ret,nreg,descr\n%s\n",mo.mtext);
				printf("Presione Intro para continuar");ingresar(linea,2);
				break;
			case 2: // borrar
				printf("Registro:");ingresar(linea,10);
				nreg=atoi(linea);
				if ( nreg >= 0 ) {
					snprintf(mi.mtext,128,"%d,%d,borrar",pid,nreg);
					msgsnd(msgid,&mi,128,0);
					msgrcv(msgid,&mo,128,mo.mtype,0);
					printf("ret,nreg,descr\n%s\n",mo.mtext);
				} else printf("%d es un registro invalido!\n",nreg);
				printf("Presione Intro para continuar");ingresar(linea,2);
				break;
			case 3: // consultar
				printf("Registro:");ingresar(linea,10);
				nreg=atoi(linea);
				if ( nreg >= 0 ) {
					snprintf(mi.mtext,128,"%d,%d,leer",pid,nreg);
					msgsnd(msgid,&mi,128,0);
					msgrcv(msgid,&mo,128,mo.mtype,0);
					printf("ret,nreg,descr\n%s\n",mo.mtext);
				} else printf("%d es un registro invalido!\n",nreg);
				printf("Presione Intro para continuar");ingresar(linea,2);
				break;
			case 4: // modificar
				printf("Registro:");ingresar(linea,10);
				nreg=atoi(linea);
				if ( nreg >= 0 ) {
					printf("Descripcion:");ingresar(linea,100);
					snprintf(mi.mtext,128,"%d,%d,%s",pid,nreg,linea);
					msgsnd(msgid,&mi,128,0);
					msgrcv(msgid,&mo,128,mo.mtype,0);
					printf("ret,nreg,descr\n%s\n",mo.mtext);
				} else printf("%d es un registro invalido!\n",nreg);
				printf("Presione Intro para continuar");ingresar(linea,2);
				break;
			case 5: // listar
				nreg=0;
				printf("Reg#\tLock\tDescripcion\n");
				do {
					memset(mi.mtext,0,128);
					memset(mo.mtext,0,128);
					snprintf(mi.mtext,128,"%d,%d,leer",pid,nreg);
					msgsnd(msgid,&mi,128,0);
					msgrcv(msgid,&mo,128,mo.mtype,0);
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
			case 6:
				printf("Registro:");ingresar(linea,10);
				nreg=atoi(linea);
				if ( nreg >= 0 ) {
					snprintf(mi.mtext,128,"%d,%d,desborrar",pid,nreg);
					msgsnd(msgid,&mi,128,0);
					msgrcv(msgid,&mo,128,mo.mtype,0);
					printf("ret,nreg,descr\n%s\n",mo.mtext);
				} else printf("%d es un registro invalido!\n",nreg);
				printf("Presione Intro para continuar");ingresar(linea,2);
				break;
			case 7: // lock
				printf("Registro:");ingresar(linea,10);
				nreg=atoi(linea);
				if ( nreg >= 0 ) {
					snprintf(mi.mtext,128,"%d,%d,lock",pid,nreg);
					msgsnd(msgid,&mi,128,0);
					msgrcv(msgid,&mo,128,mo.mtype,0);
					printf("ret,nreg,descr\n%s\n",mo.mtext);
				} else printf("%d es un registro invalido!\n",nreg);
				printf("Presione Intro para continuar");ingresar(linea,2);
				break;
			case 8: // unlock
				printf("Registro:");ingresar(linea,10);
				nreg=atoi(linea);
				if ( nreg >= 0 ) {
					snprintf(mi.mtext,128,"%d,%d,unlock",pid,nreg);
					msgsnd(msgid,&mi,128,0);
					msgrcv(msgid,&mo,128,mo.mtype,0);
					printf("ret,nreg,descr\n%s\n",mo.mtext);
				} else printf("%d es un registro invalido!\n",nreg);
				printf("Presione Intro para continuar");ingresar(linea,2);
				break;
			case 9: // transaccion, envio varios comandos, puedo provocar abrazo mortal
				do {
					memset(mi.mtext,0,128);
					memset(mo.mtext,0,128);
					printf("Descripcion/Comando {leer,borrar,desborrar,lock,unlock} (salir=fin):");ingresar(linea,100);
					if ( strncmp(linea,"salir",5) == 0 ) continue;
					printf("Registro:");ingresar(linea2,10);
					nreg=atoi(linea2);
					if ( nreg >= -1 ) {
						snprintf(mi.mtext,128,"%d,%d,%s",pid,nreg,linea);
						do {
							msgsnd(msgid,&mi,128,0);
							msgrcv(msgid,&mo,128,mo.mtype,0);
							printf("ret,nreg,descr\n%s\n",mo.mtext);
							if ( mo.mtext[0] == '0' ) sleep(2);
						} while(mo.mtext[0] == '0');
					} else printf("%d es un registro invalido!\n",nreg);
				} while(strncmp(linea,"salir",5) != 0);
				break;
			case 10:
				break;
			default:
				printf("Opcion %d invalida!\n",opcion);
		}
	} while(opcion != 10);

	return 0;
}

// ingreso por teclado
void ingresar(char *to,int n) {
	memset(to,0,n);
	fgets(to,n,stdin);
	to[strlen(to)-1] = '\0';
}
