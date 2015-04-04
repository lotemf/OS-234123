#include <asm/errno.h>
extern int errno;

//wrappers for the following system calls
//general comments
	//to modify fork.c
	//to modify PCB struct, consider init specifically
	//return -1 and change errno to EPERM (defined in errno.h)
		//what is errno
	//change entry.S to have #243,#244,#245 syscall number

/*******************************************************************************
 * get_max_proc() - Returns the max_proc field of the running process.
 * Complexity- o(n) - while n is the number of ancestors the process has
 * //comments for implementation on syscall_set_child_max_proc
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
 ******************************************************************************/
int set_child_max_proc(int maxp){
	long __res;
	__asm__ volatile (
	"movl $243, %%eax;"
	/*Needs Verification*/"movl %1, %%ebx;"						/*Moving the int value of maxp -> maybe there is a need to edit this part*/
	"int $0x80;"												/*because the size of the registers might not match*/
	"movl %%eax,%0"
	: "=m" (__res)
	: "m" (maxp)												/*This is the output operand for the maxP*/
	: "%eax","%ebx"
	);
	if ((unsigned long)(__res) >= (unsigned long)(-125)) {
	errno = -(__res); __res = -1;
	}
	return (int)(__res);

}

/*******************************************************************************
 * get_max_proc() - Returns the max_proc field of the running process.
 * Complexity- o(1)
 ******************************************************************************/
/*int get_max_proc(){
	long __res;
	__asm__ volatile (
	"movl $244, %%eax;"
	"int $0x80;"
	"movl %%eax,%0"
	: "=m" (__res)
	: "%eax"
	);
	if ((unsigned long)(__res) >= (unsigned long)(-125)) {
	errno = -(__res); __res = -1;	*/						/*This code takes the error value */
	/*}		*/												/*and stores it inside errno by using the unsigned*/
	/*return (int)(__res);	*/								/*value returned from the get_max_proc func*/

/*}**/

/*******************************************************************************
 * get_max_proc() - Returns the offspring field of the running process.
 * Complexity- o(1)
 ******************************************************************************/
/*int get_subproc_count(){
	long __res;
	__asm__ volatile (
	"movl $245, %%eax;"
	"int $0x80;"
	"movl %%eax,%0"
	: "=m" (__res)
	: "%eax"
	);
	if ((unsigned long)(__res) >= (unsigned long)(-125)) {
	errno = -(__res); __res = -1;
	}
	return (int)(__res);
}*/

