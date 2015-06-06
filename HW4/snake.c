#include <linux/errno.h>
#include <linux/module.h>
#include <linux/fs.h>

#include <linux/kernel.h>
#include <linux/sched.h>

//#include <linux/spinlock.h>
//#include <asm/semaphore.h>
//#include <linux/wait.h>


#include "snake.h"
MODULE_LICENSE("GPL");

/*******************************************************************************
		Variables to be used in the implementation of the new module
*******************************************************************************/
static int major = -1;
struct file_operations fops;
int maxGames = 0;

MODULE_PARM(maxGames,"i");	//This is the only input needed for the module
							//But still it is unclear how we receive this param
							//(maybe in the makefile...)
/******************************************************************************/



/*******************************************************************************
 	 	 Implementations of the snake module fops functions - Not ready yet
*******************************************************************************/
int snake_open(struct inode* inode, struct file* filp){
	return 0;
}
int snake_release(struct inode* inode, struct file* filp){
	return 0;
}
ssize_t snake_read(struct file* filp, char* buffer, size_t count, loff_t* f_pos){	//This function should be planned carefully
	return 0;
}
ssize_t snake_write(struct file* filp, const char* buffer, size_t count, loff_t* f_pos){	//This function should be planned carefully
	return 0;
}
/*******************************************************************************
 * snake_llseek - Overriding the default implementation of the OS
 	 	 	 	  This function doesn't do anything, and shouldn't be called		//HW4 - Lotem -  Finished...
*******************************************************************************/
loff_t snake_llseek(struct file* filp, loff_t irrelevant, int num){
	return -ENOSYS;
}
/*******************************************************************************
 * snake_ioctl - Control Commands API according to the supplied header file
*******************************************************************************/
int snake_ioctl(struct inode* inode, struct file* filp, unsigned int command, unsigned long var){
    switch(command) {
            case SNAKE_GET_WINNER:
//            	((device_private_data *)((filp)->private_data))->private_key = arg;
                break;
            case SNAKE_GET_COLOR:
            	break;
            default: return -ENOTTY;
    }
    return 0;
}
/*******************************************************************************
  Implementing the new functions available for use with the snake Module (fops)		//HW4 - Lotem - Done except last line's syntax
*******************************************************************************/
struct file_operations fops = {
        .open=          snake_open,
        .release=       snake_release,
        .read=          snake_read,
        .write=        	snake_write,
        .llseek=        snake_llseek,
        .ioctl=         snake_ioctl,
        .owner=         THIS_MODULE,	//Not sure about this part of the implementation,because we are using the SET_MODULE_OWNER macro for this in init_module
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
	SET_MODULE_OWNER(&fops);	//Not sure if it's needed

	/*Test*/printk("Hello, World!\n");
	return 0;
}
/*******************************************************************************
 * void cleanup_module -  same code as the function from the tutorials				//HW4 - Lotem - Finished...
*******************************************************************************/
void cleanup_module(void){
	int retval = unregister_chrdev(major, "snake");
    if (retval < 0){
    	printk("cleanup_module -> Failed unregistering character device\n");
    }

    /*Test*/printk("Goodbye, Cruel World!\n");
    return;
}
