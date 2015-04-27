#include <linux/kernel.h>
#include <linux/sched.h>

int sys_lshort_query_remaining_time(int pid)
{
        if (pid < 0)
        {
                return -EINVAL;
        }
        struct task_struct *target;
        if (pid == 0) {
                target = &init_task;
        } else {
                target = find_task_by_pid(pid);
        }
        if (!target) {
                return -ESRCH;
        }
        if (target->policy != SCHED_LSHORT)
        {
                return -EINVAL;
        }
        int result = 0;
        int remaining_time = target->requested_time - target->used_time;
        if (remaining_time > 0)
        {
                result = remaining_time * 1000 / HZ;            // Conversion from ticks to ms
        }
        if (remaining_time < 0)
        {
                result = 0;
        }
        return result;
}

int sys_lshort_query_overdue_time(int pid)
{
        if (pid < 0)
        {
                return -EINVAL;
        }
        struct task_struct *target;
        if (pid == 0) {
                target = &init_task;
        } else {
                target = find_task_by_pid(pid);
        }
        if (!target) {
                return -ESRCH;
        }
        if (target->policy != SCHED_LSHORT)
        {
                return -EINVAL;
        }
        int result = 0;
        int overdue_time = target->used_time - target->requested_time;
        if (overdue_time > 0)
        {
                result = overdue_time * 1000 / HZ;              // Conversion from ticks to ms
        }
        if (overdue_time <= 0)
        {
                result = 0;
        }
        return result;
}
