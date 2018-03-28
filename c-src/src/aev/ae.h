﻿/* A simple event-driven programming library. Originally I wrote this code
 * for the Jim's event-loop (Jim is a Tcl interpreter) but later translated
 * it in form of a library for easy reuse.
 *
 * Copyright (c) 2006-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


/*
事件触发后，并不立即处理，而是先做标记
然后，在每一个逻辑帧内，先处理timer，然后再处理网络消息

每个逻辑帧以10ms为单位，注意的是需要控制每一帧都是走10ms

假设：
t1 = timer时间 + 网络处理时间，
t2 = epoll阻塞时间
也即是 t = t1 + t2 = 10ms

那么将有几种情况：
1、t1<10ms,则t2=10-t1
2、t1=10ms,则t2=0,也即是说epoll立即范围，什么也不干，等到下一帧再处理
3、20ms>t1>10ms,则t2<0,也就表示这一帧花费太多时间，那么在下一帧的时候要在20-t1内处理完
4、t1>=20ms,那么下一帧就要连续处理2帧，来弥补这一帧耗费的过长的时间

这样做是为了，保证timer一定是以10ms为单位往前推进，确保定时器的精确

while{
    // timer
    // 网络
    // epoll 等待10ms
}

可以优化的地方：
1、events 不用fd作为key


*/


#ifndef __AE_H__
#define __AE_H__

#include <time.h>
#include <stdint.h>

#define AE_OK 0
#define AE_ERR -1

#define AE_NONE 0
#define AE_READABLE 1
#define AE_WRITABLE 2

/* Macros */
#define AE_NOTUSED(V) ((void) V)

struct aeEventLoop;

/* Types and data structures */
typedef void aeFileProc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);
typedef void aeBeforeSleepProc(struct aeEventLoop *eventLoop);
typedef int aeTimeProc(uint32_t id, void *clientData);

struct timer_node {
    uint32_t expire;                // 到期时间
    uint32_t id;                    // 定时器id
    int32_t level;                  // -1表示在near内
    int32_t index;
    uint32_t isuse;                 // 1表示在使用中的节点
    aeTimeProc *cb;
    void *clientData;
    struct timer_node *pre;
    struct timer_node *next;
};

struct timer_list{
    struct timer_node *head;
    struct timer_node *tail;
};

typedef struct aeTimer{
    struct timer_list near[256];
    struct timer_list tv[4][64];
    uint32_t tick;                  // 经过的滴答数,2^32个滴答
    uint32_t starttime;             // 定时器启动时的时间戳（秒）
    uint64_t currentMs;             // 当前毫秒数
    uint32_t usedNum;               // 定时器工厂大小
    struct timer_node **usedTimer;  // 定时器工厂（所有的定时器节点都是从这里产生的）
    struct timer_list freeTimer;    // 回收的定时器
} aeTimer;

/* File event structure */
typedef struct aeFileEvent {
    int mask; /* one of AE_(READABLE|WRITABLE) */
    aeFileProc *rfileProc;
    aeFileProc *wfileProc;
    void *clientData;
} aeFileEvent;


/* A fired event */
typedef struct aeFiredEvent {
    int fd;
    int mask;
} aeFiredEvent;


/* State of an event based program */
typedef struct aeEventLoop {
    int maxfd;   /* highest file descriptor currently registered */
    int setsize; /* max number of file descriptors tracked */
    time_t lastTime;     /* Used to detect system clock skew */
    aeFileEvent *events; /* Registered events */
    aeFiredEvent *fired; /* Fired events */
    aeTimer *timer; /* Time wheel */
    int stop;
    void *apidata; /* This is used for polling API specific data */
    aeBeforeSleepProc *beforesleep;
} aeEventLoop;


// 封装的通用事件
typedef struct ev{
    void * clientData;  // 指向一个session
    int read;           // =1表示有可读事件
    int write;          // =1表示有可写事件，表示上一次写入缓存已经满了，这一次缓存有空闲，可以写入上次没有写完的数据
    int error;          // =1表示有错误，需要断开连接了
};




/* Prototypes */
aeEventLoop *aeCreateEventLoop(int setsize);
void aeDeleteEventLoop(aeEventLoop *eventLoop);
void aeStop(aeEventLoop *eventLoop);

/* File Event */
int aeCreateFileEvent(aeEventLoop *eventLoop, int fd, int mask, aeFileProc *proc, void *clientData);
void aeDeleteFileEvent(aeEventLoop *eventLoop, int fd, int mask);
int aeGetFileEvents(aeEventLoop *eventLoop, int fd);

/* Process */
int aeProcessEvents(aeEventLoop *eventLoop);
void aeMain(aeEventLoop *eventLoop);
char *aeGetApiName(void);
void aeSetBeforeSleepProc(aeEventLoop *eventLoop, aeBeforeSleepProc *beforesleep);
int aeGetSetSize(aeEventLoop *eventLoop);
int aeResizeSetSize(aeEventLoop *eventLoop, int setsize);

/* Time Event */
int aeCreateTimer(aeEventLoop * eventLoop);
void aeDestroyTimer(aeEventLoop * eventLoop);
uint32_t aeAddTimer(aeEventLoop * eventLoop, uint32_t tickNum, aeTimeProc *proc, void *clientData);
void aeDelTimer(aeEventLoop * eventLoop, uint32_t id);
int32_t aeTimerUpdatetime(aeEventLoop * eventLoop);

#endif
