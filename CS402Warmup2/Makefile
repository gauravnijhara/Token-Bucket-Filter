warmup2:	my402list.o main.o
	gcc -o warmup2 -g -Wno -pthread -lm	my402list.o main.o

main.o:	main.c my402list.h
	gcc -g -c -Wall —lm	pthread main.c

my402list.o: my402list.c my402list.h
	gcc -g -c -Wall —lm	pthread my402list.c

clean:
	rm -f *.o warmup2