#include <linux/kernel.h>
#include <linux/sched.h>
#include "syscall_maxproc.h"

int sys_set_child_max_proc(int maxproc){
	printk("in CR0, system call set_child_max_proc\n");
	printk("maxproc promped value is:\t%d\n", maxproc);
	struct task_struct *curr_proc = current;

	if (curr_proc->my_limit != -1){									//If it's -1 there is no limit on it so no need to check
		if (maxproc > ((curr_proc->my_limit) - 1)){
//			errno = -EPERM;
//			return -1;
			return -EPERM;
		}
	}
	//reset child limit to no limit
	if (maxproc < 0){
		maxproc = -1;
	}
	curr_proc->set_limit = maxproc;
	return 0;
}

int sys_get_max_proc(void){
	printk("in CR0, system call get_max_proc\n");
	struct task_struct *curr_proc = current;
	    printk("The limit on the child-processes of this program is %d \n",curr_proc->set_limit);  //***newTestCode***
	if (curr_proc->my_limit == -1){
		return -EINVAL;										//Handles the case when there is no limit
//		errno = -EINVAL;
//		return -1;
	}
	return curr_proc->my_limit;
}

int sys_get_subproc_count(void){
	printk("in CR0, system call get_subproc_count\n");
	struct task_struct *curr_proc = current;
	return curr_proc->child_counter;
}
