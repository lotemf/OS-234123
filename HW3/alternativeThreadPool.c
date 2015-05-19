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

	while (!tp->destroyFlag || (tp->destroyFlag && tp->finishAllFlag){//TODO to handle finishAllTasks flag
		sem_wait(sem);
		pthread_mutex_lock(tp->tasksMutex);
		if (osIsQueueEmpty(tp->tasksQueue)){
			pthread_mutex_unlock(tp->tasksMutex);
			/*New Test Code - Lotem    -  This ensures us we don't keep trying to withdraw when we need to stop*/
			if (tp->destroyFlag) { //alon - we do not need this check.
				pthread_exit();	   // if the destroyFlag is on, we might still need to withdraw tasks
			}					   // because of finishAllFlag. if finishAllFlag is off, the thread won't enter the while again
			/*New Test Code - Lotem*/
			continue;
		}
		node = osDequeue(tp->tasksQueue);
		pthread_mutex_unlock(tp->tasksMutex);
		*(node.func)(node.param);
	}

	/*New Test Code - Lotem    -  If every thread exits when he checks the flag, we don't need to close all of them manually*/
	pthread_exit();
	/*New Test Code - Lotem*/

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
//join all the threads in the array
void joinThreadsArray(ThreadPool* tp, int untilIndex){							//HW3 - Lotem's Addition 18.5.2015
	for (int i = 0; i < untilIndex; ++i){
		pthread_join(array[i]);
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
//step 8: return thread pool struct pointer
	return tp;
}

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param) {
	ThreadPool* tp = (ThreadPool*)d;
	sem_t* sem = tp->semaphore;
	//if destroy was called, we do not insert more tasks to the queue
	if(tp->destroyFlag) {return -1;}

	OsQueue* tasksQueue = tp->tasksQueue;
	pthread_mutex_lock(tp->tasksMutex);

	FuncStruct* node = (FuncStruct*) malloc(sizeof(FuncStruct));
	if(!node) {
		printf("Memory allocation error\n");
		pthread_mutex_unlock(tp->tasksMutex);
		return -1;
	}
	node->func = computeFunc;
	node->func_param = param;
	osEnqueue(tasksQueue, node);

	sem_post(sem);

	pthread_mutex_unlock(tp->tasksMutex);

	return 0;
}


void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks){
	//first thing we do is lock the tasksQueue because maybe we don't want to withdraw new tasks
	pthread_mutex_lock(threadPool->tasksMutex);

	if 	(threadPool->destroyFlag){
		printf("tpDestroy was called on this threadPool before...\n");
		pthread_mutex_unlock(threadPool->tasksMutex);
		return;
	}
	if (!threadPool){
		printf("Illegal input values!\n");
		pthread_mutex_unlock(threadPool->tasksMutex);
		return;
	}

	threadPool->destroyFlag = true;
	if (shouldWaitForTasks != 0){									//
		threadPool->finishAllFlag = true;
		pthread_mutex_unlock(threadPool->tasksMutex);					//We want to continue run tasks from the queue
	}
	//We enter only if there tasks left in the tasks queue, and we want to run them all
	if  ((threadPool->finishAllFlag) && !osIsQueueEmpty(threadPool->tasksQueue)) {
		while (!osIsQueueEmpty(threadPool->tasksQueue)){							//Running all of the remaining tasks
			//We will wait for all of the threads to finish all of the tasks...
		}
		goto finishLabel;		//TestCode
	}
	//Before we check the tasksQueue we need to lock it, then we check if it's empty
	if (osIsQueueEmpty(threadPool->tasksQueue)){
//		pthread_mutex_lock(threadPool->semaphoreMutex);		/*Maybe we need a new mutex on the semaphore so no threads will be added to wait queue of it */
		while (sem_getvalue(threadPool->semaphore)){									//If there are no more tasks we would like to empty the semaphore's queue
			sem_post(threadPool->semaphore);
		}
//		joinThreadsArray(threadPool,threadPool->numOfThreads);					//We will wait for all of the threads to finish...
/*Mutx*/pthread_mutex_unlock(threadPool->tasksMutex);
//		pthread_mutex_unlock(threadPool->semaphoreMutex);		/*Maybe we need a new mutex on the semaphore so no threads will be added to wait queue of it */

		destroyMutex(tp);		//Maybe use goto... (because it's strictly code duplication)
		destroySemaphore(tp);	//Maybe use goto... (because it's strictly code duplication)
		destroyTasksQueue(tp);	//Maybe use goto... (because it's strictly code duplication)
		destroyThreadPool(tp);	//Maybe use goto... (because it's strictly code duplication)
		return;					//Maybe use goto... (because it's strictly code duplication)
	}

	//If we arrived here, we want to clean all of the remaining tasks without running them, and to finish the current threads tasks

}





