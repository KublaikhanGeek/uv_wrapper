#include "zlog.h"
#include "common.h"
#include "component.h"
#include <unistd.h>
#include <signal.h>

#define LOG_CONFIG_PATH "/etc/zlog.conf"
#define LOG_TAG_NAME    "ZT_LOG"
static int stop = 0;

void signal_handle(int num)
{
    dzlog_info("get a signal[%d]\n", num);
    stop = 1;
}

void signal_init()
{
    signal(SIGPIPE, SIG_IGN);
    // ctr+c
    signal(SIGINT, signal_handle);
    // kill
    signal(SIGTERM, signal_handle);
}

app_task_t tasks[] = {
    { COMPONENT, component_init, component_uninit },
};

int main(int argc, char** argv)
{
    int rc, ret = -1;
    rc = dzlog_init("./zlog.conf", LOG_TAG_NAME);
    if (rc)
    {
        printf("init zlog failed\n");
        return ret;
    }
    signal_init();

    size_t i = 0;
    for (i = 0; i < sizeof(tasks) / sizeof(tasks[0]); i++)
    {
        (tasks[i].task_init)();
    }

    while (0 == stop)
    {
        sleep(1);
    }

    for (i = 0; i < sizeof(tasks) / sizeof(tasks[0]); i++)
    {
        (tasks[i].task_uninit)();
    }

    zlog_fini();

    return 0;
}
