#include <asm/errno.h>
extern int errno;

/*------------------------------------------------------------------------------
 	 	 	 	 	 	 	 *	 Macros	*
 ------------------------------------------------------------------------------*/
#define REMAINING_TRIALS(p) (int)( (p->requested_trials) - (p->used_trials) )
#define REMAINING_TIME(p) (long)( (p->requested_time) - (p->used_time) )
#define OVERDUE_TIME(p) (REMAINING_TIME(p)*(-1))
#define IS_SHORT(p) (p->policy == SCHED_SHORT)
#define IS_OVERDUE(p) (IS_SHORT(p)) ? (OVERDUE_TIME(p) >= 0) : (0)


struct sched_param {
	int sched_priority;
	int requested_time;			/*HW2 - Lotem*/
	int trial_num;				/*HW2 - Lotem*/
};

/*------------------------------------------------------------------------------
 	 	 	 	 	 	 	 *	 The Wrappers	*
 ------------------------------------------------------------------------------*/

/*******************************************************************************
 * is_SHORT(int pid) - return 1 if the given process is a SHORT-process,
  	  	  	  	  	    or 0 if it is already overdue.
  	  	  	  	  	    If it isn't a SHORT process returns -1 (EINVAL)
 * Complexity- o(1)
 ******************************************************************************/
int is_SHORT(int pid){
	long __res;
	__asm__ volatile (
	"movl $243, %%eax;"
	"movl %1, %%ebx;"
	"int $0x80;"
	"movl %%eax,%0"
	: "=m" (__res)
	: "m" (pid)
	: "%eax","%ebx"
	);
	if ((unsigned long)(__res) >= (unsigned long)(-125)) {
	errno = -(__res); __res = -1;
	}
	return (int)(__res);

}
/*******************************************************************************
 * remaining_time(int pid) - Returns the time left for the process at the
                     current time slice, for overdue process it should return 0.
                     If it isn't a SHORT process returns -1 (EINVAL)
 * Complexity- o(1)
 ******************************************************************************/
int remaining_time(int pid){
	long __res;
	__asm__ volatile (
	"movl $244, %%eax;"
	"movl %1, %%ebx;"
	"int $0x80;"
	"movl %%eax,%0"
	: "=m" (__res)
	: "m" (pid)
	: "%eax","%ebx"
	);
	if ((unsigned long)(__res) >= (unsigned long)(-125)) {
	errno = -(__res); __res = -1;
	}
	return (int)(__res);

}
/*******************************************************************************
 * remaining_trials(int pid) - Returns the return the number of trials left for
 * 				      the SHORT process, for overdue process it should return 0.
 * 				       If it isn't a SHORT process returns -1 (EINVAL)
 * Complexity- o(1)
 ******************************************************************************/
int remaining_trials(int pid){
	long __res;
	__asm__ volatile (
	"movl $245, %%eax;"
	"movl %1, %%ebx;"
	"int $0x80;"
	"movl %%eax,%0"
	: "=m" (__res)
	: "m" (pid)
	: "%eax","%ebx"
	);
	if ((unsigned long)(__res) >= (unsigned long)(-125)) {
	errno = -(__res); __res = -1;
	}
	return (int)(__res);

}
/*******************************************************************************
 * get_scheduling_statistics(struct switch_info * tasks_info) -
					Will be used for the monitoring...
 * Complexity- ???
 ******************************************************************************/
int get_scheduling_statistics(struct switch_info * info){
		long __res;
		__asm__ volatile (
		"movl $246, %%eax;"
		"movl %1, %%ebx;"
		"int $0x80;"
		"movl %%eax,%0"
		: "=m" (__res)
		: "m" (info)
		: "%eax","%ebx"
		);
		if ((unsigned long)(__res) >= (unsigned long)(-125)) {
		errno = -(__res); __res = -1;
		}
		return (int)(__res);

	}
}
