#include <asm/errno.h>
extern int errno;
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "snake.h"
#include <sys/wait.h>
#include <assert.h>
#include <unistd.h>
#include <sys/ioctl.h>


void doMediumTask()
{
   int i;
   for (i=1; i != 0; i++)
   {
      ;
   }
}


int FirstTest()
{
	char board[1024];
	int retval = 1;

	int player1;
	int b;
	int status;
	printf("making fork\n");
	int pid1 = fork();
	if(pid1 == 0) {
		//player 1 (white)
		player1=open("/dev/snake0", O_RDWR);
		printf("process player1 has finished opening...\n");
		_exit(0);
	}
	else {
		int pid2 = fork();
		if(pid2 == 0) {
			//player 2 (black)
			player2=open("/dev/snake0", O_RDWR);
			printf("process player2 has finished opening...\n");
			retval = read(player2, board, 1024);
			printf("%d\n",retval);
			printf("Board: \n%s", board);
			doMediumTask();
			_exit(0);
		} else {
			wait(&status);
		}
		wait(&status);
	}


    close(a);
    close(b);

    return retval;
}

int SecondTest()
{

	char board[120];
	int retval = 1;

	int player1;
	int player2;
	int status;
	printf("making fork\n");
	int pid1 = fork();
	if(pid1 == 0) {
		//player 1 (white)
		player1=open("/dev/snake0", O_RDWR);
		printf("player1 has finished opening...\n");

//		retval = read(player1, board, 120);
//		printf("The return value of read is: %d\n",retval);
//		printf("Player 1 Board Print: \n %s", board);
//		doMediumTask();
//
//		//Allocating dynamically so later on we can change it to a few moves at once...
//		char * buffer;
//		buffer = malloc(sizeof(char));
//		buffer[0] = 2;
//
//		//Calling the write function of the module snake 0
//		retval = write(player1,buffer,1);
//		printf("The return value of write is: %d\n",retval);
//
//		retval = read(player1, board, 120);
//		printf("The return value of read is: %d\n",retval);
//		printf("Player 1 Board Print: \n %s", board);
//		doMediumTask();

		_exit(0);
	}
	else {
		int pid2 = fork();
		if(pid2 == 0) {
			//player 2 (black)
			player2=open("/dev/snake0", O_RDWR);
			printf("player2 has finished opening...\n");
			retval = read(player2, board, 120);
			printf("The return value of read is: %d\n",retval);
			printf("Player 2 Board Print: \n %s", board);
			doMediumTask();

			_exit(0);
		} else {
			wait(&status);
		}
		wait(&status);
	}


    close(player1);
    close(player2);

    return retval;
}

int main(){
        FirstTest();
//        SecondTest();
        return 0;
}
