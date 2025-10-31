#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/sem.h>

int main(void){
	int shmid = shmget(0xa, 0, 0);
	shmctl(shmid, IPC_RMID, 0);
	exit(0);
	}
