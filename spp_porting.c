#include "spp_global.h"

#include "unistd.h"
#include "sys/time.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "pthread.h"
#include "memory.h"

pthread_t nTimerThread;
pthread_t nUserThread;

void* CMALLOC(uint32_t length)
{
    return (void*)malloc(length);
}

uint8_t CFREE(void* pFreeAddress)
{
    free(pFreeAddress);
    //pFreeAddress = NULL;
    return 1;
}

void CB_printf(void* para)
{
    printf("timeout comming!\n");
}

//定时器线程，每一毫秒执行一次软中断函数，这个中断函数是自己的多定时器管理函数，所以在设置定时器超时宏定义的时候，值的单位是 ms
void* MultiTimer_thread(void *parameter)
{
    #ifdef DEBUG_PRINTF
    printf("\nthis is timer thread!\n");
    #endif

    int err,signo;

    struct itimerval new_time_value,old_time_value;
    new_time_value.it_interval.tv_sec = 0;
    new_time_value.it_interval.tv_usec = 1000;
    new_time_value.it_value.tv_sec = 0;
    new_time_value.it_value.tv_usec = 1;

    setitimer(ITIMER_REAL, &new_time_value,NULL);

    for(;;)
    {
        err = sigwait(&g_sigset_mask,&signo);
        if(err != 0)
        {
            #ifdef DEBUG_PRINTF
            printf("\nsigwait error! can't open multitimer task!\n");
            #endif

            return ;
        }

        if(signo == SIGALRM)
        {
            SYSTimeoutHandler(signo);
        }
    }

    return ((void*)0);
}

void* User_Thread(void *parameter)
{
    /*    user code   */
    int err,signo;
    sigset_t old_sig_mask;
    if(err = pthread_sigmask(SIG_SETMASK,&g_sigset_mask,&old_sig_mask) != 0)
    {
        printf("\nset sig mask error!\n");
        return ;
    }
    SetTimer(TIMER_0,1000,false,CB_printf,NULL);
    while(1);
}
