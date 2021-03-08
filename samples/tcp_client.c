#include "zlog.h"
#include "event_loop_thread.h"
#include "stream_client.h"
#include <unistd.h>

static int stop          = 0;
tcp_client_t* tcp_client = NULL;
static int conn_status   = 0;

void signal_handle(int num)
{
    dzlog_info("get a signal[%d]\n", num);
    tcp_client_close(tcp_client);
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
    dzlog_info("connection is open");
    conn_status = 1;
}

void on_conn_close(tcp_connection_t* conn)
{
    dzlog_info("connection is close");
    conn_status = 0;
}

void on_conn_error(tcp_connection_t* conn, const char* msg)
{
    dzlog_info("connection is error: %s", msg);
    conn_status = 0;
}

void on_read(tcp_connection_t* conn, void* data, size_t len)
{
    dzlog_info("[client] receive message: %s from server", (char*)data);
}

void* tread_run(void* arg)
{
    args_t* args = (args_t*)arg;
    tcp_client   = tcp_client_run("127.0.0.1", 7000, args->loop, on_conn, on_conn_close, on_conn_error, on_read, NULL);

    return NULL;
}

int main(int argc, char** argv)
{
    pthread_t thread;
    int rc = dzlog_init("./zlog.conf", "tcp_client");
    if (rc)
    {
        printf("init zlog failed!\n");
        return -1;
    }

    dzlog_debug("======start");

    signal_init();
    event_thread_create(&thread, NULL, tread_run, NULL);

    int index  = 0;
    char* data = "hello, server";
    while (0 == stop)
    {
        sleep(1);
        if (0 == (index % 5))
        {
            if (conn_status == 1)
            {
                tcp_client_send_data(&(tcp_client->conn), data, strlen(data) + 1);
            }
        }
        ++index;
    }

    event_thread_join(thread);
    zlog_fini();
}