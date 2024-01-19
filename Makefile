all: mts

test: test.o priority_queue.o

test.o: test.c shared.h
	gcc -Wall -c test.c

mts: mts.o priority_queue.o
	gcc -Wall -o mts mts.o priority_queue.o -lpthread

mts.o: mts.c shared.h
	gcc -Wall -c mts.c

priority_queue.o: priority_queue.c shared.h
	gcc -Wall -c priority_queue.c

clean:
	-rm -f *.o mts test
