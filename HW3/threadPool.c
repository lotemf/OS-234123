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
	while((node = osDequeue(tp->tasksQueue)) != NULL)
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
		pthread_mutex_lock(&(tp->tasksMutex));
//destroy was called and doesn't need to finish tasks
		if (tp->destroyFlag && !tp->finishAllFlag) {
//			printf("inside startRoutine. destroy flag is on, finishAll flag is off\n");
			pthread_mutex_unlock(&(tp->tasksMutex));
			return NULL;
		}
//destroy was called and need to finish all tasks
		if (tp->destroyFlag && tp->finishAllFlag) {
//			printf("inside startRoutine. destroy flag is on, finishAll flag is on\n");
			if (sem_trywait(sem) != 0) {
				//no tasks to finish
//				printf("inside startRoutine. trywait != 0\n");
				pthread_mutex_unlock(&(tp->tasksMutex));
				return NULL;
			} else{
				//wait succeeded - thread can take tasks
				if (!osIsQueueEmpty(tp->tasksQueue)) {
//					printf("inside startRoutine. trywait == 0 and tasks queue is not empty\n");
					//task queue is not empty - take task
					node = osDequeue(tp->tasksQueue);

					pthread_mutex_unlock(&(tp->tasksMutex));
					(*(node->func))(node->func_param);
					destroyFuncStruct(node);
					/*Test*/continue;
				}else {
					//task queue is empty - finish/exit thread
//					printf("inside startRoutine. trywait == 0 and tasks queue is empty\n");
					pthread_mutex_unlock(&(tp->tasksMutex));
					return NULL;
				}
			}
		}
//normal operation of thread
		pthread_mutex_unlock(&(tp->tasksMutex));
///*Test*/printf("[DEBUG-startThreadRoutine]\t Before sem_wait(sem)\n");
		sem_wait(sem);
		pthread_mutex_lock(&(tp->tasksMutex));

		if (!tp->destroyFlag) {
			node = osDequeue(tp->tasksQueue);

			pthread_mutex_unlock(&(tp->tasksMutex));
			(*(node->func))(node->func_param);
			destroyFuncStruct(node);
		}
		pthread_mutex_unlock(&(tp->tasksMutex));
	}
}

/*****************************************************************
 * Interface API Functions
 *****************************************************************/
ThreadPool* tpCreate(int numOfThreads) {

//step 1: allocate thread pool memory
	ThreadPool* tp = (ThreadPool*)malloc(sizeof(ThreadPool));
	if (!tp) {
		printf("[DEBUG-tpCreate]\tMemory allocation error in creating threads pool\n");
		return NULL;
	}
//step 2: allocate tasks queue
	tp->tasksQueue = osCreateQueue();
	if (!tp->tasksQueue) {
		printf("[DEBUG-tpCreate]\tMemory allocation error in creating tasks queue\n");
		destroyThreadsPool(tp);
		return NULL;
	}
//step 3: allocate semaphore
	if (sem_init(&(tp->semaphore), 0, 0) != 0) {
		printf("[DEBUG-tpCreate]\tMemory allocation error in creating semaphore\n");
		destroyTasksQueue(tp);
		destroyThreadsPool(tp);
		return NULL;
	}
//step 4: allocate mutex
	if (pthread_mutex_init(&(tp->tasksMutex), NULL)!= 0) {
		printf("[DEBUG-tpCreate]\tMemory allocation error in creating mutex\n");
		destroySemaphore(tp);
		destroyTasksQueue(tp);
		destroyThreadsPool(tp);
		return NULL;
	}
//step 5: allocate array
	tp->threadsArray = (pthread_t*)malloc(sizeof(pthread_t) * numOfThreads);
	if (!tp->threadsArray) {
		printf("[DEBUG-tpCreate]\tMemory allocation error in creating threads array\n");
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
		if (pthread_create(&((tp->threadsArray)[i]), NULL, startThreadRoutine,
				tp) != 0) {
			printf("[DEBUG-tpCreate]\tMemory allocation error in creating each thread\n");
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
//	/*Test*/printf("[DEBUG-tpInsertTask]\t Lotem - #1 -  numOfThreads initialized with: %d\n",tp->numOfThreads);

	FuncStruct* node = (FuncStruct*)malloc(sizeof(FuncStruct));
	node->func = computeFunc;
	node->func_param = param;


//if destroy was called, we do not insert more tasks to the queue
	pthread_mutex_lock(&(tp->tasksMutex));
	if (tp->destroyFlag) {
//		printf("inside insert, destroy flag is on\n");
		destroyFuncStruct(node);
		pthread_mutex_unlock(&(tp->tasksMutex));
		return -1;
	}
//enqueue task
//	printf("inside insert, destroy flag is off\n");
	osEnqueue(tp->tasksQueue, node);
	sem_post(&(tp->semaphore));
	pthread_mutex_unlock(&(tp->tasksMutex));

//	/*Test*/printf("[DEBUG-tpInsertTask]\t Lotem - #2 -  numOfThreads initialized with: %d\n",tp->numOfThreads);
	return 0;
}

void tpDestroy(ThreadPool* tp, int shouldWaitForTasks) {

//"while tpDestroy is waiting for tasks to finish other tpDestroy calls should be blocked"
//check that tpDestroy is not in progress
//check valid parameters

	if (!tp) {
	//	printf("Illegal input values!\n");
		return;
	}
	pthread_mutex_lock(&(tp->tasksMutex));
	if (tp->destroyFlag) {
		pthread_mutex_unlock(&(tp->tasksMutex));
	//	printf("tpDestroy was called on this threadPool before...\n");
		return;
	}
//	printf("[DEBUG]\tin tpDestroy function before updating flags\n");
//update flags value
	tp->destroyFlag = true;
	tp->finishAllFlag = (shouldWaitForTasks != 0);
//this loop is KASTACH in case we have threads that wait for tasks and destroy was called
	int i;

//	printf("[DEBUG]\tin tpDestroy function before increasing semaphore counter\n");
	for (i = 0; i < tp->numOfThreads; ++i) {
		sem_post(&(tp->semaphore));
	}
	pthread_mutex_unlock(&(tp->tasksMutex));

//wait for all threads to finish their tasks/all tasks if needed

//	printf("[DEBUG-tpDestroy]\tinside tpDestroy function before pthread_join loop\n");
	for(i = 0; i < tp->numOfThreads; ++i) {
		pthread_join((tp->threadsArray[i]), NULL);
	}

//	printf("[DEBUG-tpDestroy]\tin tpDestroy function before releasing flags\n");

//destroy for mutex_dequeueMutex
	destroyMutex(tp);
	destroySemaphore(tp);
	destroyTasksQueue(tp);
	destroyThreadsArray(tp, tp->numOfThreads);
	destroyThreadsPool(tp);

//	printf("[DEBUG-tpDestroy]\tAll memory resourced were destroyed/released\n");
	return;
}

