#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

/*
Realizar un chat entre procesos independientes usando fifo’s. Cada proceso independiente recibe
por línea de comandos 2 parámetros: el nombre del fifo de escritura y el nombre del fifo de lectura.
Puede resolver todo el problema programando un único programa (chatfifo.c) y creando dos
procesos a partir de un mismo programa, ejecutándolo desde dos terminales distintas con distintos
argumentos. Establezca un protocolo para salir del chat, por ejemplo, cuando se tipea la palabra
“bye” el proceso deja de enviar y recibir mensajes a través de los fifo’s.
Utilice línea de comandos para crear los fifo’s, ejemplo:
$ mkfifo fifo1
$ mkfifo fifo2
Abrir dos terminales y ejecutar:
$ ./chatfifo fifo1 fifo2
$ ./chatfifo fifo2 fifo1
Luego puede destruir los fifos creados:
$ unlink fifo1
$ unlink fifo2
*/

int main(int argc, char *argv[]){
	
	const char *fifo_out = argv[1];
	const char *fifo_in = argv[2];
	
	int write_first = (strcmp(fifo_out, fifo_in) < 0);

    int fd_r = -1, fd_w = -1;

    if (write_first) {
        // Este abre primero ESCRITURA (bloquea hasta que el otro abra LECTURA de ese FIFO)
        fd_w = open(fifo_out, O_WRONLY);
        if (fd_w < 0) { perror("open escritura"); return 1; }

        // Luego LECTURA (bloquea hasta que el otro abra ESCRITURA del fifo_in)
        fd_r = open(fifo_in, O_RDONLY);
        if (fd_r < 0) { perror("open lectura"); close(fd_w); return 1; }
    } else {
        // Este abre primero LECTURA
        fd_r = open(fifo_in, O_RDONLY);
        if (fd_r < 0) { perror("open lectura"); return 1; }

        // Luego ESCRITURA
        fd_w = open(fifo_out, O_WRONLY);
        if (fd_w < 0) { perror("open escritura"); close(fd_r); return 1; }
    }

	pid_t pid;
	
	pid = fork();
	
	if (pid > 0){
		// Padre
		
		while(1){
			char cmd[256];
			memset(cmd,'\0',256);
			read(fd_r, cmd, 255);
			printf("Leí %s.\n", cmd);
			
			if(strncmp(cmd, "bye", 3) == 0 ) break;
			
			}
		
	} else {
		// Hijo
			
		while(1){
			char cmd[256];
			memset(cmd, '\0', 256);
			printf("Escribir fifoescritura\n");
			gets(cmd);
			write(fd_w, cmd, 255);
			if(strncmp(cmd, "bye", 3) == 0 ) break;
			}

		}
	
	exit(1);
	}