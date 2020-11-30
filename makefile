CC = gcc -std=c89 

run: master taxi control
	./master

master: master.o colors.o control taxi
	$(CC) master.o colors.o -o master

control: control.o
	$(CC) control.o -o control

taxi: taxi.o 	
	$(CC) taxi.o -o taxi


master.o: master.c
	$(CC) -c master.c -o master.o

colors.o: include_files/colors/colors.c include_files/colors/colors.h
	$(CC) -c include_files/colors/colors.c -o colors.o


clear:
	rm *.o
	rm taxi control master

