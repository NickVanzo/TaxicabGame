CC = gcc -std=c89 

run: master taxi
	./master

master: master.o utils.o taxi
	$(CC) master.o utils.o -o master


taxi: taxi.o 	
	$(CC) taxi.o -o taxi

source: source.o
	$(CC) -c source.c -o source.o

master.o: master.c
	$(CC) -c master.c -o master.o

utils.o: utils.c
	$(CC) -c utils.c -o utils.o


clear:
	rm *.o
	rm taxi master

