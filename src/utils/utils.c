#include <string.h>
#include "utils.h"
#include "zlog.h"

void print_in_hex(unsigned char* data, size_t len)
{
    unsigned char print[MAX_PRINT_BUFSIZE] = { 0 };
    size_t index                           = 0;

    if (len > 0)
    {
        len = len > MSG_PRINT_SIZE ? MSG_PRINT_SIZE : len;

        for (index = 0; index < len; index++)
        {
            sprintf((char*)&print[3 * index], "%02x ", *data++);
        }

        dzlog_debug("%s", print);
    }
}