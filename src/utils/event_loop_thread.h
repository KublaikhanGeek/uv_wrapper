#ifndef EVENT_LOOP_THREAD_H_
#define EVENT_LOOP_THREAD_H_

#include <pthread.h>
#include "uv.h"

typedef struct args_s args_t;
typedef void* (*thread_call)(void*);

struct args_s
{
    uv_loop_t* loop;
    thread_call func;
    void* arg;
};

extern int event_thread_create(pthread_t* thread, void* (*start_routine)(void*), void* arg);
extern void event_thread_destroy(pthread_t thread);
#endif