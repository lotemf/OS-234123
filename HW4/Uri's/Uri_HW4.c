/*
 * hw4.c
 *
 *  Created on: 7 αιπε 2015
 *      Author: Yoav
 */
#include <linux/errno.h>
#include <linux/module.h>
#include <asm/semaphore.h>
#include <linux/fs.h> //for struct file_operations
#include <linux/kernel.h> //for printk()
#include <asm/uaccess.h> //for copy_from_user()
#include <linux/sched.h>
#include <linux/slab.h>         // for kmalloc
#include <linux/spinlock.h>
#include <linux/wait.h>
#include "hw4.h"
#include "encryptor.h"
MODULE_LICENSE("GPL");

#define BUF_SIZE (4*1024)       /* The buffer size, and also the longest message */
#define ZERO 0
#define ONE 1
#define TWO 2
#define EOF -1

#define MY_MODULE "hw4"
#define MIN(a,b) ((a) < (b) ? a : b)

/***********************VARIABLES***********************************/
static int major = -1;
struct file_operations fops;
int iKey = 0;

char buffer[2][BUF_SIZE];
int reading_position[2];
int writing_position[2];
int num_of_readers[2];
int num_of_writers[2];
int flag_is_full[2];
int flag_is_empty[2];
spinlock_t counters_lock[2];

//Queues
wait_queue_head_t read_wq[2];
wait_queue_head_t write_wq[2];

//Locks
struct semaphore
        read_lock[2],
        write_lock[2],
        index_lock[2];

MODULE_PARM(iKey,"i");

typedef struct
{
        int private_key;
        int minor;
} device_private_data;


static int get_minor_from_inode(struct inode* inode)
{
        return MINOR(inode->i_rdev);
}

static int check_buffer_size(int size)
{
        int mod = size % 8;                             //count is not legal!
        if (mod != 0 || size < 0)
        {
                return 1;
        }
        return 0;
}

static int get_minor_from_file(struct file *filp)
{
        return ((device_private_data *)((filp)->private_data))->minor;
}

static int is_reader(struct file* filp)
{
        if ((filp->f_mode) & FMODE_READ)
        {
                return 1;
        }
        return 0;
}

static int is_writer(struct file* filp)
{
        if ((filp->f_mode) & FMODE_WRITE)
        {
                return 1;
        }
        return 0;
}

//**************************************************************************
void cleanup_module(void)
{
        int ret = unregister_chrdev(major, MY_MODULE);
        if(ret < 0)
        {
                printk(KERN_ALERT "Error in unregister_chrdev: %d\n", ret);
        }
        return;
}




//**************************************************************************
int my_open(struct inode* inode, struct file* filp)
{
        filp->f_op = &fops;
        int minor = get_minor_from_inode(inode);

        filp->private_data = kmalloc(sizeof(device_private_data), GFP_KERNEL);
        if (filp->private_data == NULL)
        {
                return -ENOMEM;
        }
        device_private_data* data = (device_private_data*)(filp->private_data);
        data->minor = minor;
        data->private_key = iKey;

        //Update counters:
        spin_lock(counters_lock[minor]);
        if (is_reader(filp))
        {
                num_of_readers[minor]++;
        }
        if (is_writer(filp))
        {
                num_of_writers[minor]++;
        }
        spin_unlock(counters_lock[minor]);
        return 0;
}

static int get_current_num_of_readers(int minor)
{
        int result;
        spin_lock(counters_lock[minor]);
        result = num_of_readers[minor];
        spin_unlock(counters_lock[minor]);
        return result;
}

static int get_current_num_of_writers(int minor)
{
        int result;
        spin_lock(counters_lock[minor]);
        result = num_of_writers[minor];
        spin_unlock(counters_lock[minor]);
        return result;
}

//**************************************************************************
int my_release(struct inode* inode, struct file* filp)
{
        kfree(filp->private_data);
        int minor = get_minor_from_inode(inode);
        //Update counters:
        spin_lock(counters_lock[minor]);
        if ((filp->f_mode) & FMODE_READ)
        {
                num_of_readers[minor]--;
                wake_up_interruptible(&write_wq[minor]);
        }
        if ((filp->f_mode) & FMODE_WRITE)
        {
                num_of_writers[minor]--;
                wake_up_interruptible(&read_wq[minor]);
        }
        spin_unlock(counters_lock[minor]);
        return 0;
}

int get_max_to_read(int minor){
        int maxToRead = 0;
        if(flag_is_full[minor] == 0){
                        maxToRead = (((writing_position[minor] - reading_position[minor]) >= 0) ?
                                (writing_position[minor] - reading_position[minor]) :
                                (writing_position[minor] - reading_position[minor]) + BUF_SIZE);
        } else if(flag_is_full[minor] == 1){
                        maxToRead = BUF_SIZE;
        }
        return maxToRead;
}

/*
 * Alon: this function should simply read the data on the given file,
 *               copy it, and return it to the user as it is (encrypted or not).
 *               The function's return value is the total amount of bytes read from the
 *               file, or -1 if the function failed.
 */
ssize_t my_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
        if (check_buffer_size(count) != 0)
        {
                return -EINVAL;
        }

        int minor = get_minor_from_file(filp);
        int is_this_process_writer = is_writer(filp);
        int key = ((device_private_data *)((filp)->private_data))->private_key;

        down_interruptible(&read_lock[minor]);
        down_interruptible(&index_lock[minor]);

        int maxToRead = get_max_to_read(minor);
        if (maxToRead == 0)
        {
                int current_num_of_writers = get_current_num_of_writers(minor);
                if(current_num_of_writers == is_this_process_writer)
                {
                        up(&index_lock[minor]);
                        up(&read_lock[minor]);
                        return 0;
                }
                //Going to sleep until something is written into the buffer
                up(&index_lock[minor]);
                int wake_up_reason = wait_event_interruptible(read_wq[minor],
                                flag_is_empty[minor] == 0 || get_current_num_of_writers(minor) == is_this_process_writer);
                if(wake_up_reason != 0)
                {
                        up(&read_lock[minor]);
                        return -EINTR;
                }
                if (get_current_num_of_writers(minor) == is_this_process_writer)
                {
                        up(&read_lock[minor]);
                        return 0;
                }
                down_interruptible(&index_lock[minor]);
                maxToRead = get_max_to_read(minor);
        }

        int numToRead = MIN(maxToRead,count);
        int firstPartSize = 0;
        int retval = 0;
        char* tmpBuf =(char*)kmalloc(sizeof(char)*numToRead,GFP_KERNEL);
        if (!tmpBuf){
                up(&index_lock[minor]);
                up(&read_lock[minor]);
                return -ENOMEM;
        }
        int numOfParts = ( (reading_position[minor] + numToRead) > BUF_SIZE) ? TWO : ONE ;
        char* source = tmpBuf;
        if(numOfParts == ONE )
        {
                if (minor == 0)
                {
                        encryptor(&buffer[minor][reading_position[minor]], tmpBuf, numToRead, key, minor);
                }
                else if(minor == 1){            // encryptor
                        source = &buffer[minor][ reading_position[minor] ];
                }
                retval = copy_to_user(buf, source, numToRead) ? -EFAULT : 0;
                reading_position[minor] = (reading_position[minor] + numToRead) % BUF_SIZE;
        }
        else
        {
                firstPartSize = BUF_SIZE - reading_position[minor];
                if (minor == 0)
                {
                        encryptor(&buffer[minor][ reading_position[minor] ], tmpBuf, firstPartSize, key, minor);
                        encryptor(&buffer[minor][0],tmpBuf+firstPartSize , numToRead - firstPartSize, key, minor);
                }
                else if (minor == 1)
                {
                        memcpy(tmpBuf, &buffer[minor][ reading_position[minor] ], firstPartSize);
                        memcpy(tmpBuf+firstPartSize, &buffer[minor][0], numToRead - firstPartSize);
                }
                retval = copy_to_user(buf, tmpBuf, numToRead);
                reading_position[minor] = numToRead - firstPartSize;
        }
        kfree(tmpBuf);
        if(retval != 0){
                up(&index_lock[minor]);
                up(&read_lock[minor]);
                return retval;
        }
        if((writing_position[minor] == reading_position[minor]) && numToRead){
                flag_is_empty[minor] = 1;
        }
        if(numToRead != 0){                             // if we have read SOMETHING, the buffer is not full anymore
                flag_is_full[minor] = 0;
                wake_up_interruptible(&write_wq[minor]);        //Notifies any waiting writers that there is now room within the buffer
        }

        up(&index_lock[minor]);
        up(&read_lock[minor]);
        return numToRead;
}


int get_max_to_write(int minor){
        int maxToWrite = 0;
        if(flag_is_full[minor] == 0){
                maxToWrite = ( writing_position[minor] - reading_position[minor] >= 0) ?
                                        ( BUF_SIZE - (writing_position[minor] - reading_position[minor]) ) :
                                        ( reading_position[minor] - writing_position[minor] );
        } else if(flag_is_full[minor] == 1)
        {
                maxToWrite = 0;
        }
        return maxToWrite;
}

/*
 * Alon: This function should write the buffer given by the user into the file.
 *               The function assumes the given buffer is encrypted, and will therefor
 *               use the iKey to decypher each character before writing it to the file.
 *               The function returns the amount of bytes it managed to write into the
 *               file, and -1 in case of failure.
 */
ssize_t my_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
        if(check_buffer_size(count) != 0)
        {
                return -EINVAL;
        }

        int minor = get_minor_from_file(filp);
        int is_this_process_reader = is_reader(filp);
        int key = ((device_private_data *)((filp)->private_data))->private_key;

        down_interruptible(&write_lock[minor]);
        down_interruptible(&index_lock[minor]);

        int maxToWrite = get_max_to_write(minor);
        if (maxToWrite == 0)
        {
                int current_num_of_readers = get_current_num_of_readers(minor);
                if(current_num_of_readers == is_this_process_reader)
                {
                        up(&index_lock[minor]);
                        up(&write_lock[minor]);
                        return 0;
                }
                //Going to sleep until a reader clears some room in the buffer
                up(&index_lock[minor]);
                int wake_up_reason = wait_event_interruptible(write_wq[minor],
                                flag_is_full[minor] == 0 || get_current_num_of_readers(minor) == is_this_process_reader);
                if(wake_up_reason != 0)
                {
                        up(&write_lock[minor]);
                        return -EINTR;
                }
                if (get_current_num_of_readers(minor) == is_this_process_reader)
                {
                        up(&write_lock[minor]);
                        return 0;
                }
                down_interruptible(&index_lock[minor]);
                maxToWrite = get_max_to_write(minor);
        }

        int numToWrite = MIN(maxToWrite,count);
        int firstPartSize = 0;
        int retval = 0;
        char* tmpBuf =(char*)kmalloc(sizeof(char)*numToWrite,GFP_KERNEL);
        if (!tmpBuf){
                up(&index_lock[minor]);
                up(&write_lock[minor]);
                return -ENOMEM;
        }
        retval = copy_from_user(tmpBuf, buf, numToWrite); //copy the data from user
        if(retval != 0){
                kfree(tmpBuf);
                up(&index_lock[minor]);
                up(&write_lock[minor]);
                return retval;
        }

        int numOfParts = ( (writing_position[minor] + numToWrite) > BUF_SIZE) ? TWO : ONE ;
        if(numOfParts == ONE )
        {
                if (minor == 1)
                {
                        encryptor(tmpBuf, &buffer[minor][writing_position[minor]], numToWrite, key, minor);
                }
                else if(minor == 0){
                        memcpy(&buffer[minor][ writing_position[minor] ], tmpBuf, numToWrite);
                }
                writing_position[minor] = (writing_position[minor] + numToWrite) % BUF_SIZE;
        }
        else
        {
                firstPartSize = BUF_SIZE - writing_position[minor];
                if (minor == 1)
                {
                        encryptor(tmpBuf, &buffer[minor][writing_position[minor]], firstPartSize, key, minor);
                        encryptor(tmpBuf + firstPartSize, &buffer[minor][0], numToWrite - firstPartSize, key, minor);
                }
                else if (minor == 0)
                {
                        memcpy(&buffer[minor][writing_position[minor]], tmpBuf, firstPartSize);
                        memcpy(&buffer[minor][0], tmpBuf + firstPartSize, numToWrite - firstPartSize);
                }
                writing_position[minor] = (numToWrite - firstPartSize);
        }

        if(( writing_position[minor] == reading_position[minor]) && numToWrite){
                flag_is_full[minor] = 1;
        }
        if (numToWrite)
        {
                flag_is_empty[minor] = 0;
                wake_up_interruptible(&read_wq[minor]);
        }
        /*
         * If we wrote something into the buffer, this makes sure to wake up
         * any writer who may be waiting for input.
         */

        kfree(tmpBuf);
        up(&index_lock[minor]);
        up(&write_lock[minor]);
        if(retval != 0){
                return retval;
        }
        return numToWrite;
}


loff_t my_llseek(struct file *filp, loff_t a, int num) {
        return -ENOSYS;
}

int my_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) {
        switch(cmd) {
                case HW4_SET_KEY:
                        ((device_private_data *)((filp)->private_data))->private_key = arg;
                        break;

                default: return -ENOTTY;
        }
        return 0;
}

//================== INITIALIZATION AND DESTRUCTION =======================
struct file_operations fops = {         //Alon: FOPS for minor=0
        .open=          my_open,
        .release=       my_release,
        .read=          my_read,
        .write=         my_write,
        .llseek=                my_llseek,
        .ioctl=         my_ioctl,
        .owner=         THIS_MODULE,
};


//**************************************************************************
int init_module(void)
{
        int i;
        major = register_chrdev(ZERO, MY_MODULE, &fops);

        if (major < 0)
        {
                printk(KERN_ALERT "Registering char device failed with %d\n", major);
                return major;
        }

        for(i=0; i<2; i++)
        {
                reading_position[i] = 0;
                writing_position[i] = 0;
                num_of_readers[i] = 0;
                num_of_writers[i] = 0;
                flag_is_full[i] = 0;
                flag_is_empty[i] = 1;
                spin_lock_init(&counters_lock[i]);
                init_waitqueue_head(&read_wq[i]);
                init_waitqueue_head(&write_wq[i]);
                sema_init(&read_lock[i], 1);
                sema_init(&write_lock[i], 1);
                sema_init(&index_lock[i], 1);
                memset(buffer[i], 0, BUF_SIZE);
        }

        return 0;
}
