#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/sem.h>

int main(void){
	
	int semid = semget(0xa, 2, IPC_CREAT | IPC_EXCL | 0600);
	if (semid == -1) { printf("El conjunto de semáforos ya existe.\n"); exit(-1); };
	printf("Conjunto de semáforos creado.\n");
	exit(0);
	}
