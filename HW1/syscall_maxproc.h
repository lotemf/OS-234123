

//wrappers for the following system calls
//general comments
	//to modify fork.c
	//to modify PCB struct, consider init specifically
	//return -1 and change errno to EPERM (defined in errno.h)
		//what is errno
	//change entry.S to have #243,#244,#245 syscall number

int set_child_max_proc(int maxp){
//comments for implementation on syscall_set_child_max_proc
	//check negative number
	//update offspring field on PCB
	//update max_proc field on PCB
}

int get_max_proc(){
//

}

int get_subproc_count(){

}
