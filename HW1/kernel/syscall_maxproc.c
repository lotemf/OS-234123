#include <linux/kernel.h>
#include <linux/sched.h>
//TODO why did Uri include this header file here?
//#include "count_sons.h" in our case its syscall_maxproc.h


int sys_set_child_max_proc(int maxproc){
	printf("in CR0, system call set_child_max_proc\n");
	printf("maxproc promped value is:\t%d\n", maxproc);
}

int sys_get_max_proc(){
	printf("in CR0, system call get_max_proc\n");
}

int sys_get_subproc_count(){
	printf("in CR0, system call get_subproc_count\n");
}
