

//wrappers for the following system calls
//general comments
	//to modify fork.c
	//to modify PCB struct, consider init specifically
	//return -1 and change errno to EPERM (defined in errno.h)
		//what is errno
	//change entry.S to have #243,#244,#245 syscall number

int set_child_max_proc(int maxp){
//comments for implementation on syscall_set_child_max_proc
	//validation checks
		//check negative number
			//check if father max_proc <> -1
				//set max_proc to (father-1)
			//else put -1
		//set max proc to higher than father max proc -1
	//update offspring field on PCB
	//update max_proc field on PCB
//tests
	//check two/three generations max_proc field
		//goal to verify our preserved env works (enough to check father value)
	//negative parameter
		//goal to check if max_proc values in path to root are not broken
}

/*******************************************************************************
 * get_max_proc() - Returns the max_proc field.
 * Complexity- o(1)
 ******************************************************************************/
int get_max_proc(){
	long __res;
	__asm__ volatile (
	"movl $245, %%eax;"
	"movl %1, %%ebx;"
	"movl %2, %%ecx;"
	"int $0x80;"
	"movl %%eax,%0"
	: "=m" (__res)
	: "m" ((long)array), "m" (count)
	: "%eax","%ebx","%ecx"
	);
	if ((unsigned long)(__res) >= (unsigned long)(-125)) {
	errno = -(__res); __res = -1;
	}
	return (int)(__res);

}

int get_subproc_count(){
//o(1) return the offspring field.

}
`
