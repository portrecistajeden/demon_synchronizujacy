all: SYNCdaemon
SYNCdaemon: daemon.o functions.o
	gcc daemon.o functions.o -o SYNCdaemon
daemon.o: daemon.c functions.c functions.h
	gcc daemon.c -c -o daemon.o
functions.o: functions.c functions.h
	gcc functions.c -c -o functions.o
.PHONY: clean
clean:
	rm -f *.o SYNCdaemon
