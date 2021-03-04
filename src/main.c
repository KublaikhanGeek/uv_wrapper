#include "zlog.h"

#define LOG_CONFIG_PATH "/etc/zlog.conf"
#define LOG_TAG_NAME "ZT_LOG"

int main(int argc, char** argv)
{
    int rc, ret = -1;
    rc = dzlog_init(LOG_CONFIG_PATH, LOG_TAG_NAME);
    if (rc)
    {
        return ret;
    }

    zlog_fini();
    
}