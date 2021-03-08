#include "zlog.h"
#include "uv.h"
#include "event_loop_thread.h"
#include "stream_server.h"
#include <unistd.h>

static int stop          = 0;
tcp_server_t* tcp_server = NULL;

void signal_handle(int num)
{
    dzlog_info("get a signal[%d]\n", num);
    tcp_server_close(tcp_server);
    stop = 1;
}

void signal_init()
{
    // ctr+c
    signal(SIGINT, signal_handle);
    // kill
    signal(SIGTERM, signal_handle);
}

void on_conn(tcp_connection_t* conn)
{
    char dst[128] = { 0 };
    dzlog_info("a new connectioin: %s", tcp_server_get_client_addr(conn, dst, 127));
}

void on_read(tcp_connection_t* conn, void* data, size_t len)
{
    char dst[128] = { 0 };
    dzlog_info("receive message: %s from [%s]", (char*)data, tcp_server_get_client_addr(conn, dst, 127));
    tcp_server_send_data(conn, data, len);
}

void* tread_run(void* arg)
{
    args_t* args = (args_t*)arg;
    tcp_server   = tcp_server_run("0.0.0.0", 7000, args->loop, on_conn, NULL, NULL, on_read, NULL);

    return NULL;
}

int main(int argc, char** argv)
{
    pthread_t thread;
    int index = 0;
    int rc    = dzlog_init("./zlog.conf", "test");
    if (rc)
    {
        printf("init zlog failed!\n");
        return -1;
    }

    dzlog_info("======start");

    signal_init();
    event_thread_create(&thread, NULL, tread_run, NULL);

    while (0 == stop)
    {
        sleep(1);
        if (0 == (index % 5))
        {
            tcp_server_print_all_conn(tcp_server);
        }
        ++index;
    }

    event_thread_stop(thread);
    zlog_fini();
}