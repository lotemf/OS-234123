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
	//need to check which params we need
void startThreadRoutine(void* d){
	ThreadPool* tp = (ThreadPool*)d;
	sem_t* sem = tp->semaphore;
	FuncStruct node;
	OsQueue* tasksQueue = tp->tasksQueue;

	while (!tp->destroyFlag || (tp->destroyFlag && tp->finishAllFlag){//TODO to handle finishAllTasks flag
		while (!sem_wait(sem)){}
		pthread_mutex_lock(tp->tasksMutex);
		if (osIsQueueEmpty(tp->tasksQueue)){
			pthread_mutex_unlock(tp->tasksMutex);
			continue;
		}
		node = osDequeue(tp->tasksQueue);
		pthread_mutex_unlock(tp->tasksMutex);
		*(node.func)(node.param);
	}
}

//destroy thread pool allocation  - in order to destroy the whole pool you should call all of the following three functions
void destroyThreadsPool(ThreadPool* tp){
	free(tp)
}
//destroy array until index
void destroyThreadsArray(ThreadPool* tp, int untilIndex){
	for (int i = 0; i < untilIndex; ++i){
		pthread_cancel(array[i]);
	}
	free(tp->threadsArray);
}
//destroy semaphore
void destroySemaphore(ThreadPool* tp){
	sem_destroy(tp->semaphore);
	free(tp->semaphore);
}
//destroy tasks mutex
void destroyMutex(ThreadPool* tp){
	pthread_mutex_destroy(tp->tasksMutex);
	free(tp->tasksMutex);
}
//destroy tasks queue - no need to free as it is freed in osQueue
void destroyTasksQueue(ThreadPool* tp){
	tp->tasksQueueosDestroyQueue(tp->tasksQueue);
}
/*****************************************************************
 * Interface API Functions
 *****************************************************************/
ThreadPool* tpCreate(int numOfThreads){
//step 1: allocate thread pool memory
	ThreadPool* tp = malloc(sizeof(ThreadPool));
	if (!tp){
		printf("Memory allocation error\n");
		return null;
	}
//step 2: allocate tasks queue
	tp->tasksQueue = osCreateQueue();
	if (!tp->tasksQueue){
		printf("Memory allocation error\n");
		destroyThreadsPool(tp);
		return null;
	}
//step 3: allocate semaphore
	tp->semaphore = malloc(sizeof(sem_t));
	if ((!tp->semaphore) || (sem_init(tp->semaphore,0,0) != 0)){
		printf("Memory allocation error\n");
		destroyTasksQueue(tp);
		destroyThreadsPool(tp);
		return null;
	}
//step 4: allocate mutex
	tp->tasksMutex = malloc(sizeof(pthread_mutex_t));

	if ((!tp->tasksMutex) || (pthread_mutex_init(tp->tasksMutex,PTHREAD_MUTEX_ERRORCHECK) != 0)){
		printf("Memory allocation error\n");
		destroySemaphore(tp);
		destroyTasksQueue(tp);
		destroyThreadsPool(tp);
	}
//step 5: allocate array
	tp->threadsArray = malloc(sizeof(pthread_t)*numOfThreads);
	if (!tp->threadsArray){
		printf("Memory allocation error\n");
		destroyMutex(tp);
		destroySemaphore(tp);
		destroyTasksQueue(tp);
		destroyThreadsPool(tp);
	}
//step 6: creating threads
	for (int i = 0; i < numOfThreads; ++i){
		if (pthread_create(&((tp->threadsArray)[i]), null, &startThreadRoutine,)!= 0){
			printf("Memory allocation error\n");
			destroyThreadsArray(tp, i);
			destroyMutex(tp);
			destroySemaphore(tp);
			destroyTasksQueue(tp);
			destroyThreadPool(tp);
		}
	}
//step 7: init values of numOfThreads and destroyFlag
	tp->destroyFlag = false;
	tp->finishAllFlag = false;
	tp->numOfThreads = numOfThreads;
//step 8: return thread pool struct
	return tp;
}
