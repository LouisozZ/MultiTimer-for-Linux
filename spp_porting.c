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
//这个线程的代码可以不用修改，如果要修改定时器的基准时间，可以修改下面这句
//new_time_value.it_interval.tv_usec = 1000;
//上面这句表示每 1000us 即 1ms 产生一个系统时钟信号，可以修改这个值来修改基准时间
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
    //======================       此部分必须包含在每个非定时器线程中     ===========================//
    int err,signo;
    sigset_t old_sig_mask;
    if(err = pthread_sigmask(SIG_SETMASK,&g_sigset_mask,&old_sig_mask) != 0)
    {
        printf("\nset sig mask error!\n");
        return ;
    }
    //===========================================================================================//

    //***** e.g. 1****//
    uint16_t handler = 0;

    handler = spp_timer_create(1000,CB_printf,NULL);
    spp_timer_start(handler);

    //给定时器0添加定时任务，定时间隔为1000ms，是循环执行（即是定时器，不是计时器，回调函数是CB_printf，回调函数的参数是NULL
    //SetTimer(TIMER_0,1000,false,CB_printf,NULL);
    //设置一个定时器，参数说明：
    //参数一：设置的是定时器0;（可选项在 spp_def.h 中，line 44-49，是一个宏定义，需要结合 MAX_TIMER_UPPER_LIMIT 以及 g_aSPPMultiTimer 数组）
    //参数二：定时器间隔，单位ms；
    //参数三：是计时器（true）还是定时器（false）；
    //参数四：超时回调函数，格式 void cbfunciton(void*);
    //参数五：回调函数的参数，类型void*

    //***** e.g. 2****//
    //给定时器1添加定时任务，定时间隔为1000ms，但是是计时器，即1000ms之后执行了timer1的回调函数，然后就立即从全局定时器管理链表上摘取掉timer1
    //SetTimer(TIMER_1,1000,true,CB_printf,NULL);

    //***** e.g. 3****//
    //取消定时器0的定时任务，是立即取消，即程序执行之后，全局的定时器管理链表中就没有了定时器0的节点
    //CancelTimerTask(TIMER_0,CANCEL_MODE_IMMEDIATELY);
    //参数一：需要取消的定时器编号
    //参数二：取消模式，CANCEL_MODE_IMMEDIATELY（立即取消），CANCEL_MODE_AFTER_NEXT_TIMEOUT（下一次定时到达之后，执行回调之后再从全局定时器管理链表中摘取定时器0的节点）
    while(1);
}
