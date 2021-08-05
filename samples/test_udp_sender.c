#include "zlog.h"
#include "event_loop_thread.h"
#include "udp.h"
#include <unistd.h>

static int stop          = 0;
udp_socket_t* udp_handle = NULL;
pthread_t id[5];
const char* msg_str = "hello, server";

void signal_handle(int num)
{
    dzlog_info("get a signal[%d]\n", num);
    stop = 1;
    sleep(2);
    udp_handle_close(udp_handle);
}

void signal_init()
{
    // ctr+c
    signal(SIGINT, signal_handle);
    // kill
    signal(SIGTERM, signal_handle);
}

void on_error(udp_socket_t* handle, const char* msg)
{
    dzlog_info("connection is error: %s", msg);
}

void on_read(udp_socket_t* handle, const struct sockaddr* peer, void* data, size_t len)
{
    int port     = 0;
    char ip[256] = { 0 };
    if (peer->sa_family == AF_INET)
    {
        struct sockaddr_in* addr = (struct sockaddr_in*)peer;
        port                     = ntohs(addr->sin_port);
        inet_ntop(AF_INET, &(addr->sin_addr), ip, sizeof(ip));
    }
    else if (peer->sa_family == AF_INET6)
    {
        struct sockaddr_in6* addr = (struct sockaddr_in6*)peer;
        port                      = ntohs(addr->sin6_port);
        inet_ntop(AF_INET6, &(addr->sin6_addr), ip, sizeof(ip));
    }

    dzlog_info("[client] receive message: %s from [%s:%d]", (char*)data, ip, port);
    udp_send_data_ip(udp_handle, data, len, "127.0.0.1", 7000);
}

void* thread_run(void* arg)
{
    dzlog_debug("[0x%lx] thread_run", pthread_self());
    args_t* args = (args_t*)arg;
    udp_handle   = udp_handle_run(NULL, 0, args->loop, on_read, NULL, on_error);

    return NULL;
}

void* thread_send(void* args)
{
    int index = 0;
    while (0 == stop)
    {
        sleep(1);
        if (0 == stop && 0 == (index % 2))
        {
            if (udp_handle)
            {
                dzlog_debug("[0x%lx] send data ", pthread_self());
                udp_send_data_ip(udp_handle, (char*)msg_str, strlen(msg_str) + 1, "127.0.0.1", 7000);
            }
        }
        ++index;
    }

    return NULL;
}

int main(int argc, char** argv)
{
    pthread_t thread;
    int rc = dzlog_init("./zlog.conf", "udp_sender");
    if (rc)
    {
        printf("init zlog failed!\n");
        return -1;
    }

    dzlog_debug("[0x%lx] =====start", pthread_self());

    signal_init();
    event_thread_create(&thread, thread_run, NULL);

    size_t i = 0;
    for (i = 0; i < 5; i++)
    {
        pthread_create(&id[i], NULL, thread_send, NULL);
    }

    event_thread_destroy(thread);
    for (i = 0; i < 5; i++)
    {
        pthread_join(id[i], NULL);
    }
    zlog_fini();

    return 0;
}