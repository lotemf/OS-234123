#ifndef __THREAD_POOL__
#define __THREAD_POOL__

/*
 * Includes
 */
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include "osqueue.h"

/*
 * Structs  and Enums
 */
typedef struct thread_pool
{
	int numOfThreads;
	OsQueue* tasksQueue;
	pthread_t* threadsArrPtr;
	sem_t* semaphore;
	pthread_mutex_t* tasksMutex;
	bool destroyFlag;
	bool finishAllFlag;
 //semaphur
 //number of threads
 //pointer to queue of tasks
 //array of pointers to thread ids

}ThreadPool;

/*
 * Thread Pool Public Functions
 */

//step 1: create an array of pthread_t
//step 2: create all threads
//step 3: create the semaphore
//step 3: create the task queue
//TODO create destroy functions for each field in struct.

ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

#endif
