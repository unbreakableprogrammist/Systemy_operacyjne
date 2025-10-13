main: main.o hello.o
	gcc -o main main.o hello.o

main.o: main.c hello.h
	gcc -o main.o -c main.c

hello.o: hello.c hello.h
	gcc -o hello.o -c hello.c

.PHONY: clean

clean:
	rm -f *.o main