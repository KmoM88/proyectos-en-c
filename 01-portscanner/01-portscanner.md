# Analizador de Puertos TCP

## 🧠 Objetivo General

Construir un **TCP Port Scanner** en C capaz de:

- Escanear puertos abiertos en una IP o rango de IPs.
- Usar distintos modos: connect scan y eventualmente SYN scan (más avanzado).
- Mostrar tiempos de respuesta, banners, etc.
- Usar select/poll o epoll para escaneos paralelos.
- Compilar con make y/o cmake.
- (Opcional) Tener una interfaz sencilla tipo TUI (ncurses) o CLI moderna con flags.

---

## 🧩 Etapas sugeridas del proyecto

### 1. Configurar el entorno del proyecto

Estructura básica de archivos:
```
portscanner/
├── src/
│   ├── main.c
│   ├── scanner.c
│   └── scanner.h
├── include/
│   └── utils.h
├── Makefile
└── 01-portscanner.md
```

---

### 2. Implementar escaneo simple (connect scan con TCP)

Crear un programa que:

- Acepte una IP y un puerto como argumentos.
- Cree un socket TCP.
- Haga `connect()` al puerto y detecte si está abierto o no.
- Cierre el socket.

**Aprendizajes:**
- Uso de sockets (`socket()`, `connect()`, `close()`).
- Estructuras `sockaddr_in`, `inet_pton()`, `htons()`.

---

### Archivos principales del escaneo simple

Para implementar el escaneo simple, se utilizan los siguientes archivos:

- [`src/main.c`](src/main.c): Programa principal. Recibe la IP y el puerto como argumentos, llama a la función de escaneo y muestra el resultado.
- [`src/scanner.c`](src/scanner.c): Implementa la función que realiza el escaneo del puerto usando sockets TCP.  
  Utiliza funciones y estructuras de las siguientes librerías estándar de Linux/C:
  - `<stdio.h>`: Para impresión de mensajes de error (opcional).
  - `<string.h>`: Para inicializar la estructura de dirección con `memset`.
  - `<arpa/inet.h>`: Para la conversión de direcciones IP (`inet_pton`) y definición de estructuras de red.
  - `<unistd.h>`: Para cerrar el socket con `close()`.
  - `<sys/socket.h>` y `<netinet/in.h>`: Para crear el socket TCP (`socket()`), definir la familia de direcciones (`AF_INET`), el tipo de socket (`SOCK_STREAM`) y la estructura `sockaddr_in`.

  **Opciones y funciones clave utilizadas:**
  - `socket(AF_INET, SOCK_STREAM, 0)`: Crea un socket TCP IPv4.
  - `memset(&addr, 0, sizeof(addr))`: Inicializa la estructura de dirección.
  - `addr.sin_family = AF_INET`: Especifica el uso de IPv4.
  - `addr.sin_port = htons(port)`: Convierte el puerto a formato de red.
  - `inet_pton(AF_INET, ip, &addr.sin_addr)`: Convierte la IP en texto a binario.
  - `connect(sockfd, (struct sockaddr *)&addr, sizeof(addr))`: Intenta conectar al puerto especificado.
  - `close(sockfd)`: Cierra el socket.

  El valor de retorno de `scan_port` es `0` si el puerto está abierto (conexión exitosa) y distinto de cero si está cerrado o hay error.

  Estas funciones y opciones son estándar en sistemas Linux y Unix, y no requieren librerías externas adicionales.
- [`src/scanner.h`](src/scanner.h): Encabezado que declara la función de escaneo para ser usada desde `main.c`.
- [`Makefile`](Makefile): Permite compilar el proyecto fácilmente con `make`.

#### Explicación básica de los archivos

- **main.c**: Lee los argumentos de línea de comandos (IP y puerto), llama a `scan_port()` y muestra si el puerto está abierto o cerrado.
- **scanner.c**: Implementa `scan_port()`, que crea un socket, intenta conectarse al puerto especificado y retorna si está abierto o cerrado.
- **scanner.h**: Declara la función `scan_port()` para que pueda ser utilizada en otros archivos.
- **Makefile**: Define cómo compilar el proyecto y limpiar los binarios generados.

---

### Cómo compilar y probar

1. **Compilar el proyecto**

   Desde la carpeta del proyecto, ejecuta:
   ```sh
   make
   ```

2. **Abrir un puerto seguro para pruebas**

   Puedes abrir un servidor HTTP local en el puerto 8000 con Python:
   ```sh
   python3 -m http.server 8000
   ```

3. **Probar el escáner**

   - Para probar un puerto abierto (8000):
     ```sh
     ./portscanner 127.0.0.1 8000
     ```
   - Para probar un puerto cerrado (por ejemplo, 65000):
     ```sh
     ./portscanner 127.0.0.1 65000
     ```

   El programa mostrará si el puerto está abierto o cerrado según el resultado de la conexión.

---

> Puedes consultar el código fuente en los archivos mencionados para ver la implementación detallada.

### 3. Escaneo de múltiples puertos

Esta funcionalidad permite escanear un rango de puertos en una IP, mostrando si cada puerto está abierto o cerrado y el tiempo de respuesta de cada intento.

- [`src/main.c`](src/main.c): El programa acepta los siguientes modos de uso:
  - `./portscanner <IP> <PUERTO>`: Escanea un solo puerto.
  - `./portscanner <IP> -r <PUERTO_MAX>`: Escanea desde el puerto 1 hasta `<PUERTO_MAX>`.
  - `./portscanner <IP> -r <PUERTO_MIN> <PUERTO_MAX>`: Escanea desde `<PUERTO_MIN>` hasta `<PUERTO_MAX>`.
- Internamente, para cada puerto del rango, se mide el tiempo de respuesta usando la función `clock_gettime()` de la librería `<time.h>`.
- Para cada puerto, se llama a la función [`scan_port()`](src/scanner.c) y se imprime si está abierto o cerrado junto con el tiempo en milisegundos.

**Ejemplo de uso:**
```sh
# Escanear puertos 1 a 1024 en localhost
./portscanner 127.0.0.1 -r 1024

# Escanear puertos 20 a 25 en localhost
./portscanner 127.0.0.1 -r 20 25
```

**Fragmento relevante en [`src/main.c`](src/main.c):**
```c
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
```

**Aprendizajes:**
- Uso de bucles para iterar sobre rangos de puertos.
- Medición de tiempos con `clock_gettime`.
- Optimización básica y reutilización de la función de escaneo.

> Consulta el código fuente en [`src/main.c`](src/main.c) para ver la implementación completa del escaneo de múltiples puertos.

---

### 4. Soporte de múltiples hosts

Para soportar el escaneo de múltiples hosts, se agregaron dos funciones auxiliares en [`src/main.c`](src/main.c):

- **`parse_ip_range()`**:  
  Permite detectar y procesar rangos de IPs escritos como `192.168.1.1-254`.  
  Esta función separa la base de la IP (`192.168.1.`) y los valores inicial y final del rango, permitiendo iterar sobre todas las IPs del rango y escanearlas una a una.

  ```c
  int parse_ip_range(const char *input, char *base, int *start, int *end);
  ```

- **`resolve_domain()`**:  
  Permite aceptar nombres de dominio (por ejemplo, `scanme.nmap.org`) como argumento.  
  Utiliza la función estándar `getaddrinfo()` para resolver el dominio a una dirección IP antes de realizar el escaneo.

  ```c
  int resolve_domain(const char *host, char *ipstr, size_t ipstrlen);
  ```

**¿Cómo funciona en el flujo del programa?**

- Si el primer argumento es un rango de IPs, el programa itera sobre cada IP generada y ejecuta el escaneo de puertos para cada una.
- Si el primer argumento es un dominio, primero lo resuelve a una IP y luego realiza el escaneo como si fuera una IP normal.
- Si el argumento es una IP simple, el comportamiento es el mismo que antes.

**Fragmento relevante en [`src/main.c`](src/main.c):**
```c
if (parse_ip_range(ip, base, &start, &end)) {
    for (int i = start; i <= end; i++) {
        snprintf(ipstr, sizeof(ipstr), "%s%d", base, i);
        // Escaneo para cada IP generada
    }
} else if (!isdigit(ip[0])) {
    if (resolve_domain(ip, ipstr, sizeof(ipstr))) {
        // Escaneo para la IP resuelta del dominio
    }
}
```

Estas funciones permiten que el escáner sea más flexible y útil en redes reales, facilitando el escaneo masivo de hosts y el uso de nombres de dominio.

---

### 5. Concurrencia

Implementar escaneo concurrente con:

- `fork()` (procesos)
- o `pthreads`
- o `select()` / `poll()` / `epoll` (más avanzado)

**Aprendizajes:**
- Concurrencia en C.
- Manejo de procesos y/o threads.
- Control de errores complejos.

---

### 6. Banner grabbing (básico)

- Si el puerto está abierto, enviar un paquete TCP (como `HEAD / HTTP/1.1`) y leer la respuesta.

**Aprendizajes:**
- Lectura/escritura en sockets (`send()`, `recv()`).
- Parsing básico de protocolos.

---

### 7. Parámetros CLI

Agregar flags tipo:

```bash
./scanner -t 192.168.0.1 -p 1-1024 -c 50 -v
```
- `-t`: target
- `-p`: puerto o rango
- `-c`: concurrencia
- `-v`: verbose

**Aprendizajes:**
- Uso de `getopt()` o `argp` en C.
- Validación de entradas.

---

### 8. Mejorar build system con cmake

- Reorganizar el proyecto para usar cmake.
- Separar bibliotecas, flags, testing (opcional con CTest).

---

### 9. Logging, errores, y profiling

- Agregar sistema de logs.
- Medir rendimiento con herramientas como `gprof`, `valgrind`, `perf`.

---

### 10. Escaneo SYN (opcional y avanzado)

- Requiere permisos elevados (raw sockets).
- Crear paquetes TCP con bandera SYN y analizar las respuestas.
  - Si se recibe SYN-ACK: puerto abierto.
  - Si se recibe RST: puerto cerrado.

**Aprendizajes:**
- Raw sockets (`socket(AF_INET, SOCK_RAW, IPPROTO_TCP)`).
- Construcción manual de paquetes TCP/IP.
- Checksums y headers.
- Posible integración con libpcap o lectura con `recvfrom()`.

---

### 11. Extras (opcionales)

- Exportar resultados (CSV, JSON).
- Interfaz con ncurses.
- Escaneo UDP (más difícil).
- UI web (más allá de C, con CGI).

---

## 🛠️ Herramientas y técnicas clave

- `socket()`, `connect()`, `send()`, `recv()`
- `fork()`, `pthread`, `select()`, `poll()`, `epoll`
- `make`, `cmake`
- `getopt()`, `argp`
- `valgrind`, `gprof`, `strace`
- `tcpdump`, `wireshark` (para debugging)

---

## 📚 Recursos útiles

- **Libro:** [Beej’s Guide to Network Programming](https://beej.us/guide/bgnet/) (¡clásico!)
- **RFC 793:** Especificación de TCP
- `man 2 socket`, `man 2 connect`, etc.