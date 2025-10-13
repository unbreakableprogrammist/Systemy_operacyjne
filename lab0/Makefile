FLAGS = -Wall -fsanitize=address,undefined
all: main
	./main
main: main.o hello.o
	gcc $(FLAGS) -o main main.o hello.o
all2: zadanie2
	./zadanie2
zadanie2: zad2.c
	gcc $(FLAGS) -o zadanie2 zad2.c
main.o: main.c hello.h
	gcc -o main.o -c main.c

hello.o: hello.c hello.h
	gcc -o hello.o -c hello.c

.PHONY: clean

clean:
	rm -f *.o main
