/*
 * module_example.c
 *
 *  Created on: 6 αιπε 2015
 *      Author: alon
 */

#include <linux/module.h>
#include <linux/kernel.h> /* for using printk */
#include <linux/sched.h>
MODULE_LICENSE("GPL");

int version=0;
int array[3];
char* str;
MODULE_PARM(version, "i");
MODULE_PARM(array, "3i");
MODULE_PARM(str, "s");

int init_module(void)
{
        printk("The process is: \"%s\" (pid: %i)\n", current->comm, current->pid);
        printk("Version: %d\n", version);
        printk("Array: %d, %d, %d\n", array[0], array[1], array[2]);
        printk("Message: %s\n", str);
        return 0;
}

void cleanup_module(void)
{
        printk("Goodbye cruel world\n");
}
