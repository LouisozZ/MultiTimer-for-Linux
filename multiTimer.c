#include "multiTimer.h"

/**
 * @function    把一个定时任务添加到定时检测链表中
 * @parameter   一个定时器对象，可以由全局变量 g_aSPPMultiTimer 通过 TIMER_ID 映射得到
*/
static void AddTimerToCheckList(tMultiTimer* pTimer)
{
    tMultiTimer* pEarliestTimer = NULL;
    tMultiTimer* pEarliestTimer_pre = NULL;
    
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
 * @parameter1  TIMER_ID    
 * @parameter2  超时间隔时间
 * @parameter3  是否是一次性定时任务
 * @parameter4  回调函数，注意，回调函数的函数形式  void function(void*);
 * @parameter5  void* 回调函数的参数，建议用结构体强转成 void*，在回调函数中再强转回来  
 * @return      错误码 
*/
uint8_t SetTimer(uint8_t nTimerID,uint32_t nInterval,bool bIsSingleUse,TimeoutCallBack* pCallBackFunction,void* pCallBackParameter)
{
    printf("\nset timer %d\n",nTimerID);
    tMultiTimer* pChoosedTimer = NULL;
    pChoosedTimer = g_aSPPMultiTimer[nTimerID];
    pChoosedTimer->nInterval = nInterval;
    pChoosedTimer->bIsSingleUse = bIsSingleUse;
    pChoosedTimer->pTimeoutCallbackfunction = pCallBackFunction;
    pChoosedTimer->pTimeoutCallbackParameter = pCallBackParameter;

    //如果超时任务链表中已经有这个任务了，先取消，然后再设置，即重置超时任务
    if(pChoosedTimer->pNextTimer != NULL || pChoosedTimer->pPreTimer != NULL)
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
uint8_t CancelTimerTask(uint8_t nTimerID,uint8_t nCancelMode)
{
    printf("\ncancle timer %d\n",nTimerID);
    tMultiTimer* pEarliestTimer = NULL;
    tMultiTimer* pHandleTimer = NULL;
    tMultiTimer* pHandleTimer_pre = NULL;
    tMultiTimer* pChoosedTimer = NULL;

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
    tMultiTimer* pEarliestTimer = NULL;
    tMultiTimer* pWaitingToHandle = NULL;
    tMultiTimer* pEarliestTimerPreHandle = NULL;

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
    tMultiTimer* pEarliestTimer = NULL;
    tMultiTimer* pHandleTimer = NULL;

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
    for(uint8_t index = 0; index < MAX_TIMER_UPPER_LIMIT; index++)
    {
        g_aSPPMultiTimer[index] = (tMultiTimer*)CMALLOC(sizeof(tMultiTimer));
        g_aSPPMultiTimer[index]->nTimerID = g_aTimerID[index];
        g_aSPPMultiTimer[index]->nInterval = g_aDefaultTimeout[index];
        g_aSPPMultiTimer[index]->nTimeStamp = 0;
        g_aSPPMultiTimer[index]->bIsSingleUse = true;
        g_aSPPMultiTimer[index]->bIsOverflow = false;
        g_aSPPMultiTimer[index]->pTimeoutCallbackfunction = NULL;
        g_aSPPMultiTimer[index]->pTimeoutCallbackParameter = NULL;
        g_aSPPMultiTimer[index]->pNextTimer = NULL;
        g_aSPPMultiTimer[index]->pPreTimer = NULL;
        g_aSPPMultiTimer[index]->pNextHandle = NULL;
    }
    /*  如果预先规定了一些定时器，这个时候可以初始化除时间戳以外的其他值  */
    //开启应答超时任务
    //OPEN_MULTITIMER_MANGMENT();
}