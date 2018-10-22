#ifndef __SPP_PORTING_H__
#define __SPP_PORTING_H__

#include "pthread.h"

#define CDebugAssert(value) if(!(value)){printf("\n%s is not true!\nreturn...\n",#value);return;}

extern pthread_t nTimerThread;
extern pthread_t nUserThread;

void* CMALLOC(uint32_t length);
uint8_t CFREE(void* pFreeAddress);

void* MultiTimer_thread(void *parameter);
void* User_Thread(void *parameter);


#endif