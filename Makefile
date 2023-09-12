CC = gcc
CFLAGS = -Wall -g -Werror -fsanitize=address

all: ttt ttts

ttt: ttt.c
	$(CC) $(CFLAGS) ttt.c -o ttt

ttts: ttts.c
	$(CC) $(CFLAGS) ttts.c -o ttts

clean:
	rm -f ttt ttts