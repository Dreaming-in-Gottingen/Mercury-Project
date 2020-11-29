#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include <Mutex.h>
#include <Condition.h>

using namespace Mercury;

Mutex gLock;
Condition gCond;

void* thread_routine(void*)
{
    // Mutex test
    puts("[sub_thread] (step2) enter! want to get lock!");
    gLock.lock();
    puts("[sub_thread] (step4) get lock and sleep 5s! and will cause main thread wait 5s!");
    sleep(5);
    gLock.unlock();
    usleep(10*1000);

    gLock.lock();
    usleep(10*1000);
    gLock.unlock();
    usleep(10*1000);

    puts("------------------[sub_thread]-----------------------");

    // Condition test
    gLock.lock();
    puts("[sub_thread] (step7) sleep 5s then signal to test Condition!");
    sleep(5);
    gCond.signal();
    gLock.unlock();

    sleep(10);

    puts("[sub_thread] (step11) exit!!!");
    
    return NULL;
}

int main(void)
{
    puts("-------------------MutexDemo begin-------------------");
    pthread_t tid;

    // Mutex test
    gLock.lock();
    puts("[main_thread] (step1) hold lock for 5s! then sub thread can get it!");
    pthread_create(&tid, NULL, thread_routine, NULL);
    sleep(5);
    gLock.unlock();
    puts("[main_thread] (step3) release lock!");
    usleep(10*1000);
    {
        Mutex::Autolock l(gLock);
        puts("[main_thread] (step5) get lock by Autolock!");
        sleep(5);
    }

    puts("------------------[main_thread]-----------------------");

    // Condition test
    gLock.lock();
    while (1) {
        puts("[main_thread] (step6) wait forever begin!");
        gCond.wait(gLock);
        puts("[main_thread] (step8) wait forever end!");
        gLock.unlock();

        gLock.lock();
        puts("[main_thread] (step9) waitRelative(5s) begin!");
        gCond.waitRelative(gLock, 5000*1000*1000LL);
        puts("[main_thread] (step10) waitRelative(5s) end!");
        gLock.unlock();
        break;
    }
    pthread_join(tid, NULL);

    puts("-------------------MutexDemo end-------------------");

    return 0;
}
