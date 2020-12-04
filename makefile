CC = gcc -std=c89 -pedantic

run: master taxi source
	./master

master: master.o utils.o taxi
	$(CC) master.o utils.o -o master

source: source.o
	$(CC) source.o -o source

taxi: taxi.o 	
	$(CC) taxi.o -o taxi

source.o: source.o
	$(CC) -c source.c -o source.o

master.o: master.c
	$(CC) -c master.c -o master.o

utils.o: utils.c
	$(CC) -c utils.c -o utils.o


clear:
	rm *.o
	rm taxi master source

