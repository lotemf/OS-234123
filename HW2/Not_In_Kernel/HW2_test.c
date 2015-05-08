/*
                            IMPORTANT!!
                            IMPORTANT!!
                            IMPORTANT!!
    you may assume both sched_param and SCHED_SHORT will be defined in the tests code of the course (piazza)!!
                            IMPORTANT!!
                            IMPORTANT!!
                            IMPORTANT!!
*/

#include "hw2_syscalls.h"
#include <stdio.h>
#include <assert.h>


#define HZ 512
#define SCHED_SHORT    4               /* HW2 - Roy for sched_tester.c*/
#define MILI_TO_TICKS(x) (x)*HZ/1000
#define TICKS_TO_MILI(x) (x)/HZ*1000


struct sched_param {
    int sched_priority;
    int requested_time;                 /* HW2 Roy: Range: 1-5000 in miliseconds */
    int trial_num;               /* HW2 Roy: Range: 1-50 original number of trials */
};

/*******************************************************************************
*		THIS IS A DEBUG FUNCTION						**-TO_DELETE			//TODO - delete later
*******************************************************************************/
/*******************************************************************************
 ticks_to_ms(int ticks) - Converts the number of ticks that occured since the
 						  system booted to mili-seconds
 *******************************************************************************/
int ticks_to_ms(int ticks)
{
	return ((ticks * 1000) / HZ);
}

const char* policies[] =
{
        "SCHED_OTHER", //0
        "SCHED_FIFO",//1
        "SCHED_RR",//2
        "Default", // not real,
        "SCHED_SHORT",//4
        "SCHED_OVERDUE", // "SCHED_SHORT" WITH is_overdue==1
};
//TO DELETE
void print_debug(int pid)
{
        int res_debug;
        struct debug_struct* debug = malloc(sizeof( struct debug_struct));
        res_debug = hw2_debug(pid, debug);
        if(res_debug != 0){
                printf("print_debug failed\n");
                return;
        }

        char* policy_string;
        if (debug->policy != 4)
        {
                policy_string = policies[debug->policy];
        }
        else
        {
                if(debug->is_overdue == 0)
                {
                        policy_string = policies[4];
                }
                else
                {
                        policy_string = policies[5];
                }
        }

        printf("\n\t\t\t\t------------------DEBUG FOR PID=%d------------------\n",pid);


        printf("|Priority\t|Policy\t\t|requested_time(mili)\t|time_slice(mili)\t|number_of_trials\t|trial_num\t\n");
        printf("|%d\t\t|%s\t|", debug->priority, policy_string);
        printf("%d\t\t\t|%d\t\t\t|%d\t\t\t|", ticks_to_ms(debug->requested_time),remaining_time(pid), debug->trial_num);
        printf("%d\t\t", debug->trial_num_counter);
        printf("\n");


        printf("\t\t\t\t--------------------------------------------------------\n");
        printf("\n");
        free(debug);
        return;
}
///*******************************************************************************
//*		THIS IS A DEBUG FUNCTION						**-TO_DELETE			//TODO - delete later
//*******************************************************************************/

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
    } else if (id == 0) {
        doShortTask();
        _exit(0);
    }


    // check that real time cannot become short
    int real = fork();
    if (real>0)
    {

        struct sched_param realParam;
        realParam.sched_priority = 90;
        realParam.requested_time = 0;
        realParam.trial_num = 0;
        assert(sched_setscheduler(real, 2, &realParam) == 0); //make my son real

        wait(&status);
        printf("OK\n");
    } else if (real == 0) {
        doLongTask();
        assert(sched_getscheduler(getpid()) == 2); // verify that im real

        struct sched_param shortParam;
        shortParam.requested_time = 700;
        shortParam.trial_num = 30;
        assert(sched_setscheduler(getpid(), SCHED_SHORT, &shortParam) == -1);
        _exit(0);
    }
}

void testOther()
{
    int thisId = getpid();
    assert(sched_getscheduler(thisId) == 0);
    assert(is_SHORT(thisId) == -1);             //This means it a SCHED_OTHER process
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
    int expected_requested_time = 5000;
    int expected_trials = 8;
    int id = fork();
    int status;
    if (id > 0) {
        struct sched_param inputParam;
        inputParam.requested_time = expected_requested_time;
        inputParam.trial_num = expected_trials;

        sched_setscheduler(id, SCHED_SHORT, &inputParam); //make my son short
        wait(&status);
        printf("OK\n");
    } else if (id == 0) {
        struct sched_param outputParam;
        doShortTask();
        assert(sched_getscheduler(id) == SCHED_SHORT);
        assert(is_SHORT(getpid()) == 1);
        assert(sched_getparam(id, &outputParam) == 0);
        assert(outputParam.requested_time == expected_requested_time);

        //should not take more then 2 trials, its a short task
        assert(outputParam.trial_num <= expected_trials &&  outputParam.trial_num >= expected_trials-2);
        _exit(0);
    }
}

void testFork()
{
    int expected_requested_time = 5000;
    int id = fork();
    int status;
    if (id > 0) {
        //the father
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
        //if this assert fails, that mean the father process runs first, please run the test again.
        assert(remaining_time(getpid()) == expected_requested_time);
        int son = fork();
        if (son == 0)
        {
            //son
            int grandson_initial_time = remaining_time(getpid());
            assert(grandson_initial_time <= expected_requested_time/2);
            assert(grandson_initial_time > 0);
            doMediumTask();
            assert(remaining_time(getpid()) < grandson_initial_time);
            assert(is_SHORT(getpid()) == 1);
            _exit(0);
        }
        else
        {
            //father
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
        int expected_requested_time =1000;
        int expected_trials = 2;
        param.requested_time = expected_requested_time;
        param.trial_num = expected_trials;
        sched_setscheduler(id, SCHED_SHORT, &param);
        assert(sched_getscheduler(id) == SCHED_SHORT);
        wait(&status);
        printf("OK\n");
    } else if (id == 0) {
        doLongTask();
        assert(is_SHORT(getpid())==0);
        struct sched_param resultParam;
        assert(sched_getparam(getpid(), &resultParam) == 0);
        assert(resultParam.trial_num == 0);
        _exit(0);
    }
}

void testBecomingOverdueBecauseOfTime()
{
    int id = fork();
    int status;
    if (id > 0) {
        struct sched_param param;
        int expected_requested_time = 5;
        int expected_trials = 50;
        param.requested_time = expected_requested_time;
        param.trial_num = expected_trials;
        sched_setscheduler(id, SCHED_SHORT, &param);
        assert(sched_getscheduler(id) == SCHED_SHORT);
        wait(&status);
        printf("OK\n");
    } else if (id == 0) {
        int myId = getpid();
        doLongTask();
        assert(is_SHORT(getpid())==0);
        struct sched_param resultParam;
        assert(sched_getparam(getpid(), &resultParam) == 0);
        assert(resultParam.trial_num == 0);
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
        paramIn.requested_time = 7000;
        assert(sched_setparam(id, &paramIn) == -1);

        assert(sched_getparam(id, &paramOut) == 0);
        assert(paramOut.requested_time == expected_requested_time); //should be 2000


        //change requested_time fail
        paramIn.requested_time = 0;
        assert(sched_setparam(id, &paramIn) == -1);

        assert(sched_getparam(id, &paramOut) == 0);
        assert(paramOut.requested_time == expected_requested_time); //should be 2000


        int new_expected_requested_time =1000;
        //change requested_time success
        paramIn.requested_time = new_expected_requested_time;
        paramIn.trial_num = expected_trials;

        assert(sched_setparam(id, &paramIn) == 0);

        assert(sched_getparam(id, &paramOut) == 0);
        assert(paramOut.requested_time == new_expected_requested_time); //should be 1000

        wait(&status);
        printf("OK\n");
    } else if (id == 0) {
        doLongTask();
        _exit(0);
    }
}

void testScheduleRealTimeOverShort()
{
    int manager = fork();
    int status;
    int shortPID;
    int realPID;
    if(manager > 0) {
        wait(&status);
        printf("Race finished, make sure real is the winner\n");
    }
    else if (manager == 0) {
        //manager will have 2 children
        int realProcess = fork(); //make child 1, will be real
        if(realProcess > 0)
        {
            //father code
            realPID = realProcess; //store the pid of the realProcess son outside so that short son can access it
            int shortProcess = fork(); //make child 2, will be short
            if(shortProcess > 0)
            {
                wait(&status);
            }
            else if (shortProcess == 0)
            {
                struct sched_param shortParam;
                shortParam.requested_time = 2000;
                shortParam.trial_num = 50;
                sched_setscheduler(getpid(), SCHED_SHORT, &shortParam); // make shortProcess son a SHORT process
                assert(is_SHORT(getpid()) == 1);

                struct sched_param realParam;
                realParam.sched_priority = 1;
                sched_setscheduler(realPID, 1, &realParam); // make realProcess (my brother) a real process
                assert(sched_getscheduler(realPID) == 1); //make sure its a real process
                doLongTask();
                assert(is_SHORT(getpid()) == 1);
                printf("\t\tSHORT son finished\n");
                _exit(0);
            }
            wait(&status);
        }
        else if (realProcess == 0)
        {
            //real code , do very long task but still should finish first
            doLongTask();
            doLongTask();
            assert(sched_getscheduler(getpid()) == 1); //make sure its a real process
            doLongTask();
            doLongTask();
            doLongTask();
            doLongTask();
            printf("\tRT son finished\n");
            _exit(0);
        }
    _exit(0);
    }
}

void testScheduleShortOverOther()
{
    int winner;
    int id = fork();
    int status;
    if (id > 0) {
        struct sched_param param1;
        int expected_requested_time = 5000;
        int expected_trials = 50;
        param1.requested_time = expected_requested_time;
        param1.trial_num = expected_trials;

        int id2 = fork();
        if (id2 == 0)
            {
            doLongTask();
            assert(is_SHORT(getpid()) == 1);
            printf("\tSHORT son finished\n");
            _exit(0);
        }
        else
        {
            struct sched_param param2;
            param2.sched_priority = 1;
            sched_setscheduler(id, 0, &param2);             // regular SCHED_OTHER
            sched_setscheduler(id2, SCHED_SHORT, &param1);         // SHORT process
            wait(&status);
        }
        wait(&status);
        printf("Race finished, make sure SHORT is the winner\n");
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
        int expected_trials = 50;
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
            sched_setscheduler(id, SCHED_SHORT, &param1);          // SHORT process
            sched_setscheduler(id2, 0, &param2);            // regular SCHED_OTHER
            wait(&status);
        }
        wait(&status);
        printf("Race finished, make sure SHORT is the winner\n");
    } else if (id == 0) {
        doLongTask();
        assert(is_SHORT(getpid()) == 1);
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
            assert(is_SHORT(getpid())==0);
            printf("\t\tOVERDUE son finished\n"); //make sure it became overdue
            _exit(0);
        }
        else
        {
            struct sched_param param2;
            param2.sched_priority = 1;
            sched_setscheduler(id, 0, &param2);             // regular SCHED_OTHER
            sched_setscheduler(id2, SCHED_SHORT, &param1);         // SHORT_OVERDUE process
            wait(&status);
        }
        wait(&status);
        printf("Race finished, make sure OTHER is the winner\n");
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
        int expected_requested_time = 3000;
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
                wait(&status);
        }
        wait(&status);
        printf("Race finished, make sure OTHER is the winner\n");
    } else if (id == 0) {
        doLongTask();
        doLongTask();
        assert(is_SHORT(getpid())==0); //make sure it became overdue
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
            assert(is_SHORT(getpid())==0); //make sure it became overdue
            printf("\t\tOVERDUE son finished\n");
            _exit(0);
        }
        else
        {
            struct sched_param param2;
            param2.sched_priority = 1;
            sched_setscheduler(id, 0, &param2);             // regular SCHED_OTHER
            sched_setscheduler(id2, SCHED_SHORT, &param1);         // SHORT_OVERDUE process
            wait(&status);
        }
        wait(&status);
        printf("Race finished, make sure OTHER is the winner\n");
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
            wait(&status);
        }
        wait(&status);
        printf("Race finished, make sure OTHER is the winner\n");
    } else if (id == 0) {
        doLongTask();
        assert(is_SHORT(getpid())==0); //make sure it became overdue
        printf("\tOVERDUE son finished\n");
        _exit(0);
    }
}

void testSHORTRoundRobin()
{
    int SHORT1 = fork();
    if(SHORT1 > 0)
    {
        //this is the father process, the value of int SHORT1 is the son created from fork
        //make SHORT1 a short
        struct sched_param param;
        param.requested_time = 5000;
        param.trial_num = 50;
        sched_setscheduler(SHORT1, SCHED_SHORT, &param); //make the son (SHORT1) a short
        wait(NULL);
    }
    else if (SHORT1 == 0)
    {
        //this is the SHORT1
        //doLongTask();
        assert(is_SHORT(getpid())==1); //make sure im short
        int SHORT2 = fork(); //Create SHORT2
        if(SHORT2 > 0)
        {
            //this is SHORT1 code
            int SHORT3 = fork(); //Create SHORT3
            if(SHORT3 > 0)
            {
                int i = remaining_trials(getpid());
                int currentTrial = remaining_trials(getpid());
                for (i; i>1;)
                {
                    if(is_SHORT(getpid()) == 1)
                    {
                        printf("A (RR)\n");
                    }
                    else if (is_SHORT(getpid()) == 0)
                    {
                        printf("A (FIFO)\n");
                    }
                    while(currentTrial >= i)
                    {
                        currentTrial = remaining_trials(getpid());
                    }
                    i = remaining_trials(getpid());
                }
                //this is the SHORT1
                wait(NULL);
            }
            else if (SHORT3 == 0)
            {
                assert(is_SHORT(getpid())==1); //make sure im short
                //this is the SHORT3 (son of SHORT1)
                int i = remaining_trials(getpid());
                int currentTrial = remaining_trials(getpid());
                for (i; i>1;)
                {
                    if(is_SHORT(getpid()) == 1)
                    {
                        printf("C (RR)\n");
                    }
                    else if (is_SHORT(getpid()) == 0)
                    {
                        printf("C (FIFO)\n");
                    }
                    while(currentTrial >= i)
                    {
                        currentTrial = remaining_trials(getpid());
                    }
                    i = remaining_trials(getpid());
                }
                _exit(0);
            }
            wait(NULL);
        }
        else if (SHORT2 == 0)
        {
            assert(is_SHORT(getpid())==1); //make sure im short
            //this is the SHORT2 (son of SHORT1)
            int i = remaining_trials(getpid());
            int currentTrial = remaining_trials(getpid());
            for (i; i>1;)
            {
                if(is_SHORT(getpid()) == 1)
                {
                    printf("B (RR)\n");
                }
                else if (is_SHORT(getpid()) == 0)
                {
                    printf("B (FIFO)\n");
                }
                while(currentTrial >= i)
                {
                    currentTrial = remaining_trials(getpid());
                }
                i = remaining_trials(getpid());
            }
            _exit(0);
        }
        _exit(0);
    }
}

void testShortOverdueFIFO()
{
    int Manager = fork();
    int status;
    if(Manager > 0)
    {
        //this is the manager process, the value of int Manager is the son created from fork
        //make SHORT1 a short
        struct sched_param param;
        param.requested_time = 5;
        param.trial_num = 15;
        sched_setscheduler(Manager, SCHED_SHORT, &param); //make the son (SHORT1) a short
        wait(&status);
    }
    else if (Manager == 0)
    {
        //this is the SHORT1 (son of manager)
        doLongTask();
        assert(is_SHORT(getpid())==0); //make sure its overdue
        int SHORT2 = fork(); //Create SHORT2
        if(SHORT2 > 0)
        {
            //this is the SHORT1 (son of manager)
            struct sched_param param;
            param.requested_time = 5;
            param.trial_num = 15;

            int i;
            for (i=0; i<5; i++)
            {
                if(is_SHORT(getpid()) == 1)
                {
                    printf("A (RR)\n");
                }
                else if (is_SHORT(getpid()) == 0)
                {
                    printf("A (FIFO)\n");
                }
                doLongTask();
            }
            wait(&status);
            printf("OK\n");
        }
        else if (SHORT2 == 0)
        {
            assert(is_SHORT(getpid())==0); //make sure its overdue
            //this is the SHORT2 (son of SHORT1)

            int i;
            for (i=0; i<5; i++)
            {
                if(is_SHORT(getpid()) == 1)
                {
                    printf("B (RR)\n");
                }
                else if (is_SHORT(getpid()) == 0)
                {
                    printf("B (FIFO)\n");
                }
                doLongTask();
            }
            _exit(0);
        }
        _exit(0);
    }
}

void testShortOverdueFIFOWithPrints()
{
    int Manager = fork();
    int status;
    /*TEST*/printf("First entry in OVERDUE-FIFO TEST \n",Manager);
    if(Manager > 0)
    {
        //this is the manager process, the value of int Manager is the son created from fork
        //make SHORT1 a short
        struct sched_param param;
        param.requested_time = 5;
        param.trial_num = 15;
        /*TEST*/printf("Father's print debug\n");
        /*TEST*/print_debug(getpid());

        sched_setscheduler(Manager, SCHED_SHORT, &param); //make the son (SHORT1) a short
        /*TEST*/printf("Father's print debug\n");
        /*TEST*/print_debug(getpid());
        /*TEST*/printf("Manager contains: %d \n",Manager);
        wait(&status);
    }
    else if (Manager == 0)
    {
        //this is the SHORT1 (son of manager)
        /*TEST*/printf("Inside first SHORT SON \n");					//Fast-run
        /*TEST*/print_debug(getpid());
        doLongTask();
        assert(is_SHORT(getpid())==0); //make sure its overdue
        int SHORT2 = fork(); //Create SHORT2
        /*TEST*/printf("SHORT2 contains: %d \n",SHORT2);
        if(SHORT2 > 0)
        {
            /*TEST*/printf("Inside second SHORT SON \n");
            /*TEST*/print_debug(getpid());
            //this is the SHORT1 (son of manager)
            struct sched_param param;
            param.requested_time = 5;
            param.trial_num = 15;
            /*TEST*/printf("is_SHORT for SHORT1 returns : %d\n",is_SHORT(getpid()));
            /*TEST*/assert(sched_setscheduler(SHORT2, SCHED_SHORT, &param) == 0);
            int i;
            for (i=0; i<5; i++)
            {
                /*TEST*/print_debug(getpid());
                /*TEST*/printf("Inside first loop");
                if(is_SHORT(getpid()) == 1)
                {
                    printf("\t\tOVERDUE1 is in RR mode\n");
                }
                else if (is_SHORT(getpid()) == 0)
                {
                    printf("\t\tOVERDUE1 is in FIFO mode\n");
                }
                doLongTask();
            }
            wait(&status);
            printf("OK\n");
        }
        else if (SHORT2 == 0)
        {
            /*TEST*/printf("is_SHORT for SHORT2 returns : %d\n",is_SHORT(getpid()));
            /*TEST*/print_debug(getpid());
            assert(is_SHORT(getpid())==0); //make sure its overdue
            //this is the SHORT2 (son of SHORT1)

            int i;
            for (i=0; i<5; i++)
            {
                /*TEST*/print_debug(getpid());
                /*TEST*/printf("Inside second loop");
                if(is_SHORT(getpid()) == 1)
                {
                    printf("OVERDUE2 is in RR mode\n");
                }
                else if (is_SHORT(getpid()) == 0)
                {
                    printf("OVERDUE2 is in FIFO mode\n");
                }
                doLongTask();
            }
            /*TEST*/printf("The test is about to perform the first exit...\n");
            _exit(0);
            /*TEST*/print_debug(getpid());

        }
        /*TEST*/print_debug(getpid());
        /*TEST*/printf("****    ERROR! : The function never entered The second son for some reason!   *****\n");
        /*TEST*/printf("The test is about to perform the second exit...\n");
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
    assert(is_SHORT(getpid()) == 1);
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
//    printf("Testing bad parameters... \n\n");
//    testBadParams();
//
//    printf("Testing SCHED_OTHER process... \n");
//    testOther();
//
//    printf("Testing new System Calls... \n");
//    testSysCalls();
//
//    printf("Testing making son process SHORT... \n");
//    testMakeSonShort();
//
//    printf("Testing fork... \n");
//    testFork();
//
//    printf("Testing becoming overdue because of Trials... \n");
//    testBecomingOverdueBecauseOfTrials();
//
//    printf("Testing becoming overdue because of Time... \n");
//    testBecomingOverdueBecauseOfTime();

    printf("testChangeRequestedTimeForShort... \n");
    testChangeRequestedTimeForShort();

    printf("\nTesting race: RT vs. SHORT (RT is supposed to win)...\n");
    testScheduleRealTimeOverShort();

    printf("\nTesting race: SHORT vs. OTHER #1(SHORT is supposed to win)\n");
    testScheduleShortOverOther();

    printf("\nTesting race: SHORT vs. OTHER #2(SHORT is supposed to win)\n");
    testScheduleShortOverOther2();

    printf("\nTesting race: OTHER vs. OVERDUE #1(OTHER is supposed to win)\n");
    printf("The OVERDUE process was created as SHORT and consumed all of it's Trials...\n\n");
    testScheduleOtherOverOVERDUEBecauseOfTrials();

    printf("\nTesting race: OTHER vs. OVERDUE #2(OTHER is supposed to win)\n");
    printf("The OVERDUE process was created as SHORT and consumed all of it's Trials...\n\n");
    testScheduleOtherOverOVERDUEBecauseOfTrials2();

    printf("\nTesting race: OTHER vs. OVERDUE #1 (OTHER is supposed to win)\n");
    printf("The OVERDUE process was created as SHORT and consumed all of it's Time...\n\n");
    testScheduleOtherOverOVERDUEBecauseOfTime();

    printf("\nTesting race: OTHER vs. OVERDUE #2 (OTHER is supposed to win)\n");
    printf("The OVERDUE process was created as SHORT and consumed all of it's Time...\n\n");
    testScheduleOtherOverOVERDUEBecauseOfTime2();

    printf("\nTesting OVERDUE processes FIFO... \n");
    printf("You should see the FIFO behaviour in the prints, e.g A A A A B B B B... \n");
    testShortOverdueFIFO();

//    printf("Testing OVERDUE processes FIFO with prints... \n");
//    testShortOverdueFIFOWithPrints();

//    printf("\nTesting SHORT processes Round-Robin... \n");
//    printf("You should see the Round-Robin behaviour in the prints, e.g A B C A B C A... \n");
//    testSHORTRoundRobin();

    printf("Testing making this process SHORT... \n");
    testMakeShort();

    printf("Success!\n");

    return 0;
}

