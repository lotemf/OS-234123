#include <linux/errno.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/slab.h>         // for using the kmalloc function
#include <linux/spinlock.h>
#include <linux/kernel.h>
#include <linux/sched.h>

//#include <linux/spinlock.h>
//#include <asm/semaphore.h>
//#include <linux/wait.h>

#include "hw3q1.h"
#include "snake.h"
MODULE_LICENSE("GPL");

#define SNAKE_ERROR -1
#define WHITE_COLOR 4						//for the color checks
#define BLACK_COLOR 2
/*******************************************************************************
		Variables to be used in the implementation of the new module
*******************************************************************************/
/*TEST*/
static int major = -1;
struct file_operations fops;
int maxGames = 0;
Matrix* game_matrix;
int* players_num;
int* is_played;							//A flag that notes if the game started or not
//int* black_player_pid;
//int* white_player_pid;	//Every game hold the pids of the players (of the two processes)

//Locks
struct semaphore* game_sema, *write_sema;
spinlock_t* players_lock;				//The spinlock is used to check the legal number of players for each device (which is 2 players)

MODULE_PARM(maxGames,"i");	//This is the only input needed for the module
							//But still it is unclear how we receive this param
							//(maybe in the makefile...)



typedef struct {
    int minor;				//In the char Device based on this model
    int player_color;
//	int max_games;			//every device has it's own maxGames amount and minor (maybe more fields...)
} dev_private_data;

/******************************************************************************/



/*******************************************************************************
 * snake_open - This function opens a new game on the Snake device.
 	 	 	 	The process that opens the device will get an automatic
 	 	 	 	white color if it's the first process, or automatically a black
 	 	 	 	color if it's the second process.

 	 	 	 	If a process is the first to call open on this device
 	 	 	 	for the game, it should wait on the semaphore until another
 	 	 	 	process calls open, and then it's GAME ON.


 	 	 	 	P.S. - The way to contact this specific game for the two processes
 	 	 	 	that are playing in it, will be through the Minor num when
 	 	 	 	"talking" to the device driver.
*******************************************************************************/
int snake_open(struct inode* inode, struct file* fileptr){
	fileptr->f_op = &fops;
    int minor = MINOR(inode->i_rdev);										//Extracting the minor from the device

    if (players_num[minor] == 2){
    	return -EPERM;						//We check if this game is full...
    }

    fileptr->private_data = kmalloc(sizeof(dev_private_data), GFP_KERNEL);	//Allocating the memory for the struct using a kernel alloc
    if (!(fileptr->private_data)){
    	return -ENOMEM;
    }

    dev_private_data* dev_p_data = (dev_private_data*)(fileptr->private_data);
    dev_p_data->minor = minor;

    spin_lock(players_lock[minor]);
    if (!players_num[minor]){		//This means this player should get the white color
    	players_num[minor]++;
        dev_p_data->player_color = WHITE_COLOR;
    	//** make this device wait on the semaphore
    } else if (players_num[minor] == 1){
    	players_num[minor]++;
    	dev_p_data->player_color = BLACK_COLOR;
    	up(&game_sema[minor]);		//Waking up the other player process from the semaphore
    	is_played[minor] = 1;		//Setting the flag for the game start
    	return 0;					//We return from the function because we wouldn't like to make another wait on the semaphore
    }
    spin_unlock(players_lock[minor]);

    if (!is_played[minor]){
    	down_interruptible(&game_sema[minor]);					//We wait on the semaphore
    } else {
    	return Game_Init(&game_matrix[minor]);	//Tell Alon to match this signature in HW3Q1.c...
    }

    return 0;
}
/*******************************************************************************
 * snake_release - Freeing the allocated private data, and reducing the amount
 	 	 	 	   of players in the game so when we check the write/read
 	 	 	 	   values we know if it's a legal call or not
*******************************************************************************/
int snake_release(struct inode* inode, struct file* filp){

	int minor = MINOR(inode->i_rdev);

	//Update counters:
	spin_lock(players_lock[minor]);
	players_num[minor]--;
	spin_unlock(players_lock[minor]);

	kfree(filp->private_data);

	return 0;
}
/*******************************************************************************
 * snake_read - This function prints the board, using the buffer supplied.
 	 	 	 	If the buffer that was supplied to us by the user is bigger
 	 	 	 	than the string, we add "/0" to the unused buffer;

	**NOTICE - We still need to synchronize (maybe...)
*******************************************************************************/
ssize_t snake_read(struct file* filp, char* buffer, size_t count, loff_t* f_pos){	//This function should be planned carefully
	//Checking it's a legal game
	int minor = ((dev_private_data *)((filp)->private_data))->minor;
	int board_print_size;
	char* temp_buffer = kmalloc(sizeof(char)*count ,GFP_KERNEL);					//Creating a new buffer

	if ( (players_num[minor] == 2) && (is_played[minor]) ){				//This means it's a legit game and we can do a read
		Game_Print(&game_matrix[minor],temp_buffer,board_print_size);	//Calling hw3q1.c function to print the board to the temp_buffer

		int need_upholster = count - board_print_size;
		int i;
		for (i=0;i<need_upholster;i++){
			temp_buffer[board_print_size + i] = '\0';					//Adding /0 for the unused spaces in the buffer
		}

		int left_to_copy;
		left_to_copy = copy_to_user(temp_buffer,buffer,count);			//Copying the data to the user space
		kfree(temp_buffer);
		return (count - left_to_copy);									//Returns the amount of bytes copied
	}

	return -ENXIO;														//Because there is no game (device) for it
}
/*******************************************************************************
 *  snake_write - This is the complex part because we need to synchronize
 	 	 	 	  the writes between the two processes using  new semaphore
 	 	 	 	  maybe we will call it write_sema...
 	 	 	 	  and we might need a new spinlock for the update of the
 	 	 	 	  write...  //Talk to chen about it
*******************************************************************************/
ssize_t snake_write(struct file* filp, const char* buffer, size_t count, loff_t* f_pos){	//This function should be planned carefully

	return 0;
}
/*******************************************************************************
 * snake_llseek - Overriding the default implementation of the OS
 	 	 	 	  This function doesn't do anything, and shouldn't be called
*******************************************************************************/
loff_t snake_llseek(struct file* filp, loff_t irrelevant, int num){
	return -ENOSYS;																//HW4 - Lotem -  Finished...
}
/*******************************************************************************
 * snake_ioctl - Control Commands API according to the supplied header file

 	 **NOTICE - We still need to understand how to use the GET_WINNER func
*******************************************************************************/
int snake_ioctl(struct inode* inode, struct file* filp, unsigned int command, unsigned long var){
//	int minor = MINOR(inode->i_rdev);
	int color;

	switch(command) {
            case SNAKE_GET_WINNER:
				//We still need to understand how to return the winner using the hw3q1.c prog
            	//maybe we need to make a new field in the private data struct
                break;
            case SNAKE_GET_COLOR:
            	color = ( (dev_private_data *)((filp)->private_data) )->player_color;
            	if (color == -1 ){				//If the player's color wasn't initialized
            		return -ENXIO;
            	}
            	return color;
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
		printk("init_module -> Failed registering character device\n");
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
    write_sema = kmalloc(sizeof(struct semaphore)*maxGames, GFP_KERNEL);	//Not sure about the syntax here //semaphores array
    if (!write_sema){
    	kfree(players_lock);
    	kfree(game_sema);
     	return -ENOMEM;
    }
    players_num = kmalloc(sizeof(int)*maxGames, GFP_KERNEL);
    if (!players_num){
    	kfree(players_lock);
    	kfree(game_sema);
    	kfree(write_sema);
     	return -ENOMEM;
    }
    is_played = kmalloc(sizeof(int)*maxGames, GFP_KERNEL);
    if (!is_played){
    	kfree(players_lock);
    	kfree(game_sema);
    	kfree(write_sema);
    	kfree(players_num);
     	return -ENOMEM;
    }
    game_matrix = kmalloc(sizeof(Matrix)*maxGames, GFP_KERNEL);
    if (!game_matrix){
    	kfree(players_lock);
    	kfree(game_sema);
    	kfree(write_sema);
    	kfree(players_num);
    	kfree(is_played);
     	return -ENOMEM;
    }
	//   -- Should we allocate each array as the size of the maxGames we get in the input?

	int i;
	for (i=0;i<maxGames;i++){
		spin_lock_init(&players_lock[i]);
		is_played[i] = 0;
		players_num[i] = 0;
        sema_init(&game_sema[i], 1);
        sema_init(&write_sema[i], 1);

		//still need to add some more variables here
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
    kfree(write_sema);
    kfree(players_num);
    kfree(is_played);
    kfree(game_matrix);

    /*Test*/printk("Goodbye, Cruel World!\n");
    return;
}
