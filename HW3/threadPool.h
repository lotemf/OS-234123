#ifndef __THREAD_POOL__
#define __THREAD_POOL__

/*
 * Includes
 */
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "osqueue.h"

/*
 * Structs  and Enums
 */
//struct of functions to perform - should be inserted to osQueue
typedef struct function_struct{
	void (*func)(void*);
	void* func_param;
}* FuncStruct;

typedef struct thread_pool
{
	int numOfThreads;
	int numOfActive;  //alon
	pthread_t* threadsArray;
	OSQueue* tasksQueue;
	sem_t* semaphore;
	pthread_mutex_t *tasksMutex, *flagsMutex;
	bool destroyFlag;
	bool finishAllFlag;
 //semaphore
 //number of threads
 //pointer to queue of tasks
 //array of pointers to thread ids

}ThreadPool;

/*
 * Thread Pool Public Functions
 */
ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

#endif
