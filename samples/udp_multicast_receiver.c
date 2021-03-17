
#include "zlog.h"
#include "event_loop_thread.h"
#include "udp.h"
#include <unistd.h>

static int stop          = 0;
udp_socket_t* udp_handle = NULL;

void signal_handle(int num)
{
    dzlog_info("get a signal[%d]\n", num);
    udp_handle_close(udp_handle);
    stop = 1;
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

    dzlog_info("[receiver] receive message: %s from [%s:%d]", (char*)data, ip, port);
    udp_send_data_ip(udp_handle, data, strlen(data) + 1, ip, port);
}

void* thread_run(void* arg)
{
    args_t* args = (args_t*)arg;
    udp_handle   = udp_handle_run("0.0.0.0", 8000, args->loop, on_read, NULL, on_error);

    udp_set_membership(udp_handle, "224.0.0.88", "0.0.0.0", true);
    udp_set_multicast_loop(udp_handle, true);

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

    dzlog_debug("======start");

    signal_init();
    event_thread_create(&thread, thread_run, NULL);

    int index = 1;
    // const char* data = "hello, server";
    while (0 == stop)
    {
        sleep(1);
        if (0 == (index % 5))
        {
            if (udp_handle)
            {
                dzlog_debug("send message");
                //                udp_send_data_ip(udp_handle, (char*)data, strlen(data) + 1, "192.168.79.1", 7000);
            }
        }
        ++index;
    }

    event_thread_join(thread);
    zlog_fini();
}