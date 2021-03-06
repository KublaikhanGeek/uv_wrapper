#ifndef SERIAL_H_
#define SERIAL_H_
#include "uv.h"

#define MAX_BUF_LEN 4096

typedef void (*serial_on_read_func_t)(void* data, size_t len);

typedef struct serial_s serial_t;
struct serial_s
{
    uv_poll_t* poller;
    uv_async_t* async_close;
    uv_thread_t thread_id;
    int fd;
    char buf[MAX_BUF_LEN];
    serial_on_read_func_t on_data;
};

extern serial_t* serial_run(const char* device, uv_loop_t* loop, serial_on_read_func_t on_data);
extern void serial_close(serial_t* handle);
extern int serial_set_opt(serial_t* handle, int speed, int databits, char parity, int stopbits);
extern ssize_t serial_send(serial_t* handle, char* data, size_t size);

#endif