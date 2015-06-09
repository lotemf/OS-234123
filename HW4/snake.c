#include <linux/errno.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>         // for using the kmalloc function
#include <linux/spinlock.h>
#include <linux/kernel.h>
#include <linux/sched.h>

//#include <linux/spinlock.h>
//#include <asm/semaphore.h>
//#include <linux/wait.h>

#include "hw3q1.c"
#include "snake.h"
MODULE_LICENSE("GPL");

#define SNAKE_ERROR -1
#define WHITE_COLOR 4						//for the color checks
#define BLACK_COLOR 2
/*******************************************************************************
		Variables to be used in the implementation of the new module
*******************************************************************************/
static int major = -1;
struct file_operations fops;
int maxGames = 0;
int* players_num;
int* is_played;							//A flag that notes if the game started or not
int* black_player_pid;
int* white_player_pid;	//Every game hold the pids of the players (of the two processes)

//Locks
struct semaphore* game_sema;
spinlock_t* players_lock;				//The spinlock is used to check the legal number of players for each device (which is 2 players)

//Wait queue for the device, so if one of the players is playing the other one is in the wait queue
wait_queue_head_t* wait_queue;


MODULE_PARM(maxGames,"i");	//This is the only input needed for the module
							//But still it is unclear how we receive this param
							//(maybe in the makefile...)



typedef struct {
    int minor;				//In the char Device based on this model
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

    dev_private_data* dev_p_data = (device_private_data*)(filp->private_data);
    dev_p_data->minor = minor;
//    dev_p_data->max_games = maxGames;

    spin_lock(players_lock[minor]);
    if (!players_num[minor]){		//This means this player should get the white color
    	players_num[minor]++;
    	white_player_pid[minor]=getpid();				//set white color for this player
    	//** make this device wait on the semaphore
    } else if (players_num[minor] == 1){
    	players_num[minor]++;
    	dev_p_data->black_player_pid=getpid();				//set black color for this player
    	//** make this device free the semaphore (using signal or up or whatever...)
    	up(&game_sema[minor]);
    	return 0;					//We return from the function because we wouldn't like to make another wait on the semaphore
    }
    spin_unlock(players_lock[minor]);

    down_interruptible(&game_sema[minor]);					//We wait on the semaphore
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
		//Maybe we need to add some more lines here after the changes in snake_open
	spin_unlock(players_lock[minor]);

	kfree(filp->private_data);

	return 0;
}
/*******************************************************************************
 * snake_read - This function prints the board, using the buffer supplied.
 	 	 	 	If a partial print was performed, we need to edit the buffer
 	 	 	 	so the output of it will be a string (add "/0");


*******************************************************************************/
ssize_t snake_read(struct file* filp, char* buffer, size_t count, loff_t* f_pos){	//This function should be planned carefully
	//Checking it's a legal game
	int minor = ((dev_private_data *)((filp)->private_data))->minor;

	if ( (players_num[minor] == 2) && (is_played[minor]) ){		//This means it's a legit game and we can do a read
		//Use alon's Function to print the board but handle the synchronization so the second player couldn't write to the board
		//Make him change the print function in the hw3q1 so it will return a erady buffer

		char* new_buffer;
		//Get the length of the function using strlen
		//and then use copy to user...
	}
	return 0;
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
*******************************************************************************/
int snake_ioctl(struct inode* inode, struct file* filp, unsigned int command, unsigned long var){
	int minor = MINOR(inode->i_rdev);

	switch(command) {
            case SNAKE_GET_WINNER:
				//We still need to understand how to return the winner using the hw3q1.c prog
            	//maybe we need to make a new field in the private data struct
                break;
            case SNAKE_GET_COLOR:
				if (getpid() == black_player_pid[minor]){
					return BLACK_COLOR;
				} else if (getpid() == white_player_pid[minor]){
					return WHITE_COLOR;
				} else {
					return -ENOTTY;
				}
            	//maybe we need to make a new field in the private data struct
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

	//I think we need to allocate all of the memory for the vars manually
	//using kmallock because we don't know how many instances we are going to have of the same device
    players_lock = kmalloc(sizeof(spinlock_t)*maxGames, GFP_KERNEL);		//spinlocks array
    game_sema = kmalloc(sizeof(struct semaphore)*maxGames, GFP_KERNEL);		//Not sure about the syntax here //semaphores array
    players_num = kmalloc(sizeof(int)*maxGames, GFP_KERNEL);
    is_played = kmalloc(sizeof(int)*maxGames, GFP_KERNEL);
    white_player_pid = kmalloc(sizeof(int)*maxGames, GFP_KERNEL);
    black_player_pid = kmalloc(sizeof(int)*maxGames, GFP_KERNEL);

	//   -- Should we allocate each array as the size of the maxGames we get in the input?

	int i;
	for (i=0;i<maxGames;i++){
		spin_lock_init(&players_lock[i]);
		is_played[i] = 0;
		players_num[i] = 0;
		black_player_pid[i] = -1;
		white_player_pid[i] = -1;
        sema_init(&game_sema[i], 1);

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
    kfree(players_num);
    kfree(is_played);
    kfree(black_player_pid);
    kfree(white_player_pid);

    /*Test*/printk("Goodbye, Cruel World!\n");
    return;
}
