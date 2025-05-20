#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "scanner.h"

int main(int argc, char *argv[]) {
    if (argc == 3) {
        // Modo simple: escanea un solo puerto
        const char *ip = argv[1];
        int puerto = atoi(argv[2]);
        if (scan_port(ip, puerto) == 0) {
            printf("Puerto %d abierto en %s\n", puerto, ip);
        } else {
            printf("Puerto %d cerrado en %s\n", puerto, ip);
        }
        return 0;
    } else if (argc == 4 && strcmp(argv[2], "-r") == 0) {
        // Modo múltiple: ./portscanner <IP> -r <puerto_max>
        const char *ip = argv[1];
        int puerto_min = 1;
        int puerto_max = atoi(argv[3]);
        if (puerto_max < 1 || puerto_max > 65535) {
            printf("Rango de puertos inválido (1-65535)\n");
            return 1;
        }
        printf("Escaneando puertos %d a %d en %s...\n", puerto_min, puerto_max, ip);
        for (int puerto = puerto_min; puerto <= puerto_max; puerto++) {
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
        }
        return 0;
    } else if (argc == 5 && strcmp(argv[2], "-r") == 0) {
        // Modo rango: ./portscanner <IP> -r <puerto_min> <puerto_max>
        const char *ip = argv[1];
        int puerto_min = atoi(argv[3]);
        int puerto_max = atoi(argv[4]);
        if (puerto_min < 1 || puerto_max > 65535 || puerto_min > puerto_max) {
            printf("Rango de puertos inválido (1-65535 y min <= max)\n");
            return 1;
        }
        printf("Escaneando puertos %d a %d en %s...\n", puerto_min, puerto_max, ip);
        for (int puerto = puerto_min; puerto <= puerto_max; puerto++) {
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
        }
        return 0;
    } else {
        printf("Uso:\n");
        printf("  %s <IP> <PUERTO>\n", argv[0]);
        printf("  %s <IP> -r <PUERTO_MAX>\n", argv[0]);
        printf("  %s <IP> -r <PUERTO_MIN> <PUERTO_MAX>\n", argv[0]);
        printf("Ejemplo para escanear puertos 1-1024:\n");
        printf("  %s 127.0.0.1 -r 1024\n", argv[0]);
        printf("Ejemplo para escanear puertos 20-25:\n");
        printf("  %s 127.0.0.1 -r 20 25\n", argv[0]);
        return 1;
    }
}