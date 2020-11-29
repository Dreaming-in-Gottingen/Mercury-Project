#include <stdio.h>

#include <Timers.h>

int main(void)
{
    printf("-----------TimersDemo begin------------\n");

    nsecs_t cur_tm = systemTime();
    printf("systemTime(): [%ld ns]=[%ld s]\n", cur_tm, ns2s(cur_tm));

    nsecs_t timeout_tm = cur_tm + s2ns(10);
    int wait_tm_ms = toMillisecondTimeoutDelay(cur_tm, timeout_tm);
    printf("toMillisecondTimeoutDelay(): [%d ms]\n", wait_tm_ms);

    printf("-----------TimersDemo end------------\n");

    return 0;
}
