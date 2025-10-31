#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/sem.h>

int main(void){
	int shmid = shmget(0xa, 512, IPC_CREAT | IPC_EXCL | 0600);
	if (shmid == -1) { printf("La memoria compartida ya existe\n"); exit(-1); }
	printf("Memoria compartida creada.\n");
	exit(0);
	}
