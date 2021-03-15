
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

void* thread_run(void* arg)
{
    args_t* args = (args_t*)arg;
    udp_handle   = udp_handle_create(NULL, 0, args->loop, NULL, NULL, NULL);
    udp_set_broadcast(udp_handle, true);

    return NULL;
}

int main(int argc, char** argv)
{
    pthread_t thread;
    char* data = "hello, server";
    int rc     = dzlog_init("./zlog.conf", "udp_sender");
    if (rc)
    {
        printf("init zlog failed!\n");
        return -1;
    }

    dzlog_debug("======start");

    signal_init();
    event_thread_create(&thread, thread_run, NULL);

    int index = 0;
    while (0 == stop)
    {
        sleep(1);
        if (0 == (index % 2))
        {
            if (udp_handle)
            {
                dzlog_debug("send message");
                udp_send_data_ip(udp_handle, data, strlen(data) + 1, "255.255.255.255", 9000);
            }
        }
        ++index;
    }

    event_thread_join(thread);
    zlog_fini();
}