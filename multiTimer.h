#ifndef __MULTITIMER_H__
#define __MULTITIMER_H__

#define MAX_TIMER_UPPER_LIMIT   6

#define TIMER_0                 0
#define TIMER_1                 1       //timer ID
#define TIMER_2                 2
#define TIMER_3                 3
#define TIMER_4                 4
#define TIMER_5                 5

#define CANCEL_MODE_IMMEDIATELY         0xf9
#define CANCEL_MODE_AFTER_NEXT_TIMEOUT  0x9f

typedef void TimeoutCallBack(void*);

//========================================================
//                      timer结构定义
//========================================================
typedef struct tMultiTimer
{
    uint8_t nTimerID;       //
    uint32_t nInterval;     //定时时长
    uint32_t nTimeStamp;    //时间戳
    bool bIsSingleUse;      //是否单次使用
    bool bIsOverflow;       //用于解决计数溢出问题
    TimeoutCallBack *pTimeoutCallbackfunction;
    void* pTimeoutCallbackParameter;

    //双向链表指针
    struct tMultiTimer* pNextTimer;
    struct tMultiTimer* pPreTimer;
    //相同时间戳的下一个处理函数    这里可能会有隐藏的 bug，如果基础时间中断比较快，那么可能在处理多个同一时间节点的
    //回调函数的时候被下一次的中断打断，这里会引起时序错误，
    //解决方案有三种，
    //一是可以人为避免，不设置有公约数的定时时间，这样的话同一个时刻有多个定时任务的情况就小很多；
    //二是回调函数尽量少做事，快速退出定时处理函数；
    //三是另开一个线程，这个线程仅把回调函数放到一个队列中，另一个线程持续从队列中取回调函数执行，这个是没有问题的方案，但是需要支持多线程或者多任务，并且需要注意加锁
    struct tMultiTimer* pNextHandle;
    
}tMultiTimer;

//========================================================
//               实现多定时任务的相关变量
//========================================================

tMultiTimer* g_pTimeoutCheckListHead;
bool g_bIs_g_nAbsoluteTimeOverFlow;
uint32_t g_nAbsoluteTime;

//========================================================
//                      外部接口
//========================================================
void MultiTimerInit();
uint8_t SetTimer(uint8_t nTimerID,uint32_t nInterval,bool bIsSingleUse,TimeoutCallBack* pCallBackFunction,void* pCallBackParameter);
uint8_t CancelTimerTask(uint8_t nTimerID,uint8_t nCancelMode);
void CancleAllTimerTask();
void SYSTimeoutHandler(int signo);
#endif