#include <linux/kernel.h>
#include <linux/sched.h>
#include <asm/uaccess.h>

/*******************************************************************************
 * sys_is_SHORT(int pid) - Checks if the process is SHORT, using the MACROs
 * 						   defined in the header
 ******************************************************************************/
int sys_is_SHORT(int pid){
	task_t *p = NULL;
	p = find_task_by_pid(pid);
	if (p == NULL){
		return -EINVAL;
	}
	if (!(IS_SHORT(p))){
			return -EINVAL;
	}
	if (IS_OVERDUE(p)){
		return 0;
	}
	return 1;
}
/*******************************************************************************
 * sys_remaining_time(int pid) - Returns the remaining time left to run for the
 * 								 process, in ms
 ******************************************************************************/
int sys_remaining_time(int pid){
	unsigned time = 0;
	int check = sys_is_SHORT(pid);
	if (check != 1){
		return check;
	}
	task_t *p = NULL;
	p = find_task_by_pid(pid);

	time = jiffies_to_ms(REMAINING_TIME(p));
	return (int)time;

}
/*******************************************************************************
 * sys_remaining_time(int pid) - Returns the remaining trials left to run for the
 * 								 process
 ******************************************************************************/
sys_remaining_trials(int pid){
	int check = sys_is_SHORT(pid);
	if (check != 1){
		return check;
	}
	task_t *p = NULL;
	p = find_task_by_pid(pid);

	return REMAINING_TRIALS(p);

}

