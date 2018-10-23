# Linux下一种高效多定时器实现

**作者：LouisozZ**

**日期：2018.08.29**



## 运行环境说明

由于在 Linux 系统下一个进程只能设置一个时钟定时器，所以当应用需要有多个定时器来共同管理程序运行时，就需要自行实现多定时器管理。

本文就是基于这种需求，在实际编码工作的基础上总结而出，希望跟大家共享，也欢迎大家讨论指正。



## 多定时器原理

在一个进程只能有一个定时器的前提条件下，想要实现多定时器，就得用到那个进程能利用的唯一的定时器，这个定时器是由操作系统提供的，通过系统提供的接口来设置，常用的有 alarm() 和 setitimer()，不论用什么，后文统一称作系统定时接口，这两个接口的区别在很多博客里都有，不怎么清楚的可以自行搜索，这里就不再赘述（我比较懒，打字多对肾不好）。通过它们产生的定时信号作为基准时间，来管理实现多定时器。

举个栗子，糖炒板栗。利用系统定时接口设置了基准定时器，基准定时器每秒产生一个 SIGALRM 信号（系统时钟的超时时间到了之后会向进程发送信号以通知定时超时，alarm() 和 setitimer() 都是向进程发送 SIGALRM 信号，关于 Linux  ‘信号’ 的内容，可以参考 《UNIX环境高级编程》），产生两个 SIGALRM 信号的时间间隔，就是多定时器的基准时间。当然，上述的基准时间是一秒，如果你是每隔 50ms 产生一个 SIGALRM 信号，那么多定时器的基准时间就是 50ms 。当有了基准时间之后，就可以对它进行管理，可以设置多个定时任务，现有两个定时任务，Timer1_Task , Timer2_Task， 其中 Timer1_Task 的定时时长为 10 个基准时间，Timer2_Task 为 15 个基准时间，则每产生 10 个 SIGALRM 信号，就表示  Timer1_Task 定时器超时到达，执行一次  Timer1_Task  的超时任务，每产生 15 个 SIGALRM 信号，则执行一次 Timer2_Task 超时任务，当产生的 SIGALRM 信号个数是 30 （10 和 15 的最小公倍数），则 Timer1_Task 和 Timer2_Task 的超时任务都要被执行。

好了，原理讲完了，下面就是本文的重点了。



---------------------------------------    说重点专用~~阉割~~线    ---------------------------------------

---------------------------------------    说重点专用分割线    ---------------------------------------



## 高效多定时器



### 数据结构

由一个全局链表 g_pTimeoutCheckListHead 来管理超时任务。链表的每个节点是一个 tMultiTimer 结构体：

```c
	typedef void TimeoutCallBack(void*);	//回调函数格式
	typedef struct tMultiTimer
	{
    	uint16_t nTimerID;       //设置宏定义
    	uint32_t nInterval;     //定时时长
    	uint32_t nTimeStamp;    //时间戳
    	bool bIsSingleUse;      //是否单次使用
    	bool bIsOverflow;       //用于解决计数溢出问题

    	TimeoutCallBack *pTimeoutCallbackfunction;	//回调函数
    	void* pTimeoutCallbackParameter;			//回调函数参数
    	
    	struct tMultiTimer* pNextTimer;				//双向链表后驱指针
    	struct tMultiTimer* pPreTimer;				//双向链表前驱指针
    	struct tMultiTimer* pNextHandle;			//二维链表相同超时Timer节点
	}tMultiTimer;

	tMultiTimer* g_pTimeoutCheckListHead;			//管理多定时器的全局链表
	bool g_bIs_g_nAbsoluteTimeOverFlow;				//基准时间计次器溢出标志位
	uint32_t g_nAbsoluteTime;						//基准时间计次器
```

（各个成员变量的意义在后文会逐一介绍，客官莫急）

这个是一个二维双向链表，第一维根据时间戳，即绝对时间，按照先后顺序连接每一个 tMultiTimer 节点，当有多个超时任务的超时时刻是相同的时候，只有一个节点位于第一维，其余接在上一个相同超时时刻 tMultiTimer 节点的 pNextHandle 上，图示如下：

![](C:\Users\qifan\Desktop\多时钟.svg)

### 多定时管理流程

#### 超时检测与运行

首先需要调用系统定时接口，设置进程的定时器，产生 SIGALRM 信号，每一次 SIGALRM 到来时，全局的基准时间计次器 g_nAbsoluteTime 自加，由于 g_nAbsoluteTime 是无符号类型，当其溢出时，是回到 0 ，每次溢出就把 g_bIs_g_nAbsoluteTimeOverFlow 取反。

每个  tMultiTimer 节点都有 ：

	uint32_t nInterval;     		//定时时长
	uint32_t nTimeStamp;    	//时间戳

其中 nTimeStamp 这个值，是由 nInterval + g_nAbsoluteTime 计算而来，在把这个节点加入到全局链表的时刻计算的 ，这个和作为超时的绝对时间保存在结构体中，当计算的和溢出时，bIsOverflow 取反。通过这两个溢出标志位，可以用来解决溢出之后判断是否超时的问题，具体如下：

每一次基准时间超时，就检查链表的第一个节点的超时时间 nTimeStamp 是否小于全局绝对时间 g_nAbsoluteTime ，如果 g_bIs_g_nAbsoluteTimeOverFlow 与 bIsOverflow 不相等，则链表第一个节点的超时时间一定未到达，因为 bIsOverflow 的取反操作一定是先于 g_bIs_g_nAbsoluteTimeOverFlow ，如果一样则比较数值大小（初始化的时候两个溢出标志位是一样的）。当全局绝对时间大于等于第一个节点的时间戳，则把该节点及其 pNextHandle 指向的第二维链表取下，并更新 g_pTimeoutCheckListHead，然后依次执行所取下链表的回调函数。执行完之后（或者之前，根据实际情况定），判断 bIsSingleUse 成员变量，如果为 true 则表示是单次的计数器，仅执行一次，执行完回调之后则定时任务完成。如果是 false ，怎表示是定时任务，则重新执行一次添加超时任务。（添加超时任务看下一节）

#### 添加超时任务

添加超时任务（添加一个  tMultiTimer 节点到全局链表 g_pTimeoutCheckListHead 中）的时候，指定超时时长，即间隔多少个基准时间，赋值给这个任务的成员变量 nInterval ，然后计算

```c
	nTimeStamp = nInterval  + g_nAbsoluteTime; 
	if(nTimeStamp  < g_nAbsoluteTime) 
		bIsOverflow = ~bIsOverflow;
```

接着搜索 g_pTimeoutCheckListHead ，如果有相同时间戳，则添加到其 pNextHandle 指向位置，如果没有相同时间戳节点，找到比要插入的节点时间戳大的节点，然后把当前节点插入到其前方。

对于链表中已经有相同 ID 的  tMultiTimer 节点的情况，再次添加则表示更新该定时任务，取消之前的定时任务重新插入到链表中。

#### 取消超时任务

直接把对应 ID 的  tMultiTimer 节点从 g_pTimeoutCheckListHead 链表中摘掉即可。



### 效率分析

g_pTimeoutCheckListHead 链表中的 tMultiTimer 节点数量，是总共设置的超时任务数量，假设为 n，添加一个超时任务（节点）的最坏情况是遍历  n 个节点。

检查是否有任务超时所用的时间是常数时间，只检查第一个节点。

对于定时任务的再次插入问题，如果定时任务间隔时间越短，其反复被插入的次数越多，但是由于定时时间短，所以在链表中的插入位置也就越靠前，将快速找到插入点；如果定时任务间隔时间越长，越可能遍历整个链表在末尾插入，但是由于间隔时间长，重复插入的频率则很低。



与一种简单的定时器实现相比较：

```c
	if(g_nAbsoluteTime  %  4)

{

}
```

这种简单实现来说，

1：每次添加、取消一个定时任务都需要修改定时器源码，复用性不高。

2：每次检查是否有任务超时需要遍历 n 个定时任务。



### 关于多线程下的一些坑

我在实际项目中使用的环境是多线程的，有四个，我把多定时器管理放在了单独的一个线程里。由于系统定时器接口产生的信号是发送给进程的，所以所有的线程都共享这个闹钟信号。一开始我是这么想的，定时器的默认动作是杀死进程，那么给每个线程添加信号捕捉函数，这样的话闹钟信号到了之后不管是那个线程接管了，都能到我指定的处理函数去，可是实际情况并非如此，进程仍然会被杀死。

后面我用了线程信号屏蔽，把非定时器线程都设置了信号屏蔽字，即闹钟信号不被别的线程可见，这样才能正常运行，至于第一种方法为何不行，现在我还没有找到原因，还是对 Linux 的信号机制不熟，以后看有时间的话把这里搞懂吧。



main.c

```c
	//配置信号集
	sigset_t g_sigset_mask;
    sigemptyset(&g_sigset_mask);
    sigaddset(&g_sigset_mask,SIGALRM);
```



other_thread.c

```c
	sigset_t old_sig_mask;
    if(err = pthread_sigmask(SIG_SETMASK,&g_sigset_mask,&old_sig_mask) != 0)
    {
        // pthread_sigmask 设置信号屏蔽
        return ;
    }
```



mulitimer_thread

```c
void* MultiTimer_thread(void *parameter)
{
    int err,signo;
    struct itimerval new_time_value,old_time_value;

    new_time_value.it_interval.tv_sec = 0;
    new_time_value.it_interval.tv_usec = 1000;
    new_time_value.it_value.tv_sec = 0;
    new_time_value.it_value.tv_usec = 1;
    setitimer(ITIMER_REAL, &new_time_value,NULL);

    for(;;)
    {
        err = sigwait(&g_sigset_mask,&signo);//信号捕捉
        if(err != 0)
        {
            return ;
        }

        if(signo == SIGALRM)
        {
            SYSTimeoutHandler(signo);
        }
    }
    return ((void*)0);
}
```



### 定时器源码地址

	https://github.com/LouisozZ/MultiTimer-for-Linux.git