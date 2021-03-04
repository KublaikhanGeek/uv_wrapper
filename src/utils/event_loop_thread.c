#include "event_loop_thread.h"
#include "uv.h"
#include "uv_wrapper.h"

// one loop per thread
static void* event_thread(void* arg)
{
    args_t* args = (args_t*)arg;
    uv_loop_init(args->loop);
    if (args->func)
    {
        args->func(arg);
    }
    uv_run(args->loop, UV_RUN_DEFAULT);

    return NULL;
}

int event_thread_create(pthread_t* thread, uv_loop_t* loop, const pthread_attr_t* attr, void* (*start_routine)(void*),
                        void* arg)
{
    args_t args = { 0 };
    args.func   = start_routine;
    args.arg    = arg;
    args.loop   = loop;
    return pthread_create(thread, attr, event_thread, &args);
}

void event_thread_stop(pthread_t thread, uv_loop_t* loop)
{
    uv_tear_down(loop);
    pthread_join(thread, NULL);
}