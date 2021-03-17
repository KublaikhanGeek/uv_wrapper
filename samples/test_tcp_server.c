#include "zlog.h"
#include "event_loop_thread.h"
#include "tcp_server.h"
#include <unistd.h>

static int stop          = 0;
tcp_server_t* tcp_server = NULL;
const char* message      = "server to client data";

void signal_handle(int num)
{
    dzlog_info("get a signal[%d]\n", num);
    stop = 1;
    sleep(3);
    if (tcp_server)
    {
        tcp_server_close(tcp_server);
    }
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
    char msg[256] = { 0 };
    strncpy(msg, data, len);
    dzlog_info("[server] receive message: %s from [%s]", msg, tcp_server_get_client_addr(conn, dst, 127));
    tcp_server_send_data(conn, data, len);
}

void* thread_run(void* arg)
{
    args_t* args = (args_t*)arg;
    tcp_server   = tcp_server_run("0.0.0.0", 7000, args->loop, on_conn, NULL, NULL, on_read, NULL);

    return NULL;
}

void* thread_send_data(void* arg)
{
    while (0 == stop)
    {
        sleep(1);
        if (tcp_server)
        {
            tcp_server_send_data_2_all(tcp_server, (char*)message, strlen(message) + 1);
        }
    }

    return NULL;
}

int main(int argc, char** argv)
{
    pthread_t thread;
    pthread_t thread_send;
    int index = 0;
    int rc    = dzlog_init("./zlog.conf", "tcp_server");
    if (rc)
    {
        printf("init zlog failed!\n");
        return -1;
    }

    dzlog_debug("======start");

    signal_init();
    event_thread_create(&thread, thread_run, NULL);

    pthread_create(&thread_send, NULL, thread_send_data, NULL);

    while (0 == stop)
    {
        sleep(1);
        if (0 == (index % 5))
        {
            if (tcp_server)
            {
                tcp_server_print_all_conn(tcp_server);
            }
        }
        ++index;
    }

    event_thread_join(thread);
    pthread_join(thread_send, NULL);
    zlog_fini();
}