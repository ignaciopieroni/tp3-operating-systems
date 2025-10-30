#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/msg.h>
#include <fcntl.h>

int main(void){
	
	int msgid = msgget(0xa, IPC_CREAT | IPC_EXCL | 0600);
	if (msgid == -1) { printf("La cola de mensajes ya existe.\n"); exit(-1); }
	printf("Cola de msg creada...\n");
	exit(0);
	
}
