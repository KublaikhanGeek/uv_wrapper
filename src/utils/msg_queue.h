#ifndef MSG_QUEUE_H_
#define MSG_QUEUE_H_

#include <pthread.h>
#include <sys/ipc.h>
#include "common.h"

typedef void (*on_msg_t)(void* data, size_t len);

typedef struct msg_s
{
    long type;
    void* msg_buf;
    size_t msg_size;
} msg_t;

typedef struct msg_queue_s
{
    int msg_id;
    pthread_t thread_id;
    on_msg_t on_msg;
    bool is_running;
} msg_queue_t;

extern msg_queue_t* msg_queue_create(key_t msg_key, on_msg_t on_msg);
extern void msg_queue_destroy(msg_queue_t* msg_queue);
extern int msg_send(int msg_id, void* data, size_t len, long msg_type);
#endif