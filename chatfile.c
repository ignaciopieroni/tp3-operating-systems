// filechat_simple.c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#define MAXLINE 1024

typedef struct {
    char send_path[512];
    char recv_path[512];
    volatile int running;
} Ctx;

static void *sender(void *arg) {
    Ctx *ctx = (Ctx*)arg;
    char line[MAXLINE];

    while (ctx->running && fgets(line, sizeof(line), stdin)) {
        FILE *f = fopen(ctx->send_path, "a");
        if (!f) { perror("fopen send"); break; }
        // timestamp simple
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        fprintf(f, "[%04d-%02d-%02d %02d:%02d:%02d] %s",
                tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
                tm.tm_hour, tm.tm_min, tm.tm_sec, line);
        fflush(f);
        fclose(f);
    }
    return NULL;
}

static void *receiver(void *arg) {
    Ctx *ctx = (Ctx*)arg;

    // Asegurar archivo de recepción existe
    FILE *f = fopen(ctx->recv_path, "a+");
    if (!f) { perror("fopen recv"); return NULL; }
    // Empezar a leer desde el final (solo mensajes nuevos)
    fseek(f, 0, SEEK_END);

    char buf[MAXLINE];
    while (ctx->running) {
        long pos = ftell(f);
        if (fgets(buf, sizeof(buf), f)) {
            // Mostrar lo recibido
            printf("\n<< %s", buf);
            fflush(stdout);
        } else {
            clearerr(f);
            fseek(f, pos, SEEK_SET);
            usleep(200000); // 200 ms
        }
    }
    fclose(f);
    return NULL;
}

static void ensure_dir(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) return;
    if (mkdir(path, 0755) != 0 && errno != EEXIST) {
        perror("mkdir"); exit(1);
    }
}

int main(int argc, char **argv) {
    char me[128]="", peer[128]="", root[256]="/tmp/chat";
    for (int i=1; i<argc; ++i) {
        if (!strcmp(argv[i],"--me") && i+1<argc) strncpy(me, argv[++i], sizeof(me)-1);
        else if (!strcmp(argv[i],"--peer") && i+1<argc) strncpy(peer, argv[++i], sizeof(peer)-1);
        else if (!strcmp(argv[i],"--root") && i+1<argc) strncpy(root, argv[++i], sizeof(root)-1);
    }
    if (!me[0] || !peer[0]) {
        fprintf(stderr,"Uso: %s --me <usuario> --peer <usuario> [--root <dir>]\n", argv[0]);
        return 1;
    }

    ensure_dir(root);

    Ctx ctx = {0};
    snprintf(ctx.send_path, sizeof(ctx.send_path), "%s/%s_to_%s.txt", root, me, peer);
    snprintf(ctx.recv_path, sizeof(ctx.recv_path), "%s/%s_to_%s.txt", root, peer, me);
    ctx.running = 1;

    // Asegurar archivo de envío existe
    FILE *s = fopen(ctx.send_path, "a"); if (s) fclose(s);
    // Asegurar archivo de recepción existe (el hilo receiver también lo crea)
    FILE *r = fopen(ctx.recv_path, "a"); if (r) fclose(r);

    printf("Chat simple por archivos\n");
    printf("Envío → %s\n", ctx.send_path);
    printf("Recibo ← %s\n", ctx.recv_path);
    printf("Escribí y presioná Enter. Ctrl+C para salir.\n");

    pthread_t th_s, th_r;
    pthread_create(&th_s, NULL, sender, &ctx);
    pthread_create(&th_r, NULL, receiver, &ctx);

    // Esperar Ctrl+C
    // Forma simple: leer stdin en sender; acá solo dormimos
    while (1) pause();

    // (No se llega normalmente; manejar SIGINT y setear ctx.running=0)
    ctx.running = 0;
    pthread_join(th_s, NULL);
    pthread_join(th_r, NULL);
    return 0;
}
