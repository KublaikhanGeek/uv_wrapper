#include "event_loop_thread.h"
#include "serial.h"
#include "zlog.h"
#include <string.h>
#include <unistd.h>

static int stop         = 0;
serial_t* serial_handle = NULL;
const char* message     = "hello world\n";

void signal_handle(int num)
{
    dzlog_info("get a signal[%d]\n", num);
    stop = 1;
    sleep(3);
    if (serial_handle)
    {
        serial_close(serial_handle);
    }
}

void signal_init()
{
    // ctr+c
    signal(SIGINT, signal_handle);
    // kill
    signal(SIGTERM, signal_handle);
}

void on_read(void* data, size_t len)
{
    char msg[256] = { 0 };
    strncpy(msg, data, len);
    dzlog_info("receive message: %s", msg);
    serial_send(serial_handle, data, len);
}

void* thread_run(void* arg)
{
    args_t* args  = (args_t*)arg;
    serial_handle = serial_run("/dev/pts/19", args->loop, on_read);
    int ret       = serial_set_opt(serial_handle, 115200, 8, 'N', 1);
    if (ret == -1)
    {
        serial_close(serial_handle);
    }

    return NULL;
}

void* thread_send_data(void* arg)
{
    while (0 == stop)
    {
        sleep(1);
        if (serial_handle)
        {
            dzlog_debug("send message");
            serial_send(serial_handle, (char*)message, strlen(message));
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
        ++index;
    }

    event_thread_join(thread);
    pthread_join(thread_send, NULL);
    zlog_fini();
}
