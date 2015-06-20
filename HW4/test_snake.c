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
#include <assert.h>



/*******************************************************************************
						How-To-Use:

		* Please Define 5 snake devices, from 0 to 4 named:
			-snake0
			-snake1
			-snake2
			-snake3
			-snake4

		**so the tests will run correctly...
*******************************************************************************/
void printBoard(int fd) {
	char board[1024];
	read(fd, board, 1024);
	printf("\n%s\n", board);
}

#define RUN_TEST(test) do { \
        printf("Running "#test"... "); \
        if (test()) { \
            printf("[OK]\n");\
        } else { \
        	printf("[Failed]\n"); \
        	return 1; \
        } \
		fflush(stdout); \
} while(0)



void doMediumTask()
{
   int i;
   for (i=1; i != 0; i++)
   {
      ;
   }
}


bool CheckOpen()
{
	char* device = "/dev/snake0";
	char board[120];
	int retval = 1;

	int player1;
	int player2;
	int status;

	int pid1 = fork();
	if(!pid1) {

		player1=open(device, O_RDWR);				//White Player
	    close(player1);

		_exit(0);
	}
	else {
		int pid2 = fork();
		if(!pid2) {

			player2=open(device, O_RDWR);			//Black Player
			retval = read(player2, board, 120);
			assert(retval == 120);

			doMediumTask();

		    close(player2);

			_exit(0);
		} else {
			wait(&status);
		}
		wait(&status);
	}


    return retval;
}

bool CheckBadInput()
{
	char* device = "/dev/snake1";
	char board[120];
	int retval = 1;

	int player1,player2;
	int unauthorized;
	int status;

	int pid1 = fork();
	if(!pid1) {
		player1=open(device, O_RDWR);				//White Player

		int res = read(player1, NULL, 120);
		assert(res == -1);

		res = read(player1, board, 0);
		assert(res == 0);

		res = read(player1, NULL, 0);
		assert(res == 0);

		close(player1);
		_exit(0);
	}
	else {
		int pid2 = fork();
		if(!pid2) {
			player2=open(device, O_RDWR);			//Black Player

			unauthorized=open(device, O_RDWR);
			assert(unauthorized == -1);

			retval = read(player2, board, 120);
			doMediumTask();

		    close(player2);
			_exit(0);
		} else {
			wait(&status);
		}
		wait(&status);
	}

    return retval;
}

bool CheckRead()
{
	char* device = "/dev/snake2";
	char board[120];
	board[120]='\0';
	int player1;
	int player2;
	int status;
	int pid1 = fork();
	if(!pid1) {
		player1=open(device, O_RDWR);				//White Player
		assert(player1>=0);

		int j;
		for (j = 0; j < 5; j++){
			doMediumTask();
		}

		close(player1);
		_exit(0);
	} else {
		int pid2 = fork();
		if(!pid2) {
			doMediumTask();
			player2=open(device, O_RDWR);			//Black Player
			assert(player2>=0);
			int res = read(player2, board, 120);
			assert(res == 120);
			printf("\nBoard: \n%s\n", board);
			close(player2);
			_exit(0);
		} else {
			wait(&status);
		}
		wait(&status);
	}

    return true;
}

bool CheckWrite() {
	char* device = "/dev/snake3";
	int pid1 = fork();
	if (!pid1) {
		char* moves1 = "24";
		int player1;
		player1 = open(device, O_RDWR);
		assert(player1 > 0);

		int retval = write(player1, moves1, 2);
		assert(retval == 2);
		close(player1);

		_exit(0);
	} else {
		int pid2 = fork();
		if (!pid2) {
			char* moves2 = "8";

			int i;
			for (i = 0; i < i; i++){
				doMediumTask();
			}

			int player2;
			player2 = open(device, O_RDWR);
			assert(player2 > 0);
			int retval2 = write(player2, moves2, 1);
			assert(retval2 == 1);

			for (i=0;i<5;i++){
				doMediumTask();
			}

			close(player2);
			_exit(0);
		} else {
			wait(NULL);
		}
		wait(NULL);
	}
	return true;
}

bool CheckIoctl() {
	char* device = "/dev/snake4";
	int pid1 = fork();
	if (!pid1) {
		char* moves1 = "22";
		int player1 = open(device, O_RDWR);
		assert(player1 > 0);

		int retval;
		retval = ioctl(player1, SNAKE_GET_COLOR, 0);
		assert(retval == 4);

		retval = write(player1, moves1, 2);
		assert(retval == 2);

		retval = ioctl(player1, SNAKE_GET_WINNER, 0);
		assert(retval == 2);
		close(player1);
		_exit(0);
	} else {
		int pid2 = fork();
		if (!pid2) {
			char* moves2 = "8";
			doMediumTask();
			int player2 = open(device, O_RDWR);
			assert(player2 > 0);

			int retval2;
			retval2 = ioctl(player2, SNAKE_GET_COLOR, 0);
			assert(retval2 == 2);

			retval2 = write(player2, moves2, sizeof(char));
			assert(retval2 == 1);

			int i;
			for (i=0;i<5;i++){
				doMediumTask();
			}

			close(player2);
			_exit(0);
		} else {
			wait(NULL);
		}
		wait(NULL);
	}
	return true;
}
int main(){
	printf("************************************************ \n");
	printf("           Welcome to test_snake...              \n");
	printf("************************************************ \n");

	RUN_TEST(CheckBadInput);
	RUN_TEST(CheckOpen);
	RUN_TEST(CheckRead);
	RUN_TEST(CheckWrite);
	RUN_TEST(CheckIoctl);

    return 0;
}
