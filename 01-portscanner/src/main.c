#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "scanner.h"
#include <ctype.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_CONCURRENCY 32
#define MAX_PORT 65535
#define MIN_PORT 1

int parse_ip_range(const char *input, char *base, int *start, int *end) {
    const char *last_dot = strrchr(input, '.');
    const char *dash = strchr(input, '-');
    if (!last_dot || !dash || dash < last_dot) return 0;
    strncpy(base, input, last_dot - input + 1);
    base[last_dot - input + 1] = '\0';
    *start = atoi(last_dot + 1);
    *end = atoi(dash + 1);
    return (*start > 0 && *end >= *start && *end <= 254);
}

int resolve_domain(const char *host, char *ipstr, size_t ipstrlen) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    if (getaddrinfo(host, NULL, &hints, &res) != 0) return 0;
    struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
    inet_ntop(AF_INET, &addr->sin_addr, ipstr, ipstrlen);
    freeaddrinfo(res);
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc >= 3) {
        char base[32], ipstr[INET_ADDRSTRLEN];
        int start, end;
        const char *ip = argv[1];

        if (parse_ip_range(ip, base, &start, &end)) {
            for (int i = start; i <= end; i++) {
                snprintf(ipstr, sizeof(ipstr), "%s%d", base, i);
                printf("\nEscaneando host %s...\n", ipstr);
                char *fake_argv[6];
                fake_argv[0] = argv[0];
                fake_argv[1] = ipstr;
                int k = 2;
                for (int j = 2; j < argc; j++) fake_argv[k++] = argv[j];
                main(argc, fake_argv);
            }
            return 0;
        } else if (!isdigit(ip[0])) {
            if (resolve_domain(ip, ipstr, sizeof(ipstr))) {
                printf("Dominio %s resuelto a %s\n", ip, ipstr);
                char *fake_argv[20];
                fake_argv[0] = argv[0];
                fake_argv[1] = ipstr;
                int k = 2;
                for (int j = 2; j < argc; j++) fake_argv[k++] = argv[j];
                main(argc, fake_argv);
                return 0;
            } else {
                printf("No se pudo resolver el dominio %s\n", ip);
                return 1;
            }
        }

        int puerto_min = MIN_PORT;
        int puerto_max = MAX_PORT;
        int concurrency = MAX_CONCURRENCY;
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-r") == 0) {
                // Captura el rango de puertos
                if (i + 2 < argc && isdigit(argv[i + 1][0]) && isdigit(argv[i + 2][0])) {
                    puerto_min = atoi(argv[i + 1]);
                    puerto_max = atoi(argv[i + 2]);
                    if (puerto_min < MIN_PORT || puerto_max > MAX_PORT || puerto_min > puerto_max) {
                        printf("Error: Rango de puertos inválido. Debe estar entre %d y %d.\n", MIN_PORT, MAX_PORT);
                        return 1;
                    }
                    i += 2;
                } else if (i + 1 < argc && isdigit(argv[i + 1][0])) {
                    puerto_min = MIN_PORT;
                    puerto_max = atoi(argv[i + 1]);
                    if (puerto_min < MIN_PORT || puerto_min > MAX_PORT) {
                        printf("Error: Puerto inválido. Debe estar entre %d y %d.\n", MIN_PORT, MAX_PORT);
                        return 1;
                    }
                    i += 1; 
                } else {
                    printf("Error: Debe proporcionar un rango válido después de -r.\n");
                    return 1;
                }
            } else if (strcmp(argv[i], "-c") == 0) {
                // Captura la concurrencia
                if (i + 1 < argc && isdigit(argv[i + 1][0])) {
                    concurrency = atoi(argv[i + 1]);
                    if (concurrency < 1) concurrency = 1;
                    if (concurrency > 256) concurrency = 256;
                    i += 1;
                } else {
                    printf("Error: Debe proporcionar un valor válido para la concurrencia después de -c.\n");
                    return 1;
                }
                i += 1;
            }
        }

        if (puerto_max > 0) {
            printf("Escaneando puertos %d a %d en %s (concurrencia: %d)...\n", puerto_min, puerto_max, ip, concurrency);
            int running = 0;
            for (int puerto = puerto_min; puerto <= puerto_max; puerto++) {
                pid_t pid = fork();
                if (pid == 0) {
                    // Proceso hijo: escanea el puerto y termina
                    struct timespec start, end;
                    clock_gettime(CLOCK_MONOTONIC, &start);
                    int res = scan_port(ip, puerto);
                    clock_gettime(CLOCK_MONOTONIC, &end);
                    double ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                                (end.tv_nsec - start.tv_nsec) / 1e6;
                    if (res == 0) {
                        printf("Puerto %5d [ABIERTO]  (%.2f ms)\n", puerto, ms);
                    } else {
                        printf("Puerto %5d [cerrado]  (%.2f ms)\n", puerto, ms);
                    }
                    exit(0);
                } else if (pid > 0) {
                    running++;
                    if (running >= concurrency) {
                        wait(NULL);
                        running--;
                    }
                } else {
                    perror("fork");
                }
            }
            while (running-- > 0) wait(NULL);
            return 0;
        }


        if (argc == 3) {
            int puerto = atoi(argv[2]);
            if (scan_port(ip, puerto) == 0) {
                printf("Puerto %d abierto en %s\n", puerto, ip);
            } else {
                printf("Puerto %d cerrado en %s\n", puerto, ip);
            }
            return 0;
        }
    }

    // Ayuda/uso
    printf("Uso:\n");
    printf("  %s <IP> <PUERTO>\n", argv[0]);
    printf("  %s <IP> -r <PUERTO_MAX>\n", argv[0]);
    printf("  %s <IP> -r <PUERTO_MIN> <PUERTO_MAX>\n", argv[0]);
    printf("  %s <IP> -r <PUERTO_MIN> <PUERTO_MAX> -c <CONCURRENCIA>\n", argv[0]);
    printf("Ejemplo para escanear puertos 1-1024 con concurrencia 16:\n");
    printf("  %s 127.0.0.1 -r 1 1024 -c 16\n", argv[0]);
    return 1;
}