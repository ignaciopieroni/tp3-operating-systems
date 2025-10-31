
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <errno.h>

#define KEY 0xA
#define SZ  512
#define SLOT 256

void P(int semid, int sem){
    struct sembuf op = { .sem_num = sem, .sem_op = -1, .sem_flg = 0 };
    semop(semid, &op, 1);
}
void V(int semid, int sem){
    struct sembuf op = { .sem_num = sem, .sem_op = +1, .sem_flg = 0 };
    semop(semid, &op, 1);
}
void Z(int semid, int sem){
    struct sembuf op = { .sem_num = sem, .sem_op = 0, .sem_flg = 0 };
    semop(semid, &op, 1);
}

int main(int argc, char *argv[]){
    int shmid = shmget(KEY, SZ, IPC_CREAT | 0666);
    if (shmid < 0) { perror("shmget"); return 1; }
    char *shmp = shmat(shmid, NULL, 0);
    if (shmp == (void*)-1) { perror("shmat"); return 1; }

    int semid = semget(KEY, 2, 0666);
    if (semid < 0) { perror("semget"); return 1; }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid > 0){
        // Padre: LEE en segundos 256 bytes (slot 1)
        char msj[257];
        do{
            P(semid, 1);                          // había datos -> consumir (queda vacío)
            memcpy(msj, shmp + SLOT, SLOT);
            msj[256] = '\0';
            if (msj[0] != '\0')
                printf("Leído de segundos 256bytes: %s\n", msj);
        } while (strncmp(msj, "chau", 4) != 0);

    } else {
        // Hijo: ESCRIBE en primeros 256 bytes (slot 0)
        char msj[257];
        memset(shmp + 0, '\0', SLOT);
        do{
            printf("Ingrese mensaje en primeros 256bytes\n");
            if (!fgets(msj, 256, stdin)) break;
            msj[256] = '\0';
            Z(semid, 0);                          // esperar que slot0 esté vacío
            memcpy(shmp + 0, msj, SLOT);          // escribir
            V(semid, 0);                          // marcar lleno
        } while (strncmp(msj, "chau", 4) != 0);
        _exit(0);
    }
    return 0;
}
