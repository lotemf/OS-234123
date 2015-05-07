#include <asm/errno.h>
extern int errno;

// hw2 - cz - switch info struct for monitoring
typedef struct switch_info {
	int previous_pid;
    int next_pid;
    int previous_policy;
    int next_policy;
    unsigned long time;
    int reason;
} switch_info_t;


/*------------------------------------------------------------------------------
* HW2	Macros	and param Struct
* HW2 - Lotem
 ------------------------------------------------------------------------------*/

#define REMAINING_TRIALS(p) (int)( (p->requested_trials) - (p->used_trials) )
#define REMAINING_TIME(p) (long)(p->time_slice)	/* HW2 - Lotem */
#define IS_SHORT(p) (p->policy == SCHED_SHORT)
#define IS_OVERDUE(p) (IS_SHORT(p) && ( ((p)->used_trials >= (p)->requested_trials) \
			|| (p)->time_slice == 0) ) /* HW2 - Alon */


/*******************************************************************************
*		THIS IS A DEBUG FUNCTION						**-TO_DELETE			//TODO - delete later
*******************************************************************************/
struct debug_struct
{
    int priority;
	int policy;
    int requested_time;                 /* HW2 Roy: Range: 1-5000 in miliseconds */
    int trial_num;               /* HW2 Roy: Range: 1-50 original number of trials */
    int trial_num_counter;
	int is_overdue;
    int time_slice;
};
/*******************************************************************************/


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
 * get_scheduling_statistic(struct switch_info * tasks_info) -
 * Complexity- o(1)
 ******************************************************************************/
int get_scheduling_statistic(struct switch_info * info){
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
/*******************************************************************************
 * 						DEBUG PRINTS FUNC
 ******************************************************************************/
int hw2_debug(int pid, struct debug_struct* debug)                  /*247 - for debug using*/
{
    unsigned int res;
    __asm__ volatile (
            "movl $247, %%eax;"
            "movl %1, %%ebx;"
            "movl %2, %%ecx;"
            "int $0x80;"
            "movl %%eax,%0"
            : "=m" (res)
            : "m" (pid), "m" ((long)debug)
            : "%eax","%ebx","%ecx"
    );
     if (res>=(unsigned long)(-125))
    {
        errno = -res;
        res = -1;
    }
    return (int) res;
}
