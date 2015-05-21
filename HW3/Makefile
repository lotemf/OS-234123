all:	osqueue.c threadPool.c threadPool.h

	gcc -pthread -c -Wall -Werror -D_GNU_SOURCE threadPool.c osqueue.c
	ar rcs libthreadPool.a threadPool.o osqueue.o


	gcc -L. test.c -lthreadPool -lpthread  -o a.out
	./a.out
clean:

	rm threadPool.o osqueue.o libthreadPool.a