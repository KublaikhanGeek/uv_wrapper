/******************************************************************************
 * Copyright (c) 2007-2019, ZeroTech Co., Ltd.
 * All rights reserved.
 *******************************************************************************
 * File name     : udp.c
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
#include "udp.h"

static void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff)
{
    if (!handle)
    {
        return;
    }

    udp_socket_t* udp_handle = (udp_socket_t*)handle->data;
    buff->base               = udp_handle->buf;
    buff->len                = MAX_BUF_LEN;
}

static void on_read_cb(uv_udp_t* req, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags)
{
    udp_socket_t* udp_handle = (udp_socket_t*)req->data;

    if (nread > 0)
    {
        if (udp_handle->on_data)
        {
            udp_handle->on_data(udp_handle, addr, buf->base, (size_t)nread);
        }
    }
    else if (nread == 0)
    {
        // dzlog_debug("on_read fire, but recv nothing");
    }
    else
    {
        if (udp_handle->on_error)
        {
            udp_handle->on_error(udp_handle, "Read error!");
        }
    }
}

static void on_send_cb(uv_udp_send_t* req, int status)
{
    dzlog_debug("on_send_cb: %d", status);
    udp_socket_t* udp_handle = (udp_socket_t*)req->data;
    if (udp_handle->on_send)
    {
        udp_handle->on_send(udp_handle, status);
    }
    if (req)
    {
        free(req);
    }

    dzlog_debug("[0x%x] on_send_cb", uv_thread_self());
    if (udp_handle->async_send_flag)
    {
        uv_sem_post(&(udp_handle->sem));
    }
}

static void close(udp_socket_t* udp_handle)
{
    if (!udp_handle)
    {
        return;
    }

    uv_mutex_destroy(&(udp_handle->mutex));
    uv_sem_destroy(&(udp_handle->sem));
    uv_close((uv_handle_t*)&(udp_handle->async_send), NULL);
    uv_close((uv_handle_t*)&(udp_handle->async_close), NULL);

    uv_udp_recv_stop(&udp_handle->udp);
    if (!uv_is_closing((uv_handle_t*)&(udp_handle->udp)))
    {
        uv_close((uv_handle_t*)&udp_handle->udp, NULL);
    }

    free(udp_handle);
}

static void async_close_cb(uv_async_t* handle)
{
    udp_socket_t* udp_handle = handle->data;
    if (!udp_handle)
    {
        return;
    }

    close(udp_handle);
}

static void send_data(udp_socket_t* handle)
{
    uv_udp_send_t* send_req = (uv_udp_send_t*)malloc(sizeof(uv_udp_send_t));
    if (!send_req)
    {
        if (handle->on_error)
        {
            handle->on_error(handle, "allocate memory failed");
        }
        dzlog_error("allocate memory failed");
        if (handle->async_send_flag)
        {
            uv_sem_post(&(handle->sem));
        }
        return;
    }

    send_req->data = handle;
    uv_udp_send(send_req, &(handle->udp), handle->data1, 1, handle->data2, on_send_cb);
}

static void async_send_cb(uv_async_t* handle)
{
    udp_socket_t* udp_handle = (udp_socket_t*)handle->data;
    send_data(udp_handle);
}

/*
 功能： 创建udp服务端实例
 参数：
 *  server_addr: 服务监听的地址
 *  server_port：服务监听的端口
 *  mode: 0 生产模式 1 开发模式(在此模式下回打印吞吐量的并发量
 *  on_data: 接收数据的回调函数
 *  on_error: 出错回调函数
 *  on_send: 异步发送数据完成后的回调函数
 返回结果：udp服务端实例
 */
udp_socket_t* udp_handle_create(const char* addr, int port, uv_loop_t* loop, udp_handle_on_data_func_t on_data,
                                udp_handle_on_send_func_t on_send, udp_handle_on_error_func_t on_error)
{
    if (!loop)
    {
        return NULL;
    }

    int r                    = 0;
    udp_socket_t* udp_handle = (udp_socket_t*)calloc(1, sizeof(udp_socket_t));
    udp_handle->on_data      = on_data;
    udp_handle->on_send      = on_send;
    udp_handle->on_error     = on_error;
    udp_handle->uvloop       = loop;

    uv_mutex_init(&(udp_handle->mutex));
    uv_sem_init(&(udp_handle->sem), 0);
    uv_udp_init(udp_handle->uvloop, &(udp_handle->udp));
    uv_async_init(udp_handle->uvloop, &(udp_handle->async_close), async_close_cb);
    uv_async_init(udp_handle->uvloop, &(udp_handle->async_send), async_send_cb);
    udp_handle->async_close.data = udp_handle;
    udp_handle->async_send.data  = udp_handle;
    udp_handle->udp.data         = udp_handle;
    udp_handle->thread_id        = uv_thread_self();

    if (addr != NULL)
    {
        struct sockaddr bind_addr = { 0 };
        if (strchr(addr, ':'))
        {
            uv_ip6_addr(addr, port, (struct sockaddr_in6*)&bind_addr);
        }
        else
        {
            uv_ip4_addr(addr, port, (struct sockaddr_in*)&bind_addr);
        }
        uv_udp_bind(&(udp_handle->udp), (const struct sockaddr*)&bind_addr, 0);
    }

    r = uv_udp_recv_start(&(udp_handle->udp), alloc_buffer, on_read_cb);
    if (r)
    {
        dzlog_error("start udp failed:%d", r);
        return NULL;
    }

    return udp_handle;
}

/*
 功能： 发送数据到特定地址，地址用struct sockaddr 表示
 参数：
 *  udp_handle: udp实例
 *  data: 数据指针
 *  size：数据大小
 *  addr：目标地址
 返回结果：无
 说明：此函数调用会出发on_send回调函数调用
 */
void udp_send_data(udp_socket_t* udp_handle, char* data, size_t size, const struct sockaddr* addr)
{
    if (!udp_handle || !data)
    {
        return;
    }

    if (udp_handle->thread_id == uv_thread_self())
    {
        dzlog_debug("[0x%x] udp_send_data sync", uv_thread_self());
        udp_handle->async_send_flag = false;
        uv_buf_t buf                = uv_buf_init(data, (unsigned int)size);
        udp_handle->data1           = &buf;
        udp_handle->data2           = addr;
        send_data(udp_handle);
    }
    else
    {
        uv_mutex_lock(&(udp_handle->mutex));
        dzlog_debug("[0x%x] udp_send_data async start", uv_thread_self());
        udp_handle->async_send_flag = true;
        uv_buf_t buf                = uv_buf_init(data, (unsigned int)size);
        udp_handle->data1           = &buf;
        udp_handle->data2           = addr;
        udp_handle->async_send.data = udp_handle;
        uv_async_send(&(udp_handle->async_send));
        uv_sem_wait(&(udp_handle->sem));
        dzlog_debug("[0x%x] udp_send_data async end", uv_thread_self());
        uv_mutex_unlock(&(udp_handle->mutex));
    }
}

/*
 功能： 发送数据到特定ip和端口
 参数：
 *  udp_handle: udp实例
 *  data: 数据指针
 *  size：数据大小
 *  ip：目标地址
 *  port: 目标端口
 返回结果：无
 说明：此函数调用会出发on_send回调函数调用
 */
void udp_send_data_ip(udp_socket_t* udp_handle, char* data, size_t size, const char* ip, unsigned short port)
{
    if (!udp_handle)
    {
        return;
    }

    if (strchr(ip, ':'))
    {
        uv_ip6_addr(ip, port, (struct sockaddr_in6*)&(udp_handle->send_addr));
    }
    else
    {
        uv_ip4_addr(ip, port, (struct sockaddr_in*)&(udp_handle->send_addr));
    }
    udp_send_data(udp_handle, data, size, &(udp_handle->send_addr));
}

/*
 功能： 关闭并释放udp实例
 参数：
 *  udp_handle: udp实例
 返回结果：无
 */
void udp_handle_close(udp_socket_t* udp_handle)
{
    if (!udp_handle)
    {
        return;
    }

    if (udp_handle->thread_id == uv_thread_self())
    {
        close(udp_handle);
    }
    else
    {
        uv_async_send(&(udp_handle->async_close));
    }
}

/*
 功能： 设置udp实例的广播开关
 参数：
 *  udp_handle: udp实例
 *  on：1 on 0 off
 返回结果：0为成功 -1失败
 */
int udp_set_broadcast(udp_socket_t* udp_handle, bool on)
{
    return uv_udp_set_broadcast(&(udp_handle->udp), on);
}

//加入组播
int udp_set_membership(udp_socket_t* udp_handle, const char* multicast_addr, const char* interface_addr, bool on)
{
    return uv_udp_set_membership(&(udp_handle->udp), multicast_addr, interface_addr, on);
}
//设置组播回环标志
int udp_set_multicast_loop(udp_socket_t* udp_handle, bool on)
{
    return uv_udp_set_multicast_loop(&(udp_handle->udp), on);
}
//设置组播TTL
int udp_set_multicast_ttl(udp_socket_t* udp_handle, int ttl)
{
    return uv_udp_set_multicast_ttl(&(udp_handle->udp), ttl);
}
//设置组播发送和接收数据接口
int udp_set_multicast_interface(udp_socket_t* udp_handle, const char* interface_addr)
{
    return uv_udp_set_multicast_interface(&(udp_handle->udp), interface_addr);
}