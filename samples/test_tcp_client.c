#include "zlog.h"
#include "event_loop_thread.h"
#include "tcp_client.h"
#include <unistd.h>

static int stop          = 0;
tcp_client_t* tcp_client = NULL;
static int conn_status   = 0;
pthread_t id[5];
const char* thread_info[5] = { "000000000000", "1111111111111", "2222222222222", "333333333333", "444444444444" };

const char* message = "hello, server\\n";
void signal_handle(int num)
{
    dzlog_info("get a signal[%d]\n", num);
    stop = 1;
    sleep(3);
    tcp_client_close(tcp_client);
}

void signal_init()
{
    signal(SIGPIPE, SIG_IGN);
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
    tcp_client_send_data(conn, data, len);
}

void* tread_run(void* arg)
{
    args_t* args = (args_t*)arg;
    tcp_client   = tcp_client_run("127.0.0.1", 7000, args->loop, on_conn, on_conn_close, on_conn_error, on_read, NULL);

    return NULL;
}

void* thread_send(void* args)
{
    int i = 0;
    while (0 == stop)
    {
        if (conn_status == 1)
        {
            // if (strcmp(args, "000000000000") == 0)
            {
                dzlog_debug("[%s----0x%lx] [%d] send data --start--", (char*)args, pthread_self(), i);
            }

            tcp_client_send_data(&(tcp_client->conn), (char*)message, strlen(message) + 1);
            // if (strcmp(args, "000000000000") == 0)
            {
                dzlog_debug("[%s----0x%lx] [%d] send data --end--", (char*)args, pthread_self(), i);
            }
            ++i;
        }
        sleep(1);
    }

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
    event_thread_create(&thread, tread_run, NULL);

    for (size_t i = 0; i < 5; i++)
    {
        pthread_create(&id[i], NULL, thread_send, (void*)thread_info[i]);
    }

    event_thread_join(thread);
    for (size_t i = 0; i < 5; i++)
    {
        pthread_join(id[i], NULL);
    }
    zlog_fini();
}