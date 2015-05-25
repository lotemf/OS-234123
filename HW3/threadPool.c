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
//destroy thread pool allocation  - in order to destroy the whole pool you should call all of the following three functions
void destroyThreadsPool(ThreadPool* tp) {
	free (tp);
}
//destroy array until index
void destroyThreadsArray(ThreadPool* tp, int untilIndex) {
	free(tp->threadsArray);
}
//destroy semaphore
void destroySemaphore(ThreadPool* tp) {
	sem_destroy(&(tp->semaphore));
}
//destroy tasks and flags mutex's
void destroyMutex(ThreadPool* tp) {
	pthread_mutex_destroy(&(tp->tasksMutex));
}
//destroy tasks queue - no need to free as it is freed in OSQueue
void destroyTasksQueue(ThreadPool* tp) {
	FuncStruct* node;
	for (node = osDequeue(tp->tasksQueue); node != NULL; node = osDequeue(tp->tasksQueue))
//	while((node = osDequeue(tp->tasksQueue)) != NULL)
	{
	   free(node);
	}
	osDestroyQueue(tp->tasksQueue);
}
//destroy function struct, in OSQueue - currently not in use
void destroyFuncStruct(FuncStruct* fStruct) {
	free(fStruct);
}

//start routine function for threads
void* startThreadRoutine(void* d) {
	ThreadPool* tp = (ThreadPool*) d;
	sem_t* sem = &(tp->semaphore);
	FuncStruct* node;

	while (1) {
//destroy was called and doesn't need to finish tasks
		if (tp->destroyFlag && !tp->finishAllFlag) {
			return NULL;
		}
//destroy was called and need to finish all tasks
		if (tp->destroyFlag && tp->finishAllFlag) {
			if (sem_trywait(sem) != 0) {
				return NULL;
			} else{
				//wait succeeded - thread can take tasks
				pthread_mutex_lock(&(tp->tasksMutex));
				if (!osIsQueueEmpty(tp->tasksQueue)) {
					//task queue is not empty - take task
					node = osDequeue(tp->tasksQueue);
					pthread_mutex_unlock(&(tp->tasksMutex));
					(*(node->func))(node->func_param);
					destroyFuncStruct(node);
					continue;
				}else {
					//task queue is empty - finish/exit thread
					pthread_mutex_unlock(&(tp->tasksMutex));
					sem_post(&(tp->semaphore)); //CHEN - in oder to maintain correct semaphore value
					return NULL;
				}
			}
		}
//normal operation of thread
		sem_wait(sem);
		if (!tp->destroyFlag) {
			pthread_mutex_lock(&(tp->tasksMutex));
			node = osDequeue(tp->tasksQueue);
			pthread_mutex_unlock(&(tp->tasksMutex));
			(*(node->func))(node->func_param);
			destroyFuncStruct(node);
		}
	}
}

/*****************************************************************
 * Interface API Functions
 *****************************************************************/
ThreadPool* tpCreate(int numOfThreads) {

//step 1: allocate thread pool memory
	ThreadPool* tp = (ThreadPool*)malloc(sizeof(ThreadPool));
	if (!tp) {
		return NULL;
	}
//step 2: allocate tasks queue
	tp->tasksQueue = osCreateQueue();
	if (!tp->tasksQueue) {
		destroyThreadsPool(tp);
		return NULL;
	}
//step 3: allocate semaphore
	if (sem_init(&(tp->semaphore), 0, 0) != 0) {
		destroyTasksQueue(tp);
		destroyThreadsPool(tp);
		return NULL;
	}
//step 4: allocate mutex
	if (pthread_mutex_init(&(tp->tasksMutex), NULL)!= 0){
		destroySemaphore(tp);
		destroyTasksQueue(tp);
		destroyThreadsPool(tp);
		return NULL;
	}
//step 5: allocate array
	tp->threadsArray = (pthread_t*)malloc(sizeof(pthread_t) * numOfThreads);
	if (!tp->threadsArray) {
		destroyMutex(tp);
		destroySemaphore(tp);
		destroyTasksQueue(tp);
		destroyThreadsPool(tp);
		return NULL;
	}
//step 6: init values of numOfThreads and destroyFlag
	tp->destroyFlag = false;
	tp->finishAllFlag = false;
	tp->numOfThreads = numOfThreads;
//step 7: creating threads
	int i;
	for (i = 0; i < numOfThreads; ++i) {
		if (pthread_create(&((tp->threadsArray)[i]), NULL, startThreadRoutine, tp) != 0) {
			destroyThreadsArray(tp, i);
			destroyMutex(tp);
			destroySemaphore(tp);
			destroyTasksQueue(tp);
			destroyThreadsPool(tp);
			return NULL;
		}
	}
//step 8: return thread pool struct pointer
	return tp;
}

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc)(void *),void* param) {
	ThreadPool* tp = threadPool;
	FuncStruct* node = (FuncStruct*)malloc(sizeof(FuncStruct));

//if destroy was called, we do not insert more tasks to the queue
	if (tp->destroyFlag) {
		destroyFuncStruct(node);
		return -1;
	}
	if (!node){
		return -1;
	}
	node->func = computeFunc;
	node->func_param = param;

	//enqueue task
	pthread_mutex_lock(&(tp->tasksMutex));
	osEnqueue(tp->tasksQueue, node);
	pthread_mutex_unlock(&(tp->tasksMutex));
	sem_post(&(tp->semaphore));

	return 0;
}

void tpDestroy(ThreadPool* tp, int shouldWaitForTasks) {

	if (!tp) {
		return;
	}
	if (tp->destroyFlag) {
		return;
	}
	tp->destroyFlag = true;
	tp->finishAllFlag = (shouldWaitForTasks != 0);

	int i;
	for (i = 0; i <= (tp->numOfThreads) ; ++i) {
		sem_post(&(tp->semaphore));
	}

//wait for all threads to finish their tasks/all tasks if needed
	for(i = 0; i < tp->numOfThreads; ++i) {
		pthread_join((tp->threadsArray[i]), NULL);
	}

//destroy for mutex_dequeueMutex
	destroyMutex(tp);
	destroySemaphore(tp);
	destroyTasksQueue(tp);
	destroyThreadsArray(tp, tp->numOfThreads);
	destroyThreadsPool(tp);
	return;
}

