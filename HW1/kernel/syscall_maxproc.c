#include <linux/kernel.h>
#include <linux/sched.h>

int sys_set_child_max_proc(int maxproc){
	struct task_struct *curr_proc = current;

	if (curr_proc->my_limit != -1){									//If it's -1 there is no limit on it so no need to check
		if (maxproc > ((curr_proc->my_limit) - 1)){
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
	struct task_struct *curr_proc = current;
	if (curr_proc->my_limit == -1){
		return -EINVAL;										//Handles the case when there is no limit
	}
	return curr_proc->my_limit;
}

int sys_get_subproc_count(void){
	struct task_struct *curr_proc = current;
	return curr_proc->child_counter;
}
