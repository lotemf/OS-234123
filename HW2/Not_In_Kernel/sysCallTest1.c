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

void testMakeShort()
{
        int thisId = getpid();
        struct sched_param param;
        int expected_requested_time = 30000;
        int expected_trial_num = 8;
        param.requested_time = expected_requested_time;
        param.trial_num = expected_trial_num;
        sched_setscheduler(thisId, SCHED_SHORT, &param);
        assert(sched_getscheduler(thisId) == SCHED_SHORT);
        assert(sched_getparam(thisId, &param) == 0);
        assert(param.requested_time == expected_requested_time);
        assert(param.trial_num == expected_trial_num);
        int i;
        doMediumTask();
        assert(sched_getparam(thisId, &param) == 0);			//This means getparam worked...
        int afterTime = remaining_time(thisId);
        assert(afterTime > 0);
//        printf("The amount of time remaining is %d \n",afterTime);
        assert(afterTime < expected_requested_time);						//Can't check this yet...
        int usedTrials = param.trial_num; 																	//there is still all the work to be done in schedule() function
//        printf("The amount of used tirals  are %d \n",usedTrials);

        printf("OK\n");
}

void testMakeSonShort()
{
        int id = fork();
        int status;
        if (id > 0) {
                struct sched_param param;
                int expected_requested_time = 30000;
                int expected_trials = 8;
                param.requested_time = expected_requested_time;
                param.trial_num = expected_trials;
                sched_setscheduler(id, SCHED_SHORT, &param);
                assert(sched_getscheduler(id) == SCHED_SHORT);
                assert(sched_getparam(id, &param) == 0);
                assert(param.requested_time == expected_requested_time);
                assert(param.trial_num <= expected_trials);
                wait(&status);
                printf("OK\n");
        } else if (id == 0) {
                doShortTask();
                _exit(0);
        }
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

void testFork()
{
        int expected_requested_time = 30000;
        int id = fork();
        int status;
        if (id > 0) {
                struct sched_param param;
                int expected_trials = 8;
                param.requested_time = expected_requested_time;
                param.trial_num = expected_trials;
                sched_setscheduler(id, SCHED_SHORT, &param);
                assert(sched_getscheduler(id) == SCHED_SHORT);
                assert(sched_getparam(id, &param) == 0);
                assert(param.requested_time == expected_requested_time);
                assert(param.trial_num == expected_trials);
                wait(&status);
                printf("OK\n");
        } else if (id == 0) {
                assert(remaining_time(getpid()) == expected_requested_time);
                int son = fork();
                if (son == 0)
                {
                        int grandson_initial_time = remaining_time(getpid());
                        assert(grandson_initial_time < (expected_requested_time*2)/3);
                        assert(grandson_initial_time > 0);
                        doMediumTask();
                        assert(remaining_time(getpid()) < grandson_initial_time);
                        _exit(0);
                }
                else
                {
                        assert(remaining_time(getpid()) < expected_requested_time/2);
                        wait(&status);
                }
                _exit(0);
        }
}

void testBecomingOverdue()					// HW2 - Lotem - NOT SURE HOW TO RUN THIS TEST HERE...
{
        int id = fork();
        int status;
        if (id > 0) {
                struct sched_param param;
                int expected_requested_time = 2;
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
//            	struct sched_param overDueparam;
//                int expected_requested_time = 2;
//                int expected_trials = 2;
//                overDueparam.requested_time = expected_requested_time;
//                overDueparam.trial_num = expected_trials;
//                sched_setscheduler(id, SCHED_SHORT, &overDueparam);
//                assert(sched_getscheduler(id) == SCHED_SHORT);
//                assert(sched_getparam(id, &overDueparam) == 0);
//                assert(overDueparam.trial_num == expected_trials);
                int myId = getpid();
                int i = remaining_time(myId);
                for (i; i < 2; )
                {
                        i = remaining_time(myId);
                        doShortTask();
                }
//                assert(sched_getparam(myId, &overDueparam) == 0);
//                assert((!overDueparam.trial_num) || (!overDueparam.requested_time));			//Checking if it's Overdue
                _exit(0);
        }
}
//
//void testScheduleRealTimeOverLShort()
//{
//        int id = fork();
//        int status;
//        if (id > 0) {
//                struct sched_param param1;
//                int expected_requested_time = 30000;
//                int expected_level = 8;
//                param1.lshort_params.requested_time = expected_requested_time;
//                param1.lshort_params.level = expected_level;
//
//                int id2 = fork();
//                if (id2 == 0)
//                {
//                        doMediumTask();
//                        printf("\tRT son finished\n");
//                        _exit(0);
//                }
//                else
//                {
//                        struct sched_param param2;
//                        param2.sched_priority = 1;
//                        sched_setscheduler(id, SCHED_LSHORT, &param1);  // LSHORT process
//                        sched_setscheduler(id2, 1, &param2);            //FIFO RealTime process
//                }
//                wait(&status);
//                wait(&status);
//                printf("OK\n");
//        } else if (id == 0) {
//                doMediumTask();
//                printf("\t\tLSHORT son finished\n");
//                _exit(0);
//        }
//}
//
//void testScheduleRealTimeOverLShort2()
//{
//        int id = fork();
//        int status;
//        if (id > 0) {
//                struct sched_param param1;
//                int expected_requested_time = 30000;
//                int expected_level = 8;
//                param1.lshort_params.requested_time = expected_requested_time;
//                param1.lshort_params.level = expected_level;
//
//                int id2 = fork();
//                if (id2 == 0)
//                {
//                        doMediumTask();
//                        printf("\t\tLSHORT son finished\n");
//                        _exit(0);
//                }
//                else
//                {
//                        struct sched_param param2;
//                        param2.sched_priority = 1;
//                        sched_setscheduler(id, 1, &param2);                     //FIFO RealTime process
//                        sched_setscheduler(id2, SCHED_LSHORT, &param1); // LSHORT process
//                }
//                wait(&status);
//                wait(&status);
//                printf("OK\n");
//        } else if (id == 0) {
//                doMediumTask();
//                printf("\tRT son finished\n");
//                _exit(0);
//        }
//}
//
//void testScheduleLShortOverOther()
//{
//        int id = fork();
//        int status;
//        if (id > 0) {
//                struct sched_param param1;
//                int expected_requested_time = 30000;
//                int expected_level = 8;
//                param1.lshort_params.requested_time = expected_requested_time;
//                param1.lshort_params.level = expected_level;
//
//                int id2 = fork();
//                if (id2 == 0)
//                {
//                        doMediumTask();
//                        printf("\tLSHORT son finished\n");
//                        _exit(0);
//                }
//                else
//                {
//                        struct sched_param param2;
//                        param2.sched_priority = 1;
//                        sched_setscheduler(id, 0, &param2);             // regular SCHED_OTHER
//                        sched_setscheduler(id2, SCHED_LSHORT, &param1);         // LSHORT process
//                }
//                wait(&status);
//                wait(&status);
//                printf("OK\n");
//        } else if (id == 0) {
//                doMediumTask();
//                printf("\t\tSCHED_OTHER son finished\n");
//                _exit(0);
//        }
//}
//
//void testScheduleLShortOverOther2()
//{
//        int id = fork();
//        int status;
//        if (id > 0) {
//                struct sched_param param1;
//                int expected_requested_time = 30000;
//                int expected_level = 8;
//                param1.lshort_params.requested_time = expected_requested_time;
//                param1.lshort_params.level = expected_level;
//
//                int id2 = fork();
//                if (id2 == 0)
//                {
//                        doMediumTask();
//                        printf("\t\tSCHED_OTHER son finished\n");
//                        _exit(0);
//                }
//                else
//                {
//                        struct sched_param param2;
//                        param2.sched_priority = 1;
//                        sched_setscheduler(id2, 0, &param2);            // regular SCHED_OTHER
//                        sched_setscheduler(id, SCHED_LSHORT, &param1);          // LSHORT process
//                }
//                wait(&status);
//                wait(&status);
//                printf("OK\n");
//        } else if (id == 0) {
//                doMediumTask();
//                printf("\tLSHORT son finished\n");
//                _exit(0);
//        }
//}
//
//void testScheduleOtherOverOverdue()
//{
//        int id = fork();
//        int status;
//        if (id > 0) {
//                struct sched_param param1;
//                int expected_requested_time = 1;
//                int expected_level = 8;
//                param1.lshort_params.requested_time = expected_requested_time;
//                param1.lshort_params.level = expected_level;
//
//                int id2 = fork();
//                if (id2 == 0)
//                {
//                        doMediumTask();
//                        printf("\t\tOVERDUE son finished\n");
//                        _exit(0);
//                }
//                else
//                {
//                        struct sched_param param2;
//                        param2.sched_priority = 1;
//                        sched_setscheduler(id, 0, &param2);             // regular SCHED_OTHER
//                        sched_setscheduler(id2, SCHED_LSHORT, &param1);         // LSHORT process
//                }
//                wait(&status);
//                wait(&status);
//                printf("OK\n");
//        } else if (id == 0) {
//                doMediumTask();
//                printf("\tSCHED_OTHER son finished\n");
//                _exit(0);
//        }
//}
//
//void testScheduleOtherOverOverdue2()
//{
//        int id = fork();
//        int status;
//        if (id > 0) {
//                struct sched_param param1;
//                int expected_requested_time = 1;
//                int expected_level = 8;
//                param1.lshort_params.requested_time = expected_requested_time;
//                param1.lshort_params.level = expected_level;
//
//                int id2 = fork();
//                if (id2 == 0)
//                {
//                        doMediumTask();
//                        printf("\tSCHED_OTHER son finished\n");
//                        _exit(0);
//                }
//                else
//                {
//                        struct sched_param param2;
//                        param2.sched_priority = 1;
//                        sched_setscheduler(id2, 0, &param2);            // regular SCHED_OTHER
//                        sched_setscheduler(id, SCHED_LSHORT, &param1);          // LSHORT process
//                }
//                wait(&status);
//                wait(&status);
//                printf("OK\n");
//        } else if (id == 0) {
//                doMediumTask();
//                printf("\t\tOVERDUE son finished\n");
//                _exit(0);
//        }
//}
//
//void testOverdueRoundRobin()
//{
//        int id = fork();
//        int status;
//        if (id > 0) {
//                struct sched_param param1;
//                int expected_requested_time = 1;
//                int expected_level = 8;
//                param1.lshort_params.requested_time = expected_requested_time;
//                param1.lshort_params.level = expected_level;
//
//                int id2 = fork();
//                if (id2 == 0)
//                {
//                        int i, j;
//                        for (i=0; i < 4; i++)
//                        {
//                                doMediumTask();
//                                printf("\t\tSon2\n");
//                        }
//                        _exit(0);
//                }
//                else
//                {
//                        struct sched_param param2;
//                        param2.lshort_params.requested_time = expected_requested_time;
//                        param2.lshort_params.level = expected_level;
//                        sched_setscheduler(id2, SCHED_LSHORT, &param1);
//                        sched_setscheduler(id, SCHED_LSHORT, &param2);
//                }
//                wait(&status);
//                wait(&status);
//                printf("OK\n");
//        } else if (id == 0) {
//                int i, j;
//                for (i=0; i < 4; i++)
//                {
//                        doMediumTask();
//                        printf("\tSon1\n");
//                }
//                _exit(0);
//        }
//}

int main()
{

    	printf("Testing bad parameters... ");
    	testBadParams();

        printf("Testing new System Calls... ");
        testSysCalls();

        printf("Testing SCHED_OTHER process... ");
        testOther();

        printf("Testing SCHED_SHORT process... ");
        testMakeShort();

        printf("Testing making son process SHORT... ");
        testMakeSonShort();

        printf("Testing becoming overdue... ");
        testBecomingOverdue();

//        printf("Testing fork... ");
//        testFork();						//TODO - HW2 - Check with Alon why it's not working
//
//        printf("Testing Overdues Round-Robin... \n");
//        testOverdueRoundRobin();
//
//        printf("Testing race: RT vs. LSHORT (RT is supposed to win)\n");
//        testScheduleRealTimeOverLShort();
//
//        printf("Testing race: RT vs. LSHORT #2 (RT is supposed to win)\n");
//        testScheduleRealTimeOverLShort2();
//
//        printf("Testing race: LSHORT vs. OTHER (LSHORT is supposed to win)\n");
//        testScheduleLShortOverOther();
//
//        printf("Testing race: LSHORT vs. OTHER #2(LSHORT is supposed to win)\n");
//        testScheduleLShortOverOther2();
//
//        printf("Testing race: OTHER vs. OVERDUE (OTHER is supposed to win)\n");
//        testScheduleOtherOverOverdue();
//
//        printf("Testing race: OTHER vs. OVERDUE #2 (OTHER is supposed to win)\n");
//        testScheduleOtherOverOverdue2();
//
//        printf("Testing making this process LSHORT... ");
//        testMakeLshort();
//        printf("Success!\n");
        return 0;
}
