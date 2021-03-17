/******************************************************************************
 * Copyright (c) 2007-2019, ZeroTech Co., Ltd.
 * All rights reserved.
 *******************************************************************************
 * File name     : serial.h
 * Description   :
 * Version       : v1.0
 * Create Time   : 2021/2/25
 * Author        : yuanshunbao
 * Modify history:
 *******************************************************************************
 * Modify Time   Modify person  Modification
 * ------------------------------------------------------------------------------
 *
 *******************************************************************************/
#ifndef SERIAL_H_
#define SERIAL_H_
#include "uv.h"

#define MAX_BUF_LEN 4096

typedef void (*serial_on_read_func_t)(void* data, size_t len);

typedef struct serial_s serial_t;
struct serial_s
{
    uv_poll_t poller;
    int fd;
    char buf[MAX_BUF_LEN];
    serial_on_read_func_t on_data;
};

extern serial_t* serial_run(const char* device, int bauds, uv_loop_t* loop, serial_on_read_func_t on_data);
extern void serial_close(serial_t* handle);
extern int serial_set_opt(serial_t* handle, int speed, int databits, char parity, int stopbits);
extern ssize_t serial_send(serial_t* handle, char* data, size_t size);

#endif