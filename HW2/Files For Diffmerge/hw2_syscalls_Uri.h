#include <asm/errno.h>
extern int errno;
/************************************************
 *                               DEFINES
 ***********************************************/
#define SCHED_LSHORT    3               /* HW2 - Uri */

/************************************************
 *                              STRUCTS
 ***********************************************/
struct switch_info {                    /*HW2-yoav*/
        int previous_pid;
        int next_pid;
        int previous_policy;
        int next_policy;
        unsigned long time;
        int reason;
};

struct lshort_sched_param {             // HW2 - Uri
        int requested_time;
        int level;
};

struct sched_param {                    // HW2 - Uri
        union {
                int sched_priority;
                struct lshort_sched_param lshort_params;
        };
};
/************************************************
 *                              wrapper functions
 ***********************************************/
int lshort_query_remaining_time(int pid)
{
    unsigned int res;
    __asm__
    (
                    "int $0x80;"
                    : "=a" (res)
                    : "0" (243) ,"b" (pid)
                    : "memory"
    );
    if (res>=(unsigned long)(-125))
    {
            errno = -res;
            res = -1;
    }
    return (int) res;
}
/************************************************/
int lshort_query_overdue_time(int pid)
{
    unsigned int res;
    __asm__
    (
                    "int $0x80;"
                    : "=a" (res)
                    : "0" (244) ,"b" (pid)
                    : "memory"
    );
    if (res>=(unsigned long)(-125))
    {
            errno = -res;
            res = -1;
    }
    return (int) res;
}
/************************************************/
int get_scheduling_statistic(struct switch_info * tasks_info)   /*HW2-yoav*/
{
    unsigned int res;
    __asm__
    (
                    "int $0x80;"
                    : "=a" (res)
                    : "0" (245) ,"b" (tasks_info)
                    : "memory"
    );
    if (res>=(unsigned long)(-125))
    {
            errno = -res;
            res = -1;
    }
    return (int) res;
}