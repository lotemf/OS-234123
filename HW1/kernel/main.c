#include "syscall_maxproc.h"
#include <stdio.h>
#include <assert.h>

int main(){
	printf("call to syscall 243, set child max proc\n");
	set_child_max_proc(27);
	printf("call to syscall 244, get max proc\n");
	get_max_proc();
	printf("call to syscall 245, get subproc count\n");
	get_subproc_count();


	return 0;
}
