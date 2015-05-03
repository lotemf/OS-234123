#include "syscalls_HW2.h"
#include <stdio.h>
#include <assert.h>

#define HZ 512

void doLongTask()
{
        long i;
        for (i=1; i > 0; i++)
        {
                ;
        }
}

void doShortTask()
{
        short i;
        for (i=1; i != 0; i++)
        {
                ;
        }
}

void doMediumTask()
{
        int j;
        for(j=0; j<4000; j++)
        {
                doShortTask();
        }
}

void testBadParams()
{
        int id = fork();
        int status;
        if (id>0)
        {
                struct sched_param param;
                int expected_requested_time = 7;
                int expected_trials = 51;
                param.requested_time = expected_requested_time;
                param.trial_num = expected_trials;
                assert(sched_setscheduler(id, SCHED_SHORT, &param) == -1);
                assert(errno = 22);
//                printf("The process policy is %d \n",sched_getscheduler(id));
                assert(sched_getscheduler(id) == 0);

                expected_requested_time = 5100000;
                expected_trials = 7;
                param.requested_time = expected_requested_time;
                param.trial_num = expected_trials;
                assert(sched_setscheduler(id, SCHED_SHORT, &param) == -1);
                assert(errno = 22);
                assert(sched_getscheduler(id) == 0);
                wait(&status);
                printf("OK\n");
        } else if (id == 0) {
                doShortTask();
                _exit(0);
        }
}

void testOther()
{
        int thisId = getpid();
        assert(sched_getscheduler(thisId) == 0);
        assert(is_SHORT(thisId) == -1);				//This means it a SCHED_OTHER process
        assert(errno == 22);
        assert(remaining_time(thisId) == -1);
        assert(errno == 22);
        assert(remaining_trials(thisId) == -1);
        assert(errno == 22);
        printf("OK\n");
}

void testSysCalls()
{
        int id = fork();
        int status;
        if (id > 0)
        {
            assert(is_SHORT(id) == -1);
            assert(errno == 22);							//because it's not a SHORT process
            assert(remaining_time(id) == -1);
            assert(errno == 22);							//because it's not a SHORT process
            assert(remaining_trials(id) == -1);
            assert(errno == 22);

            struct sched_param param;
            int expected_requested_time = 30000;
            int expected_trials = 8;
            param.requested_time = expected_requested_time;
            param.trial_num = expected_trials;
            sched_setscheduler(id, SCHED_SHORT, &param);
            int remaining_time1 = remaining_time(id);
            int remaining_trials1 = remaining_trials(id);
            assert(remaining_time1 <= expected_requested_time);
            assert(remaining_time1 > 0);
            assert(remaining_trials1 > 1);
            wait(&status);
            printf("OK\n");
        } else if (id == 0) {
                doShortTask();
                _exit(0);
        }
}

void testMakeShort()
{
        int thisId = getpid();
        struct sched_param inputParam,outputParam;
        int expected_requested_time = 30000;
        int expected_trial_num = 8;
        inputParam.requested_time = expected_requested_time;
        inputParam.trial_num = expected_trial_num;
        sched_setscheduler(thisId, SCHED_SHORT, &inputParam);
        assert(sched_getscheduler(thisId) == SCHED_SHORT);
        assert(sched_getparam(thisId, &outputParam) == 0);
        assert(outputParam.requested_time == expected_requested_time);
        assert(outputParam.trial_num == expected_trial_num);
        int i;
        doMediumTask();
        assert(sched_getparam(thisId, &outputParam) == 0);			//This means getparam worked...
        int afterTime = remaining_time(thisId);
        assert(afterTime > 0);
//        printf("The amount of time remaining is %d \n",afterTime);
        assert(afterTime < expected_requested_time);						//Can't check this yet...
        int usedTrials = outputParam.trial_num; 																	//there is still all the work to be done in schedule() function
//        printf("The amount of used tirals  are %d \n",usedTrials);

        printf("OK\n");
}

void testMakeSonShort()
{
        int id = fork();
        int status;
        if (id > 0) {
            struct sched_param inputParam,outputParam;
            int expected_requested_time = 30000;
            int expected_trials = 8;
            inputParam.requested_time = expected_requested_time;
            inputParam.trial_num = expected_trials;
            sched_setscheduler(id, SCHED_SHORT, &inputParam);
            assert(sched_getscheduler(id) == SCHED_SHORT);
            assert(sched_getparam(id, &outputParam) == 0);
            assert(outputParam.requested_time == expected_requested_time);
            assert(outputParam.trial_num <= expected_trials);
            wait(&status);
            printf("OK\n");
        } else if (id == 0) {
            doShortTask();
            _exit(0);
        }
}

void testFork()
{
        int expected_requested_time = 30000;
        int id = fork();
        int status;
        if (id > 0) {
                struct sched_param inputParam,outputParam;
                int expected_trials = 8;
                inputParam.requested_time = expected_requested_time;
                inputParam.trial_num = expected_trials;
                sched_setscheduler(id, SCHED_SHORT, &inputParam);
                assert(sched_getscheduler(id) == SCHED_SHORT);
                assert(sched_getparam(id, &outputParam) == 0);
                assert(outputParam.requested_time == expected_requested_time);
                assert(outputParam.trial_num <= expected_trials);
                wait(&status);
                printf("OK\n");
        } else if (id == 0) {
//        		printf("The remaining time of the running process is: %d \n",remaining_time(getpid()));
                assert(remaining_time(getpid()) == expected_requested_time);
                int son = fork();
                if (son == 0)
                {
                	int grandson_initial_time = remaining_time(getpid());
                    assert(grandson_initial_time <= expected_requested_time/2);
//                    assert(grandson_initial_time <= expected_requested_time/2);	//TODO - Check Trial_number
                    assert(grandson_initial_time > 0);
                    doMediumTask();
                    assert(remaining_time(getpid()) < grandson_initial_time);
                    _exit(0);
                }
                else
                {
                    assert(remaining_time(getpid()) <= expected_requested_time/2);
                    wait(&status);
                }
                _exit(0);
        }
}

void testBecomingOverdue()					// HW2 - Lotem - NOT SURE HOW TO RUN THIS TEST HERE...
{											//because we need to check the trial num
        int id = fork();
        int status;
        if (id > 0) {
                struct sched_param param;
                int expected_requested_time =5;
                int expected_trials = 2;
                param.requested_time = expected_requested_time;
                param.trial_num = expected_trials;
                sched_setscheduler(id, SCHED_SHORT, &param);
                assert(sched_getscheduler(id) == SCHED_SHORT);
                assert(sched_getparam(id, &param) == 0);
                assert(param.trial_num == expected_trials);
                wait(&status);
                printf("OK\n");
        } else if (id == 0) {
                int myId = getpid();
                int i = remaining_time(myId);
                for (i; i < 2; )
                {
                        i = remaining_time(myId);
                        doShortTask();
                }
                _exit(0);
        }
}

void testScheduleRealTimeOverShort()
{
        int id = fork();
        int status;
        if (id > 0) {
                struct sched_param param1;
                int expected_requested_time = 300000;
                int expected_trials = 8;
                param1.requested_time = expected_requested_time;
                param1.trial_num = expected_trials;

                int id2 = fork();
                if (id2 == 0)
                {
                        doMediumTask();
                        printf("\tRT son finished\n");
                        _exit(0);
                }
                else
                {
                        struct sched_param param2;
                        param2.sched_priority = 1;
                        sched_setscheduler(id, SCHED_SHORT, &param1);  // SHORT process
                        sched_setscheduler(id2, 1, &param2);            //FIFO RealTime process
                }
                wait(&status);
                wait(&status);
                printf("OK\n");
        } else if (id == 0) {
                doMediumTask();
                printf("\t\tSHORT son finished\n");
                _exit(0);
        }
}

void testScheduleRealTimeOverShort2()
{
        int id = fork();
        int status;
        if (id > 0) {
                struct sched_param param1;
                int expected_requested_time = 30000;
                int expected_trials = 8;
                param1.requested_time = expected_requested_time;
                param1.trial_num = expected_trials;

                int id2 = fork();
                if (id2 == 0)
                {
                        doMediumTask();
                        printf("\t\tSHORT son finished\n");
                        _exit(0);
                }
                else
                {
                        struct sched_param param2;
                        param2.sched_priority = 1;
                        sched_setscheduler(id, 1, &param2);                     //FIFO RealTime process
                        sched_setscheduler(id2, SCHED_SHORT, &param1); // SHORT process
                }
                wait(&status);
                wait(&status);
                printf("OK\n");
        } else if (id == 0) {
                doMediumTask();
                printf("\tRT son finished\n");
                _exit(0);
        }
}

void testScheduleShortOverOther()
{
        int id = fork();
        int status;
        if (id > 0) {
            struct sched_param param1;
            int expected_requested_time = 30000;
            int expected_trials = 8;
            param1.requested_time = expected_requested_time;
            param1.trial_num = expected_trials;

            int id2 = fork();
            if (id2 == 0)
            	{
                 doMediumTask();
                 printf("\tSHORT son finished\n");
                 _exit(0);
            }
            else
            {
                struct sched_param param2;
                param2.sched_priority = 1;
                sched_setscheduler(id, 0, &param2);             // regular SCHED_OTHER
                sched_setscheduler(id2, SCHED_SHORT, &param1);         // SHORT process
            }
            wait(&status);
            wait(&status);
            printf("OK\n");
        } else if (id == 0) {
            doMediumTask();
            printf("\t\tSCHED_OTHER son finished\n");
            _exit(0);
        }
}

void testScheduleShortOverOther2()
{
        int id = fork();
        int status;
        if (id > 0) {
            struct sched_param param1;
            int expected_requested_time = 30000;
            int expected_trials = 8;
            param1.requested_time = expected_requested_time;
            param1.trial_num = expected_trials;

            int id2 = fork();
            if (id2 == 0){
            	doMediumTask();
                printf("\t\tSCHED_OTHER son finished\n");
                _exit(0);
            }
            else
            {
                struct sched_param param2;
                param2.sched_priority = 1;
                sched_setscheduler(id2, 0, &param2);            // regular SCHED_OTHER
                sched_setscheduler(id, SCHED_SHORT, &param1);          // SHORT process
            }
            wait(&status);
            wait(&status);
            printf("OK\n");
        } else if (id == 0) {
            doMediumTask();
            printf("\tSHORT son finished\n");
            _exit(0);
        }
}

void testScheduleOtherOverOverdue()												//TODO - this test hangs the system
{
        int id = fork();
        int status;
        if (id > 0) {
                struct sched_param param1;
                int expected_requested_time = 1;
                int expected_trials = 8;
                param1.requested_time = expected_requested_time;
                param1.trial_num = expected_trials;

                int id2 = fork();
                if (id2 == 0)
                {
                        doMediumTask();
                        printf("\t\tOVERDUE son finished\n");
                        _exit(0);
                }
                else
                {
                        struct sched_param param2;
                        param2.sched_priority = 1;
                        sched_setscheduler(id, 0, &param2);             // regular SCHED_OTHER
                        sched_setscheduler(id2, SCHED_SHORT, &param1);         // SHORT process
                }
                wait(&status);
                wait(&status);
                printf("OK\n");
        } else if (id == 0) {
                doMediumTask();
                printf("\tSCHED_OTHER son finished\n");
                _exit(0);
        }
}

void testScheduleOtherOverOverdue2()
{
        int id = fork();
        int status;
        if (id > 0) {
                struct sched_param param1;
                int expected_requested_time = 1;
                int expected_trials = 8;
                param1.requested_time = expected_requested_time;
                param1.trial_num = expected_trials;

                int id2 = fork();
                if (id2 == 0)
                {
                        doMediumTask();
                        printf("\tSCHED_OTHER son finished\n");
                        _exit(0);
                }
                else
                {
                        struct sched_param param2;
                        param2.sched_priority = 1;
                        sched_setscheduler(id2, 0, &param2);            // regular SCHED_OTHER
                        sched_setscheduler(id, SCHED_SHORT, &param1);          // SHORT process
                }
                wait(&status);
                wait(&status);
                printf("OK\n");
        } else if (id == 0) {
                doMediumTask();
                printf("\t\tOVERDUE son finished\n");
                _exit(0);
        }
}

void testSHORTRoundRobin()				//TODO - Change it to SHORT RR
{
        int id = fork();
        int status;
        if (id > 0) {
                struct sched_param param1;
                int expected_requested_time = 100;
                int expected_trials = 500;
                param1.requested_time = expected_requested_time;
                param1.trial_num = expected_trials;

                int id2 = fork();
                if (id2 == 0)
                {
                        int i, j;
                        for (i=0; i < 4; i++)
                        {
                                doMediumTask();
                                printf("\t\tSon2\n");
                        }
                        _exit(0);
                }
                else
                {
                        struct sched_param param2;
                        param2.requested_time = expected_requested_time;
                        param2.trial_num = expected_trials;
                        sched_setscheduler(id2, SCHED_SHORT, &param1);
                        sched_setscheduler(id, SCHED_SHORT, &param2);
                }
                wait(&status);
                wait(&status);
                printf("OK\n");
        } else if (id == 0) {
                int i, j;
                for (i=0; i < 4; i++)
                {
                        doMediumTask();
                        printf("\tSon1\n");
                }
                _exit(0);
        }
}

int main()
{

//    	printf("Testing bad parameters... ");
//    	testBadParams();
//
//        printf("Testing SCHED_OTHER process... ");
//        testOther();
//
//        printf("Testing new System Calls... ");
//        testSysCalls();
//
//        printf("Testing making this process SHORT... ");
//        testMakeShort();
//
//        printf("Testing making son process SHORT... ");
//        testMakeSonShort();
//
//        printf("Testing fork... ");
//        testFork();																//TODO - HW2 - Check why the system freezes with multiple tests...
//
//        printf("Testing becoming overdue... ");
//        testBecomingOverdue();
//
//        printf("Testing race: RT vs. SHORT (RT is supposed to win)\n");		//TODO - !!! ERROR !!! -  Here we get an opposite outcome
//        testScheduleRealTimeOverShort();										//TODO -
//
//        printf("Testing race: RT vs. LSHORT #2 (RT is supposed to win)\n");
//        testScheduleRealTimeOverShort2();
//
//        printf("Testing race: SHORT vs. OTHER (SHORT is supposed to win)\n");
//        testScheduleShortOverOther();
//
//        printf("Testing race: SHORT vs. OTHER #2(SHORT is supposed to win)\n");
//        testScheduleShortOverOther2();
//
//        printf("Testing SHORT processes Round-Robin... \n");
//        testSHORTRoundRobin();

	//**Important Notice:

	/****************************************************************************/
	/*    Until this point we are passing all of the tests except RT vs. SHORT	*/
	/*	  -The two test below makes the system freeze...						*/
	/****************************************************************************/
	//- TODO: The alleged reason for freeze is Kernel Panic that has something
	//		  to do with the OVERDUE_SHORT processes...
	//		  It seems that something is wrong with the way these processes run
	//		  in FIFO....      -- Not sure yet ehat is the cause...

////
        /*NOTICE*/printf("Testing race: OTHER vs. OVERDUE (OTHER is supposed to win)\n");	//TODO - This test causes the system to freeze :(
        /*Fucks Up The System*/testScheduleOtherOverOverdue();
//
//        printf("Testing race: OTHER vs. OVERDUE #2 (OTHER is supposed to win)\n");//TODO - This test causes the system to freeze :(
//        testScheduleOtherOverOverdue2();
//
        return 0;
}
