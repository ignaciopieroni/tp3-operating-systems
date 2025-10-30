#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#define MTEXT_MAX 512


/*
2. Desarrolle un programa de chat usando Cola de Mensajes. Repita el mismo esquema de procesos
que en el punto anterior (2 procesos independientes con un hijo cada uno). Puede resolverse con una
única cola de mensajes en donde se escriben mensajes de distinto tipo (al menos requerirá mensajes
de 2 tipos distintos). Puede resolverlo con un único programa, del cual crea dos procesos
independientes y recibe argumentos por línea de comandos.
*/

// chatmsg.c — Chat bidireccional con una cola de mensajes SysV y dos tipos
// Terminal A: ./chatmsg 1 2
// Terminal B: ./chatmsg 2 1
// Protocolo de salida: escribir "bye" (con o sin \n)

struct msg {
    long mtype;              // >0 obligatorio
    char texto[MTEXT_MAX];   // payload
};

int main(int argc, char *argv[]) {
    
    int msgid = msgget(0xa, 0);
    
    int r_type = atol(argv[1]);
    int w_type = atol(argv[2]);
    
    pid_t pid = fork();
    
    if (pid > 0){
		// Padre lee su tipo
		struct msg mensaje;
		do{ 
			memset(&mensaje, 0, sizeof(struct msg));
			msgrcv(msgid, &mensaje, 512, r_type, 0);  
			printf("Mensaje recibido: %s\n", mensaje.texto);
			} while(strncmp(mensaje.texto, "bye", 3));
		
		} else {
			// Hijo escribe su tipo
			struct msg mensaje;
			do{
				printf("Ingrese mensaje tipo %d.\n", w_type);
				fgets(mensaje.texto, 512, stdin);
				mensaje.mtype = w_type;
				msgsnd(msgid, &mensaje, 512, 0);
				
				}while(strncmp(mensaje.texto, "bye", 3));
			}
    
    exit(0);
}
