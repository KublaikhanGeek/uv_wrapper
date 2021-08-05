#include "msg_queue.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include "zlog.h"

static void* msg_thread(void* arg)
{
    msg_queue_t* args = (msg_queue_t*)arg;
    if (!args)
    {
        dzlog_error("msg_thread error: args is NULL");
        return NULL;
    }

    dzlog_debug("msg_thread ---start---");
    msg_t msg;
    ssize_t msgsize;
    while (args->is_running)
    {
        msgsize = msgrcv(args->msg_id, &msg, sizeof(msg_t) - sizeof(long), 0, 0);
        if (-1 == msgsize)
        {
            if (errno == EIDRM)
            {
                dzlog_error("message queue is destroyed");
                break;
            }
            else if (errno == E2BIG)
            {
                dzlog_error("the value of mtext is greater than msgsz");
                msgsize = msgrcv(args->msg_id, &msg, sizeof(msg_t) - sizeof(long), MSG_NOERROR, 0);
            }
            continue;
        }

        if (msgsize != (sizeof(msg_t) - sizeof(long)))
        {
            continue;
        }

        if (msg.msg_buf != NULL)
        {
            if (args->on_msg)
            {
                (args->on_msg)(msg.msg_buf, msg.msg_size);
            }

            if (msg.msg_buf)
            {
                free(msg.msg_buf);
            }
        }
    }

    dzlog_debug("msg_thread ---END---");

    return NULL;
}

msg_queue_t* msg_queue_create(key_t msg_key, on_msg_t on_msg)
{
    msg_queue_t* msg_queue = (msg_queue_t*)malloc(sizeof(msg_queue_t));
    if (!msg_queue)
    {
        dzlog_error("msg_queue malloc failed");
        return NULL;
    }

    msg_queue->msg_id = msgget(msg_key, IPC_CREAT | IPC_EXCL | 0666);
    if (-1 == msg_queue->msg_id)
    {
        msg_queue->msg_id = msgget(msg_key, IPC_CREAT | 0666);
        if (-1 == msg_queue->msg_id)
        {
            dzlog_error("msgget create failed");
            free(msg_queue);
            return NULL;
        }
        else
        {
            // delete old msg
            msgctl(msg_queue->msg_id, IPC_RMID, NULL);
            // create new msg
            msg_queue->msg_id = msgget(msg_key, IPC_CREAT | 0666);
            if (-1 == msg_queue->msg_id)
            {
                dzlog_error("msgget create failed");
                free(msg_queue);
                return NULL;
            }
        }
    }

    dzlog_debug("msgid is: %d", msg_queue->msg_id);
    msg_queue->on_msg     = on_msg;
    msg_queue->is_running = true;
    pthread_create(&(msg_queue->thread_id), NULL, msg_thread, msg_queue);

    return msg_queue;
}

void msg_queue_destroy(msg_queue_t* msg_queue)
{
    msg_queue->is_running = false;
    msgctl(msg_queue->msg_id, IPC_RMID, NULL);
    pthread_join(msg_queue->thread_id, NULL);
    if (msg_queue)
    {
        free(msg_queue);
    }
}

int msg_send(int msg_id, void* data, size_t len, long msg_type)
{
    if ((msg_id < 0) | (!data))
    {
        return -1;
    }

    char* buf = (char*)malloc(len);
    if (!buf)
    {
        return -1;
    }

    msg_t msg;
    memcpy(buf, data, len);
    msg.type     = msg_type <= 0 ? 1 : msg_type;
    msg.msg_buf  = buf;
    msg.msg_size = len;

    //  printf("send msgid is %d srcModId %d \n",msgId,srcModId);
    dzlog_debug("msgid is: %d", msg_id);
    int status = msgsnd(msg_id, (const void*)&msg, sizeof(msg_t) - sizeof(long), 0);
    if (-1 == status)
    {
        dzlog_error("msgsnd send failed : %s", strerror(errno));
        free(buf);
        return -1;
    }

    return 0;
}