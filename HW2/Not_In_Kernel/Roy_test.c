#include "hw2_syscalls.h"
#include <stdio.h>
#include <assert.h>

#define HZ 512
#define MILI_TO_TICKS(x) (x)*HZ/1000
#define TICKS_TO_MILI(x) (x)/HZ*1000

const char* policies[] =
{
        "SCHED_OTHER", //0
        "SCHED_FIFO",//1
        "SCHED_RR",//2
        "Default", // not real, 
        "SCHED_SHORT",//4
        "SCHED_OVERDUE", // "SCHED_SHORT" WITH is_overdue==1
};

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
        printf("%d\t\t\t|%d\t\t\t|%d\t\t\t|", TICKS_TO_MILI(debug->requested_time),remaining_time(pid), debug->trial_num);
        printf("%d\t\t", debug->trial_num_counter);
        printf("\n");


        printf("\t\t\t\t--------------------------------------------------------\n");
        printf("\n");
        free(debug);
        return;
}


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
            assert(is_SHORT(getpid()) == 1);
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
        assert(sched_getparam(id, &param) == 0);
        assert(param.trial_num == expected_trials);
        wait(&status);
        printf("OK\n");
    } else if (id == 0) {
        doLongTask();
        assert(is_SHORT(getpid())==0);
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
        assert(sched_getparam(id, &param) == 0);
        assert(param.trial_num == expected_trials);
        wait(&status);
        printf("OK\n");
    } else if (id == 0) {
        int myId = getpid();
        doLongTask();
        assert(is_SHORT(getpid())==0);
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
        doLongTask();
        _exit(0);
    }
}


void testScheduleRealTimeOverShort()
{
    int manager = fork();
    int status;
    if(manager > 0)
    {
        struct sched_param param;
        param.sched_priority = 1;
        sched_setscheduler(manager, 1, &param); // make manager RT
        //the manager
        wait(&status);
        printf("OK\n");
    }

    else if (manager == 0)
    {
        int id = fork();
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
                        wait(&status);
                }                
                wait(&status);
                printf("OK\n");
        } else if (id == 0) {
                doLongTask();
                assert(is_SHORT(getpid()) == 1);
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
                wait(&status);
            }
            wait(&status);
            printf("OK\n");
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
                printf("OK\n");
        } else if (id == 0) {
                doLongTask();
                doLongTask();
                assert(is_SHORT(getpid())==0);
                printf("\tOVERDUE son finished\n"); //make sure it became overdue
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
                        wait(&status);
                }
                wait(&status);
                printf("OK\n");
        } else if (id == 0) {
                doLongTask();
                assert(is_SHORT(getpid())==0); //make sure it became overdue
                printf("\tOVERDUE son finished\n");
                _exit(0);
        }
}

void testSHORTRoundRobin()
{
    int Manager = fork();
    int status;
    if(Manager > 0)
    {
        //this is the manager process, the value of int Manager is the son created from fork

        //make SHORT1 a short
        struct sched_param param;
        param.requested_time = 5000;
        param.trial_num = 15;
        sched_setscheduler(Manager, SCHED_SHORT, &param); //make the son (SHORT1) a short
        wait(&status);
        printf("OK\n");
    }
    else if (Manager == 0)
    {
        //this is the SHORT1 (son of manager)
        doLongTask();
        int SHORT2 = fork(); //Create SHORT2
        if(SHORT2 > 0)
        {
            //this is the SHORT1 (son of manager)
            struct sched_param param;
            param.requested_time = 5000;
            param.trial_num = 15;
            sched_setscheduler(SHORT2, SCHED_SHORT, &param); //make the son (SHORT2) a short    
            
            int i = remaining_trials(getpid());
            int currentTrial = remaining_trials(getpid());
            
            for (i; i>1;)
            {
                if(is_SHORT(getpid()) == 1)
                {
                    printf("\t\tSHORT1 is in RR mode\n");
                }
                else if (is_SHORT(getpid()) == 0)
                {
                    printf("\t\tSHORT1 is in FIFO mode\n");
                }
                while(currentTrial >= i)
                {
                    currentTrial = remaining_trials(getpid());
                }
                i = remaining_trials(getpid());
            }
            wait(&status);
            printf("OK\n");
        }
        else if (SHORT2 == 0)
        {
            doLongTask();
            //this is the SHORT2 (son of SHORT1)
            int i = remaining_trials(getpid());
            int currentTrial = remaining_trials(getpid());
            
            for (i; i>1;)
            {
                if(is_SHORT(getpid()) == 1)
                {
                    printf("SHORT2 is in RR mode\n");
                }
                else if (is_SHORT(getpid()) == 0)
                {
                    printf("SHORT2 is in FIFO mode\n");
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
            sched_setscheduler(SHORT2, SCHED_SHORT, &param); //make the son (SHORT2) a short    
            
            int i;
            for (i=0; i<5; i++)
            {
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
            doLongTask();
            assert(is_SHORT(getpid())==0); //make sure its overdue
            //this is the SHORT2 (son of SHORT1)

            int i;
            for (i=0; i<5; i++)
            {
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
    printf("Testing bad parameters... ");
    testBadParams();

    printf("Testing SCHED_OTHER process... ");
    testOther();

    printf("Testing new System Calls... ");
    testSysCalls();

    printf("Testing making son process SHORT... ");
    testMakeSonShort();

    printf("Testing fork... ");
    testFork();

    printf("Testing becoming overdue because of Trials... ");
    testBecomingOverdueBecauseOfTrials();

    printf("Testing becoming overdue because of Time... ");
    testBecomingOverdueBecauseOfTime();

    printf("testChangeRequestedTimeForShort... ");
    testChangeRequestedTimeForShort();

    printf("Testing race: RT vs. SHORT (RT is supposed to win)...\n");          
    testScheduleRealTimeOverShort();                                        

    printf("Testing race: SHORT vs. OTHER (SHORT is supposed to win)\n");
    testScheduleShortOverOther();
 
    printf("Testing race: OTHER vs. OVERDUE #1(OTHER is supposed to win)\n");
    printf("The OVERDUE process was created as SHORT and consumed all of it's Trials...\n\n");
    testScheduleOtherOverOVERDUEBecauseOfTrials();

    printf("Testing race: OTHER vs. OVERDUE #2(OTHER is supposed to win)\n");
    printf("The OVERDUE process was created as SHORT and consumed all of it's Trials...\n\n");
    testScheduleOtherOverOVERDUEBecauseOfTrials2();

    printf("Testing race: OTHER vs. OVERDUE #1 (OTHER is supposed to win)\n");  
    printf("The OVERDUE process was created as SHORT and consumed all of it's Time...\n\n");  
    testScheduleOtherOverOVERDUEBecauseOfTime();
     
    printf("Testing race: OTHER vs. OVERDUE #2 (OTHER is supposed to win)\n");
    printf("The OVERDUE process was created as SHORT and consumed all of it's Time...\n\n");  
    testScheduleOtherOverOVERDUEBecauseOfTime2();

    printf("Testing SHORT_OVERDUE processes FIFO run... \n");
    testShortOverdueFIFO();

//    printf("Testing SHORT processes Round-Robin run... \n");
//    testSHORTRoundRobin();

    printf("Testing making this process SHORT... ");
    testMakeShort();
    
    printf("Success!\n");
        
    return 0;
}

