#include "hw2_syscalls.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

//defines
#define MAX_TO_MONITOR 150
#define MEDIUM_TIME 3000


//helper structs
const char* policy[] =
{
                        "SCHED_OTHER",
                        "SCHED_FIFO",
                        "SCHED_RR",
						"SCHED_FUCKERS_FROM_LAST_SEMESETER",
                        "SCHED_SHORT"
};
const char* context_switching_reason[] =
{
                        "Default",
                        "A task was created",
                        "A task ended",
                        "A task yields the CPU",
                        "An SHORT-process became overdue",
                        "A previous task goes out for waiting",
                        "A task with higher priority returns from waiting",
                        "The time slice of the previous task has ended"
};

//helper functions
void printMonitoringUsage(switch_info_t switch_info){
	printf("\tcurent pid: %20d\n", switch_info.previous_pid);
	printf("\tnext pid: %20d, \n", switch_info.next_pid);
	printf("\tpolicy: %20s\n", policy[switch_info.previous_policy]);
	printf("\tnext policy is: %s, \n", policy[switch_info.next_policy]);
	printf("\ttime in ticks is: %s, \n", switch_info.time);
	switch (switch_info.reason) {
		case 0:
			printf("\treason is Default, means reason of context switch wasn't monitoreds\n\n");
			break;
		case 1:
			printf("\treason for context switch is:\t a task was created\n\n");
			break;
		case 2:
			printf("\treason for context switch is:\t a task was ended\n\n");
			break;
		case 3:
			printf("\treason for context switch is:\t a task yields the CPU\n\n");
			break;
		case 4:
			printf("\treason for context switch is:\t a SHORT process became overdue\n\n");
			break;
		case 5:
			printf("\treason for context switch is:\t a previous task goes out for waiting\n\n");
			break;
		case 6:
			printf("\treason for context switch is:\t a task with higher priority returns from waiting\n\n");
			break;
		case 7:
			printf("\treason for context switch is:\t the time slice of previous task has ended\n\n");
			break;
		default:
			printf("\tvalue of reason is %d, this is not legal value for reason\n\n", switch_info.reason);
			break;
	}
}
int fibonaci(int n)
{
        if (n < 2)
        {
                return n;
        }
        return fibonaci(n-1) + fibonaci(n-2);
}

int main(int argc, char** argv)
{
	int num, i, count, my_pid, temp_pid, fib_num, trial_num;
	switch_info_t array[MAX_TO_MONITOR];

	if ((argc==1)||((argc-1) % 2) != 0)
	{
		printf ("Wrong number of arguments\n");
		return -1;
	}

	my_pid = getpid();

	for (i = 1; i < argc; i += 2) {
		trial_num = atoi(argv[i]);
		fib_num = atoi(argv[i + 1]);

		temp_pid = fork();
		if (temp_pid>0)//father process
		{
			struct sched_param s_params;
			s_params.requested_time = MEDIUM_TIME;
			s_params.trial_num = trial_num;
			sched_setscheduler(temp_pid, 4, &s_params);

		}
		else{//child process
			sched_yield();
			//sched_setscheduler(getpid(), 4, &s_params);
			fibonaci(fib_num);
			exit(0);
		}
	}

	while (wait(&temp_pid) != -1);

	//init monitored array
	for (i=0;i<150;i++){
		array[i].previous_pid = -1;
		array[i].next_pid = -1;
		array[i].previous_policy = -1;
		array[i].next_policy = -1;
		array[i].time = -1;
		array[i].reason = 0;
	}

	count = get_scheduling_statistic(array);

	printf("result value of get sched stats is: %20d\n\n", count);

	for (i = 0; i < count; i++) {
		printf("printing monitoring entry: %2d\n\n", i);
		printMonitoringUsage(array[i]);
	}
	return 0;
}
