CC = gcc -std=c89 -pedantic

run: master taxi source ipcKey.key
	./master

master: master.o utils.o taxi
	$(CC) master.o utils.o -o master

source: source.o
	$(CC) source.o -o source

taxi: taxi.o utils.c	
	$(CC) taxi.o utils.o -o taxi -lm

rebuild: 
	make clear
	make 

source.o: source.c
	$(CC) -c source.c -o source.o

master.o: master.c
	$(CC) -c master.c -o master.o

utils.o: utils.c
	$(CC) -c utils.c -o utils.o


clear:
	rm *.o
	rm taxi master source

