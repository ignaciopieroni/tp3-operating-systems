#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/sem.h>

/*
3. Desarrolle un programa de chat usando Memoria Compartida. Repita el mismo esquema de
procesos que en el punto anterior (2 procesos independientes con un hijo cada uno). Puede
resolverse con una única área de memoria con espacio para dos mensajes (digamos, de 256 bytes
cada una, ésta será la limitación del programa, no acepta mensajes de más de 256 caracteres). Se
requerirá de dos semáforos para que, cuando un proceso escriba un mensaje no permita que otro
proceso lo lea hasta que la escritura no se haya completado y cuando un proceso lea, impedir que
otro proceso escriba el mensaje que se está leyendo
*/

int main(int argc, char *argv[]){
	
	int semid = semget(0xa, 0, 0);
	semctl(semid, 0, SETVAL, 1);
	semctl(semid, 1, SETVAL, 0);

	exit(0);
	}
