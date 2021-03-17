/******************************************************************************
 * Copyright (c) 2007-2019, ZeroTech Co., Ltd.
 * All rights reserved.
 *******************************************************************************
 * File name     : defines.h
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

#ifndef DEFINES_H_
#define DEFINES_H_

#include <stdlib.h>
#include <string.h>
#include "zlog.h"
#include "uv_wrapper.h"
#include "../../utils/queue.h"

#define MAX_BUF_LEN 4096

#define CALLBACK_FIELDS                     \
    tcp_conn_on_open_func_t on_conn_open;   \
    tcp_conn_on_error_func_t on_conn_error; \
    tcp_conn_on_close_func_t on_conn_close; \
    tcp_conn_on_read_func_t on_read;        \
    tcp_conn_on_write_func_t on_write;

typedef struct tcp_client_s tcp_client_t;
typedef struct tcp_server_s tcp_server_t;
typedef struct tcp_connection_s tcp_connection_t;
typedef struct udp_socket_s udp_socket_t;

/* udp事件回调 */
typedef void (*udp_handle_on_data_func_t)(udp_socket_t* handle, const struct sockaddr* peer, void* data,
                                          size_t len);                                // udp数据读取回调函数
typedef void (*udp_handle_on_send_func_t)(udp_socket_t* handle, int status);          // udp发送数据回调函数
typedef void (*udp_handle_on_error_func_t)(udp_socket_t* handle, const char* errmsg); //错误回调函数

/* tcp事件回调 */
typedef void (*tcp_conn_on_open_func_t)(tcp_connection_t* conn);  // tcp链接创建的回调函数
typedef void (*tcp_conn_on_close_func_t)(tcp_connection_t* conn); // tcp链接关闭的回调函数
typedef void (*tcp_conn_on_read_func_t)(tcp_connection_t* conn, void* data, size_t len); // tcp数据读取回调函数
typedef void (*tcp_conn_on_error_func_t)(tcp_connection_t* conn, const char* errmsg);    //服务错误回调函数
typedef void (*tcp_conn_on_write_func_t)(tcp_connection_t* conn,
                                         int status); //异步发送数据回到函数, status 为0表示发送成功，否则发送失败

typedef enum
{
    true  = 1,
    false = 0
} bool;

struct tcp_connection_s
{
    uv_tcp_t* session;
    void* data;           // 一般指向父结构，例如tcp_client_t或tcp_server_t
    void* data2;          // 一般用于发送数据等其他数据
    bool async_send_flag; // 异步发送标志
    QUEUE node;
};

struct tcp_client_s
{
    uv_loop_t* uvloop;
    uv_async_t async_close; // 关闭客户端事件通知句柄
    uv_async_t async_send;  // 发送数据事件通知句柄
    uv_mutex_t mutex;
    uv_sem_t sem;
    uv_connect_t* connect_req;
    tcp_connection_t conn;
    uv_thread_t thread_id;
    char buf[MAX_BUF_LEN];
    CALLBACK_FIELDS
};

struct tcp_server_s
{
    char name[32]; // tcp服务器名字
    const char* server_addr;
    int port;
    uv_loop_t* uvloop;
    uv_tcp_t server;        // 服务句柄
    uv_async_t async_close; // 关闭客户端事件通知句柄
    uv_async_t async_send;  // 发送数据事件通知句柄
    uv_mutex_t mutex;
    uv_sem_t sem;
    uv_thread_t thread_id;
    int conn_count;  // 当前链接总数
    bool is_closing; // 停止标志
    char buf[MAX_BUF_LEN];
    QUEUE sessions; // queue of sessions
    CALLBACK_FIELDS
};

struct udp_socket_s
{
    uv_loop_t* uvloop;
    uv_udp_t udp;           // udp的uv实例
    uv_async_t async_close; // 关闭客户端事件通知句
    uv_async_t async_send;  // 发送数据事件通知句柄
    uv_mutex_t mutex;
    uv_sem_t sem;
    uv_thread_t thread_id;
    struct sockaddr send_addr;
    void* data1;          //公共数据
    void* data2;          //公共数据
    bool async_send_flag; // 异步发送标志
    char buf[MAX_BUF_LEN];

    //各个回调函数
    udp_handle_on_data_func_t on_data;
    udp_handle_on_send_func_t on_send;
    udp_handle_on_error_func_t on_error;
};

#endif