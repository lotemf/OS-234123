/*
 *  kernel/sched.c
 *
 *  Kernel scheduler and related syscalls
 *
 *  Copyright (C) 1991-2002  Linus Torvalds
 *
 *  1996-12-23  Modified by Dave Grothe to fix bugs in semaphores and
 *              make semaphores SMP safe
 *  1998-11-19	Implemented schedule_timeout() and related stuff
 *		by Andrea Arcangeli
 *  2002-01-04	New ultra-scalable O(1) scheduler by Ingo Molnar:
 *  		hybrid priority-list and round-robin design with
 *  		an array-switch method of distributing timeslices
 *  		and per-CPU runqueues.  Additional code by Davide
 *  		Libenzi, Robert Love, and Rusty Russell.
 */

#include <linux/mm.h>
#include <linux/nmi.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/highmem.h>
#include <linux/smp_lock.h>
#include <asm/mmu_context.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/kernel_stat.h>

/*
 * Convert user-nice values [ -20 ... 0 ... 19 ]
 * to static priority [ MAX_RT_PRIO..MAX_PRIO-1 ],
 * and back.
 */
#define NICE_TO_PRIO(nice)	(MAX_RT_PRIO + (nice) + 20)
#define PRIO_TO_NICE(prio)	((prio) - MAX_RT_PRIO - 20)
#define TASK_NICE(p)		PRIO_TO_NICE((p)->static_prio)

/*
 * 'User priority' is the nice value converted to something we
 * can work with better when scaling various scheduler parameters,
 * it's a [ 0 ... 39 ] range.
 */
#define USER_PRIO(p)		((p)-MAX_RT_PRIO)
#define TASK_USER_PRIO(p)	USER_PRIO((p)->static_prio)
#define MAX_USER_PRIO		(USER_PRIO(MAX_PRIO))

/*
 * These are the 'tuning knobs' of the scheduler:
 *
 * Minimum timeslice is 10 msecs, default timeslice is 150 msecs,
 * maximum timeslice is 300 msecs. Timeslices get refilled after
 * they expire.
 */
#define MIN_TIMESLICE		( 10 * HZ / 1000)
#define MAX_TIMESLICE		(300 * HZ / 1000)
#define CHILD_PENALTY		95
#define PARENT_PENALTY		100
#define EXIT_WEIGHT		3
#define PRIO_BONUS_RATIO	25
#define INTERACTIVE_DELTA	2
#define MAX_SLEEP_AVG		(2*HZ)
#define STARVATION_LIMIT	(2*HZ)

/*
 * If a task is 'interactive' then we reinsert it in the active
 * array after it has expired its current timeslice. (it will not
 * continue to run immediately, it will still roundrobin with
 * other interactive tasks.)
 *
 * This part scales the interactivity limit depending on niceness.
 *
 * We scale it linearly, offset by the INTERACTIVE_DELTA delta.
 * Here are a few examples of different nice levels:
 *
 *  TASK_INTERACTIVE(-20): [1,1,1,1,1,1,1,1,1,0,0]
 *  TASK_INTERACTIVE(-10): [1,1,1,1,1,1,1,0,0,0,0]
 *  TASK_INTERACTIVE(  0): [1,1,1,1,0,0,0,0,0,0,0]
 *  TASK_INTERACTIVE( 10): [1,1,0,0,0,0,0,0,0,0,0]
 *  TASK_INTERACTIVE( 19): [0,0,0,0,0,0,0,0,0,0,0]
 *
 * (the X axis represents the possible -5 ... 0 ... +5 dynamic
 *  priority range a task can explore, a value of '1' means the
 *  task is rated interactive.)
 *
 * Ie. nice +19 tasks can never get 'interactive' enough to be
 * reinserted into the active array. And only heavily CPU-hog nice -20
 * tasks will be expired. Default nice 0 tasks are somewhere between,
 * it takes some effort for them to get interactive, but it's not
 * too hard.
 */

#define SCALE(v1,v1_max,v2_max) \
	(v1) * (v2_max) / (v1_max)

#define DELTA(p) \
	(SCALE(TASK_NICE(p), 40, MAX_USER_PRIO*PRIO_BONUS_RATIO/100) + \
		INTERACTIVE_DELTA)

#define TASK_INTERACTIVE(p) \
	((p)->prio <= (p)->static_prio - DELTA(p))

/*
 * TASK_TIMESLICE scales user-nice values [ -20 ... 19 ]
 * to time slice values.
 *
 * The higher a process's priority, the bigger timeslices
 * it gets during one round of execution. But even the lowest
 * priority process gets MIN_TIMESLICE worth of execution time.
 */

#define TASK_TIMESLICE(p) (MIN_TIMESLICE + \
	((MAX_TIMESLICE - MIN_TIMESLICE) * (MAX_PRIO-1-(p)->static_prio)/39))

/*
 * These are the runqueue data structures:
 */

#define BITMAP_SIZE ((((MAX_PRIO+1+7)/8)+sizeof(long)-1)/sizeof(long))

typedef struct runqueue runqueue_t;

struct prio_array {
	int nr_active;
	unsigned long bitmap[BITMAP_SIZE];
	list_t queue[MAX_PRIO];
};

/*
 * This is the main, per-CPU runqueue data structure.
 *
 * Locking rule: those places that want to lock multiple runqueues
 * (such as the load balancing or the process migration code), lock
 * acquire operations must be ordered by ascending &runqueue.
 */
struct runqueue {
	spinlock_t lock;
	unsigned long nr_running, nr_switches, expired_timestamp;
	signed long nr_uninterruptible;
	task_t *curr, *idle;

	prio_array_t *active, *expired,*SHORT , *SHORT_OVERDUE, arrays[4]; 	//Hw2 addition - Lotem 29.4.15

	int prev_nr_running[NR_CPUS];
	task_t *migration_thread;
	list_t migration_queue;

	//hw2 - cz - monitoring member fields
	switch_info_t record_array[TOTAL_MAX_TO_MONITOR]; //recorded array
	int record_idx;	//index for recording array, holds the next available idx in record arr to write- init to 0
	int p_events_count; //counter of the process switching events - init to 0
	int is_round_completed_flag;//flag for sys_get_stats -init to 0

} ____cacheline_aligned;

static struct runqueue runqueues[NR_CPUS] __cacheline_aligned;

#define cpu_rq(cpu)		(runqueues + (cpu))
#define this_rq()		cpu_rq(smp_processor_id())
#define task_rq(p)		cpu_rq((p)->cpu)
#define cpu_curr(cpu)		(cpu_rq(cpu)->curr)

#define rt_task(p)		 (((p)->prio < MAX_RT_PRIO) && ((p)->policy != SCHED_SHORT))		//HW2 - Lotem - 30.4.15



/*
 * Default context-switch locking:
 */
#ifndef prepare_arch_schedule
# define prepare_arch_schedule(prev)	do { } while(0)
# define finish_arch_schedule(prev)	do { } while(0)
# define prepare_arch_switch(rq)	do { } while(0)
# define finish_arch_switch(rq)		spin_unlock_irq(&(rq)->lock)
#endif


/*------------------------------------------------------------------------------
  	  	  	  	  	  	  HW2 Additions
------------------------------------------------------------------------------*/

/*******************************************************************************
 ticks_to_ms(int ticks) - Converts the number of ticks that occured since the
 						  system booted to mili-seconds
 *******************************************************************************/
int ticks_to_ms(int ticks)
{
	return ((ticks * 1000) / HZ);
}
/*******************************************************************************
 ms_to_ticks(int ms) - Converts the number of mili-seconds to ticks
 *******************************************************************************/
int ms_to_ticks(int ms)
{
	int ticks = (ms * HZ) / 1000;

	if (((ticks * 1000) / HZ) < ms)
	{
		ticks += 1;
	}
	return ticks;
}
/**
 * hw2 - cz -monitoring function
 * ` method provides interrupt safety operation of zeroing the events counter
 * this must be added to prevent situation of not recording an event due to interrupt
 */
void zero_switching_events_count(){
	unsigned long flags;
	runqueue_t* rq = this_rq();
	local_irq_save(flags); //lock
	rq->p_events_count = 0;
	local_irq_restore(flags); //unlock
}
void update_switch_info_struct(switch_info_t* info,int pr_pid,
				int nxt_pid, int pr_pol, int nxt_pol, int time, int reason){
	info->previous_pid = pr_pid;
    info->next_pid = nxt_pid;
	info->previous_policy = pr_pol;
	info->next_policy = nxt_pol;
	info->time = time;
	info->reason = reason;
}

/*------------------------------------------------------------------------------
  	  	  	  	  	  ***End of	HW2 Additions
------------------------------------------------------------------------------*/

/*
 * task_rq_lock - lock the runqueue a given task resides on and disable
 * interrupts.  Note the ordering: we can safely lookup the task_rq without
 * explicitly disabling preemption.
 */
static inline runqueue_t *task_rq_lock(task_t *p, unsigned long *flags)
{
	struct runqueue *rq;

repeat_lock_task:
	local_irq_save(*flags);
	rq = task_rq(p);
	spin_lock(&rq->lock);
	if (unlikely(rq != task_rq(p))) {
		spin_unlock_irqrestore(&rq->lock, *flags);
		goto repeat_lock_task;
	}
	return rq;
}

static inline void task_rq_unlock(runqueue_t *rq, unsigned long *flags)
{
	spin_unlock_irqrestore(&rq->lock, *flags);
}

/*
 * rq_lock - lock a given runqueue and disable interrupts.
 */
static inline runqueue_t *this_rq_lock(void)
{
	runqueue_t *rq;

	local_irq_disable();
	rq = this_rq();
	spin_lock(&rq->lock);

	return rq;
}

static inline void rq_unlock(runqueue_t *rq)
{
	spin_unlock(&rq->lock);
	local_irq_enable();
}

/*
 * Adding/removing a task to/from a priority array:
 */
static inline void dequeue_task(struct task_struct *p, prio_array_t *array)
{
	array->nr_active--;
	list_del(&p->run_list);
	if (list_empty(array->queue + p->prio))
		__clear_bit(p->prio, array->bitmap);
}

static inline void enqueue_task(struct task_struct *p, prio_array_t *array)
{
	list_add_tail(&p->run_list, array->queue + p->prio);
	__set_bit(p->prio, array->bitmap);
	array->nr_active++;
	p->array = array;
}

static inline int effective_prio(task_t *p)
{
	int bonus, prio;

	/*
	 * Here we scale the actual sleep average [0 .... MAX_SLEEP_AVG]
	 * into the -5 ... 0 ... +5 bonus/penalty range.
	 *
	 * We use 25% of the full 0...39 priority range so that:
	 *
	 * 1) nice +19 interactive tasks do not preempt nice 0 CPU hogs.
	 * 2) nice -20 CPU hogs do not get preempted by nice 0 tasks.
	 *
	 * Both properties are important to certain workloads.
	 */
	bonus = MAX_USER_PRIO*PRIO_BONUS_RATIO*p->sleep_avg/MAX_SLEEP_AVG/100 -
			MAX_USER_PRIO*PRIO_BONUS_RATIO/100/2;

	prio = p->static_prio - bonus;
	if (prio < MAX_RT_PRIO)
		prio = MAX_RT_PRIO;
	if (prio > MAX_PRIO-1)
		prio = MAX_PRIO-1;
	return prio;
}

static inline void activate_task(task_t *p, runqueue_t *rq)
{
	unsigned long sleep_time = jiffies - p->sleep_timestamp;
	prio_array_t *array = NULL;							//HW2 - Lotem 3.5.15

	//HW2 - Lotem 30.4.15
	if (!IS_SHORT(p)) {
		array = rq->active;
		if (!rt_task(p) && sleep_time) {							//We don't have interactive SHORT processes, so we don't need to grant them a bonus
			/*
			 * This code gives a bonus to interactive tasks. We update
			 * an 'average sleep time' value here, based on
			 * sleep_timestamp. The more time a task spends sleeping,
			 * the higher the average gets - and the higher the priority
			 * boost gets as well.
			 */
			p->sleep_avg += sleep_time;
			if (p->sleep_avg > MAX_SLEEP_AVG)
				p->sleep_avg = MAX_SLEEP_AVG;
			p->prio = effective_prio(p);
		}
		/***********************************************************************
		 *  HW2  - Making sure no dynamic prio calculation is done for SHORT processes
		 *  	   //1 - When a SHORT process is activated we only add it to the correct rq
		 **********************************************************************/
	} else {											//HW2 - Lotem 30.4.15
             array = rq->SHORT;
             if (IS_OVERDUE(p)) {
                     array = rq->SHORT_OVERDUE;
                     p->prio = OVERDUE_PRIO;
             } else {
                     p->prio = p->static_prio;	//1
             }
    }
	/***********				 End of HW2 additions					******/  //HW2 - Lotem 30.4.15

	enqueue_task(p, array);
	rq->nr_running++;
}

static inline void deactivate_task(struct task_struct *p, runqueue_t *rq)
{
	rq->nr_running--;
	if (p->state == TASK_UNINTERRUPTIBLE)
		rq->nr_uninterruptible++;
	dequeue_task(p, p->array);
	p->array = NULL;
}

static inline void resched_task(task_t *p)
{
#ifdef CONFIG_SMP
	int need_resched;

	need_resched = p->need_resched;
	wmb();
	set_tsk_need_resched(p);
	if (!need_resched && (p->cpu != smp_processor_id()))
		smp_send_reschedule(p->cpu);
#else
	set_tsk_need_resched(p);
#endif
}

#ifdef CONFIG_SMP

/*
 * Wait for a process to unschedule. This is used by the exit() and
 * ptrace() code.
 */
void wait_task_inactive(task_t * p)
{
	unsigned long flags;
	runqueue_t *rq;

repeat:
	rq = task_rq(p);
	if (unlikely(rq->curr == p)) {
		cpu_relax();
		goto repeat;
	}
	rq = task_rq_lock(p, &flags);
	if (unlikely(rq->curr == p)) {
		task_rq_unlock(rq, &flags);
		goto repeat;
	}
	task_rq_unlock(rq, &flags);
}

/*
 * Kick the remote CPU if the task is running currently,
 * this code is used by the signal code to signal tasks
 * which are in user-mode as quickly as possible.
 *
 * (Note that we do this lockless - if the task does anything
 * while the message is in flight then it will notice the
 * sigpending condition anyway.)
 */
void kick_if_running(task_t * p)
{
	if (p == task_rq(p)->curr)
		resched_task(p);
}
#endif

/*
 * Wake up a process. Put it on the run-queue if it's not
 * already there.  The "current" process is always on the
 * run-queue (except when the actual re-schedule is in
 * progress), and as such you're allowed to do the simpler
 * "current->state = TASK_RUNNING" to mark yourself runnable
 * without the overhead of this.
 */
static int try_to_wake_up(task_t * p, int sync)
{
	unsigned long flags;
	int success = 0;
	long old_state;
	runqueue_t *rq;

repeat_lock_task:
	rq = task_rq_lock(p, &flags);
	old_state = p->state;
	if (!p->array) {
		/*
		 * Fast-migrate the task if it's not running or runnable
		 * currently. Do not violate hard affinity.
		 */
		if (unlikely(sync && (rq->curr != p) &&
			(p->cpu != smp_processor_id()) &&
			(p->cpus_allowed & (1UL << smp_processor_id())))) {

			p->cpu = smp_processor_id();
			task_rq_unlock(rq, &flags);
			goto repeat_lock_task;
		}
		if (old_state == TASK_UNINTERRUPTIBLE)
			rq->nr_uninterruptible--;
		activate_task(p, rq);
		/*
		 * If sync is set, a resched_task() is a NOOP
		 */

		/********************		HW2 - Lotem 30.4.15		*******************
		* We need to check specifically for SHORT processes, because we don't want OTHER
		* processes with better prio to call resched_task.
		*
		*
		* So when do we need to call resched_task ?
		//1 - If p is RT and current isn't - p wins,but if current is RT we check the prio
		//2 - If p is SHORT, and current is OTHER or OVERDUE - p wins ,but if current is SHORT we check the prio
		//3 - If p is OTHER, and current is OVERDUE - p wins, but if current is OTHER we check the prio
		//4 - If p is OVERDUE, and current is IDLE - p wins
		***********************************************************************/
		int reschedCheck=0;
		if (rt_task(p) && (!rt_task(rq->curr) || (rt_task(rq->curr) && (p->prio < rq->curr->prio )) ) ){
			reschedCheck=1;
		}
		if (IS_SHORT(p) && ( (rq->curr->policy == SCHED_OTHER) || IS_OVERDUE(rq->curr) || (IS_SHORT(rq->curr) && (p->prio < rq->curr->prio)) ) ){
			reschedCheck=1;
		}
		if ((p->policy == SCHED_OTHER) && ( IS_OVERDUE(rq->curr) || ( (rq->curr->policy == SCHED_OTHER) && (p->prio < rq->curr->prio)) ) ){
				reschedCheck=1;
		}
		if (IS_OVERDUE(p) && (rq->curr->pid == 0) ){
				reschedCheck=1;
		}

		/* Making sure we are not switching the same process*/
		if ((reschedCheck == 1) && ((rq->curr) != p) ){
			(rq->curr)->reason = A_task_with_higher_priority_returns_from_waiting; //hw2 - cz - monitoring
			resched_task(rq->curr);
		}
		/********		End of HW2 Additions - Lotem 30.4.15		********/
		success = 1;
	}
	p->state = TASK_RUNNING;
	task_rq_unlock(rq, &flags);

	return success;
}

int wake_up_process(task_t * p)
{
	return try_to_wake_up(p, 0);
}

void wake_up_forked_process(task_t * p)
{
	runqueue_t *rq = this_rq_lock();

	/* HW2 - alon the father gives up the cpu */
	if (IS_SHORT(current)){
		if(IS_OVERDUE(current))
		{
			if (current->array == rq->SHORT){
				dequeue_task(current, rq->SHORT);
				current->prio = OVERDUE_PRIO;
				enqueue_task(current, rq->SHORT_OVERDUE);
			}
		}
		else
		{
			dequeue_task(current, rq->SHORT);
			enqueue_task(current, rq->SHORT);
		}
	}
	/* HW2 - alon end of additions */

	p->state = TASK_RUNNING;
	if (!rt_task(p)) {
		/*
		 * We decrease the sleep average of forking parents
		 * and children as well, to keep max-interactive tasks
		 * from forking tasks that are max-interactive.
		 */
		current->sleep_avg = current->sleep_avg * PARENT_PENALTY / 100;
		p->sleep_avg = p->sleep_avg * CHILD_PENALTY / 100;
		p->prio = effective_prio(p);
	}

	p->cpu = smp_processor_id();
	activate_task(p, rq);

	rq_unlock(rq);
}

/*
 * Potentially available exiting-child timeslices are
 * retrieved here - this way the parent does not get
 * penalized for creating too many processes.
 *
 * (this cannot be used to 'generate' timeslices
 * artificially, because any timeslice recovered here
 * was given away by the parent in the first place.)
 */
void sched_exit(task_t * p)
{
	__cli();
	if (p->first_time_slice) {
		current->time_slice += p->time_slice;
		if (unlikely(current->time_slice > MAX_TIMESLICE))
			current->time_slice = MAX_TIMESLICE;
	}
	__sti();
	/*
	 * If the child was a (relative-) CPU hog then decrease
	 * the sleep_avg of the parent as well.
	 */
	if (p->sleep_avg < current->sleep_avg)
		current->sleep_avg = (current->sleep_avg * EXIT_WEIGHT +
			p->sleep_avg) / (EXIT_WEIGHT + 1);
}

#if CONFIG_SMP
asmlinkage void schedule_tail(task_t *prev)
{
	finish_arch_switch(this_rq());
	finish_arch_schedule(prev);
}
#endif

static inline task_t * context_switch(task_t *prev, task_t *next)
{
	struct mm_struct *mm = next->mm;
	struct mm_struct *oldmm = prev->active_mm;

	if (unlikely(!mm)) {
		next->active_mm = oldmm;
		atomic_inc(&oldmm->mm_count);
		enter_lazy_tlb(oldmm, next, smp_processor_id());
	} else
		switch_mm(oldmm, mm, next, smp_processor_id());

	if (unlikely(!prev->mm)) {
		prev->active_mm = NULL;
		mmdrop(oldmm);
	}

	/* Here we just switch the register state and the stack. */
	switch_to(prev, next, prev);

	return prev;
}

unsigned long nr_running(void)
{
	unsigned long i, sum = 0;

	for (i = 0; i < smp_num_cpus; i++)
		sum += cpu_rq(cpu_logical_map(i))->nr_running;

	return sum;
}

unsigned long nr_uninterruptible(void)
{
	unsigned long i, sum = 0;

	for (i = 0; i < smp_num_cpus; i++)
		sum += cpu_rq(cpu_logical_map(i))->nr_uninterruptible;

	return sum;
}

unsigned long nr_context_switches(void)
{
	unsigned long i, sum = 0;

	for (i = 0; i < smp_num_cpus; i++)
		sum += cpu_rq(cpu_logical_map(i))->nr_switches;

	return sum;
}

#if CONFIG_SMP
/*
 * Lock the busiest runqueue as well, this_rq is locked already.
 * Recalculate nr_running if we have to drop the runqueue lock.
 */
static inline unsigned int double_lock_balance(runqueue_t *this_rq,
	runqueue_t *busiest, int this_cpu, int idle, unsigned int nr_running)
{
	if (unlikely(!spin_trylock(&busiest->lock))) {
		if (busiest < this_rq) {
			spin_unlock(&this_rq->lock);
			spin_lock(&busiest->lock);
			spin_lock(&this_rq->lock);
			/* Need to recalculate nr_running */
			if (idle || (this_rq->nr_running > this_rq->prev_nr_running[this_cpu]))
				nr_running = this_rq->nr_running;
			else
				nr_running = this_rq->prev_nr_running[this_cpu];
		} else
			spin_lock(&busiest->lock);
	}
	return nr_running;
}

/*
 * Current runqueue is empty, or rebalance tick: if there is an
 * inbalance (current runqueue is too short) then pull from
 * busiest runqueue(s).
 *
 * We call this with the current runqueue locked,
 * irqs disabled.
 */
static void load_balance(runqueue_t *this_rq, int idle)
{
	int imbalance, nr_running, load, max_load,
		idx, i, this_cpu = smp_processor_id();
	task_t *next = this_rq->idle, *tmp;
	runqueue_t *busiest, *rq_src;
	prio_array_t *array;
	list_t *head, *curr;

	/*
	 * We search all runqueues to find the most busy one.
	 * We do this lockless to reduce cache-bouncing overhead,
	 * we re-check the 'best' source CPU later on again, with
	 * the lock held.
	 *
	 * We fend off statistical fluctuations in runqueue lengths by
	 * saving the runqueue length during the previous load-balancing
	 * operation and using the smaller one the current and saved lengths.
	 * If a runqueue is long enough for a longer amount of time then
	 * we recognize it and pull tasks from it.
	 *
	 * The 'current runqueue length' is a statistical maximum variable,
	 * for that one we take the longer one - to avoid fluctuations in
	 * the other direction. So for a load-balance to happen it needs
	 * stable long runqueue on the target CPU and stable short runqueue
	 * on the local runqueue.
	 *
	 * We make an exception if this CPU is about to become idle - in
	 * that case we are less picky about moving a task across CPUs and
	 * take what can be taken.
	 */
	if (idle || (this_rq->nr_running > this_rq->prev_nr_running[this_cpu]))
		nr_running = this_rq->nr_running;
	else
		nr_running = this_rq->prev_nr_running[this_cpu];

	busiest = NULL;
	max_load = 1;
	for (i = 0; i < smp_num_cpus; i++) {
		int logical = cpu_logical_map(i);

		rq_src = cpu_rq(logical);
		if (idle || (rq_src->nr_running < this_rq->prev_nr_running[logical]))
			load = rq_src->nr_running;
		else
			load = this_rq->prev_nr_running[logical];
		this_rq->prev_nr_running[logical] = rq_src->nr_running;

		if ((load > max_load) && (rq_src != this_rq)) {
			busiest = rq_src;
			max_load = load;
		}
	}

	if (likely(!busiest))
		return;

	imbalance = (max_load - nr_running) / 2;

	/* It needs an at least ~25% imbalance to trigger balancing. */
	if (!idle && (imbalance < (max_load + 3)/4))
		return;

	nr_running = double_lock_balance(this_rq, busiest, this_cpu, idle, nr_running);
	/*
	 * Make sure nothing changed since we checked the
	 * runqueue length.
	 */
	if (busiest->nr_running <= nr_running + 1)
		goto out_unlock;

	/*
	 * We first consider expired tasks. Those will likely not be
	 * executed in the near future, and they are most likely to
	 * be cache-cold, thus switching CPUs has the least effect
	 * on them.
	 */
	if (busiest->expired->nr_active)
		array = busiest->expired;
	else
		array = busiest->active;

new_array:
	/* Start searching at priority 0: */
	idx = 0;
skip_bitmap:
	if (!idx)
		idx = sched_find_first_bit(array->bitmap);
	else
		idx = find_next_bit(array->bitmap, MAX_PRIO, idx);
	if (idx == MAX_PRIO) {
		if (array == busiest->expired) {
			array = busiest->active;
			goto new_array;
		}
		goto out_unlock;
	}

	head = array->queue + idx;
	curr = head->prev;
skip_queue:
	tmp = list_entry(curr, task_t, run_list);

	/*
	 * We do not migrate tasks that are:
	 * 1) running (obviously), or
	 * 2) cannot be migrated to this CPU due to cpus_allowed, or
	 * 3) are cache-hot on their current CPU.
	 */

#define CAN_MIGRATE_TASK(p,rq,this_cpu)					\
	((jiffies - (p)->sleep_timestamp > cache_decay_ticks) &&	\
		((p) != (rq)->curr) &&					\
			((p)->cpus_allowed & (1UL << (this_cpu))))

	curr = curr->prev;

	if (!CAN_MIGRATE_TASK(tmp, busiest, this_cpu)) {
		if (curr != head)
			goto skip_queue;
		idx++;
		goto skip_bitmap;
	}
	next = tmp;
	/*
	 * take the task out of the other runqueue and
	 * put it into this one:
	 */
	dequeue_task(next, array);
	busiest->nr_running--;
	next->cpu = this_cpu;
	this_rq->nr_running++;
	enqueue_task(next, this_rq->active);
	if (next->prio < current->prio)
		set_need_resched();
	if (!idle && --imbalance) {
		if (curr != head)
			goto skip_queue;
		idx++;
		goto skip_bitmap;
	}
out_unlock:
	spin_unlock(&busiest->lock);
}

/*
 * One of the idle_cpu_tick() or the busy_cpu_tick() function will
 * gets called every timer tick, on every CPU. Our balancing action
 * frequency and balancing agressivity depends on whether the CPU is
 * idle or not.
 *
 * busy-rebalance every 250 msecs. idle-rebalance every 1 msec. (or on
 * systems with HZ=100, every 10 msecs.)
 */
#define BUSY_REBALANCE_TICK (HZ/4 ?: 1)
#define IDLE_REBALANCE_TICK (HZ/1000 ?: 1)

static inline void idle_tick(void)
{
	if (jiffies % IDLE_REBALANCE_TICK)
		return;
	spin_lock(&this_rq()->lock);
	load_balance(this_rq(), 1);
	spin_unlock(&this_rq()->lock);
}

#endif

/*
 * We place interactive tasks back into the active array, if possible.
 *
 * To guarantee that this does not starve expired tasks we ignore the
 * interactivity of a task if the first expired task had to wait more
 * than a 'reasonable' amount of time. This deadline timeout is
 * load-dependent, as the frequency of array switched decreases with
 * increasing number of running tasks:
 */
#define EXPIRED_STARVING(rq) \
		((rq)->expired_timestamp && \
		(jiffies - (rq)->expired_timestamp >= \
			STARVATION_LIMIT * ((rq)->nr_running) + 1))

/*
 * This function gets called by the timer code, with HZ frequency.
 * We call it with interrupts disabled.
 */
void scheduler_tick(int user_tick, int system)
{
	int cpu = smp_processor_id();
	runqueue_t *rq = this_rq();
	task_t *p = current;

	if (p == rq->idle) {
		if (local_bh_count(cpu) || local_irq_count(cpu) > 1)
			kstat.per_cpu_system[cpu] += system;
#if CONFIG_SMP
		idle_tick();
#endif
		return;
	}
	if (TASK_NICE(p) > 0)
		kstat.per_cpu_nice[cpu] += user_tick;
	else
		kstat.per_cpu_user[cpu] += user_tick;
	kstat.per_cpu_system[cpu] += system;

	/* Task might have expired already, but not scheduled off yet */
	//hw2 -cz
	if ((p->array != rq->active) && (!IS_SHORT(p))) {				//That means it must be expired
		set_tsk_need_resched(p);
		p->reason = The_time_slice_of_the_previous_task_has_ended;	//hw2 - cz - monitoring
		return;
	}
	spin_lock(&rq->lock);
	if (unlikely(rt_task(p))) {
		/*
		 * RR tasks need a special form of timeslice management.
		 * FIFO tasks have no timeslices.
		 */
		if ((p->policy == SCHED_RR) && !--p->time_slice) {
			p->time_slice = TASK_TIMESLICE(p);
			p->first_time_slice = 0;
			set_tsk_need_resched(p);
			p->reason = The_time_slice_of_the_previous_task_has_ended;	//hw2 - cz - monitoring

			/* put it at the end of the queue: */
			dequeue_task(p, rq->active);
			enqueue_task(p, rq->active);
		}
		goto out;
	}
	/*
	 * The task was running during this tick - update the
	 * time slice counter and the sleep average. Note: we
	 * do not update a process's priority until it either
	 * goes to sleep or uses up its timeslice. This makes
	 * it possible for interactive tasks to use up their
	 * timeslices at their highest priority levels.
	 */
	if (p->sleep_avg)
		p->sleep_avg--;

	/***************************************************************************
	 * 	HW2 Lotem -   -- Main change of the Schedueling Policy --

	 	 1 - We first check if a SHORT process is indeed SHORT and not OVERDUE
	 	     then we calculate it's new TimeSlice and Used Trials
	 	     (**If the process is OVERDUE we DO NOTHING! - because it's FIFO)

	 	 2 - If the next  TimeSlice is 0 or the Used Trials are at max value,
	 	     we dequeue it from SHORT prio_array_t and move it to SHORT_OVERDUE
	 	     and we change it's prio to 0 (all OVERDUE processes have same prio)

	 	 3 - If it's still a SHORT process after the changes we dequeue and enqueue
	 	     in the SHORT prio array, as in RR policy with it's new TimeSlice
	 **************************************************************************/
	if (IS_SHORT(p)){
		if ((!IS_OVERDUE(p)) && (!--p->time_slice)){		//1
			p->used_trials++;
			int next_time_slice = ((p->requested_time)/(p->used_trials));
			p->time_slice = next_time_slice;
			dequeue_task(p, rq->SHORT);
			set_tsk_need_resched(p);

			if (IS_OVERDUE(p) || !next_time_slice){							//2
				p->prio = OVERDUE_PRIO;
				p->used_trials = p->requested_trials + 1;
				enqueue_task(p, rq->SHORT_OVERDUE);
				p->reason = A_SHORT_process_became_overdue;	//hw2 - cz - monitoring
			}else {
				enqueue_task(p, rq->SHORT);				//3
				p->reason = The_time_slice_of_the_previous_task_has_ended; //hw2 - cz - monitoring
			}

		}
	/*************      End of HW2 Lotem  - 30.4.2015     	*******************/

	} else if (!--p->time_slice) {
		dequeue_task(p, rq->active);
		set_tsk_need_resched(p);
		p->reason = The_time_slice_of_the_previous_task_has_ended; //hw2 - cz - monitoring
		p->prio = effective_prio(p);
		p->first_time_slice = 0;
		p->time_slice = TASK_TIMESLICE(p);

		if (!TASK_INTERACTIVE(p) || EXPIRED_STARVING(rq)) {
			if (!rq->expired_timestamp)
				rq->expired_timestamp = jiffies;
			enqueue_task(p, rq->expired);
		} else
			enqueue_task(p, rq->active);
	}
out:
#if CONFIG_SMP
	if (!(jiffies % BUSY_REBALANCE_TICK))
		load_balance(rq, 0);
#endif
	spin_unlock(&rq->lock);
}

void scheduling_functions_start_here(void) { }

/*
 * 'schedule()' is the main scheduler function.
 */
asmlinkage void schedule(void)
{
	task_t *prev, *next;
	runqueue_t *rq;
	prio_array_t *array;
	list_t *queue;
	int idx;

	if (unlikely(in_interrupt()))
		BUG();

need_resched:
	prev = current;
	rq = this_rq();

	release_kernel_lock(prev, smp_processor_id());
	prepare_arch_schedule(prev);
	prev->sleep_timestamp = jiffies;
	spin_lock_irq(&rq->lock);

	switch (prev->state) {
	case TASK_INTERRUPTIBLE:
		if (unlikely(signal_pending(prev))) {
			prev->state = TASK_RUNNING;
			break;
		}
	default:
		deactivate_task(prev, rq);
	case TASK_RUNNING:
		;
	}
#if CONFIG_SMP
pick_next_task:
#endif
	if (unlikely(!rq->nr_running)) {
#if CONFIG_SMP
		load_balance(rq, 1);
		if (rq->nr_running)
			goto pick_next_task;
#endif
		next = rq->idle;
		current->reason = A_previous_task_goes_out_for_waiting;	//hw2 - cz - monitoring
		rq->expired_timestamp = 0;
		goto switch_tasks;
	}
	//HW2 - Lotem 30.4.15
	array = rq->active;
	if (((rq->active)->nr_active || (rq->expired)->nr_active)) {		//There are SCHED_OTHER processes in the system
		if (unlikely(!array->nr_active)) {
			/*
			 * Switch the active and expired arrays.
			 */
			rq->active = rq->expired;
			rq->expired = array;
			array = rq->active;
			rq->expired_timestamp = 0;
		}
	}

 	/***************************************************************************
 	* HW2 - Lotem 30.4.15 - Searching the next process to run based
 	* 					    on the priority, according to the PDF
 	*
	//1 - If the prio isn't Real-Time, search next SCHED_SHORT to run
	//2 - Only if all of the other processes array are empty, we check in
	*     the SHORT_OVERDUE array
 	***************************************************************************/
	idx = sched_find_first_bit(array->bitmap);

	if ((rq->SHORT->nr_active) && !(idx<MAX_RT_PRIO))	//1
	{
		array = rq->SHORT;
		idx = sched_find_first_bit(array->bitmap);
	}
	if ((rq->SHORT_OVERDUE->nr_active)&&(!rq->SHORT->nr_active)&&(!rq->active->nr_active)&&(!rq->expired->nr_active))
	{
		array = rq->SHORT_OVERDUE;							//2
		idx = sched_find_first_bit(array->bitmap);
	}
	/*************      End of HW2 Lotem  - 30.4.2015     	*******************/

	queue = array->queue + idx;
	next = list_entry(queue->next, task_t, run_list);

switch_tasks:
	prefetch(next);
	clear_tsk_need_resched(prev);

	if (likely(prev != next)) {
		rq->nr_switches++;
		rq->curr = next;
//hw2 - cz - context switch event happening, lets record!
		if (rq->p_events_count < PROCESS_MAX_TO_MONITOR){
			update_switch_info_struct(&((rq->record_array)[rq->record_idx]),
										prev->pid,
										next->pid,
										prev->policy,
										next->policy,
										jiffies,
										prev->reason);
			rq->p_events_count++;
			INC_RECORD_IDX(rq);
		}
//hw2 - cz - end of recording/monitoring additions
		prepare_arch_switch(rq);
		prev = context_switch(prev, next);
		barrier();
		rq = this_rq();
		finish_arch_switch(rq);
	} else
		spin_unlock_irq(&rq->lock);
	finish_arch_schedule(prev);

	reacquire_kernel_lock(current);
	if (need_resched())
		goto need_resched;
}

//hw2 - cz - adding sys_get_scheduling_statistic sys call
//this sys call returns an array of tasks info, sorted from the oldest to newest switch info events
int sys_get_scheduling_statistic(switch_info_t* tasks_info){

	unsigned long flags;
	runqueue_t *rq;
	int idx;
	int actual_idx;
	int result = 0;


	if (!tasks_info) {
		return -EINVAL;
	}

	local_irq_save(flags);
    rq = this_rq();
    actual_idx = rq->record_idx;
    switch_info_t returned_record[TOTAL_MAX_TO_MONITOR];

    if (rq->is_round_completed_flag){

    	for (idx = 0; idx < TOTAL_MAX_TO_MONITOR; idx++){

    		COPY_SWITCH_INFO_STRUCT(returned_record[idx], (rq->record_array)[actual_idx]);
    		if (++actual_idx == TOTAL_MAX_TO_MONITOR){
    			actual_idx = 0;
    		}
    		result++;
    	}
    } else{

    	for (idx = actual_idx - 1; idx >= 0; idx--){
    		COPY_SWITCH_INFO_STRUCT(returned_record[idx], (rq->record_array)[idx]);
    		result++;
    	}
    }
    local_irq_restore(flags);

    if (copy_to_user(tasks_info, returned_record, sizeof(struct switch_info[TOTAL_MAX_TO_MONITOR]))) {

    		return -EFAULT;
    }

    return result;
}



/*
 * The core wakeup function.  Non-exclusive wakeups (nr_exclusive == 0) just
 * wake everything up.  If it's an exclusive wakeup (nr_exclusive == small +ve
 * number) then we wake all the non-exclusive tasks and one exclusive task.
 *
 * There are circumstances in which we can try to wake a task which has already
 * started to run but is not in state TASK_RUNNING.  try_to_wake_up() returns
 * zero in this (rare) case, and we handle it by continuing to scan the queue.
 */
static inline void __wake_up_common(wait_queue_head_t *q, unsigned int mode, int nr_exclusive, int sync)
{
	struct list_head *tmp;
	unsigned int state;
	wait_queue_t *curr;
	task_t *p;

	list_for_each(tmp, &q->task_list) {
		curr = list_entry(tmp, wait_queue_t, task_list);
		p = curr->task;
		state = p->state;
		if ((state & mode) && try_to_wake_up(p, sync) &&
			((curr->flags & WQ_FLAG_EXCLUSIVE) && !--nr_exclusive))
				break;
	}
}

void __wake_up(wait_queue_head_t *q, unsigned int mode, int nr_exclusive)
{
	unsigned long flags;

	if (unlikely(!q))
		return;

	spin_lock_irqsave(&q->lock, flags);
	__wake_up_common(q, mode, nr_exclusive, 0);
	spin_unlock_irqrestore(&q->lock, flags);
}

#if CONFIG_SMP

void __wake_up_sync(wait_queue_head_t *q, unsigned int mode, int nr_exclusive)
{
	unsigned long flags;

	if (unlikely(!q))
		return;

	spin_lock_irqsave(&q->lock, flags);
	if (likely(nr_exclusive))
		__wake_up_common(q, mode, nr_exclusive, 1);
	else
		__wake_up_common(q, mode, nr_exclusive, 0);
	spin_unlock_irqrestore(&q->lock, flags);
}

#endif
 
void complete(struct completion *x)
{
	unsigned long flags;

	spin_lock_irqsave(&x->wait.lock, flags);
	x->done++;
	__wake_up_common(&x->wait, TASK_UNINTERRUPTIBLE | TASK_INTERRUPTIBLE, 1, 0);
	spin_unlock_irqrestore(&x->wait.lock, flags);
}

void wait_for_completion(struct completion *x)
{
	spin_lock_irq(&x->wait.lock);
	if (!x->done) {
		DECLARE_WAITQUEUE(wait, current);

		wait.flags |= WQ_FLAG_EXCLUSIVE;
		__add_wait_queue_tail(&x->wait, &wait);
		do {
			__set_current_state(TASK_UNINTERRUPTIBLE);
			spin_unlock_irq(&x->wait.lock);
			current->reason = A_previous_task_goes_out_for_waiting;	//hw2 - cz - monitoring
			schedule();
			spin_lock_irq(&x->wait.lock);
		} while (!x->done);
		__remove_wait_queue(&x->wait, &wait);
	}
	x->done--;
	spin_unlock_irq(&x->wait.lock);
}

#define	SLEEP_ON_VAR				\
	unsigned long flags;			\
	wait_queue_t wait;			\
	init_waitqueue_entry(&wait, current);

#define	SLEEP_ON_HEAD					\
	spin_lock_irqsave(&q->lock,flags);		\
	__add_wait_queue(q, &wait);			\
	spin_unlock(&q->lock);

#define	SLEEP_ON_TAIL						\
	spin_lock_irq(&q->lock);				\
	__remove_wait_queue(q, &wait);				\
	spin_unlock_irqrestore(&q->lock, flags);

void interruptible_sleep_on(wait_queue_head_t *q)
{
	SLEEP_ON_VAR

	current->state = TASK_INTERRUPTIBLE;
	SLEEP_ON_HEAD

    current->reason = A_previous_task_goes_out_for_waiting;	//hw2 - cz - monitoring
	schedule();
	SLEEP_ON_TAIL
}

long interruptible_sleep_on_timeout(wait_queue_head_t *q, long timeout)
{
	SLEEP_ON_VAR

	current->state = TASK_INTERRUPTIBLE;
    current->reason = A_previous_task_goes_out_for_waiting;	//hw2 - cz - monitoring

	SLEEP_ON_HEAD
	timeout = schedule_timeout(timeout);
	SLEEP_ON_TAIL

	return timeout;
}

void sleep_on(wait_queue_head_t *q)
{
	SLEEP_ON_VAR
	
	current->state = TASK_UNINTERRUPTIBLE;
	SLEEP_ON_HEAD

    current->reason = A_previous_task_goes_out_for_waiting;	//hw2 - cz - monitoring
	schedule();
	SLEEP_ON_TAIL
}

long sleep_on_timeout(wait_queue_head_t *q, long timeout)
{
	SLEEP_ON_VAR
	
	current->state = TASK_UNINTERRUPTIBLE;
    current->reason = A_previous_task_goes_out_for_waiting;	//hw2 - cz - monitoring

	SLEEP_ON_HEAD
	timeout = schedule_timeout(timeout);
	SLEEP_ON_TAIL

	return timeout;
}

void scheduling_functions_end_here(void) { }

void set_user_nice(task_t *p, long nice)
{
	unsigned long flags;
	prio_array_t *array;
	runqueue_t *rq;

	if (TASK_NICE(p) == nice || nice < -20 || nice > 19)
		return;
	/*
	 * We have to be careful, if called from sys_setpriority(),
	 * the task might be in the middle of scheduling on another CPU.
	 */
	rq = task_rq_lock(p, &flags);
	if (rt_task(p)) {
		p->static_prio = NICE_TO_PRIO(nice);
		goto out_unlock;
	}
	array = p->array;
	if (array)
		dequeue_task(p, array);
	p->static_prio = NICE_TO_PRIO(nice);
	p->prio = NICE_TO_PRIO(nice);

	/*************			HW2 addition   -Lotem 28.4.15 23.00			******/
	if (IS_OVERDUE(p)){
		p->prio=OVERDUE_PRIO;						//According to the PDF we should set the priority
	}												//of SHORT_OVERDUE processes to be the same
	/*************			End of HW2 addition   -Lotem 28.4.15 23.00			******/

	if (array) {
		enqueue_task(p, array);
		/*
		 * If the task is running and lowered its priority,
		 * or increased its priority then reschedule its CPU:
		 */
		if ((NICE_TO_PRIO(nice) < p->static_prio) || (p == rq->curr)){
			p->reason = A_task_with_higher_priority_returns_from_waiting;	//hw2 - cz - monitoring
			resched_task(rq->curr);
		}
	}
out_unlock:
	task_rq_unlock(rq, &flags);
}

#ifndef __alpha__

/*
 * This has been replaced by sys_setpriority.  Maybe it should be
 * moved into the arch dependent tree for those ports that require
 * it for backward compatibility?
 */

asmlinkage long sys_nice(int increment)
{
	long nice;

	/*
	 *	Setpriority might change our priority at the same moment.
	 *	We don't have to worry. Conceptually one call occurs first
	 *	and we have a single winner.
	 */
	if (increment < 0) {
		if (!capable(CAP_SYS_NICE))
			return -EPERM;
		if (increment < -40)
			increment = -40;
	}
	if (increment > 40)
		increment = 40;

	nice = PRIO_TO_NICE(current->static_prio) + increment;
	if (nice < -20)
		nice = -20;
	if (nice > 19)
		nice = 19;
	set_user_nice(current, nice);
	return 0;
}

#endif

/*
 * This is the priority value as seen by users in /proc
 *
 * RT tasks are offset by -200. Normal tasks are centered
 * around 0, value goes from -16 to +15.
 */
int task_prio(task_t *p)
{
	return p->prio - MAX_USER_RT_PRIO;
}

int task_nice(task_t *p)
{
	return TASK_NICE(p);
}

int idle_cpu(int cpu)
{
	return cpu_curr(cpu) == cpu_rq(cpu)->idle;
}

static inline task_t *find_process_by_pid(pid_t pid)
{
	return pid ? find_task_by_pid(pid) : current;
}

static int setscheduler(pid_t pid, int policy, struct sched_param *param)
{
	struct sched_param lp;
	int retval = -EINVAL;
	prio_array_t *array;
	unsigned long flags;
	runqueue_t *rq;
	task_t *p;

	if (!param || pid < 0)
		goto out_nounlock;

	retval = -EFAULT;
	if (copy_from_user(&lp, param, sizeof(struct sched_param)))
		goto out_nounlock;

	/*
	 * We play safe to avoid deadlocks.
	 */
	read_lock_irq(&tasklist_lock);

	p = find_process_by_pid(pid);

	retval = -ESRCH;
	if (!p)
		goto out_unlock_tasklist;

	/*
	 * To be able to change p->policy safely, the apropriate
	 * runqueue lock must be held.
	 */
	rq = task_rq_lock(p, &flags);


	/***************************************************************************
         HW2 - Preventing SHORT process from changing it's policy
         	   and enforcing the trial_num, and requested_time fields:
         	   The Course Demands are:
         	   1.(New trial_num == Current trial_num)
         	   2.(New requsetd_time < Current requested_time)
	 **************************************************************************/
	if (p->policy == SCHED_SHORT) {
    	if (policy == -1){			//That means set_param called setscheduler
    		if ((lp.requested_time <= 0) || (lp.requested_time > (5000)) ){
    			retval = -EINVAL;												// HW2 - Lotem 5.5.15 16.00
    			goto out_unlock;
    		}
    		p->requested_time = ms_to_ticks(lp.requested_time);
    		retval = 0;															//Success
    		goto out_unlock;
    	}
    	else {
    		retval = -EPERM;													// HW2 - Lotem 5.5.15 16.00
    		goto out_unlock;
    	}

    }
    /***		End of HW2 Additions by Lotem			***/


	if (policy < 0)
		policy = p->policy;
	else {
		retval = -EINVAL;
		if (policy != SCHED_FIFO && policy != SCHED_RR &&
				policy != SCHED_OTHER  && policy != SCHED_SHORT)		// HW2 - Lotem 28.4.15 21.00
			goto out_unlock;
	}

	/*
	 * Valid priorities for SCHED_FIFO and SCHED_RR are
	 * 1..MAX_USER_RT_PRIO-1, valid priority for SCHED_OTHER is 0.
	 */
	retval = -EINVAL;


	/*******************************************************************************
	         HW2 - Setting the input values for a SHORT process, according to
	         	   the HW demands
	 *******************************************************************************/
    if (policy == SCHED_SHORT) {
    	if (((p->uid != current->uid) && (current->uid != 0)) || rt_task(p)) {				//If it's not root or if it's another user
            retval = -EPERM;
            goto out_unlock;
        }
        if ((lp.trial_num < 1) || (lp.trial_num > 50)){
            goto out_unlock;													//Checking input values
        }
        if ((lp.requested_time <= 0) || (lp.requested_time > (5000))){			//TODO - Change it back to 5000 after the tests
            goto out_unlock;
        }
        current->reason = A_task_with_higher_priority_returns_from_waiting;		//hw2 - cz - monitoring

        array = p->array;
        if (array){
        	deactivate_task(p, task_rq(p));
        }

        p->requested_trials = lp.trial_num;
        current->need_resched = 1;
        p->requested_time = ms_to_ticks(lp.requested_time);						// Converting requested time to ticks
        p->time_slice = ms_to_ticks(lp.requested_time);							// /already_used_trials;		//Not sure If I should use the ms_to_ticks MACRO
        p->prio = p->static_prio;												/*According to the PDF...*/
        p->policy = policy;
        p->rt_priority = 0;

        if (array) {
        	activate_task(p, task_rq(p));
        }
        retval = 0;

        goto out_unlock;
    }
    /*******				End of HW2 additions 				************/	//Lotem - 28.4.2015 - 22.00

	if (lp.sched_priority < 0 || lp.sched_priority > MAX_USER_RT_PRIO-1)
		goto out_unlock;
	if ((policy == SCHED_OTHER) != (lp.sched_priority == 0))
		goto out_unlock;

	retval = -EPERM;
	if ((policy == SCHED_FIFO || policy == SCHED_RR) &&
	    !capable(CAP_SYS_NICE))
		goto out_unlock;
	if ((current->euid != p->euid) && (current->euid != p->uid) &&
	    !capable(CAP_SYS_NICE))
		goto out_unlock;

	array = p->array;
	if (array)
		deactivate_task(p, task_rq(p));
	retval = 0;
	p->policy = policy;
	p->rt_priority = lp.sched_priority;
	if (policy != SCHED_OTHER)
		p->prio = MAX_USER_RT_PRIO-1 - p->rt_priority;
	else
		p->prio = p->static_prio;
	if (array)
		activate_task(p, task_rq(p));

out_unlock:
	task_rq_unlock(rq, &flags);
out_unlock_tasklist:
	read_unlock_irq(&tasklist_lock);

out_nounlock:
	return retval;
}

asmlinkage long sys_sched_setscheduler(pid_t pid, int policy,
				      struct sched_param *param)
{
	return setscheduler(pid, policy, param);
}

asmlinkage long sys_sched_setparam(pid_t pid, struct sched_param *param)
{
	return setscheduler(pid, -1, param);
}

asmlinkage long sys_sched_getscheduler(pid_t pid)
{
	int retval = -EINVAL;
	task_t *p;

	if (pid < 0)
		goto out_nounlock;

	retval = -ESRCH;
	read_lock(&tasklist_lock);
	p = find_process_by_pid(pid);
	if (p)
		retval = p->policy;
	read_unlock(&tasklist_lock);

out_nounlock:
	return retval;
}

asmlinkage long sys_sched_getparam(pid_t pid, struct sched_param *param)
{
	struct sched_param lp;
	int retval = -EINVAL;
	task_t *p;

	if (!param || pid < 0)
		goto out_nounlock;

	read_lock(&tasklist_lock);
	p = find_process_by_pid(pid);
	retval = -ESRCH;
	if (!p)
		goto out_unlock;
	//HW2 addition to work with new added parameters - Lotem 28.4.2015 23.00
	if (!IS_SHORT(p)){
		lp.sched_priority = p->rt_priority;
	} else {
		lp.trial_num = p->requested_trials;
		lp.requested_time = ticks_to_ms(p->requested_time);								//TODO - HW2 Lotem 7.5.15
		if (IS_OVERDUE(p)){
			lp.trial_num = 0;													//TODO - HW2 - Lotem 7.5.15 16.00
		}
	}
	read_unlock(&tasklist_lock);

	/*
	 * This one might sleep, we cannot do it with a spinlock held ...
	 */
	retval = copy_to_user(param, &lp, sizeof(*param)) ? -EFAULT : 0;

out_nounlock:
	return retval;

out_unlock:
	read_unlock(&tasklist_lock);
	return retval;
}

/**
 * sys_sched_setaffinity - set the cpu affinity of a process
 * @pid: pid of the process
 * @len: length in bytes of the bitmask pointed to by user_mask_ptr
 * @user_mask_ptr: user-space pointer to the new cpu mask
 */
asmlinkage int sys_sched_setaffinity(pid_t pid, unsigned int len,
				      unsigned long *user_mask_ptr)
{
	unsigned long new_mask;
	int retval;
	task_t *p;

	if (len < sizeof(new_mask))
		return -EINVAL;

	if (copy_from_user(&new_mask, user_mask_ptr, sizeof(new_mask)))
		return -EFAULT;

	new_mask &= cpu_online_map;
	if (!new_mask)
		return -EINVAL;

	read_lock(&tasklist_lock);

	p = find_process_by_pid(pid);
	if (!p) {
		read_unlock(&tasklist_lock);
		return -ESRCH;
	}

	/*
	 * It is not safe to call set_cpus_allowed with the
	 * tasklist_lock held.  We will bump the task_struct's
	 * usage count and then drop tasklist_lock.
	 */
	get_task_struct(p);
	read_unlock(&tasklist_lock);

	if (!capable(CAP_SYS_NICE))
		new_mask &= p->cpus_allowed_mask;
	if (capable(CAP_SYS_NICE))
		p->cpus_allowed_mask |= new_mask;
	if (!new_mask) {
		retval = -EINVAL;
		goto out_unlock;
	}

	retval = -EPERM;
	if ((current->euid != p->euid) && (current->euid != p->uid) &&
			!capable(CAP_SYS_NICE))
		goto out_unlock;

	retval = 0;
	set_cpus_allowed(p, new_mask);

out_unlock:
	free_task_struct(p);
	return retval;
}

/**
 * sys_sched_getaffinity - get the cpu affinity of a process
 * @pid: pid of the process
 * @len: length in bytes of the bitmask pointed to by user_mask_ptr
 * @user_mask_ptr: user-space pointer to hold the current cpu mask
 */
asmlinkage int sys_sched_getaffinity(pid_t pid, unsigned int len,
				      unsigned long *user_mask_ptr)
{
	unsigned int real_len;
	unsigned long mask;
	int retval;
	task_t *p;

	real_len = sizeof(mask);
	if (len < real_len)
		return -EINVAL;

	read_lock(&tasklist_lock);

	retval = -ESRCH;
	p = find_process_by_pid(pid);
	if (!p)
		goto out_unlock;

	retval = 0;
	mask = p->cpus_allowed & cpu_online_map;

out_unlock:
	read_unlock(&tasklist_lock);
	if (retval)
		return retval;
	if (copy_to_user(user_mask_ptr, &mask, real_len))
		return -EFAULT;
	return real_len;
}

asmlinkage long sys_sched_yield(void)
{
	runqueue_t *rq = this_rq_lock();
	prio_array_t *array = current->array;
	int i;

	/*HW2 - Lotem - SHORT (and SHORT_OVERDUE) processes use only one LList like
	 RT processes so if they yield the CPU we just dequeue/enqueue them to the back of the list*/

	if (unlikely(rt_task(current)) || IS_SHORT(current)) {				//HW2 - Lotem 1.5.15
		list_del(&current->run_list);
		list_add_tail(&current->run_list, array->queue + current->prio);
		goto out_unlock;
	}

	list_del(&current->run_list);
	if (!list_empty(array->queue + current->prio)) {
		list_add(&current->run_list, array->queue[current->prio].next);
		goto out_unlock;
	}
	__clear_bit(current->prio, array->bitmap);

	i = sched_find_first_bit(array->bitmap);

	if (i == MAX_PRIO || i <= current->prio)
		i = current->prio;
	else
		current->prio = i;

	list_add(&current->run_list, array->queue[i].next);
	__set_bit(i, array->bitmap);

out_unlock:
	current->reason = A_task_yields_the_CPU;//hw2 - cz - monitoring
	spin_unlock(&rq->lock);

	schedule();

	return 0;
}


asmlinkage long sys_sched_get_priority_max(int policy)
{
	int ret = -EINVAL;

	switch (policy) {
	case SCHED_FIFO:
	case SCHED_RR:
		ret = MAX_USER_RT_PRIO-1;
		break;
	case SCHED_OTHER:
		ret = 0;
		break;
    case SCHED_SHORT:                              //HW2 - Lotem 1.5.15
        ret = 0;
        break;
	}
	return ret;
}

asmlinkage long sys_sched_get_priority_min(int policy)
{
	int ret = -EINVAL;

	switch (policy) {
	case SCHED_FIFO:
	case SCHED_RR:
		ret = 1;
		break;
	case SCHED_OTHER:
		ret = 0;
    case SCHED_SHORT:                              //HW2 - Lotem 1.5.15
         ret = 0;
         break;
	}
	return ret;
}

asmlinkage long sys_sched_rr_get_interval(pid_t pid, struct timespec *interval)
{
	int retval = -EINVAL;
	struct timespec t;
	task_t *p;

	if (pid < 0)
		goto out_nounlock;

	retval = -ESRCH;
	read_lock(&tasklist_lock);
	p = find_process_by_pid(pid);
	if (p)
		jiffies_to_timespec(p->policy & SCHED_FIFO ?
					 0 : TASK_TIMESLICE(p), &t);
	read_unlock(&tasklist_lock);
	if (p)
		retval = copy_to_user(interval, &t, sizeof(t)) ? -EFAULT : 0;
out_nounlock:
	return retval;
}

static void show_task(task_t * p)
{
	unsigned long free = 0;
	int state;
	static const char * stat_nam[] = { "R", "S", "D", "Z", "T", "W" };

	printk("%-13.13s ", p->comm);
	state = p->state ? __ffs(p->state) + 1 : 0;
	if (((unsigned) state) < sizeof(stat_nam)/sizeof(char *))
		printk(stat_nam[state]);
	else
		printk(" ");
#if (BITS_PER_LONG == 32)
	if (p == current)
		printk(" current  ");
	else
		printk(" %08lX ", thread_saved_pc(&p->thread));
#else
	if (p == current)
		printk("   current task   ");
	else
		printk(" %016lx ", thread_saved_pc(&p->thread));
#endif
	{
		unsigned long * n = (unsigned long *) (p+1);
		while (!*n)
			n++;
		free = (unsigned long) n - (unsigned long)(p+1);
	}
	printk("%5lu %5d %6d ", free, p->pid, p->p_pptr->pid);
	if (p->p_cptr)
		printk("%5d ", p->p_cptr->pid);
	else
		printk("      ");
	if (p->p_ysptr)
		printk("%7d", p->p_ysptr->pid);
	else
		printk("       ");
	if (p->p_osptr)
		printk(" %5d", p->p_osptr->pid);
	else
		printk("      ");
	if (!p->mm)
		printk(" (L-TLB)\n");
	else
		printk(" (NOTLB)\n");

	{
		extern void show_trace_task(task_t *tsk);
		show_trace_task(p);
	}
}

char * render_sigset_t(sigset_t *set, char *buffer)
{
	int i = _NSIG, x;
	do {
		i -= 4, x = 0;
		if (sigismember(set, i+1)) x |= 1;
		if (sigismember(set, i+2)) x |= 2;
		if (sigismember(set, i+3)) x |= 4;
		if (sigismember(set, i+4)) x |= 8;
		*buffer++ = (x < 10 ? '0' : 'a' - 10) + x;
	} while (i >= 4);
	*buffer = 0;
	return buffer;
}

void show_state(void)
{
	task_t *p;

#if (BITS_PER_LONG == 32)
	printk("\n"
	       "                         free                        sibling\n");
	printk("  task             PC    stack   pid father child younger older\n");
#else
	printk("\n"
	       "                                 free                        sibling\n");
	printk("  task                 PC        stack   pid father child younger older\n");
#endif
	read_lock(&tasklist_lock);
	for_each_task(p) {
		/*
		 * reset the NMI-timeout, listing all files on a slow
		 * console might take alot of time:
		 */
		touch_nmi_watchdog();
		show_task(p);
	}
	read_unlock(&tasklist_lock);
}

/*
 * double_rq_lock - safely lock two runqueues
 *
 * Note this does not disable interrupts like task_rq_lock,
 * you need to do so manually before calling.
 */
static inline void double_rq_lock(runqueue_t *rq1, runqueue_t *rq2)
{
	if (rq1 == rq2)
		spin_lock(&rq1->lock);
	else {
		if (rq1 < rq2) {
			spin_lock(&rq1->lock);
			spin_lock(&rq2->lock);
		} else {
			spin_lock(&rq2->lock);
			spin_lock(&rq1->lock);
		}
	}
}

/*
 * double_rq_unlock - safely unlock two runqueues
 *
 * Note this does not restore interrupts like task_rq_unlock,
 * you need to do so manually after calling.
 */
static inline void double_rq_unlock(runqueue_t *rq1, runqueue_t *rq2)
{
	spin_unlock(&rq1->lock);
	if (rq1 != rq2)
		spin_unlock(&rq2->lock);
}

void __init init_idle(task_t *idle, int cpu)
{
	runqueue_t *idle_rq = cpu_rq(cpu), *rq = cpu_rq(idle->cpu);
	unsigned long flags;

	__save_flags(flags);
	__cli();
	double_rq_lock(idle_rq, rq);

	idle_rq->curr = idle_rq->idle = idle;
	deactivate_task(idle, rq);
	idle->array = NULL;
	idle->prio = MAX_PRIO;
	idle->state = TASK_RUNNING;
	idle->cpu = cpu;
	double_rq_unlock(idle_rq, rq);
	set_tsk_need_resched(idle);
	__restore_flags(flags);
}

extern void init_timervecs(void);
extern void timer_bh(void);
extern void tqueue_bh(void);
extern void immediate_bh(void);

void __init sched_init(void)
{
	runqueue_t *rq;
	int i, j, k, idx;

	for (i = 0; i < NR_CPUS; i++) {
		prio_array_t *array;

		rq = cpu_rq(i);
		rq->active = rq->arrays;
		rq->expired = rq->arrays + 1;
		rq->SHORT = rq-> arrays + 2;				//HW2 - Lotem - 30.4.15
		rq->SHORT_OVERDUE = rq-> arrays +3;			//HW2 - Lotem - 30.4.15
		spin_lock_init(&rq->lock);
		INIT_LIST_HEAD(&rq->migration_queue);

		for (j = 0; j < 4; j++) {					//HW2 - Lotem - 30.4.15
			array = rq->arrays + j;
			for (k = 0; k < MAX_PRIO; k++) {
				INIT_LIST_HEAD(array->queue + k);
				__clear_bit(k, array->bitmap);
			}
			// delimiter for bitsearch
			__set_bit(MAX_PRIO, array->bitmap);
		}
		//hw2 - cz - init monitoring fields
		rq->record_idx = 0;
		rq->p_events_count = 0;
		rq->is_round_completed_flag = 0;
		for (idx = 0; idx < TOTAL_MAX_TO_MONITOR; idx++){
			update_switch_info_struct(&(rq->record_array[idx]),0,0,0,0,0,0);
		}
		//hw2 - cz - end of monitoring fields init
	}
	/*
	 * We have to do a little magic to get the first
	 * process right in SMP mode.
	 */
	rq = this_rq();
	rq->curr = current;
	rq->idle = current;
	wake_up_process(current);

	init_timervecs();
	init_bh(TIMER_BH, timer_bh);
	init_bh(TQUEUE_BH, tqueue_bh);
	init_bh(IMMEDIATE_BH, immediate_bh);

	/*
	 * The boot idle thread does lazy MMU switching as well:
	 */
	atomic_inc(&init_mm.mm_count);
	enter_lazy_tlb(&init_mm, current, smp_processor_id());
}

#if CONFIG_SMP

/*
 * This is how migration works:
 *
 * 1) we queue a migration_req_t structure in the source CPU's
 *    runqueue and wake up that CPU's migration thread.
 * 2) we down() the locked semaphore => thread blocks.
 * 3) migration thread wakes up (implicitly it forces the migrated
 *    thread off the CPU)
 * 4) it gets the migration request and checks whether the migrated
 *    task is still in the wrong runqueue.
 * 5) if it's in the wrong runqueue then the migration thread removes
 *    it and puts it into the right queue.
 * 6) migration thread up()s the semaphore.
 * 7) we wake up and the migration is done.
 */

typedef struct {
	list_t list;
	task_t *task;
	struct semaphore sem;
} migration_req_t;

/*
 * Change a given task's CPU affinity. Migrate the process to a
 * proper CPU and schedule it away if the CPU it's executing on
 * is removed from the allowed bitmask.
 *
 * NOTE: the caller must have a valid reference to the task, the
 * task must not exit() & deallocate itself prematurely.  The
 * call is not atomic; no spinlocks may be held.
 */
void set_cpus_allowed(task_t *p, unsigned long new_mask)
{
	unsigned long flags;
	migration_req_t req;
	runqueue_t *rq;

	new_mask &= cpu_online_map;
	if (!new_mask)
		BUG();

	rq = task_rq_lock(p, &flags);
	p->cpus_allowed = new_mask;
	/*
	 * Can the task run on the task's current CPU? If not then
	 * migrate the process off to a proper CPU.
	 */
	if (new_mask & (1UL << p->cpu)) {
		task_rq_unlock(rq, &flags);
		goto out;
	}
	/*
	 * If the task is not on a runqueue (and not running), then
	 * it is sufficient to simply update the task's cpu field.
	 */
	if (!p->array && (p != rq->curr)) {
		p->cpu = __ffs(p->cpus_allowed);
		task_rq_unlock(rq, &flags);
		goto out;
	}
	init_MUTEX_LOCKED(&req.sem);
	req.task = p;
	list_add(&req.list, &rq->migration_queue);
	task_rq_unlock(rq, &flags);
	wake_up_process(rq->migration_thread);

	down(&req.sem);
out:
}

static int migration_thread(void * bind_cpu)
{
	struct sched_param param = { sched_priority: MAX_RT_PRIO-1 };
	int cpu = cpu_logical_map((int) (long) bind_cpu);
	runqueue_t *rq;
	int ret;

	daemonize();
	sigfillset(&current->blocked);
	set_fs(KERNEL_DS);
	/*
	 * The first migration thread is started on CPU #0. This one can
	 * migrate the other migration threads to their destination CPUs.
	 */
	if (cpu != 0) {
		while (!cpu_rq(cpu_logical_map(0))->migration_thread)
			yield();
		set_cpus_allowed(current, 1UL << cpu);
	}
	printk("migration_task %d on cpu=%d\n", cpu, smp_processor_id());
	ret = setscheduler(0, SCHED_FIFO, &param);

	rq = this_rq();
	rq->migration_thread = current;

	sprintf(current->comm, "migration_CPU%d", smp_processor_id());

	for (;;) {
		runqueue_t *rq_src, *rq_dest;
		struct list_head *head;
		int cpu_src, cpu_dest;
		migration_req_t *req;
		unsigned long flags;
		task_t *p;

		spin_lock_irqsave(&rq->lock, flags);
		head = &rq->migration_queue;
		current->state = TASK_INTERRUPTIBLE;
		if (list_empty(head)) {
			spin_unlock_irqrestore(&rq->lock, flags);
			schedule();
			continue;
		}
		req = list_entry(head->next, migration_req_t, list);
		list_del_init(head->next);
		spin_unlock_irqrestore(&rq->lock, flags);

		p = req->task;
		cpu_dest = __ffs(p->cpus_allowed);
		rq_dest = cpu_rq(cpu_dest);
repeat:
		cpu_src = p->cpu;
		rq_src = cpu_rq(cpu_src);

		local_irq_save(flags);
		double_rq_lock(rq_src, rq_dest);
		if (p->cpu != cpu_src) {
			double_rq_unlock(rq_src, rq_dest);
			local_irq_restore(flags);
			goto repeat;
		}
		if (rq_src == rq) {
			p->cpu = cpu_dest;
			if (p->array) {
				deactivate_task(p, rq_src);
				activate_task(p, rq_dest);
			}
		}
		double_rq_unlock(rq_src, rq_dest);
		local_irq_restore(flags);

		up(&req->sem);
	}
}

void __init migration_init(void)
{
	int cpu;

	current->cpus_allowed = 1UL << cpu_logical_map(0);
	for (cpu = 0; cpu < smp_num_cpus; cpu++)
		if (kernel_thread(migration_thread, (void *) (long) cpu,
				CLONE_FS | CLONE_FILES | CLONE_SIGNAL) < 0)
			BUG();
	current->cpus_allowed = -1L;

	for (cpu = 0; cpu < smp_num_cpus; cpu++)
		while (!cpu_rq(cpu_logical_map(cpu))->migration_thread)
			schedule_timeout(2);
}
#endif

#if LOWLATENCY_NEEDED
#if LOWLATENCY_DEBUG

static struct lolat_stats_t *lolat_stats_head;
static spinlock_t lolat_stats_lock = SPIN_LOCK_UNLOCKED;

void set_running_and_schedule(struct lolat_stats_t *stats)
{
	spin_lock(&lolat_stats_lock);
	if (stats->visited == 0) {
		stats->visited = 1;
		stats->next = lolat_stats_head;
		lolat_stats_head = stats;
	}
	stats->count++;
	spin_unlock(&lolat_stats_lock);

	if (current->state != TASK_RUNNING)
		set_current_state(TASK_RUNNING);
	schedule();
}

void show_lolat_stats(void)
{
	struct lolat_stats_t *stats = lolat_stats_head;

	printk("Low latency scheduling stats:\n");
	while (stats) {
		printk("%s:%d: %lu\n", stats->file, stats->line, stats->count);
		stats->count = 0;
		stats = stats->next;
	}
}

#else	/* LOWLATENCY_DEBUG */

void set_running_and_schedule()
{
	if (current->state != TASK_RUNNING)
		__set_current_state(TASK_RUNNING);
	schedule();
}

#endif	/* LOWLATENCY_DEBUG */

int ll_copy_to_user(void *to_user, const void *from, unsigned long len)
{
	while (len) {
		unsigned long n_to_copy = len;
		unsigned long remainder;

		if (n_to_copy > 4096)
			n_to_copy = 4096;
		remainder = copy_to_user(to_user, from, n_to_copy);
		if (remainder)
			return remainder + len;
		to_user = ((char *)to_user) + n_to_copy;
		from = ((char *)from) + n_to_copy;
		len -= n_to_copy;
		conditional_schedule();
	}
	return 0;
}

int ll_copy_from_user(void *to, const void *from_user, unsigned long len)
{
	while (len) {
		unsigned long n_to_copy = len;
		unsigned long remainder;

		if (n_to_copy > 4096)
			n_to_copy = 4096;
		remainder = copy_from_user(to, from_user, n_to_copy);
		if (remainder)
			return remainder + len;
		to = ((char *)to) + n_to_copy;
		from_user = ((char *)from_user) + n_to_copy;
		len -= n_to_copy;
		conditional_schedule();
	}
	return 0;
}

#ifdef CONFIG_LOLAT_SYSCTL
struct low_latency_enable_struct __enable_lowlatency = { 0, };
#endif

#endif	/* LOWLATENCY_NEEDED */

