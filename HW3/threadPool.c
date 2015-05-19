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
	OsQueue* tasksQueue = tp->tasksQueue; //maybe inside the while...
	//TODO check if the head of the queue changes between parallel threads

	while (!tp->destroyFlag || (tp->destroyFlag && tp->finishAllFlag){//TODO to handle finishAllTasks flag
		sem_wait(sem);
		pthread_mutex_lock(tp->tasksMutex);
		if (osIsQueueEmpty(tp->tasksQueue)){
			pthread_mutex_unlock(tp->tasksMutex);
			//if the queue is empty and destroyFlag is on, a thread should end on anyway
			if(tp->destroyFlag) { // alon
				--tp->numOfActive;
				pthread_exit();
			}
			continue;
		}
		//TODO add mutex_dequeueMutex and lock it here **
		if(tp->destroyFlag && !tp->finishAllFlag) {		//if the queue is not empty, but destroyFlag is on &
			//if true, unlock it here **				//finishAllFlag is off, a thread will "continue" and
			pthread_mutex_unlock(tp->tasksMutex);		//then will end
			continue; // or just --tp->numOfActive + pthread_exit()
		}
		//unlock it here **
		node = osDequeue(tp->tasksQueue);
		pthread_mutex_unlock(tp->tasksMutex);
		*(node.func)(node.param);

	}
	--tp->numOfActive;
	pthread_exit();
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
//destroy function struct, in osQueue - currently not in use
void destroyFuncStruct(FuncStruct* fStruct){
	free(fStruct);
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
		return null;
	}
//step 5: allocate array
	tp->threadsArray = malloc(sizeof(pthread_t)*numOfThreads);
	if (!tp->threadsArray){
		printf("Memory allocation error\n");
		destroyMutex(tp);
		destroySemaphore(tp);
		destroyTasksQueue(tp);
		destroyThreadsPool(tp);
		return null;
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
			return null;
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

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param) {
	//ThreadPool* tp = (ThreadPool*)d;  - not clear what it supposed to be.
	ThreadPool* tp = threadPool;
	sem_t* sem = tp->semaphore;
	FuncStruct* node;
	OsQueue* tasksQueue = tp->tasksQueue;

	//if destroy was called, we do not insert more tasks to the queue
	if(tp->destroyFlag) {
		return -1;
	}

	node->func = computeFunc;
	node->func_param = param;

	pthread_mutex_lock(tp->tasksMutex);
	if (tp->destroyFlag){//we check again inside lock
		pthread_mutex_unlock(tp->tasksMutex);
		return -1;
	}else {
//todo consider sem_post outside/inside the lock
		osEnqueue(tasksQueue, node);
		pthread_mutex_unlock(tp->tasksMutex);
		sem_post(sem);
	}
	return 0;
}


void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks){
	//first thing we do is lock the tasksQueue because maybe we don't want to withdraw new tasks

	if 	(threadPool->destroyFlag){
		//cz - this case is not going to be tested
		printf("tpDestroy was called on this threadPool before...\n");
		return;
	}
	if (!threadPool){
		printf("Illegal input values!\n");
		return;
	}
	// TODO lock the mutex_dequeueMutex here
	threadPool->destroyFlag = true;
	threadPool->finishAllFlag = (shouldWaitForTasks != 0);
	// unlock it here

	/*if (shouldWaitForTasks != 0){
		while (threadPool->numOfActive > 0)
			sem_post(threadPool->semaphore);
	}*/
	//on anyway, we need to let all threads to finish. the conditions are taking care by the startRoutine
	while (threadPool->numOfActive > 0)
		sem_post(threadPool->semaphore);

	/*while (!osIsQueueEmpty(threadPool->tasksQueue)) {
		if (shouldWaitForTasks != 0){
			//unlock
			//join all threads
			//
		}else{
			//unlock and cancel all threads
		}
	}*/
	//destroy for mutex_dequeueMutex
	destroyMutex(tp);
	destroySemaphore(tp);
	destroyTasksQueue(tp);
	destroyThreadsArray(tp, tp->numOfThreads)
	destroyThreadPool(tp);

	return;
}





