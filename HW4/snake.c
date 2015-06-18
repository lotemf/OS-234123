#include <linux/errno.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <asm/semaphore.h>

#include "snake.h"
#include "hw3q1.h"

MODULE_LICENSE("GPL");

/*******************************************************************************
		Defines
*******************************************************************************/
//Color definitions
#define WHITE_PLAYER 4						//Players colors return values
#define BLACK_PLAYER 2
#define WHITE_PLAYER_IN_GAME 1				//Players colors in hw3q1.c for the color checks
#define BLACK_PLAYER_IN_GAME -1

//is_played flag values
#define GAME_FINISHED 2
#define GAME_STILL_PLAYING 1
#define GAME_NOT_STARTED 0

//Error Codes
#define PLAYER_LEFT -10						//An error code updated from the piazza

//Return value
#define NO_WINNER_YET -1
#define TIE 5
/*******************************************************************************
		Module Variables
*******************************************************************************/
static int major = -1;
struct file_operations fops;
int maxGames = 0;

//Arrays of the module for each of the devices (Games)
Matrix* game_matrix;
int* players_num;						//The data of every game, that is not tethered
int* is_played;							//to a specific player, will be held in these
int* game_winner;						//data structures
int* next_players_turn;
int* white_counters,*black_counters;

//Locks
struct semaphore* game_sema, *white_write_sema, *black_write_sema;
int * white_write_sema_counter, *black_write_sema_counter;
spinlock_t* players_lock;			 	//The spinlock is used to check the legal
									   	// number of players for each device (which is 2 players)

MODULE_PARM(maxGames,"i");				//This is the only input needed for the module
										//But still it is unclear how we receive this param
										//(maybe in the makefile...)

typedef struct {
    int minor;							//Saving the minor inside the PLAYER's private_data
    int player_color;					//to allow different functions this player is using
 } dev_private_data;						//to know to which game it is connected

/*******************************************************************************
 * static int convert_player_color - Converts the player's color from the define
 * 									 in snake.c to the define in hw3q1.h
 * 									 (Because of the inconsistency)
*******************************************************************************/
static int convert_player_color (int player_color){
	if (player_color == WHITE_PLAYER){
		return WHITE_PLAYER_IN_GAME;
	}
	return BLACK_PLAYER_IN_GAME;
}
/*******************************************************************************
 * snake_open - Opens a new game on the Snake device.
 	 	 	 	The process that opens the device will get an automatic
 	 	 	 	white color if it's the first process, or automatically a black
 	 	 	 	color if it's the second process.

 	 	 	 	If a process is the first to call open on this device
 	 	 	 	for the game, it should wait on the semaphore until another
 	 	 	 	process calls open,wakes the 1st process and then it's GAME ON.


 	 	 	 	P.S. - The way to contact this specific game for the two processes
 	 	 	 	that are playing in it, will be through the Minor num when
 	 	 	 	"talking" to the device driver.
*******************************************************************************/
int snake_open(struct inode* inode, struct file* fileptr){
	fileptr->f_op = &fops;
    int minor = MINOR(inode->i_rdev);							//Extracting the minor from the device

    //Legal game status checks
    if (players_num[minor] == 2){								//Checking if this game is already full...
    	return -EPERM;
    }
    if (is_played[minor] == PLAYER_LEFT){						//If one of the player's left the game
    	return PLAYER_LEFT;
    }

    //Setting the file pointer of this process's (player) proiate data
    fileptr->private_data = kmalloc(sizeof(dev_private_data), GFP_KERNEL);
    if (!(fileptr->private_data)){
    	return -ENOMEM;
    }

    //Setting the private_data for this player
    dev_private_data* dev_p_data = (dev_private_data*)(fileptr->private_data);
    dev_p_data->minor = minor;

    spin_lock(players_lock[minor]);
    if (!players_num[minor]){									//This means this player should get the white color,
    	players_num[minor]++;									//because he's the first to arrive
        dev_p_data->player_color = WHITE_PLAYER;
        //**NOTICE - This player goes to sleep OUTSIDE the spinlock on the semaphore until the second one wakes him up
    } else if (players_num[minor] == 1){
    	players_num[minor]++;
    	dev_p_data->player_color = BLACK_PLAYER;
    	is_played[minor] = GAME_STILL_PLAYING;					//Setting the flag for the game start
     	if (!Game_Init(&(game_matrix[minor]))){					//Only the black player can Initiate the game
        	return -ENXIO;
        }
    	up(&game_sema[minor]);									//Waking up the other player process from the semaphore
    }
    spin_unlock(players_lock[minor]);

//    /*TEST*/printk("[Debug-Snake]->The number of players in snake %d game is: %d \n",minor,players_num[minor]);

    //Checking if the game should start, or the player needs to sleep on the semaphore
    if (is_played[minor] == GAME_NOT_STARTED){					//This check is done outside of the spinlock
    	down_interruptible(&game_sema[minor]);					//to make sure we will not send a player to sleep while holding the CPU
    }
	printk("\t[DEBUG]:\tFinished Driver open - Snake_Open\n");
    return 0;
}
/*******************************************************************************
 * snake_release - Frees the allocated private data, and reducing the amount
 	 	 	 	   of players in the game so when we check the write/read
 	 	 	 	   values we know if it's a legal call or not

 	 	 **NOTICE - A situation that the flag of a game is GAME_STILL_PLAYING
 	 	 	 	 	and the amount of players in it is 1 cannot happen..
 	 	 	 	 	It should correspond to the PLAYER_LEFT change we make here
*******************************************************************************/
int snake_release(struct inode* inode, struct file* filp){

	int minor = MINOR(inode->i_rdev);

	//Update counters:
	spin_lock(players_lock[minor]);
	players_num[minor]--;					//TODO - Keep editing the notes
	is_played[minor] = PLAYER_LEFT;			//Setting the flag to PLAYER_LEFT for this game
	spin_unlock(players_lock[minor]);

	//Waking up the semaphores
	int i;
	for (i=0;i<white_write_sema_counter[minor];i++){
		up(&white_write_sema[minor]);
	}
	for (i=0;i<black_write_sema_counter[minor];i++){
		up(&black_write_sema[minor]);
	}
	kfree(filp->private_data);

	return 0;
}
/*******************************************************************************
 * snake_read - Prints the board, using the buffer supplied.
 	 	 	 	If the buffer that was supplied to us by the user is bigger
 	 	 	 	than the string, we add "/0" to the unused buffer (upholstring)

	**NOTICE - We still need to synchronize (maybe...)
*******************************************************************************/
ssize_t snake_read(struct file* filptr, char* buffer, size_t count, loff_t* f_pos){	//This function should be planned carefully
	printk("\t[DEBUG]\tInside snake_read\n");

	//Input Checks
	if (!count){
		return 0;
	}

	if (!buffer){
		printk("\t[DEBUG]\tInside snake_read, buffer is NULL\n");
		return -EFAULT;
	}

	int minor = ((dev_private_data *)((filptr)->private_data))->minor;

	//Legal game status checks
	if (is_played[minor] == PLAYER_LEFT){
    	return PLAYER_LEFT;
    }

	int board_print_size = 0;
	//Creating a new, temp buffer
	char* temp_buffer = kmalloc(sizeof(char)*100 ,GFP_KERNEL);
	int idx;
	for(idx=0;idx<100;idx++){
		temp_buffer[idx] = '\0';
	}

	//Extracting the board print from the game to the buffer supplied
	if (players_num[minor] == 2){				//This means it's a legit game (finished/still playing) and we can do a read
		printk("\t[DEBUG]\tInside snake_read BEFORE game_print\n");
		Game_Print(&game_matrix[minor],temp_buffer,&board_print_size);	//Calling hw3q1.c function to print the board to the temp_buffer
		printk("\t[DEBUG]\tInside snake_read AFTER game_print\n");
		//Copying the temp_buffer that we got form user space to kernel space
		int left_to_copy;
		left_to_copy = copy_to_user(buffer,temp_buffer,count);			//Copying the data to the user space
		kfree(temp_buffer);
		printk("\t[DEBUG]\tInside snake_read AFTER copy_to_user\n");
		return (count - left_to_copy);									//Returns the amount of bytes copied
	}

	printk("\t[DEBUG]\tInside snake_read Before -ENXIO\n");
	kfree(temp_buffer);
	return -ENXIO;				//Because there is no game (device) for it
}
/*******************************************************************************
 *  snake_write - Enters a move from the player to the game.
 	 	 	 	  Uses the hw3q1.c game to change the board, by calling it's
 	 	 	 	  update function.

 	 	 	 	- The synchronization in this function is to make sure that
 	 	 	 	  the two players are playing in turns, so each of them wakes
 	 	 	 	  the other up from the semaphore when they are finished making
 	 	 	 	  the move, and then he goes to sleep on the semaphore.

 	 	 	 	 **NOTICE - We may need to implement a buffer for there is a
 	 	 	 	  	  	    possibility that the player will send a few moves
 	 	 	 	  	  	    at once, and we need to handle this...
*******************************************************************************/
ssize_t snake_write(struct file* filptr, const char* buffer, size_t count, loff_t* f_pos){

	//Input Checks
	if (!count){
		printk("\t[DEBUG]\tInside snake_write, count is 0\n");
		return 0;
	}

	int player_color = ((dev_private_data *)((filptr)->private_data))->player_color;
	printk("\t[DEBUG]\tInside snake_write, platyer color is %d\n", player_color);
	int minor = ((dev_private_data *)((filptr)->private_data))->minor;

	//Legal game status checks
	if (is_played[minor] == PLAYER_LEFT){
		printk("\t[DEBUG]\tInside snake_write, PLAYER LEFT condition\n");
		return PLAYER_LEFT;
    }
	if (is_played[minor] == GAME_FINISHED){
		printk("\t[DEBUG]\tInside snake_write, GAME_FINISHED condition\n");
    	return -EACCES;
    }

    int i;
	for (i = 0; i < count; ++i) {

	    //Synchronization - Check
	    if (player_color != next_players_turn[minor]){
	    	if (player_color == WHITE_PLAYER){
				printk("\t[Write-DEBUG]\t White player just went to sleep on the white sema\n");
				white_write_sema_counter[minor]++;
		    	down_interruptible(&white_write_sema[minor]);
	    	} else {
	    		printk("\t[Write-DEBUG]\t Black player just went to sleep on the black sema\n");
	    		black_write_sema_counter[minor]++;
	    		down_interruptible(&black_write_sema[minor]);				//If it's not the player's turn he goes to sleep
	    	}
	    }

	    //We need to check again every time we wake up
		//Legal game status checks
	    //the other player performed illegal move/game was finished/null terminating string input
	    if ((buffer[i] == '\0')||(is_played[minor] == PLAYER_LEFT) ||(is_played[minor] == GAME_FINISHED)){
	    	return i;
	    }
		//We have to convert the player's color to match HW3Q1.h convention
		Player playerInGame = convert_player_color(player_color);
		int move = buffer[i] - '0';									//Loading the move from the input
	    //Legal move check
		if ( (move == DOWN) || (move == LEFT) || (move == RIGHT) || (move == UP) ){
			//The actual gameplay
			int res;
			/*TEST*/printk("\t[Write-DEBUG]\tcasting from %c to %d\n",buffer[i],move);

			//Calling the gameplay changing function with the data
			res = Game_Update(&(game_matrix[minor]),playerInGame,move,&(white_counters[minor]), &(black_counters[minor]));

			/*TEST*/printk("\t[Write-DEBUG]\tafter game_update, player color is: %d\n", player_color);

			//Only if the game has ended for some reason we enter this part of the code
			if (res != KEEP_PLAYING){
				printk("\t[Write-DEBUG]\tInside snake_write, inside 'res!=KEEP_PLAYING'\n");
				switch (res){
					case WHITE_PLAYER:
						printk("\t[Write-DEBUG]\tInside snake_write, WhitePlayer finished\n");
						is_played[minor] = GAME_FINISHED;
						game_winner[minor] = WHITE_PLAYER;
						break;

					case BLACK_PLAYER:
						printk("\t[Write-DEBUG]\tInside snake_write, BlackPlayer finished\n");
						is_played[minor] = GAME_FINISHED;
						game_winner[minor] = BLACK_PLAYER;
						break;

					default:
						printk("\t[Write-DEBUG]\tInside snake_write, TIE return value\n");
						return -EIO;		//TODO - return value in case of TIE
				}
			}

			//Synchronization - Set the next turn if the game hasn't ended yet
			if (is_played[minor] == GAME_STILL_PLAYING){
				printk("\t[Write-DEBUG]\t , in for loop - sync block,GAME_STILL_PLAYING\n");
				spin_lock(players_lock[minor]);
				if (player_color == WHITE_PLAYER){
					printk("\t[Write-DEBUG]\t, determining next turn to black\n");
					next_players_turn[minor] = BLACK_PLAYER;	//We need to change the next turn
				} else {
					printk("\t[Write-DEBUG]\t , determining next turn to white\n");
					next_players_turn[minor] = WHITE_PLAYER;
				}
				spin_unlock(players_lock[minor]);
			}

			//Waking up the other player to play his turn
			if(player_color == WHITE_PLAYER){
				printk("\t[Write-DEBUG]\t The WHITE player just woke up the Black sema -1\n");
				if (black_write_sema_counter[minor] > 0){
					black_write_sema_counter[minor]--;
					up(&black_write_sema[minor]);
				}
			} else {
				printk("\t[Write-DEBUG]\t The BLACK player just woke up the White sema -1\n");
				if (white_write_sema_counter[minor] > 0) {
					white_write_sema_counter[minor]--;
					up(&white_write_sema[minor]);
				}
			}

		} else {												//If there was an illegal input, the player who made it loses the game

			//Setting the winner
			/*TEST*/printk("\t[Write-DEBUG]\t The contents of the buffer from the input is: %c\n",buffer[i]);
			if (player_color == BLACK_PLAYER){
				game_winner[minor] = WHITE_PLAYER;
			} else {
				game_winner[minor] = BLACK_PLAYER;
			}


			//Finishing the game because of an illegal move
			is_played[minor] = GAME_FINISHED;

			//We don't want to leave the other player asleep if the game has ended
			if(player_color == WHITE_PLAYER){
				printk("\t[Write-DEBUG]\t The WHITE player just woke up the Black sema -1\n");
				if (black_write_sema_counter[minor] > 0){
					black_write_sema_counter[minor]--;
					up(&black_write_sema[minor]);
				}
			} else {
				printk("\t[Write-DEBUG]\t The BLACK player just woke up the White sema -1\n");
				if (white_write_sema_counter[minor] > 0) {
					white_write_sema_counter[minor]--;
					up(&white_write_sema[minor]);
				}
			}

			printk("\t[Write-DEBUG]\tInside snake_write, before return EPERM\n");
			return -EPERM;
		}
	}

	//In case we receive a single char
	if (count == 1){
		return 1;
	}
    return i;
}
/*******************************************************************************
 * snake_llseek - Overriding the default implementation of the OS
 	 	 	 	  This function doesn't do anything, and shouldn't be called
*******************************************************************************/
loff_t snake_llseek(struct file* filptr, loff_t irrelevant, int num){
	return -ENOSYS;																//HW4 - Lotem -  Finished...
}
/*******************************************************************************
 * snake_ioctl - Control Commands API according to the supplied header file

	SNAKE_GET_WINNER - We check if the game is still being played,
					   and only if it has ended we return the winner
					   (We return the data from the device...)

	SNAKE_GET_COLOR - We return the value from the private_data of the process
*******************************************************************************/
int snake_ioctl(struct inode* inode, struct file* filptr, unsigned int command, unsigned long var){

	int minor = MINOR(inode->i_rdev);

	//Legal game status checks
    if (is_played[minor] == PLAYER_LEFT){
    	return PLAYER_LEFT;
    }

	switch(command) {
            case SNAKE_GET_WINNER:
				if (is_played[minor] == GAME_STILL_PLAYING){					//If the game is still being played there is no winner
					return NO_WINNER_YET;
				}
				if ( (is_played[minor] == GAME_FINISHED) && (game_winner[minor] == TIE) ){
							return TIE;
				}
				/*TEST*/if (is_played[minor] == GAME_NOT_STARTED){			//Not suppose to happen...
					return -ENXIO;;
				}
				if ( (is_played[minor] == GAME_FINISHED) && !game_winner[minor]){
					return -ENXIO;							//If the game hasn't started or finished with no winner
				}
				return game_winner[minor];
                break;

            case SNAKE_GET_COLOR:
            	//If the player's color wasn't initialized, it wouldn't been in this game
            	return ((dev_private_data *)((filptr)->private_data))->player_color;
            	break;

            default: return -ENOTTY;
    }
    return 0;
}
/*******************************************************************************
  Implementing the new functions available for use with the snake Module (fops)
*******************************************************************************/
struct file_operations fops = {
        .open=          snake_open,
        .release=       snake_release,
        .read=          snake_read,
        .write=        	snake_write,											//HW4 - Lotem - Finished...
        .llseek=        snake_llseek,
        .ioctl=         snake_ioctl,
        .owner=         THIS_MODULE,
};
/*******************************************************************************
 *	int init_module - Calls a few functions to initialize the char device
*******************************************************************************/
int init_module(void){

	major = register_chrdev(0, "snake", &fops);		//Getting the major Dynamically, by sending 0 as first input param
	if (major < 0){
		printk("[DEBUG]init_module -> Failed registering character device\n");
		return major;
	}

	//Using kmallock because we don't know how many instances (games) we are going to have of the same device

    players_lock = kmalloc(sizeof(spinlock_t)*maxGames, GFP_KERNEL);		//spinlocks array
    if (!players_lock){
    	return -ENOMEM;
    }
    game_sema = kmalloc(sizeof(struct semaphore)*maxGames, GFP_KERNEL);		//Not sure about the syntax here //semaphores array
    if (!game_sema){
    	kfree(players_lock);
     	return -ENOMEM;
    }
    white_write_sema = kmalloc(sizeof(struct semaphore)*maxGames, GFP_KERNEL);	//Not sure about the syntax here //semaphores array
    black_write_sema = kmalloc(sizeof(struct semaphore)*maxGames, GFP_KERNEL);	//Not sure about the syntax here //semaphores array

    if (!white_write_sema || !black_write_sema){
    	kfree(players_lock);
    	kfree(game_sema);
     	return -ENOMEM;
    }
    players_num = kmalloc(sizeof(int)*maxGames, GFP_KERNEL);
    if (!players_num){
    	kfree(players_lock);
    	kfree(game_sema);
    	kfree(white_write_sema);
    	kfree(black_write_sema);
     	return -ENOMEM;
    }
    is_played = kmalloc(sizeof(int)*maxGames, GFP_KERNEL);
    if (!is_played){
    	kfree(players_lock);
    	kfree(game_sema);
    	kfree(white_write_sema);
    	kfree(black_write_sema);
    	kfree(players_num);
     	return -ENOMEM;
    }
    game_matrix = kmalloc(sizeof(Matrix)*maxGames, GFP_KERNEL);
    if (!game_matrix){
    	kfree(players_lock);
    	kfree(game_sema);
    	kfree(white_write_sema);
    	kfree(black_write_sema);
    	kfree(players_num);
    	kfree(is_played);
     	return -ENOMEM;
    }
    game_winner = kmalloc(sizeof(int)*maxGames, GFP_KERNEL);
    if (!game_winner){
    	kfree(players_lock);
    	kfree(game_sema);
    	kfree(white_write_sema);
    	kfree(black_write_sema);
    	kfree(players_num);
    	kfree(is_played);
    	kfree(game_matrix);
     	return -ENOMEM;
    }
    next_players_turn = kmalloc(sizeof(int)*maxGames, GFP_KERNEL);
    if (!next_players_turn){
    	kfree(players_lock);
    	kfree(game_sema);
    	kfree(white_write_sema);
    	kfree(black_write_sema);
    	kfree(players_num);
    	kfree(is_played);
    	kfree(game_matrix);
    	kfree(game_winner);
     	return -ENOMEM;
    }

    white_counters = kmalloc(sizeof(int)*maxGames, GFP_KERNEL);
    black_counters = kmalloc(sizeof(int)*maxGames, GFP_KERNEL);
      if (!white_counters || !black_counters){
      	kfree(players_lock);
      	kfree(game_sema);
    	kfree(white_write_sema);
    	kfree(black_write_sema);
    	kfree(players_num);
      	kfree(is_played);
      	kfree(game_matrix);
      	kfree(game_winner);
      	kfree(next_players_turn);
       	return -ENOMEM;
      }

      white_write_sema_counter = kmalloc(sizeof(int)*maxGames, GFP_KERNEL);
      black_write_sema_counter = kmalloc(sizeof(int)*maxGames, GFP_KERNEL);
        if (!white_write_sema_counter || !black_write_sema_counter){
        	kfree(players_lock);
        	kfree(game_sema);
        	kfree(white_write_sema);
        	kfree(black_write_sema);
        	kfree(players_num);
        	kfree(is_played);
        	kfree(game_matrix);
        	kfree(game_winner);
        	kfree(next_players_turn);
        	kfree(white_counters);
        	kfree(black_counters);
         	return -ENOMEM;
        }


    //   -- Should we allocate each array as the size of the maxGames we get in the input?

	int i;
	for (i=0;i<maxGames;i++){
		spin_lock_init(&players_lock[i]);
		is_played[i] = GAME_NOT_STARTED;
		game_winner[i] = GAME_NOT_STARTED;
		players_num[i] = 0;
		next_players_turn[i] = WHITE_PLAYER;		//According to the PDF the white player starts first
        sema_init(&game_sema[i], 0);				//We initialize the semaphores with 0 so a player will immediately go to sleep
        sema_init(&white_write_sema[i], 0);
        white_write_sema_counter[i]=0;
        sema_init(&black_write_sema[i], 0);
        black_write_sema_counter[i]=0;
        white_counters[i] = K;
        black_counters[i] = K;

	}

	/*Test*/printk("Hello, World!\n");

	return 0;
}
/*******************************************************************************
 * void cleanup_module -  same code as the function from the tutorials
*******************************************************************************/
void cleanup_module(void){
	int retval = unregister_chrdev(major, "snake");
    if (retval < 0){															//HW4 - Lotem - Finished...
    	printk("cleanup_module -> Failed unregistering character device\n");
    }

    //Freeing all of the arrays
    kfree(players_lock);
    kfree(game_sema);
    kfree(white_write_sema);
    kfree(black_write_sema);
    kfree(players_num);
    kfree(is_played);
    kfree(game_winner);
    kfree(game_matrix);
    kfree(next_players_turn);
    kfree(white_counters);
    kfree(black_counters);


    /*Test*/printk("Goodbye, Cruel World!\n");
    return;
}
