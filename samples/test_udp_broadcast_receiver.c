
#include "zlog.h"
#include "event_loop_thread.h"
#include "udp.h"
#include <unistd.h>

static int stop          = 0;
udp_socket_t* udp_handle = NULL;

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

    dzlog_info("[receiver] receive message: %s from [%s:%d]", (char*)data, ip, port);
}

void* thread_run(void* arg)
{
    args_t* args = (args_t*)arg;
    udp_handle   = udp_handle_run("0.0.0.0", 9000, args->loop, on_read, NULL, on_error);

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

    while (0 == stop)
    {
        sleep(1);
    }

    event_thread_destroy(thread);
    zlog_fini();

    return 0;
}