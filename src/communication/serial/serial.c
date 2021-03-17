/******************************************************************************
 * Copyright (c) 2007-2019, ZeroTech Co., Ltd.
 * All rights reserved.
 *******************************************************************************
 * File name     : serial.c
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

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
#include "serial.h"
#include "zlog.h"

static speed_t get_baudrate(int bauds);
static ssize_t serial_read(serial_t* handle);
static int serial_bytes_availble(int fd);
static void serial_poll_ev(uv_poll_t* handle, int status, int events);

// Convert integer baudrate to terminos.
static speed_t get_baudrate(int bauds)
{
    switch (bauds)
    {
    case 150:
        return B150;
    case 200:
        return B200;
    case 300:
        return B300;
    case 600:
        return B600;
    case 1200:
        return B1200;
    case 1800:
        return B1800;
    case 2400:
        return B2400;
    case 4800:
        return B4800;
    case 9600:
        return B9600;
    case 19200:
        return B19200;
    case 38400:
        return B38400;
    case 57600:
        return B57600;
    case 115200:
        return B115200;
    case 460800:
        return B460800;
    default:
        return B115200;
    }
}

// We got a poll event.
static void serial_poll_ev(uv_poll_t* handle, int status, int events)
{
    if (status < 0)
    {
        dzlog_warn("Serial polling error: %s", uv_strerror(status));
        return;
    }

    serial_t* serial_handle = (serial_t*)handle->data;
    if (events & UV_READABLE)
    {
        serial_read(serial_handle);
    }
    else if (events & UV_WRITABLE)
    {
        dzlog_info("device fd is writable");
    }
}

// Count the number of bytes availables.
static int serial_bytes_availble(int fd)
{
    int count;
    ioctl(fd, FIONREAD, &count);
    return count;
}

// Read |count| bytes from the device.
static ssize_t serial_read(serial_t* handle)
{
    if (!handle)
    {
        return -1;
    }

    int size = serial_bytes_availble(handle->fd);
    if (size <= 0)
    {
        return size;
    }

    ssize_t ret = read(handle->fd, handle->buf, MAX_BUF_LEN);
    if (ret == -1)
    {
        dzlog_warn("Error when reading: %s", strerror(errno));
    }
    else
    {
        dzlog_warn("reading buf size: %d", ret);
        if (handle->on_data)
        {
            handle->on_data(handle->buf, (size_t)ret);
        }
    }
    return ret;
}

static void close_handle(serial_t* handle)
{
    uv_close((uv_handle_t*)&(handle->async_close), NULL);
    uv_poll_stop(&(handle->poller));
    uv_close((uv_handle_t*)&(handle->poller), NULL);

    if (handle->fd > 0)
    {
        close(handle->fd);
    }

    free(handle);
}

static void async_close_cb(uv_async_t* handle)
{
    serial_t* serial_handle = (serial_t*)handle->data;
    if (serial_handle)
    {
        close_handle(serial_handle);
    }
}

// Open the serial device.
serial_t* serial_run(const char* device, uv_loop_t* loop, serial_on_read_func_t on_data)
{
    serial_t* serial_handle = (serial_t*)malloc(sizeof(serial_t));
    if (!serial_handle)
    {
        return NULL;
    }

    serial_handle->fd      = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    serial_handle->on_data = on_data;
    if (serial_handle->fd < 0)
    {
        dzlog_error("Can't open serial device %s", device);
        free(serial_handle);
        return NULL;
    }

    if (0 == isatty(STDIN_FILENO))
    {
        dzlog_error("standard input is not a terminal device");
    }

    uv_poll_init(loop, &(serial_handle->poller), serial_handle->fd);
    uv_poll_start(&(serial_handle->poller), UV_READABLE, serial_poll_ev);
    uv_async_init(loop, &(serial_handle->async_close), async_close_cb);
    serial_handle->thread_id        = uv_thread_self();
    serial_handle->poller.data      = serial_handle;
    serial_handle->async_close.data = serial_handle;

    return serial_handle;
}

int serial_set_opt(serial_t* handle, int speed, int databits, char parity, int stopbits)
{
    if (!handle)
    {
        return -1;
    }

    struct termios newtio;
    struct termios oldtio;
    if (tcgetattr(handle->fd, &oldtio) != 0)
    {
        return -1;
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag |= CLOCAL | CREAD;
    newtio.c_cflag &= ~CSIZE;

    switch (databits)
    {
    case 7:
        newtio.c_cflag |= CS7;
        break;
    case 8:
        newtio.c_cflag |= CS8;
        break;
    }

    switch (parity)
    {
    case 'O':
        newtio.c_cflag |= PARENB;
        newtio.c_cflag |= PARODD;
        newtio.c_iflag |= (INPCK | ISTRIP);
        break;
    case 'E':
        newtio.c_iflag |= (INPCK | ISTRIP);
        newtio.c_cflag |= PARENB;
        newtio.c_cflag &= ~PARODD;
        break;
    case 'N':
        newtio.c_cflag &= ~PARENB;
        break;
    }

    speed_t rate = get_baudrate(speed);
    cfsetispeed(&newtio, rate);
    cfsetospeed(&newtio, rate);

    if (1 == stopbits)
    {
        newtio.c_cflag &= ~CSTOPB;
    }
    else if (2 == stopbits)
    {
        newtio.c_cflag |= CSTOPB;
    }
    //设置最少字符和等待时间,对于接收字符和等待时间没有特别的要求时
    newtio.c_cc[VTIME] = 0; // 0ms timeout
    newtio.c_cc[VMIN]  = 1; // read len depend read's third param

    tcflush(handle->fd, TCIFLUSH);
    if ((tcsetattr(handle->fd, TCSANOW, &newtio)) != 0)
    {
        dzlog_error("com set error");
        return -1;
    }
    return 0;
}

// Close the serial device.
void serial_close(serial_t* handle)
{
    if (!handle)
    {
        return;
    }

    if (handle->thread_id == uv_thread_self())
    {
        close_handle(handle);
    }
    else
    {
        uv_async_send(&(handle->async_close));
    }
}

ssize_t serial_send(serial_t* handle, char* data, size_t size)
{
    if (!handle)
    {
        return -1;
    }

    ssize_t ret = write(handle->fd, data, size);
    if (ret == -1)
        dzlog_warn("Error when writing: %s", strerror(errno));
    return ret;
}