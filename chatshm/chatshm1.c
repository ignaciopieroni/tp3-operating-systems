
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
// esperar a que el semáforo valga 0 (slot vacío)
void Z(int semid, int sem){
    struct sembuf op = { .sem_num = sem, .sem_op = 0, .sem_flg = 0 };
    semop(semid, &op, 1);
}

int main(int argc, char *argv[]){
    // SHM: crear/abrir 512 bytes
    int shmid = shmget(KEY, SZ, IPC_CREAT | 0666);
    if (shmid < 0) { perror("shmget"); return 1; }
    char *shmp = shmat(shmid, NULL, 0);
    if (shmp == (void*)-1) { perror("shmat"); return 1; }

    // SEM: crear 2 si no existen; si los creo, inicializo en 0 (vacío)
    int semid = semget(KEY, 2, IPC_CREAT | IPC_EXCL | 0666);
    if (semid >= 0) {
        if (semctl(semid, 0, SETVAL, 0) < 0) perror("semctl 0");
        if (semctl(semid, 1, SETVAL, 0) < 0) perror("semctl 1");
        memset(shmp, 0, SZ);
    } else if (errno == EEXIST) {
        semid = semget(KEY, 2, 0666);
        if (semid < 0) { perror("semget"); return 1; }
    } else { perror("semget"); return 1; }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid > 0){
        // Padre: LEE en primeros 256 bytes (slot 0)
        char msj[257];
        do{
            P(semid, 0);                         // había datos -> consumir (queda vacío)
            memcpy(msj, shmp + 0, SLOT);
            msj[256] = '\0';
            if (msj[0] != '\0')
                printf("Leído de primeros 256bytes: %s\n", msj);
        } while (strncmp(msj, "chau", 4) != 0);

    } else {
        // Hijo: ESCRIBE en segundos 256 bytes (slot 1)
        char msj[257];
        memset(shmp + SLOT, '\0', SLOT);
        do{
            printf("Ingrese mensaje en segundos 256bytes\n");
            if (!fgets(msj, 256, stdin)) break;
            msj[256] = '\0';
            Z(semid, 1);                          // esperar que slot1 esté vacío
            memcpy(shmp + SLOT, msj, SLOT);       // escribir
            V(semid, 1);                          // marcar lleno
        } while (strncmp(msj, "chau", 4) != 0);
        _exit(0);
    }
    return 0;
}
