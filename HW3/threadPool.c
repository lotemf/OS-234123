/*
 * threadPool.c
 *
 *  Created on: May 18, 2015
 *      Author: czarfati
 */

#include "threadPool.h"
/*****************************************************************
 * Defines
 ****************************************************************/
/*****************************************************************
 * Helper Functions
 ****************************************************************/
//start routine function for threads
void* startThreadRoutine(void* d) {
	ThreadPool* tp = (ThreadPool*) d;
	sem_t* sem = tp->semaphore;
	FuncStruct node;

	while (1) {
		pthread_mutex_lock(tp->tasksMutex);
//destroy was called and doesn't need to finish tasks
		if (tp->destroyFlag && !tp->finishAllFlag) {
			pthread_mutex_unlock(tp->tasksMutex);
			pthread_exit(NULL);
		}
//destroy was called and need to finish all tasks
		if (tp->destroyFlag && tp->finishAllFlag) {
			if (sem_trywait(sem) != 0) {
				//no tasks to finish
				pthread_mutex_unlock(tp->tasksMutex);
				pthread_exit(NULL);
			} else{
				//wait succeeded - thread can take tasks
				if (!osIsQueueEmpty(tp->tasksQueue)) {
					//task queue is not empty - take task
					node = (FuncStruct) osDequeue(tp->tasksQueue);
					pthread_mutex_unlock(tp->tasksMutex);
					(*(node->func))(node->func_param);
				}else {
					//task queue is empty - finish/exit thread
					pthread_mutex_unlock(tp->tasksMutex);
					pthread_exit(NULL);
				}
			}
		}
//normal operation of thread
		pthread_mutex_unlock(tp->tasksMutex);
		sem_wait(sem);
		pthread_mutex_lock(tp->tasksMutex);
		node = (FuncStruct) osDequeue(tp->tasksQueue);
		pthread_mutex_unlock(tp->tasksMutex);
		(*(node->func))(node->func_param);
	}
}

//destroy thread pool allocation  - in order to destroy the whole pool you should call all of the following three functions
void destroyThreadsPool(ThreadPool* tp) {
	free (tp);
}
//destroy array until index
void destroyThreadsArray(ThreadPool* tp, int untilIndex) {
	int i;
	for (i = 0; i < untilIndex; ++i) {
		pthread_cancel(tp->threadsArray[i]);
	}
	free(tp->threadsArray);
}
//destroy semaphore
void destroySemaphore(ThreadPool* tp) {
	sem_destroy(tp->semaphore);
	free(tp->semaphore);
}
//destroy tasks and flags mutex's
void destroyMutex(ThreadPool* tp) {
	pthread_mutex_destroy(tp->tasksMutex);
	free(tp->tasksMutex);
}
//destroy tasks queue - no need to free as it is freed in OSQueue
void destroyTasksQueue(ThreadPool* tp) {
	osDestroyQueue(tp->tasksQueue);
}
//destroy function struct, in OSQueue - currently not in use
void destroyFuncStruct(FuncStruct* fStruct) {
	free(fStruct);
}
/*****************************************************************
 * Interface API Functions
 *****************************************************************/
ThreadPool* tpCreate(int numOfThreads) {

//step 1: allocate thread pool memory
	ThreadPool* tp = malloc(sizeof(ThreadPool));
	if (!tp) {
		printf("Memory allocation error\n");
		return NULL;
	}
//step 2: allocate tasks queue
	tp->tasksQueue = osCreateQueue();
	if (!tp->tasksQueue) {
		printf("Memory allocation error\n");
		destroyThreadsPool(tp);
		return NULL;
	}
//step 3: allocate semaphore
	tp->semaphore = malloc(sizeof(sem_t));
	if ((!tp->semaphore) || (sem_init(tp->semaphore, 0, 0) != 0)) {
		printf("Memory allocation error\n");
		destroyTasksQueue(tp);
		destroyThreadsPool(tp);
		return NULL;
	}
//step 4: allocate mutex
	tp->tasksMutex = malloc(sizeof(pthread_mutex_t));
	if ((!tp->tasksMutex)|| (pthread_mutex_init(tp->tasksMutex, NULL)!= 0)) {
		printf("Memory allocation error\n");
		destroySemaphore(tp);
		destroyTasksQueue(tp);
		destroyThreadsPool(tp);
		return NULL;
	}
//step 5: allocate array
	tp->threadsArray = malloc(sizeof(pthread_t) * numOfThreads);
	if (!tp->threadsArray) {
		printf("Memory allocation error\n");
		destroyMutex(tp);
		destroySemaphore(tp);
		destroyTasksQueue(tp);
		destroyThreadsPool(tp);
		return NULL;
	}
//step 6: creating threads
	int i;
	for (i = 0; i < numOfThreads; ++i) {
		if (pthread_create(&((tp->threadsArray)[i]), NULL, &startThreadRoutine,
				tp) != 0) {
			printf("Memory allocation error\n");
			destroyThreadsArray(tp, i);
			destroyMutex(tp);
			destroySemaphore(tp);
			destroyTasksQueue(tp);
			destroyThreadsPool(tp);
			return NULL;
		}
	}
//step 7: init values of numOfThreads and destroyFlag
	tp->destroyFlag = false;
	tp->finishAllFlag = false;
	tp->numOfThreads = numOfThreads;
	tp->numOfActive = numOfThreads;
//step 8: return thread pool struct pointer
	return tp;
}

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc)(void *),void* param) {
	ThreadPool* tp = threadPool;
	sem_t* sem = tp->semaphore;
	OSQueue* tasksQueue = tp->tasksQueue;
	FuncStruct node;
	node->func = computeFunc;
	node->func_param = param;

//if destroy was called, we do not insert more tasks to the queue
	pthread_mutex_lock(tp->tasksMutex);
	if (tp->destroyFlag) {
		pthread_mutex_unlock(tp->tasksMutex);
		return -1;
	}
//enqueue task
	osEnqueue(tasksQueue, node);
	sem_post(sem);
	pthread_mutex_unlock(tp->tasksMutex);

	return 0;
}

void tpDestroy(ThreadPool* tp, int shouldWaitForTasks) {

//"while tp detroy is waiting for tasks to finish other tpdestroy calls should be blocked"
//check that tpDestroy is not in progress
//check valid parameteres

	if (!tp) {
		printf("Illegal input values!\n");
		return;
	}

	pthread_mutex_lock(tp->tasksMutex);
	if (tp->destroyFlag) {
		pthread_mutex_unlock(tp->tasksMutex);
		printf("tpDestroy was called on this threadPool before...\n");
		return;
	}
	printf("[DEBUG]\tin tpDestroy function before updating flags\n");
//update flags value
	tp->destroyFlag = true;
	tp->finishAllFlag = (shouldWaitForTasks != 0);
	pthread_mutex_unlock(tp->tasksMutex);
//this loop is KASTACH in case we have threads that wait for tasks and destroy was called
	int i;
	printf("[DEBUG]\tin tpDestroy function before increasing semaphore counter\n");
	for (i = 0; i < tp->numOfThreads; ++i) {
		sem_post(tp->semaphore);
	}
//wait for all threads to finish their tasks/all tasks if needed
//TODO printf of check if threads are alive
	printf("[DEBUG]\tin tpDestroy function before pthread_join loop\n");
	for (i = 0; i < tp->numOfThreads; ++i) {
		pthread_join(tp->threadsArray[i], NULL);
	}
	printf("[DEBUG]\tin tpDestroy function before releasing flags\n");
//destroy for mutex_dequeueMutex
	destroyMutex(tp);
	destroySemaphore(tp);
	destroyTasksQueue(tp);
	destroyThreadsArray(tp, tp->numOfThreads);
	destroyThreadsPool(tp);

	return;
}

