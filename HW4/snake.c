#include <linux/errno.h>
#include <linux/module.h>
#include <linux/fs.h>

#include <linux/kernel.h>
#include <linux/sched.h>

//#include <linux/spinlock.h>
//#include <asm/semaphore.h>
//#include <linux/wait.h>

#include "hw3q1.c"
#include "snake.h"
MODULE_LICENSE("GPL");

#define WHITE 1						//for the color checks
#define BLACK 2
/*******************************************************************************
		Variables to be used in the implementation of the new module
*******************************************************************************/
static int major = -1;
struct file_operations fops;
int maxGames = 0;

int* players_num;
spinlock_t* players_lock;				//The spinlock is used to check the legal number of players for each device (which is 2 players)

//Wait queue for the device, so if one of the players is playing the other one is in the wait queue
wait_queue_head_t* wait_queue;



MODULE_PARM(maxGames,"i");	//This is the only input needed for the module
							//But still it is unclear how we receive this param
							//(maybe in the makefile...)

typedef struct {
    int minor;			//In the char Device based on this model
	int max_games;		//every device has it's own maxGames amount and minor (maybe more fields...)
} dev_private_data;

/******************************************************************************/



/*******************************************************************************
 * snake_open - The implementation is as shown in the video tutorial (20) by
  	  	  	    Leonid, with an addition - a check of the amount of players
  	  	  	    permitted for this device (2)

  	  	  	    **Notice - Not finished yet, the "Under Construction" part
  	  	  	    		   of the code is in pseudo-code as comment inside
  	  	  	    		   the spinlock...
*******************************************************************************/
int snake_open(struct inode* inode, struct file* fileptr){
	fileptr->f_op = &fops;
    int minor = MINOR(inode->i_rdev);										//Extracting the minor from the device

    fileptr->private_data = kmalloc(sizeof(dev_private_data), GFP_KERNEL);	//Allocating the memory for the struct using a kernel alloc
    if (!(fileptr->private_data)){
    	return -ENOMEM;
    }

    dev_private_data* dev_p_data = (device_private_data*)(filp->private_data);
    dev_p_data->minor = minor;
    dev_p_data->max_games = maxGames;

    spin_lock(players_lock[minor]);
    if (!players_num[minor]){		//This means this player should get the
    	players_num[minor]++;
    	//set white color for this player
    	//block the usage of this device until black player arrives
    	//maybe using a semaphore...
    } else if (players_num[minor] == 1){
    	players_num++;
    	//set black color for this player
    	//unblock the usage of this device
    	//maybe using a semaphore...
    } else {
    	return -EPERM;		//because there are already 2 players, so it's not permitted
    }
    spin_unlock(players_lock[minor]);

	return 0;
}
/*******************************************************************************
 * snake_release -
*******************************************************************************/
int snake_release(struct inode* inode, struct file* filp){

	kfree(filp->private_data);
	int minor = get_minor_from_inode(inode);
	//Update counters:
	spin_lock(players_lock[minor]);
	players_num[minor]--;
	wake_up_interruptible(&wait_queue[minor]);
		//Maybe we need to add some more lines here after the changes in snake_open
	spin_unlock(players_lock[minor]);
	return 0;
}
/*******************************************************************************
 * snake_read -
*******************************************************************************/
ssize_t snake_read(struct file* filp, char* buffer, size_t count, loff_t* f_pos){	//This function should be planned carefully
	return 0;
}
/*******************************************************************************
 *  snake_write -
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
    switch(command) {
            case SNAKE_GET_WINNER:
				//We still need to understand how to return the winner
            	//maybe we need to make a new field in the private data struct
                break;
            case SNAKE_GET_COLOR:
				//We still need to understand how to return the current player's color
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

	//   -- Should we allocate each array as the size of the maxGames we get in the input?

	int i;
	for (i=0;i<maxGames;i++){
		spin_lock_init(&players_lock[i]);
		players_num[i] = 0;

		//still need to add some more variables here, and decide about the malloc
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

    /*Test*/printk("Goodbye, Cruel World!\n");
    return;
}
