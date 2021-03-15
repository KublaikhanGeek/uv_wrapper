#include "event_loop_thread.h"
#include "uv.h"
#include "uv_wrapper.h"
#include <stdlib.h>
#include "zlog.h"

// one loop per thread
static void* event_thread(void* arg)
{
    args_t* args = (args_t*)arg;
    if (!args)
    {
        dzlog_error("event_thread error: args is NULL");
        return NULL;
    }

    uv_loop_init(args->loop);
    args->loop->data = args;
    if (args->func)
    {
        args->func(arg);
    }

    dzlog_debug("event_loop ---start---");
    uv_run(args->loop, UV_RUN_DEFAULT);

    /* cleanup */
    uv_tear_down(args->loop);
    if (args->loop)
    {
        free(args->loop);
    }
    free(args);
    dzlog_debug("event_thread ---END---");

    return NULL;
}

int event_thread_create(pthread_t* thread, void* (*start_routine)(void*), void* arg)
{
    args_t* args = (args_t*)malloc(sizeof(args_t));
    if (!args)
    {
        dzlog_error("event_thread_create error: malloc args failed");
        return -1;
    }

    uv_loop_t* loop = (uv_loop_t*)malloc(sizeof(uv_loop_t));
    if (!args)
    {
        dzlog_error("event_thread_create error: malloc loop failed");
        return -1;
    }

    args->func = start_routine;
    args->arg  = arg;
    args->loop = loop;
    return uv_thread_create(thread, event_thread, args);
}

void event_thread_join(pthread_t thread)
{
    uv_thread_join(thread);
}