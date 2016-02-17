warmup2:	my402list.o main.o
	gcc -o warmup2 -g -Wno -pthread my402list.o main.o -lm

main.o:	main.c my402list.h
	gcc -g -c -Wall -pthread main.c -lm

my402list.o: my402list.c my402list.h
	gcc -g -c -Wall -pthread my402list.c -lm

clean:
	rm -f *.o warmup2
