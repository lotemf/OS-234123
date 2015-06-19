#include <stdio.h>
#include <stdlib.h>		// For exit() and srand()
#include <unistd.h>		// For fork() and close()
#include <sys/wait.h>	// For wait()
#include <errno.h>		// Guess
#include <pthread.h>	// Testing with threads
#include <semaphore.h>	// For use with threads
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>		// For time(), used in srand()
#include "snake.h"		// For the ioctl functions
#include <sys/ioctl.h>
#include <string.h>
#include <stdbool.h>
#include <asm/errno.h>
extern int errno;

#define ASSERT(expr) do { \
	if(!(expr)) { \
		printf("\nAssertion failed %s (%s:%d).\n", #expr, __FILE__, __LINE__); \
		return false; \
	} else { \
		printf("."); \
	} \
	fflush(stdout); \
} while (0)

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

	char moduleName[13];
	char* moduleBase = "/dev/snake";
	int moduleNum = 0;
	int doNotPrint = 0;

char* getModule()
	{
		int i;
		for (i = 0; i < 13; ++i) { moduleName[i] = '\0'; }
				
		char moduleIndex[4]; 
		for (i = 0; i < 4; ++i) { moduleIndex[i] = '\0'; }
			
		sprintf(moduleIndex, "%d", moduleNum); //convert int to string

		strcat(moduleName , moduleBase );	
		strcat(moduleName , moduleIndex );
		moduleNum++;

		if (doNotPrint==0)
		{
			printf("Entering driver: %s\n",moduleName);
		}
		return moduleName;
	}
void doMediumTask()
	{
	   int j;
	   for(j=0; j<1000; j++)
	   {
	      short i;
	      for (i=1; i != 0; i++)
	      {
	         ;
	      }
	   }
	}


	void doLongTask()
	{
	   int j;
	   for(j=0; j<30000; j++)
	   {
	      short i;
	      for (i=1; i != 0; i++)
	      {
	         ;
	      }
	   }
	}
void printBoard(int fd) {
	char board[1024];
	read(fd, board, 1024);
	printf("\n%s\n", board);
}
	
bool testCreateNewGame() {
	char* dev = getModule();
	int cp1 = fork();
	if (!cp1) { //white player
		int fd1 = open(dev, O_RDWR);
		ASSERT(fd1 > 0);
		ASSERT(ioctl(fd1, SNAKE_GET_COLOR, 0) == 4);
		doLongTask();
		close(fd1);
		_exit(0);	
	} else {
		int cp2 = fork();
		if (!cp2) { // black player
			doLongTask();
			int fd2 = open(dev, O_RDWR);
			ASSERT(fd2 > 0);
			ASSERT(ioctl(fd2, SNAKE_GET_COLOR, 0) == 2);
			close(fd2);
			_exit(0);
		} else { //try to join into active game
			int cp3 = fork();
			if (!cp3) {
				doLongTask();
				doMediumTask();
				int fd3 = open(dev, O_RDWR);
				ASSERT(fd3 == -EPERM);
				_exit(0);
			} else {
			 wait(NULL);
		}
		 wait(NULL);
	}
	 wait(NULL);
}
	int cp4 = fork();
	if (!cp4) { //try to join to a finished game
		doLongTask();
		doMediumTask();
		int fd4 = open(dev, O_RDWR);
		ASSERT(fd4 == -EPERM);
		_exit(0);
	} else {
		 wait(NULL);
	}
	return true;
}

/*bool testllseek() {
	char* dev = getModule();
	int fd = open(dev, O_RDWR);
	ASSERT(fd > 0);
	ASSERT(lseek64(fd,0,0,NULL,0) == -ENOSYS);
	close(fd);
	return true;
}*/
bool test_write_param() {
	char* dev = getModule();
	int ch1 = fork();
	if (!ch1) { //white player
		char x = '2';
		int fd1 = open(dev, O_RDWR);
		ASSERT(fd1 > 0);
		ASSERT(ioctl(fd1, SNAKE_GET_COLOR, 0) == 4); 
		ASSERT(write(fd1, &x, 0) == 0); 
		ASSERT(write(fd1, NULL, 0) == 0);
		ASSERT(write(fd1, NULL, 1) == -1);
		ASSERT(errno == EFAULT);		
		close(fd1);
		_exit(0);
	} else {
		int ch2 = fork();
		if (!ch2) { //black player
			doLongTask();
			int fd2 = open(dev, O_RDWR);
			ASSERT(fd2 > 0);
			doLongTask();
			close(fd2);
			_exit(0);	
		} else {
			wait(NULL);
		}
		wait(NULL);
	}
	return true;
}

bool test_write_invalid_white_move() {
	char* dev = getModule();
	int ch1 = fork();
	if (!ch1) { //white player
		char* x = "2a";
		int fd1 = open(dev, O_RDWR);
		ASSERT(fd1 > 0);
		ASSERT(ioctl(fd1, SNAKE_GET_COLOR, 0) == 4);
		ASSERT(write(fd1, x, 2*sizeof(char)) == -1); 
		ASSERT(errno == ECHILD);
		int ioctlRes = ioctl(fd1, SNAKE_GET_WINNER, 0);
		printf("%d\n", ioctlRes);
		printf("%d\n", ioctlRes);
		printf("%d\n", ioctlRes);
		ASSERT(ioctlRes == 2); 
		doLongTask();
		close(fd1);
		_exit(0);
	} else {
		int ch2 = fork();
		if (!ch2) { //black player
			char y = '8';
			doLongTask();
			int fd2 = open(dev, O_RDWR);
			ASSERT(fd2 > 0);
			ASSERT(ioctl(fd2, SNAKE_GET_COLOR, 0) == 2);
			write(fd2, &y, sizeof(char));
			ASSERT(ioctl(fd2, SNAKE_GET_WINNER, 0) == 2);
			close(fd2);
			_exit(0);	
		} else {
			wait(NULL);
		}
		wait(NULL);
	}
	return true;
}
 
bool test_write_invalid_black_move() {
	char* dev = getModule();
	int ch1 = fork();
	if (!ch1) { //white player
		char* x = "26";
		int fd1 = open(dev, O_RDWR);
		ASSERT(fd1 > 0);
		ASSERT(ioctl(fd1, SNAKE_GET_COLOR, 0) == 4);
		ASSERT(write(fd1, x, 2*sizeof(char)));  
		doLongTask();
		close(fd1);
		_exit(0);
	} else {
		int ch2 = fork();
		if (!ch2) { //black player
			char* y = "8a";
			doLongTask();
			int fd2 = open(dev, O_RDWR);
			ASSERT(fd2 > 0);
			ASSERT(ioctl(fd2, SNAKE_GET_COLOR, 0) == 2);
			write(fd2, y, 2*sizeof(char));
			ASSERT(ioctl(fd2, SNAKE_GET_WINNER, 0) == 4);
			close(fd2);
			_exit(0);	
		} else {
			wait(NULL);
		}
		wait(NULL);
	}
	return true;
}

bool test_write_white_play_first() {
	char* dev = getModule();
	int ch1 = fork();
	if (!ch1) { //white player
		char x = 'a';
		int fd1 = open(dev, O_RDWR);
		ASSERT(fd1 > 0);
		ASSERT(ioctl(fd1, SNAKE_GET_COLOR, 0) == 4);
		doMediumTask(); //make sure we switched to the black player
		/*input is invalid so the 
		 *game is over with the black win.
		 */
		 
		ASSERT(write(fd1, &x, sizeof(char)) == -1); 
		ASSERT(errno == ECHILD);
		ASSERT(ioctl(fd1, SNAKE_GET_WINNER, 0) == 2); 
		doLongTask();
		close(fd1);
		_exit(0);
	} else {
		int ch2 = fork();
		if (!ch2) { //black player
			char y = '2';
			doLongTask();
			int fd2 = open(dev, O_RDWR);
			ASSERT(fd2 > 0);
			ASSERT(ioctl(fd2, SNAKE_GET_COLOR, 0) == 2);
			 ASSERT(write(fd2, &y, sizeof(char)) == -1);
			ASSERT(errno == ECHILD); 
			ASSERT(ioctl(fd2, SNAKE_GET_WINNER, 0) == 2);
			close(fd2);
			_exit(0);	
		} else {
			wait(NULL);
		}
		wait(NULL);
	}
	return true;
}

bool test_write_white_enter_a_wall() {
	char* dev = getModule();
	int ch1 = fork();
	if (!ch1) { //white player
		char* x = "24";
		int fd1 = open(dev, O_RDWR);
		ASSERT(fd1 > 0);
		ASSERT(ioctl(fd1, SNAKE_GET_COLOR, 0) == 4);
		ASSERT(write(fd1, x, 2*sizeof(char)) == 2); 
		ASSERT(ioctl(fd1, SNAKE_GET_WINNER, 0) == 2); 
		close(fd1);
		_exit(0);
	} else {
		int ch2 = fork();
		if (!ch2) { //black player
			char* y = "8";
			doLongTask();
			int fd2 = open(dev, O_RDWR);
			ASSERT(fd2 > 0);
			ASSERT(ioctl(fd2, SNAKE_GET_COLOR, 0) == 2);
			ASSERT(write(fd2, y, sizeof(char)) == 1);
			doLongTask();
			close(fd2);
			_exit(0);	
		} else {
			wait(NULL);
		}
		wait(NULL);
	}
	return true;
}
bool test_write_black_enter_a_wall() {
	char* dev = getModule();
	int ch1 = fork();
	if (!ch1) { //white player
		char* x = "26";
		int fd1 = open(dev, O_RDWR);
		ASSERT(fd1 > 0);
		ASSERT(ioctl(fd1, SNAKE_GET_COLOR, 0) == 4);
		//printf("%d\n", write(fd1, x, 2*sizeof(char)));
		ASSERT(write(fd1, x, 2*sizeof(char)) == 2);
		//printf("bug 1\n");
		doLongTask();
		//printBoard(fd1);
		close(fd1);
		_exit(0);
	} else {
		int ch2 = fork();
		if (!ch2) { //black player
			char* y = "84";
			doLongTask();
			int fd2 = open(dev, O_RDWR);
			ASSERT(fd2 > 0);
			ASSERT(ioctl(fd2, SNAKE_GET_COLOR, 0) == 2);
			//printf("%d\n", write(fd2, y, 2*sizeof(char)));
			ASSERT(write(fd2, y, 2*sizeof(char)) == 2);
			ASSERT(ioctl(fd2, SNAKE_GET_WINNER, 0) == 4);
			//printBoard(fd2);
			close(fd2);
			_exit(0);	
		} else {
			wait(NULL);
		}
		wait(NULL);
	}
	return true;
}
void doMediumTaskAndPrint()
	{
	   int j;
	   for(j=0; j<1000; j++)
	   {
	      short i;
	      for (i=1; i != 0; i++)
	      {
	         ;
	      }
	   }
	   printf("The other process shouldn't print before this line\n");
	}
bool test_write_block() {
	char* dev = getModule();
	int ch1 = fork();
	if (!ch1) { //white player
		char* x = "26";
		int fd1 = open(dev, O_RDWR);
		ASSERT(fd1 > 0);
		ASSERT(ioctl(fd1, SNAKE_GET_COLOR, 0) == 4);
		ASSERT(write(fd1, x, 2*sizeof(char)) == 2); 
		printf("white process ended write\n");
		close(fd1);
		_exit(0);
	} else {
		int ch2 = fork();
		if (!ch2) { //black player
			char* y = "8";
			doLongTask();
			int fd2 = open(dev, O_RDWR);
			ASSERT(fd2 > 0);
			ASSERT(ioctl(fd2, SNAKE_GET_COLOR, 0) == 2);
			doMediumTaskAndPrint();
			ASSERT(write(fd2, y, sizeof(char)) == 1);
			doLongTask();
			close(fd2);
			_exit(0);	
		} else {
			wait(NULL);
		}
		wait(NULL);
	}
	return true;
}

bool test_write_no_error_after_win() {
	char* dev = getModule();
	int ch1 = fork();
	if (!ch1) { //white player
		char* x = "266";
		int fd1 = open(dev, O_RDWR);
		ASSERT(fd1 > 0);
		ASSERT(ioctl(fd1, SNAKE_GET_COLOR, 0) == 4);
		ASSERT(write(fd1, x, 3*sizeof(char)) == 2);
		ASSERT(ioctl(fd1, SNAKE_GET_WINNER, 0) == 4);
		close(fd1);
		_exit(0);
	} else {
		int ch2 = fork();
		if (!ch2) { //black player
			char* y = "84";
			doLongTask();
			int fd2 = open(dev, O_RDWR);
			ASSERT(fd2 > 0);
			ASSERT(ioctl(fd2, SNAKE_GET_COLOR, 0) == 2);
			ASSERT(write(fd2, y, 2*sizeof(char)) == 2);
		//	ASSERT(ioctl(fd2, SNAKE_GET_WINNER, 0) == 4);
			doLongTask();
			close(fd2);
			_exit(0);	
		} else {
			wait(NULL);
		}
		wait(NULL);
	}
	return true;
}

bool test_write_clash() {
	char* dev = getModule();
	int ch1 = fork();
	if (!ch1) { //white player
		char* x = "22";
		int fd1 = open(dev, O_RDWR);
		ASSERT(fd1 > 0);
		ASSERT(ioctl(fd1, SNAKE_GET_COLOR, 0) == 4);
		ASSERT(write(fd1, x, 2*sizeof(char)) == 2); 
		ASSERT(ioctl(fd1, SNAKE_GET_WINNER, 0) == 2);
		close(fd1);
		_exit(0);
	} else {
		int ch2 = fork();
		if (!ch2) { //black player
			char* y = "8";
			doLongTask();
			int fd2 = open(dev, O_RDWR);
			ASSERT(fd2 > 0);
			ASSERT(ioctl(fd2, SNAKE_GET_COLOR, 0) == 2);
			ASSERT(write(fd2, y, sizeof(char)) == 1);
			doLongTask();
			close(fd2);
			_exit(0);	
		} else {
			wait(NULL);
		}
		wait(NULL);
	}
	return true;
}
bool test_write_seires_of_moves() {
	char* dev = getModule();
	int ch1 = fork();
	if (!ch1) { //white player
		char* x = "266";
		int fd1 = open(dev, O_RDWR);
		ASSERT(fd1 > 0);
		ASSERT(ioctl(fd1, SNAKE_GET_COLOR, 0) == 4);
		ASSERT(write(fd1, x, 3*sizeof(char)) == 3); 
		doLongTask();
		close(fd1);
		_exit(0);
	} else {
		int ch2 = fork();
		if (!ch2) { //black player
			char* y = "866";
			doLongTask();
			int fd2 = open(dev, O_RDWR);
			ASSERT(fd2 > 0);
			ASSERT(ioctl(fd2, SNAKE_GET_COLOR, 0) == 2);
			ASSERT(write(fd2, y, 3*sizeof(char)) == 3);
			printBoard(fd2);
			close(fd2);
			_exit(0);	
		} else {
			wait(NULL);
		}
		wait(NULL);
	}
	return true;
}

bool test_write_error_process_exit() {
	char* dev = getModule();
	int ch1 = fork();
	if (!ch1) { //white player
		int fd1 = open(dev, O_RDWR);
		ASSERT(fd1 > 0);
		ASSERT(ioctl(fd1, SNAKE_GET_COLOR, 0) == 4);
		close(fd1);
		_exit(0);
	} else {
		int ch2 = fork();
		if (!ch2) { //black player
			char* y = "866";
			doLongTask();
			int fd2 = open(dev, O_RDWR);
			ASSERT(fd2 > 0);
			doLongTask();
			ASSERT(write(fd2, y, 3*sizeof(char)) == -1);
			ASSERT(errno == ECHILD);
			close(fd2);
			_exit(0);	
		} else {
			wait(NULL);
		}
		wait(NULL);
	}
	return true;
}

bool test_read_param() {
	char* dev = getModule();
	int ch1 = fork();
	if (!ch1) { //white player
		char* buf;
		int fd1 = open(dev, O_RDWR);
		ASSERT(fd1 > 0);
		ASSERT(ioctl(fd1, SNAKE_GET_COLOR, 0) == 4);
		ASSERT(read(fd1, buf, 0) == 0);
		ASSERT(read(fd1, NULL, 0) == 0);
		ASSERT(read(fd1, NULL, 1) == -1);
		ASSERT(errno == EFAULT);
		close(fd1);
		_exit(0);
	} else {
		int ch2 = fork();
		if (!ch2) { //black player
			doLongTask();
			int fd2 = open(dev, O_RDWR);
			ASSERT(fd2 > 0);
			doLongTask();
			close(fd2);
			_exit(0);	
		} else {
			wait(NULL);
		}
		wait(NULL);
	}
	return true;
}

bool test_read_error_process_exit() {
	char* dev = getModule();
	int ch1 = fork();
	if (!ch1) { //white player
		int fd1 = open(dev, O_RDWR);
		ASSERT(fd1 > 0);
		ASSERT(ioctl(fd1, SNAKE_GET_COLOR, 0) == 4);
		close(fd1);
		_exit(0);
	} else {
		int ch2 = fork();
		if (!ch2) { //black player
			char* buf;
			doLongTask();
			int fd2 = open(dev, O_RDWR);
			ASSERT(fd2 > 0);
			doLongTask();
			ASSERT(read(fd2, buf, sizeof(char)) == -1);
			ASSERT(errno == ECHILD);
			close(fd2);
			_exit(0);	
		} else {
			wait(NULL);
		}
		wait(NULL);
	}
	return true;
}

bool test_read_short_buf() {
	char* dev = getModule();
	int ch1 = fork();
	if (!ch1) { //white player
		char buf[30];
		int fd1 = open(dev, O_RDWR);
		ASSERT(fd1 > 0);
		ASSERT(ioctl(fd1, SNAKE_GET_COLOR, 0) == 4);
		printf("print short board:\n");
		ASSERT(read(fd1, buf, 29) == 29);
		buf[29] = '\0';
		printf("\n%s\n",buf);
		printf("print full board:\n");
		printBoard(fd1);
		close(fd1);
		_exit(0);
	} else {
		int ch2 = fork();
		if (!ch2) { //black player
			doLongTask();
			int fd2 = open(dev, O_RDWR);
			ASSERT(fd2 > 0);
			doLongTask();
			close(fd2);
			_exit(0);	
		} else {
			wait(NULL);
		}
		wait(NULL);
	}
	return true;
}

bool test_ioctl_param() {
	char* dev = getModule();
	int ch1 = fork();
	if (!ch1) { //white player
		char buf[30];
		int fd1 = open(dev, O_RDWR);
		ASSERT(fd1 > 0);
		ASSERT(ioctl(fd1, SNAKE_GET_COLOR, 0) == 4);
		ASSERT(ioctl(fd1, 0, 0) == -1);
		ASSERT(errno == ENOTTY);
		close(fd1);
		_exit(0);
	} else {
		int ch2 = fork();
		if (!ch2) { //black player
			doLongTask();
			int fd2 = open(dev, O_RDWR);
			ASSERT(fd2 > 0);
			doLongTask();
			close(fd2);
			_exit(0);	
		} else {
			wait(NULL);
		}
		wait(NULL);
	}
	return true;
}

bool test_ioctl_error_process_exit() {
	char* dev = getModule();
	int ch1 = fork();
	if (!ch1) { //white player
		int fd1 = open(dev, O_RDWR);
		ASSERT(fd1 > 0);
		ASSERT(ioctl(fd1, SNAKE_GET_COLOR, 0) == 4);
		close(fd1);
		_exit(0);
	} else {
		int ch2 = fork();
		if (!ch2) { //black player
			char* buf;
			doLongTask();
			int fd2 = open(dev, O_RDWR);
			ASSERT(fd2 > 0);
			doLongTask();
			ASSERT(ioctl(fd2, SNAKE_GET_COLOR, 0) == -1);
			ASSERT(errno == ECHILD);
			ASSERT(ioctl(fd2, SNAKE_GET_WINNER, 0) == -1);
			ASSERT(errno == ECHILD);
			close(fd2);
			_exit(0);	
		} else {
			wait(NULL);
		}
		wait(NULL);
	}
	return true;
}
 
bool test_ioctl_get_color() {
	char* dev = getModule();
	int ch1 = fork();
	if (!ch1) { //white player
		int fd1 = open(dev, O_RDWR);
		ASSERT(fd1 > 0);
		ASSERT(ioctl(fd1, SNAKE_GET_COLOR, 0) == 4);
		doLongTask();
		close(fd1);
		_exit(0);
	} else {
		int ch2 = fork();
		if (!ch2) { //black player
			char* buf;
			doLongTask();
			int fd2 = open(dev, O_RDWR);
			ASSERT(fd2 > 0);
			ASSERT(ioctl(fd2, SNAKE_GET_COLOR, 0) == 2);
			doLongTask();
			close(fd2);
			_exit(0);	
		} else {
			wait(NULL);
		}
		wait(NULL);
	}
	return true;
}
/****** ioctl get winner tested already *****/
/**************** TEST SYNCHRONIZTIOAN************/
typedef struct thread_param {
	int fd;
	char* in;
	int in_size;
	char buf[1024];
	int buf_size;
	int out;
	int ioctl_param;
} th_param;
void* thread_write_rutine(void* param) {
	th_param* p = param;
	p->out = write(p->fd, p->in, p->in_size);
	return NULL;
}
void* thread_read_rutine(void* param) {
	th_param* p = param;
	p->out = read(p->fd, p->buf, p->buf_size);
	return NULL;
}
void* thread_ioctal_rutine(void* param) {
	th_param* p = param;
	p->out = ioctl(p->fd, p->ioctl_param, 0);
	return NULL;
}
bool test_synchronization() {
	char* dev = getModule();
	int ch1 = fork();
	if (!ch1) { //white player
		int fd1 = open(dev, O_RDWR);
		ASSERT(fd1 > 0);
		ASSERT(ioctl(fd1, SNAKE_GET_COLOR, 0) == 4);
		pthread_t p_write;
		th_param p1;
		p1.fd = fd1;
		p1.in = "a";
		p1.in_size = sizeof(char);
		p1.buf_size = 0;
		p1.out = 0;
		p1.ioctl_param = 0;
		pthread_t p_read;
		th_param p2;
		p2.fd = fd1;
		p2.in = NULL;
		p2.in_size =  0;
		p2.buf_size = 1024;
		p2.out = 0;
		p2.ioctl_param = 0;
		pthread_t p_ioctl;
		th_param p3;
		p3.fd = fd1;
		p3.in = NULL;
		p3.in_size =  0;
		p3.buf_size = 0;
		p3.out = 0;
		p3.ioctl_param = SNAKE_GET_WINNER;
		pthread_create(&p_write, NULL, thread_write_rutine, (void*)&p1);
		pthread_create(&p_read, NULL, thread_read_rutine, (void*)&p2);
		pthread_join(p_write, NULL);
		pthread_join(p_read, NULL);
		pthread_create(&p_ioctl, NULL, thread_ioctal_rutine, (void*)&p3);
		ASSERT(ioctl(fd1, SNAKE_GET_COLOR, 0) == 4);
		pthread_join(p_ioctl, NULL);
		printf("The write thread wrote a - this is an illegal char\n");
		printf("so we expect to the Black player win:\n");
		printf("the thread says player %d won\n", p3.out);
		ASSERT(ioctl(fd1, SNAKE_GET_WINNER, 0) == 2);
		ASSERT(ioctl(fd1, SNAKE_GET_WINNER, 0) == p3.out);
		ASSERT(p2.out > 0);
		ASSERT(p1.out == -1);
		printf("the thread read the next board:\n");
		printf("%s\n", p2.buf);
		printf("current board:");	
		printBoard(fd1);
		doLongTask();
		close(fd1);
		_exit(0);
	} else {
		int ch2 = fork();
		if (!ch2) { //black player
			char* y = "8";
			doLongTask();
			int fd2 = open(dev, O_RDWR);
			ASSERT(fd2 > 0);
			ASSERT(ioctl(fd2, SNAKE_GET_COLOR, 0) == 2);
			ASSERT(write(fd2, y, sizeof(char)) == -1 );
			doLongTask();
			doLongTask();
			doLongTask();
			doLongTask();
			close(fd2);
			_exit(0);	
		} else {
			wait(NULL);
		}
		wait(NULL);
	}
	return true;
}
int main() {
	RUN_TEST(testCreateNewGame);
	//RUN_TEST(testllseek);
	RUN_TEST(test_write_param);
	RUN_TEST(test_write_invalid_white_move);
	RUN_TEST(test_write_invalid_black_move);
	RUN_TEST(test_write_white_play_first);
	RUN_TEST(test_write_white_enter_a_wall);
	RUN_TEST(test_write_black_enter_a_wall);
	RUN_TEST(test_write_block);
	RUN_TEST(test_write_no_error_after_win);
	RUN_TEST(test_write_clash);
	RUN_TEST(test_write_seires_of_moves); 
	RUN_TEST(test_write_error_process_exit);
	RUN_TEST(test_read_param);
	RUN_TEST(test_read_error_process_exit);
	RUN_TEST(test_read_short_buf);
	RUN_TEST(test_ioctl_param);
	RUN_TEST(test_ioctl_error_process_exit);
	RUN_TEST(test_ioctl_get_color);
	RUN_TEST(test_synchronization);
	return 0;
}
