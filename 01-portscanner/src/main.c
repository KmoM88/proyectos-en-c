#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "scanner.h"
#include <ctype.h>
#include <netdb.h>
#include <arpa/inet.h>

// Función auxiliar para saber si es un rango de IPs tipo 192.168.1.1-254
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

// Función auxiliar para resolver dominios
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
    // --- NUEVO: Soporte para rangos de IPs y dominios ---
    if (argc >= 3) {
        char base[32], ipstr[INET_ADDRSTRLEN];
        int start, end;
        const char *ip = argv[1];

        if (parse_ip_range(ip, base, &start, &end)) {
            // Es un rango de IPs
            for (int i = start; i <= end; i++) {
                snprintf(ipstr, sizeof(ipstr), "%s%d", base, i);
                printf("\nEscaneando host %s...\n", ipstr);
                // Reutiliza la lógica existente para cada IP
                char *fake_argv[5];
                fake_argv[0] = argv[0];
                fake_argv[1] = ipstr;
                for (int j = 2; j < argc; j++) fake_argv[j] = argv[j];
                // Llama recursivamente al main para cada IP
                if (argc == 3)
                    main(3, fake_argv);
                else if (argc == 4)
                    main(4, fake_argv);
                else if (argc == 5)
                    main(5, fake_argv);
            }
            return 0;
        } else if (!isdigit(ip[0])) {
            // Es un dominio
            if (resolve_domain(ip, ipstr, sizeof(ipstr))) {
                printf("Dominio %s resuelto a %s\n", ip, ipstr);
                // Reutiliza la lógica existente para la IP resuelta
                char *fake_argv[5];
                fake_argv[0] = argv[0];
                fake_argv[1] = ipstr;
                for (int j = 2; j < argc; j++) fake_argv[j] = argv[j];
                if (argc == 3)
                    main(3, fake_argv);
                else if (argc == 4)
                    main(4, fake_argv);
                else if (argc == 5)
                    main(5, fake_argv);
                return 0;
            } else {
                printf("No se pudo resolver el dominio %s\n", ip);
                return 1;
            }
        }
    }

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