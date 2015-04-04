#include <linux/kernel.h>
#include <linux/sched.h>
#include "syscall_maxproc.h"
//TODO why did Uri include this header file here?
//#include "count_sons.h" in our case its syscall_maxproc.h

/*
int sys_set_child_max_proc(int maxproc){
	printk("in CR0, system call set_child_max_proc\n");
	printk("maxproc promped value is:\t%d\n", maxproc);

	struct task_struct *curr_proc;
	struct task_struct *father_proc;
	curr_proc = current;
	father_proc = curr_proc->p_pptr;

//if maxproc val is negative limit should be reset
	//opt 1 - father has no limit - ok no limit
	//opt 2 - father has a limit - ok father's limit minus 1
	if (maxproc < 0){
		if (father_proc->max_proc_num == -1){
			curr_proc->max_proc_num = -1; // this is default value
			return 0;
		}
		else {
			curr_proc->max_proc_num = father_proc->max_proc_set - 1;
			return 0;
		}
	}
//if maxproc val is higher than father limit
	//opt 1 - father has no limit - ok
	//opt 2 - father has a limit - error
	if (maxproc >= (father_proc->max_proc_num)-1){
		if (father_proc->max_proc_num == -1){
			curr_proc->max_proc_set = maxproc;
//			curr_proc->max_proc_num = maxproc;
			return 0;
		}
		else{
			return - EPERM;
		}
	}
//if father maxproc val is lower than the set value of son
	if (father_proc->max_proc_num <= maxproc){
		curr_proc->max_proc_num = father_proc->max_proc_num - 1;
		curr_proc->max_proc_set = maxproc;
		return 0;
	}
//if father maxproc val is not defined then just define ours
	if ((father_proc->max_proc_num == -1) &&
			(father_proc->max_proc_set == -1)){
		curr_proc->max_proc_num = maxproc;
		curr_proc->max_proc_set = maxproc;
		return 0;
	}
}
*/

int sys_set_child_max_proc(int maxproc){
	printk("in CR0, system call set_child_max_proc\n");
	printk("maxproc promped value is:\t%d\n", maxproc);
	struct task_struct *curr_proc = current;

	if (maxproc > ((curr_proc->my_limit) - 1)){
		return -EPERM;
	}
	curr_proc->set_limit = maxproc;
	return 0;
}

int sys_get_max_proc(){
	printk("in CR0, system call get_max_proc\n");
	struct task_struct *curr_proc = current;
	return current->my_limit;
}

int sys_get_subproc_count(){
	printk("in CR0, system call get_subproc_count\n");
	struct task_struct *curr_proc = current;
	return current->child_counter;
}
