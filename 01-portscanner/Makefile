CC=gcc
CFLAGS=-Wall -I./src
SRC=src/main.c src/scanner.c
OUT=portscanner

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC)

clean:
	rm -f $(OUT)
