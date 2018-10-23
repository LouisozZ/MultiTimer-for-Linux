#include "spp_global.h"

#include "sys/time.h"
#include "pthread.h"
#include "signal.h"

int main()
{
    //定时器全局管理结构的初始化
    spp_timer_init();

    int err;

    //======================       此部分必须包含在 main() 中     =================================//
    sigset_t old_sig_mask;
    sigemptyset(&g_sigset_mask);
    sigaddset(&g_sigset_mask,SIGALRM);

    if(err = pthread_sigmask(SIG_SETMASK,&g_sigset_mask,&old_sig_mask) != 0)
    {
        printf("\nset sig mask error!\n");
        return ;
    }
    //===========================================================================================//
    
    err = pthread_create(&nUserThread,NULL,User_Thread,NULL);
    if(err != 0)
    {
        printf("\nCan't create multi timer thread!\n");
        return 0;
    }
    

    err = pthread_create(&nTimerThread,NULL,MultiTimer_thread,NULL);
    if(err != 0)
    {
        printf("\nCan't create multi timer thread!\n");
        return 0;
    }

    pthread_join(nTimerThread,NULL);
    pthread_join(nUserThread,NULL);

    return 0;
}