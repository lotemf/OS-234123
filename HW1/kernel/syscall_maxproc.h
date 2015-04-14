#include <asm/errno.h>
extern int errno;

int set_child_max_proc(int maxp){
	long __res;
	__asm__ volatile (
	"movl $243, %%eax;"
	"movl %1, %%ebx;"						/*Moving the int value of maxp -> maybe there is a need to edit this part*/
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
 * get_max_proc(void) - Returns the max_proc field of the running process.
 * Complexity- o(1)
 ******************************************************************************/
int get_max_proc(void){
	long __res;
	__asm__ volatile (
	"movl $244, %%eax;"
	"int $0x80;"
	"movl %%eax,%0"
	: "=m" (__res)
	:
	: "%eax"
	);
	if ((unsigned long)(__res) >= (unsigned long)(-125)) {
	errno = -(__res); __res = -1;							/*This code takes the error value */
	}														/*and stores it inside errno by using the unsigned*/
	return (int)(__res);									/*value returned from the get_max_proc func*/

}

/*******************************************************************************
 * get_max_proc(void) - Returns the offspring field of the running process.
 * Complexity- o(1)
 ******************************************************************************/
int get_subproc_count(void){
	long __res;
	__asm__ volatile (
	"movl $245, %%eax;"
	"int $0x80;"
	"movl %%eax,%0"
	: "=m" (__res)
	:
	: "%eax"
	);
	if ((unsigned long)(__res) >= (unsigned long)(-125)) {
	errno = -(__res); __res = -1;
	}
	return (int)(__res);
}

