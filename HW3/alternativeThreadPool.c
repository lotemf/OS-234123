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



/*******************************************************************************
 * A few unsettled things:
 * 	1.Not sure about the joining of the threads (line # 224) because I didn't
 * 	  know how to utilize the new use af the numOfActive in my implementation
 * 	2.If you decide to change the mutexes, this code needs to be rechecked
 * 	3.There is a while loop (line # 208) , that we need to make sure doesn't
 * 	  count as busy-wait according to the course's definitions
 * 	4.I have added a function that is based on Chen's cancel all,
 * 	  that utilizes a for loop to join all of the threads
 * 	5.Maybe there is an option to unlock the tasksQueue a lot sooner,
 * 	  after we set the flags  (We didn't add the lock on the flags yet..)
 ******************************************************************************/
void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks){

	//first thing we do is lock the tasksQueue because maybe we don't want to withdraw new tasks
	pthread_mutex_lock(threadPool->tasksMutex);

	//Input Checks
	if (threadPool->destroyFlag){
		printf("tpDestroy was called on this threadPool before...\n");
		pthread_mutex_unlock(threadPool->tasksMutex);
		return;
	}
	if (!threadPool){
		printf("Illegal input values!\n");
		pthread_mutex_unlock(threadPool->tasksMutex);
		return;
	}

	//Here we set the destroy flag on, and then we check if we need to set the finishAllFlag on...
	threadPool->destroyFlag = true;
	if (shouldWaitForTasks != 0){
		threadPool->finishAllFlag = true;
		pthread_mutex_unlock(threadPool->tasksMutex);					//We want to continue run tasks from the queue
	}

	//We enter this scope only if there are tasks left in the tasks queue, and we want to run them all because of the flag
	if  ((threadPool->finishAllFlag) && !osIsQueueEmpty(threadPool->tasksQueue)) {
		while (!osIsQueueEmpty(threadPool->tasksQueue)){							//Running all of the remaining tasks
					//We will wait for all of the threads to finish all of the tasks...
		}

		goto finishLabel;
	}

	/*	**Important Notice - Because we only unlock the mutex on the tasks queue, if shouldWaitForTaks is lit
						     when we get to this scope, the mutex lock on the tasks queue is still locked
						     or we would have jumped to the finishLabel, and return form tpDestroy			*/

	if (osIsQueueEmpty(threadPool->tasksQueue)){
		while (sem_getvalue(threadPool->semaphore)){			//If there are no more tasks we would like to empty the semaphore's queue
			sem_post(threadPool->semaphore);
		}
	}

	if (threadPool->numOfActive){
		joinThreadsArray(threadPool,threadPool->numOfThreads);		//We will wait for all of the threads to finish...
	}
	pthread_mutex_unlock(threadPool->tasksMutex);

finishLabel:													//Freeing all of the mallocs
		destroyMutex(tp);
		destroySemaphore(tp);
		destroyTasksQueue(tp);
		destroyThreadPool(tp);
		return;
}
