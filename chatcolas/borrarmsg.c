#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/msg.h>
#include <fcntl.h>

/*
2. Desarrolle un programa de chat usando Cola de Mensajes. Repita el mismo esquema de procesos
que en el punto anterior (2 procesos independientes con un hijo cada uno). Puede resolverse con una
única cola de mensajes en donde se escriben mensajes de distinto tipo (al menos requerirá mensajes
de 2 tipos distintos). Puede resolverlo con un único programa, del cual crea dos procesos
independientes y recibe argumentos por línea de comandos.
*/

int main(int argc, char *argv[]){
	int msgid = msgget(0xa, 0);
	int rc = msgctl(msgid, IPC_RMID, 0);
	if (rc == -1) { printf("La cola de mensajes no existe.\n"); exit(-1); }
	printf("Cola de mensajes borrada existosamente.\n");
	exit(0);
	
	}
