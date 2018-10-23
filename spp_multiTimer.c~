#include "spp_global.h"

sigset_t g_sigset_mask;

//========================================================
//               实现多定时任务的相关变量
//========================================================
//定时器实例的个数
uint16_t MAX_TIMER_UPPER_LIMIT = 50;

//定时器ID记录
uint6_t TIMER_ID_RECORD = 0;

//全局定时器链表
tSppMultiTimer* g_pTimeoutCheckListHead;
//全局绝对时间
uint32_t g_nAbsoluteTime;
//绝对时间超时与否
bool g_bIs_g_nAbsoluteTimeOverFlow;

//定时器实例数组
tSppMultiTimer** g_aSPPMultiTimer;
//========================================================

/**
 * @function    把一个定时任务添加到定时检测链表中
 * @parameter   一个定时器对象，可以由全局变量 g_aSPPMultiTimer 通过 TIMER_ID 映射得到
*/
static void AddTimerToCheckList(tSppMultiTimer* pTimer)
{
    tSppMultiTimer* pEarliestTimer = NULL;
    tSppMultiTimer* pEarliestTimer_pre = NULL;
    
    CDebugAssert(pTimer->nInterval != 0);

    pTimer->nTimeStamp = g_nAbsoluteTime + pTimer->nInterval;
    if(pTimer->nTimeStamp < g_nAbsoluteTime)
        pTimer->bIsOverflow = !(pTimer->bIsOverflow);
    if(g_pTimeoutCheckListHead == NULL)
    {
        g_pTimeoutCheckListHead = pTimer;
        g_pTimeoutCheckListHead->pNextTimer = NULL;
        g_pTimeoutCheckListHead->pPreTimer = NULL;
        g_pTimeoutCheckListHead->pNextHandle = NULL;
        return;
    }
    else
    {
        pEarliestTimer = g_pTimeoutCheckListHead;
        while(pEarliestTimer != NULL)
        {
            //如果超时时间小于新加的timer则直接跳过；
            if((pEarliestTimer->bIsOverflow != pTimer->bIsOverflow) || (pEarliestTimer->nTimeStamp < pTimer->nTimeStamp))
            {
                pEarliestTimer_pre = pEarliestTimer;
                pEarliestTimer = pEarliestTimer->pNextTimer;
            }    
            else
            {
                if(pEarliestTimer->nTimeStamp == pTimer->nTimeStamp)    //超时时刻相等，直接添加到相同时刻处理列表的列表头
                {
                    pTimer->pNextHandle = pEarliestTimer->pNextHandle;
                    pEarliestTimer->pNextHandle = pTimer;
                    return;
                }
                else                                                    //找到了超时时刻大于新加入timer的第一个节点
                {
                    if(pEarliestTimer->pPreTimer == NULL)               //新加入的是最早到达超时时刻的，添加到链表头
                    {
                        pEarliestTimer->pPreTimer = pTimer;
                        pTimer->pNextTimer = pEarliestTimer;
                        pTimer->pPreTimer = NULL;
                        pTimer->pNextHandle = NULL;
                        g_pTimeoutCheckListHead = pTimer;
                        return;
                    }
                    else                                                //中间节点
                    {
                        pEarliestTimer->pPreTimer->pNextTimer = pTimer;
                        pTimer->pNextTimer = pEarliestTimer;
                        pTimer->pPreTimer = pEarliestTimer->pPreTimer;
                        pEarliestTimer->pPreTimer = pTimer;
                        pTimer->pNextHandle = NULL;
                        return;
                    }
                }
            }  
        }
        if(pEarliestTimer == NULL)                                      //新加入的timer超时时间是最晚的那个
        {
            pEarliestTimer_pre->pNextTimer = pTimer;
            pTimer->pPreTimer = pEarliestTimer_pre;
            pTimer->pNextTimer = NULL;
            pTimer->pNextHandle = NULL;
        }
        return;
    }
}

/**
 * @function    设置一个定时任务，指定超时间隔与回调函数，当超时到来，自动执行回调
 * @parameter1  TIMER_ID    参看spp_def.h 定时器字段
 * @parameter2  超时间隔时间
 * @parameter3  是否是一次性定时任务
 * @parameter4  回调函数，注意，回调函数的函数形式  void function(void*);
 * @parameter5  void* 回调函数的参数，建议用结构体强转成 void*，在回调函数中再强转回来  
 * @return      错误码 
*/
uint8_t SetTimer(uint16_t nTimerID,uint32_t nInterval,bool bIsSingleUse,TimeoutCallBack* pCallBackFunction,void* pCallBackParameter)
{
    printf("\nset timer %d\n",nTimerID);
    tSppMultiTimer* pChoosedTimer = NULL;
    pChoosedTimer = g_aSPPMultiTimer[nTimerID];
    pChoosedTimer->nInterval = nInterval;
    pChoosedTimer->bIsSingleUse = bIsSingleUse;
    pChoosedTimer->pTimeoutCallbackfunction = pCallBackFunction;
    pChoosedTimer->pTimeoutCallbackParameter = pCallBackParameter;

    //如果超时任务链表中已经有这个任务了，先取消，然后再设置，即重置超时任务
    if(pChoosedTimer->pNextTimer != NULL || pChoosedTimer->pPreTimer != NULL || pChoosedTimer->pNextHandle != NULL)
        CancelTimerTask(nTimerID,CANCEL_MODE_IMMEDIATELY);
        
    AddTimerToCheckList(pChoosedTimer);
    return 0;
}

/**
 * @function    取消超时检测链表中的指定超时任务
 * @parameter1  要取消的超时任务的ID
 * @parameter2  模式选择，是立即取消，还是下次执行后取消
 * @return      错误码
*/
uint8_t CancelTimerTask(uint16_t nTimerID,uint8_t nCancelMode)
{
    printf("\ncancle timer %d\n",nTimerID);
    tSppMultiTimer* pEarliestTimer = NULL;
    tSppMultiTimer* pHandleTimer = NULL;
    tSppMultiTimer* pHandleTimer_pre = NULL;
    tSppMultiTimer* pChoosedTimer = NULL;

    pEarliestTimer = g_pTimeoutCheckListHead;
    pChoosedTimer = g_aSPPMultiTimer[nTimerID];

    if(nCancelMode == CANCEL_MODE_IMMEDIATELY)
    {
        while(pEarliestTimer != NULL)
        {
            pHandleTimer = pEarliestTimer;
            pHandleTimer_pre = NULL;
            while(pHandleTimer != NULL)
            {
                if(pHandleTimer->nTimerID == nTimerID)
                {
                    if(pHandleTimer_pre == NULL)
                    {
                        if(pHandleTimer->pNextHandle != NULL)
                        {
                            pEarliestTimer = pHandleTimer->pNextHandle;
                            pEarliestTimer->pPreTimer = pHandleTimer->pPreTimer;
                            if(pHandleTimer->pPreTimer != NULL)
                                pHandleTimer->pPreTimer->pNextTimer = pEarliestTimer;
                            pEarliestTimer->pNextTimer = pHandleTimer->pNextTimer;
                            if(pHandleTimer->pNextTimer != NULL)
                                pHandleTimer->pNextTimer->pPreTimer = pEarliestTimer;
                            pHandleTimer->pNextTimer = NULL;
                            pHandleTimer->pPreTimer = NULL;
                            pHandleTimer->pNextHandle = NULL;
                        }
                        else
                        {
                            if(pEarliestTimer->pPreTimer == NULL)
                            {
                                g_pTimeoutCheckListHead = pEarliestTimer->pNextTimer;
                                g_pTimeoutCheckListHead->pPreTimer = NULL;
                                pEarliestTimer->pNextTimer = NULL;
                            }
                            else if(pEarliestTimer->pNextTimer == NULL)
                            {
                                pEarliestTimer->pPreTimer->pNextTimer = NULL;
                                pEarliestTimer->pPreTimer = NULL;
                            }
                            else
                            {
                                pEarliestTimer->pPreTimer->pNextTimer = pEarliestTimer->pNextTimer;
                                pEarliestTimer->pNextTimer->pPreTimer = pEarliestTimer->pPreTimer;
                                pEarliestTimer->pPreTimer = NULL;
                                pEarliestTimer->pNextTimer = NULL;
                            }
                        }
                    }
                    else
                    {
                        pHandleTimer_pre->pNextHandle = pHandleTimer->pNextHandle;
                        pHandleTimer->pNextHandle = NULL;
                    }
                    return 0;
                }
                else
                {
                    pHandleTimer_pre = pHandleTimer;
                    pHandleTimer = pHandleTimer_pre->pNextHandle;
                }
            }
            pEarliestTimer = pEarliestTimer->pNextTimer;
        }
        #ifdef DEBUG_PRINTF
        printf("\nThere is no this timer task!\n");
        #endif
        return 2;   //出错，超时检测链表中没有这个超时任务
    }
    else if(nCancelMode == CANCEL_MODE_AFTER_NEXT_TIMEOUT)
    {
        pChoosedTimer->bIsSingleUse = true;
        return 0;
    }
    else
    {
        return 1;   //出错，模式错误，不认识该模式
    }
}
/**
 * @function    定时器处理函数，用于检测是否有定时任务超时，如果有则调用该定时任务的回调函数，并更新超时检测链表
 *              更新动作：如果超时的那个定时任务不是一次性的，则将新的节点加入到检测超时链表中，否则直接删掉该节点；
 * @parameter   
 * @return
*/
void SYSTimeoutHandler(int signo)
{
    //printf("\nenter SYSTimeoutHandler\n");
    if(signo != SIGALRM)
        return;
    tSppMultiTimer* pEarliestTimer = NULL;
    tSppMultiTimer* pWaitingToHandle = NULL;
    tSppMultiTimer* pEarliestTimerPreHandle = NULL;

    if(g_pTimeoutCheckListHead != NULL)
    {
        if((g_pTimeoutCheckListHead->nTimeStamp <= g_nAbsoluteTime) && (g_pTimeoutCheckListHead->bIsOverflow == g_bIs_g_nAbsoluteTimeOverFlow))
        {
            pWaitingToHandle = g_pTimeoutCheckListHead;
            g_pTimeoutCheckListHead = g_pTimeoutCheckListHead->pNextTimer;
            if(g_pTimeoutCheckListHead != NULL)
                g_pTimeoutCheckListHead->pPreTimer = NULL;
            pWaitingToHandle->pNextTimer = NULL;

            pEarliestTimer = pWaitingToHandle;
            while(pEarliestTimer != NULL)
            {
                pEarliestTimerPreHandle = pEarliestTimer;
                pEarliestTimer = pEarliestTimer->pNextHandle;
                pEarliestTimerPreHandle->pNextHandle = NULL;
                pEarliestTimerPreHandle->pNextTimer = NULL;
                pEarliestTimerPreHandle->pPreTimer = NULL;
                pEarliestTimerPreHandle->pTimeoutCallbackfunction(pEarliestTimerPreHandle->pTimeoutCallbackParameter);
                if(!(pEarliestTimerPreHandle->bIsSingleUse))
                    AddTimerToCheckList(pEarliestTimerPreHandle);
            }
        }
    }
    
    g_nAbsoluteTime++;
    if(g_nAbsoluteTime == 0)
        g_bIs_g_nAbsoluteTimeOverFlow = !g_bIs_g_nAbsoluteTimeOverFlow;
    
    return ;
}

void CancleAllTimerTask()
{
    tSppMultiTimer* pEarliestTimer = NULL;
    tSppMultiTimer* pHandleTimer = NULL;

    while(g_pTimeoutCheckListHead != NULL)
    {
        pEarliestTimer = g_pTimeoutCheckListHead;
        g_pTimeoutCheckListHead = g_pTimeoutCheckListHead->pNextTimer;

        while(pEarliestTimer != NULL)
        {
            pHandleTimer = pEarliestTimer;
            pEarliestTimer = pEarliestTimer->pNextHandle;

            pHandleTimer->pNextHandle = NULL;
            pHandleTimer->pNextTimer = NULL;
            pHandleTimer->pPreTimer = NULL;
            pHandleTimer->bIsOverflow = false;
        }
    }
    g_bIs_g_nAbsoluteTimeOverFlow = false;
    g_nAbsoluteTime = 0;
    return;
}

void MultiTimerInit()
{    
    g_pTimeoutCheckListHead = NULL;
    g_bIs_g_nAbsoluteTimeOverFlow = false;
    g_nAbsoluteTime = 0;
    TIMER_ID_RECORD = 0;

    g_aSPPMultiTimer = (tSppMultiTimer**)calloc(sizeof(tSppMultiTimer*),MAX_TIMER_UPPER_LIMIT);
    return ;
}

void spp_timer_init()
{
    MultiTimerInit();
}

uint16_t spp_timer_create(uint32_t interval_ms,TimeoutCallBack cb,void* pCallBackParameter)
{
    if(TIMER_ID_RECORD >= MAX_TIMER_UPPER_LIMIT)
    {
        MAX_TIMER_UPPER_LIMIT += 50;
        if(MAX_TIMER_UPPER_LIMIT < 50)
            return -1;

        g_aSPPMultiTimer = (tSppMultiTimer**)realloc(g_aSPPMultiTimer,sizeof(tSppMultiTimer*)*MAX_TIMER_UPPER_LIMIT);
    }

    g_aSPPMultiTimer[TIMER_ID_RECORD] = (tSppMultiTimer*)CMALLOC(sizeof(tSppMultiTimer));
    g_aSPPMultiTimer[TIMER_ID_RECORD]->nTimerID = TIMER_ID_RECORD;
    g_aSPPMultiTimer[TIMER_ID_RECORD]->nInterval = interval;
    g_aSPPMultiTimer[TIMER_ID_RECORD]->nTimeStamp = 0;
    g_aSPPMultiTimer[TIMER_ID_RECORD]->bIsSingleUse = true;
    g_aSPPMultiTimer[TIMER_ID_RECORD]->bIsOverflow = false;
    g_aSPPMultiTimer[TIMER_ID_RECORD]->pTimeoutCallbackfunction = cb;
    g_aSPPMultiTimer[TIMER_ID_RECORD]->pTimeoutCallbackParameter = pCallBackParameter;
    g_aSPPMultiTimer[TIMER_ID_RECORD]->pNextTimer = NULL;
    g_aSPPMultiTimer[TIMER_ID_RECORD]->pPreTimer = NULL;
    g_aSPPMultiTimer[TIMER_ID_RECORD]->pNextHandle = NULL;

    return TIMER_ID_RECORD++;
}

void spp_timer_start(uint16_t handler)
{
    tSppMultiTimer* pChoosedTimer = NULL;
    pChoosedTimer = g_aSPPMultiTimer[handler];
    pChoosedTimer->bIsSingleUse = false;

    //如果超时任务链表中已经有这个任务了，先取消，然后再设置，即重置超时任务
    if(pChoosedTimer->pNextTimer != NULL || pChoosedTimer->pPreTimer != NULL || pChoosedTimer->pNextHandle != NULL)
        CancelTimerTask(nTimerID,CANCEL_MODE_IMMEDIATELY);
        
    AddTimerToCheckList(pChoosedTimer);
    return ;
}

void spp_timer_stop(uint16_t handler)
{
    CancelTimerTask(handler,CANCEL_MODE_IMMEDIATELY);
}
