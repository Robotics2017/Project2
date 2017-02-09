
# default project named create2
create2: main.c serial.o
	gcc -Wall main.c serial.o -o create2

serial.o: serial.c serial.h
	gcc -Wall serial.c -c

clean:
	rm create2 serial.o

