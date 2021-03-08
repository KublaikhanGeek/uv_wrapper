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
typedef struct tcp_send_data_cb_s tcp_send_data_cb_t;

/* 服务端事件回调 */
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
    void* data;
    QUEUE node;
};

struct tcp_client_s
{
    uv_loop_t* uvloop;
    uv_async_t async; // 事件通知句柄
    uv_connect_t* connect_req;
    tcp_connection_t conn;
    char buf[MAX_BUF_LEN];
    CALLBACK_FIELDS
};

struct tcp_server_s
{
    char name[32]; // tcp服务器名字
    const char* server_addr;
    int port;
    uv_loop_t* uvloop;
    uv_tcp_t server;  // 服务句柄
    uv_async_t async; // 事件通知句柄
    int conn_count;   // 当前链接总数
    bool is_closing;  // 停止标志
    char buf[MAX_BUF_LEN];
    QUEUE sessions; // queue of sessions
    CALLBACK_FIELDS
};

#endif