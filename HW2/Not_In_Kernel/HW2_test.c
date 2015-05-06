#include "hw2_syscalls.h"
#include <stdio.h>
#include <assert.h>

#define HZ 512


void printMonitoringUsage(int reason){
    printf("\n the integer value of reason should be between 0 to 7\n, reason value is:\t %d", reason);
    switch (reason) {
        case 0:
            printf("\n reason is Default, means reason of context switch wasn't monitored\n\n");
            break;
        case 1:
            printf("\n reason for context switch is:\t a task was created\n\n");
            break;
        case 2:
            printf("\n reason for context switch is:\t a task was ended\n\n");
            break;
        case 3:
            printf("\n reason for context switch is:\t a task yields the CPU\n\n");
            break;
        case 4:
            printf("\n reason for context switch is:\t a SHORT process became overdue\n\n");
            break;
        case 5:
            printf("\n reason for context switch is:\t a previous task goes out for waiting\n\n");
            break;
        case 6:
            printf("\n reason for context switch is:\t a task with higher priority returns from waiting\n\n");
            break;
        case 7:
            printf("\n reason for context switch is:\t the time slice of previous task has ended\n\n");
            break;
        default:
            printf("\n value of reason is %d, this is not legal value for reason\n\n", reason);
            break;
    }

}

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
        for(j=0; j<1000; j++)
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
        assert(sched_getscheduler(id) == 0);

        expected_requested_time = 5001;
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
        assert(is_SHORT(thisId) == -1);             //This means it's a SCHED_OTHER process
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
            assert(errno == 22);                            //because it's not a SHORT process
            assert(remaining_time(id) == -1);
            assert(errno == 22);                            //because it's not a SHORT process
            assert(remaining_trials(id) == -1);
            assert(errno == 22);

            struct sched_param param;
            int expected_requested_time = 5000;
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


void testMakeSonShort()
{
        int id = fork();
        int status;
        if (id > 0) {
            struct sched_param inputParam,outputParam;
            int expected_requested_time = 5000;
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
        int expected_requested_time = 5000;
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
                assert(remaining_time(getpid()) == expected_requested_time);
                int son = fork();
                if (son == 0)
                {
                    int grandson_initial_time = remaining_time(getpid());
                    assert(grandson_initial_time <= expected_requested_time/2);
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

void testBecomingOverdueBecauseOfTrials()
{
        int id = fork();
        int status;
        if (id > 0) {
                struct sched_param param;
                int expected_requested_time =5000;
                int expected_trials = 4;
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
        		/*TEST*/assert(remaining_trials(myId) >= 0);
        		int i = remaining_trials(myId);
        		/*TEST*/assert(i >= 0);
        		/*TEST*/printf("\t\tThe amount of remaining trials for the Son are: %d\n",i);
                  for (i; i < 1; )
                  {
                	  i = remaining_trials(myId);
                      doShortTask();
                  }
                printf("is_SHORT returns %d\n",is_SHORT(getpid()));
                assert(is_SHORT(getpid()) == 0);             					//Checks if it's a SCHED_OVERDUE process - Lotem 5.5.15
                _exit(0);
        }
}

void testBecomingOverdueBecauseOfTime()
{
        int id = fork();
        int status;
        if (id > 0) {
                struct sched_param param;
                int expected_requested_time =5;
                int expected_trials = 50;
                param.requested_time = expected_requested_time;
                param.trial_num = expected_trials;
                sched_setscheduler(id, SCHED_SHORT, &param);
                assert(sched_getscheduler(id) == SCHED_SHORT);
                assert(sched_getparam(id, &param) == 0);
                assert(param.trial_num == expected_trials);
                wait(&status);
                printf("OK\n");
        } else if (id == 0) {
                doLongTask();							  						//New code  - Lotem 5.5.15
                assert(is_SHORT(getpid()) == 0);             					//Checks if it's a SCHED_OVERDUE process - Lotem 5.5.15
                _exit(0);
        }
}


void testChangeRequestedTimeForShort()
{
    int id = fork();
    int status;
    if (id > 0) {
    //the father
    struct sched_param paramIn, paramOut ;
    int expected_requested_time =2000;
    int expected_trials = 50;
    paramIn.requested_time = expected_requested_time;
    paramIn.trial_num = expected_trials;

    sched_setscheduler(id, SCHED_SHORT, &paramIn); //make son short
    assert(sched_getscheduler(id) == SCHED_SHORT);

    assert(sched_getparam(id, &paramOut) == 0);
    assert(paramOut.requested_time == expected_requested_time); //should be 2000

    //change requested_time fail
    paramIn.requested_time = 3000;
    assert(sched_setparam(id, &paramIn) == -1);

    assert(sched_getparam(id, &paramOut) == 0);
    assert(paramOut.requested_time == expected_requested_time); //should be 2000

    int new_expected_requested_time =1000;
    //change requested_time fail because of different trial_num
    paramIn.requested_time = new_expected_requested_time;
    paramIn.trial_num = 40;
    assert(sched_setparam(id, &paramIn) ==-1);

    assert(sched_getparam(id, &paramOut) == 0);
    assert(paramOut.requested_time == expected_requested_time); //should be 2000


    //change requested_time success
    paramIn.requested_time = new_expected_requested_time;
    paramIn.trial_num = expected_trials;
    assert(sched_setparam(id, &paramIn) == 0);

    assert(sched_getparam(id, &paramOut) == 0);
    assert(paramOut.requested_time == new_expected_requested_time); //should be 1000

    wait(&status);
    printf("OK\n");
    } else if (id == 0) {
        _exit(0);
    }
}



void testScheduleRealTimeOverShort()
{
    int manager = fork();
    int statusManager;
    if(manager > 0)
    {
        struct sched_param param;
        param.sched_priority = 1;
        sched_setscheduler(manager, 1, &param); // make manager RT
        //the manager
        wait(&statusManager);
        printf("OK\n");
    }

    else if (manager == 0)
    {
        int id = fork();
        int status;
        if (id > 0) {
                struct sched_param param1;
                int expected_requested_time = 5000;
                int expected_trials = 8;
                param1.requested_time = expected_requested_time;
                param1.trial_num = expected_trials;

                int id2 = fork();
                if (id2 == 0)
                {
                        doLongTask();
                        printf("\tRT son finished\n");
                        _exit(0);
                }
                else
                {
                        struct sched_param param2;
                        param2.sched_priority = 1;
                        sched_setscheduler(id, SCHED_SHORT, &param1); // SHORT process
                        sched_setscheduler(id2, 1, &param2);                     //FIFO RealTime process
                }
                wait(&status);
                wait(&status);
                printf("OK\n");
        } else if (id == 0) {
                doLongTask();
                printf("\t\tSHORT son finished\n");
                _exit(0);
        }
         _exit(0);
    }
}


void testScheduleShortOverOther()
{
        int id = fork();
        int status;
        if (id > 0) {
            struct sched_param param1;
            int expected_requested_time = 5000;
            int expected_trials = 8;
            param1.requested_time = expected_requested_time;
            param1.trial_num = expected_trials;

            int id2 = fork();
            if (id2 == 0)
                {
                doLongTask();
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
            doLongTask();
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
            int expected_requested_time = 3000;
            int expected_trials = 8;
            param1.requested_time = expected_requested_time;
            param1.trial_num = expected_trials;

            int id2 = fork();
            if (id2 == 0){
                doLongTask();
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
            doLongTask();
            printf("\tSHORT son finished\n");
            _exit(0);
        }
}

void testScheduleOtherOverOVERDUEBecauseOfTrials()
{
        int id = fork();
        int status;
        if (id > 0) {
                struct sched_param param1;
                int expected_requested_time = 5000;
                int expected_trials = 2;
                param1.requested_time = expected_requested_time;
                param1.trial_num = expected_trials;

                int id2 = fork();
                if (id2 == 0)
                {
                        doLongTask();
                        doLongTask();
                    	/*TEST*/
                        assert(!is_SHORT(getpid()));
                    	/*TEST*/
                        printf("\t\tOVERDUE son finished\n");
                        _exit(0);
                }
                else
                {
                        struct sched_param param2;
                        param2.sched_priority = 1;
                        sched_setscheduler(id, 0, &param2);             // regular SCHED_OTHER
                        sched_setscheduler(id2, SCHED_SHORT, &param1);         // SHORT_OVERDUE process
                }
                wait(&status);
                wait(&status);
                printf("OK\n");
        } else if (id == 0) {
                doLongTask();
                printf("\tSCHED_OTHER son finished\n");
                _exit(0);
        }
}

void testScheduleOtherOverOVERDUEBecauseOfTrials2()
{
        int id = fork();
        int status;
        if (id > 0) {
                struct sched_param param1;
                int expected_requested_time = 5000;
                int expected_trials = 2;
                param1.requested_time = expected_requested_time;
                param1.trial_num = expected_trials;

                int id2 = fork();
                if (id2 == 0)
                {
                                        doLongTask();
                        printf("\t\tOTHER son finished\n");
                        _exit(0);
                }
                else
                {
                        struct sched_param param2;
                        param2.sched_priority = 1;
                        sched_setscheduler(id2, 0, &param2);             // regular SCHED_OTHER
                        sched_setscheduler(id, SCHED_SHORT, &param1);         // SHORT_OVERDUE process
                }
                wait(&status);
                wait(&status);
                printf("OK\n");
        } else if (id == 0) {
                                doLongTask();
                printf("\tOVERDUE son finished\n");
                _exit(0);
        }
}

void testScheduleOtherOverOVERDUEBecauseOfTime()
{
        int id = fork();
        int status;
        if (id > 0) {
                struct sched_param param1;
                int expected_requested_time = 5;
                int expected_trials = 49;
                param1.requested_time = expected_requested_time;
                param1.trial_num = expected_trials;

                int id2 = fork();
                if (id2 == 0)
                {
                                        doLongTask();
                        printf("\t\tOVERDUE son finished\n");
                        _exit(0);
                }
                else
                {
                        struct sched_param param2;
                        param2.sched_priority = 1;
                        sched_setscheduler(id, 0, &param2);             // regular SCHED_OTHER
                        sched_setscheduler(id2, SCHED_SHORT, &param1);         // SHORT_OVERDUE process
                }
                wait(&status);
                wait(&status);
                printf("OK\n");
        } else if (id == 0) {
                                doLongTask();
                printf("\tSCHED_OTHER son finished\n");
                _exit(0);
        }
}

void testScheduleOtherOverOVERDUEBecauseOfTime2()
{
        int id = fork();
        int status;
        if (id > 0) {
                struct sched_param param1;
                int expected_requested_time = 5;
                int expected_trials = 49;
                param1.requested_time = expected_requested_time;
                param1.trial_num = expected_trials;

                int id2 = fork();
                if (id2 == 0)
                {
                                        doLongTask();
                        printf("\t\tOTHER son finished\n");
                        _exit(0);
                }
                else
                {
                        struct sched_param param2;
                        param2.sched_priority = 1;
                        sched_setscheduler(id2, 0, &param2);             // regular SCHED_OTHER
                        sched_setscheduler(id, SCHED_SHORT, &param1);         // SHORT_OVERDUE process
                }
                wait(&status);
                wait(&status);
                printf("OK\n");
        } else if (id == 0) {
                                doLongTask();
                printf("\tOVERDUE son finished\n");
                _exit(0);
        }
}

void testSHORTRoundRobin()
{
    int manager = fork();
    int statusManager;
    if(manager > 0)
    {
        struct sched_param param;
        param.sched_priority = 1;
        sched_setscheduler(manager, 1, &param); // make manager RT
        //the manager
        wait(&statusManager);
        printf("OK\n");
    }

    else if (manager == 0)
    {
        doLongTask();

        int id = fork();
        int status;
        if (id > 0) {
            struct sched_param param1;
            int expected_requested_time = 5000;
            int expected_trials = 49;
            param1.requested_time = expected_requested_time;
            param1.trial_num = expected_trials;

            int id2 = fork();
            if (id2 == 0)
            {
                int i, j;
                for (i=0; i < 4; i++)
                {
                    doLongTask();
                    if(is_SHORT(getpid()) == 1)
                    {
                        printf("\t\tSon2 is in RR mode\n");
                    }
                    else if (is_SHORT(getpid()) == 0)
                    {
                        printf("\t\tSon2 is in FIFO mode\n");
                    }
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
        } else if (id == 0)
        {
            int i, j;
            for (i=0; i < 4; i++)
            {
                doLongTask();
                if(is_SHORT(getpid()) == 1)
                {
                    printf("Son1 is in RR mode\n");
                }
                else if (is_SHORT(getpid()) == 0)
                {
                    printf("Son1 is in FIFO mode\n");
                }
            }
            _exit(0);
        }
        _exit(0);
    }
}

void testMakeShort()
{
        int thisId = getpid();
        struct sched_param inputParam,outputParam;
        int expected_requested_time = 5000;
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
        assert(sched_getparam(thisId, &outputParam) == 0);
        int afterTime = remaining_time(thisId);
        assert(afterTime > 0);
        assert(afterTime < expected_requested_time);
        int usedTrials = outputParam.trial_num;
        printf("OK\n");
}

int main()
{

//    printf("Testing bad parameters... ");
//    testBadParams();
//
//    printf("Testing SCHED_OTHER process... ");
//    testOther();
//
//    printf("Testing new System Calls... ");
//    testSysCalls();
//
//    printf("Testing making son process SHORT... ");
//    testMakeSonShort();
//
//    printf("Testing fork... ");
//    testFork();
//
    printf("Testing becoming overdue because of Trials... ");
    testBecomingOverdueBecauseOfTrials();
//
//    printf("Testing becoming overdue because of Time... ");
//    testBecomingOverdueBecauseOfTime();
//
//    printf("Testing SHORT processes Round-Robin... \n");
//    testSHORTRoundRobin();
//
//    printf("testChangeRequestedTimeForShort... ");							//TODO - This test's results are depended on you implementation
//    testChangeRequestedTimeForShort();										//		 (there wasn't a specific demand...)
//
//    printf("Testing race: RT vs. SHORT (RT is supposed to win)...\n");
//    testScheduleRealTimeOverShort();
//
//    printf("Testing race: SHORT vs. OTHER (SHORT is supposed to win)\n");
//    testScheduleShortOverOther();

    printf("Testing race: OTHER vs. OVERDUE #2(OTHER is supposed to win)\n");
    printf("The OVERDUE process was created as SHORT and consumed all of it's Trials...\n");
    testScheduleOtherOverOVERDUEBecauseOfTrials();

//
//    printf("Testing race: OTHER vs. OVERDUE #1 (OTHER is supposed to win)\n");
//    printf("The OVERDUE process was created as SHORT and consumed all of it's Time...\n");
//    testScheduleOtherOverOVERDUEBecauseOfTime();
//
//
//    printf("Testing race: OTHER vs. OVERDUE #2 (OTHER is supposed to win)\n");
//    printf("The OVERDUE process was created as SHORT and consumed all of it's Time...\n");
//    testScheduleOtherOverOVERDUEBecauseOfTime2();
//
//    printf("Testing making this process SHORT... ");
//    testMakeShort();
//
//    printf("Success!\n");

    return 0;
}
