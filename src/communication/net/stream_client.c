/******************************************************************************
 * Copyright (c) 2007-2019, ZeroTech Co., Ltd.
 * All rights reserved.
 *******************************************************************************
 * File name     : stream_server.h
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

#include "stream_client.h"

/*
 功能： 工具函数，用于接收数据时分配存储数据的内存的回调函数
 参数：
 *  handle: libuv的句柄，一般情况不适用
 *  suggested_size：建议分配的内存大小
 *  buff：返回结果
 返回结果：无
 */
static void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff)
{
    tcp_client_t* tcp_client = (tcp_client_t*)handle->data;
    buff->base               = tcp_client->buf;
    buff->len                = MAX_BUF_LEN;
}

static void on_read(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
{
    tcp_client_t* tcp_client = (tcp_client_t*)handle->data;
    if (nread > 0)
    {
        if (tcp_client->on_read)
        {
            tcp_client->on_read(&(tcp_client->conn), buf->base, (size_t)nread);
        }
    }
    else if (nread == 0)
    {
        // nothing recv
    }
    else
    {
        dzlog_error("read data failed");
        uv_read_stop(handle);
        if (tcp_client->on_conn_close)
        {
            tcp_client->on_conn_close(&(tcp_client->conn));
        }
        uv_close((uv_handle_t*)handle, NULL);
        free(tcp_client->connect_req);
        free(tcp_client->conn.session);
        free(tcp_client);
    }
}

static void on_connect(uv_connect_t* req, int status)
{
    tcp_client_t* tcp_client = (tcp_client_t*)req->data;
    if (status == -1)
    {
        dzlog_error("error on_write_end");
        if (tcp_client->on_conn_error)
        {
            tcp_client->on_conn_error(&(tcp_client->conn), "error connect to remote");
        }
    }
    else
    {
        if (tcp_client->on_conn_open)
        {
            tcp_client->on_conn_open(&(tcp_client->conn));
        }
    }
    req->handle->data = tcp_client;
    uv_read_start(req->handle, alloc_buffer, on_read);
}

static void on_close_connection(uv_handle_t* handle)
{
    tcp_client_t* tcp_client = (tcp_client_t*)handle->data;
    if (tcp_client->on_conn_close)
    {
        tcp_client->on_conn_close(&(tcp_client->conn));
    }

    free(tcp_client->connect_req);
    free(tcp_client->conn.session);
    free(tcp_client);
}

static void on_client_write(uv_write_t* req, int status)
{
    if (status < 0)
    {
        dzlog_error("Write error!");
    }

    tcp_client_t* tcp_client = (tcp_client_t*)req->data;
    if (tcp_client->on_write) //客户端回调函数调用
    {
        tcp_client->on_write(&(tcp_client->conn), status);
    }

    if (req)
    {
        free(req);
    }
}

/*******************************************************************************
 * function name : tcp_client_run
 * description	 : start a tcp client
 * param[in]  	 :
 * param[out] 	 : none
 * return 		 : tcp_client_t handle
 *******************************************************************************/
tcp_client_t* tcp_client_run(const char* server_addr, int server_port, uv_loop_t* loop, tcp_conn_on_open_func_t on_open,
                             tcp_conn_on_read_func_t on_data, tcp_conn_on_close_func_t on_close,
                             tcp_conn_on_error_func_t on_error, tcp_conn_on_write_func_t on_send)
{
    tcp_client_t* tcp_client = (tcp_client_t*)calloc(1, sizeof(tcp_client_t));
    if (tcp_client == NULL)
    {
        return NULL;
    }

    tcp_client->uvloop       = loop;
    tcp_client->conn.session = (uv_tcp_t*)calloc(1, sizeof(uv_tcp_t));
    if (tcp_client->conn.session == NULL)
    {
        free(tcp_client);
        return NULL;
    }
    tcp_client->connect_req = (uv_connect_t*)calloc(1, sizeof(uv_connect_t));
    if (tcp_client->connect_req == NULL)
    {
        free(tcp_client->conn.session);
        free(tcp_client);
        return NULL;
    }
    tcp_client->on_conn_open  = on_open;
    tcp_client->on_read       = on_data;
    tcp_client->on_conn_close = on_close;
    tcp_client->on_conn_error = on_error;
    tcp_client->on_write      = on_send;
    tcp_client->conn.data     = tcp_client;

    uv_tcp_init(tcp_client->uvloop, tcp_client->conn.session);
    struct sockaddr send_addr = { 0 };
    if (strchr(server_addr, ':'))
    {
        uv_ip6_addr(server_addr, server_port, (struct sockaddr_in6*)&send_addr);
    }
    else
    {
        uv_ip4_addr(server_addr, server_port, (struct sockaddr_in*)&send_addr);
    }
    tcp_client->connect_req->data = tcp_client;
    uv_tcp_connect(tcp_client->connect_req, tcp_client->conn.session, (const struct sockaddr*)&send_addr, on_connect);
    return tcp_client;
}

/*******************************************************************************
 * function name : tcp_client_send_data
 * description	 : send data
 * param[in]     : tcp_client; data; size
 * param[out] 	 : none
 * return 		 : none
 *******************************************************************************/
void tcp_client_send_data(tcp_client_t* tcp_client, char* data, size_t size)
{
    uv_write_t* write_req = (uv_write_t*)malloc(sizeof(uv_write_t));
    if (write_req == NULL)
    {
        if (tcp_client->on_conn_error)
        {
            tcp_client->on_conn_error(&(tcp_client->conn), "allocate memory failed");
        }
        dzlog_error("allocate  memory failed");
        return;
    }

    uv_buf_t buf    = uv_buf_init(data, (unsigned int)size);
    write_req->data = tcp_client;
    uv_write(write_req, (uv_stream_t*)tcp_client->conn.session, &buf, 1, on_client_write);
}

/*******************************************************************************
 * function name : tcp_client_close
 * description	 : close tcp connection
 * param[in]     : tcp_client
 * param[out] 	 : none
 * return 		 : none
 *******************************************************************************/
void tcp_client_close(tcp_client_t* tcp_client)
{
    if (tcp_client && tcp_client->conn.session)
    {
        uv_read_stop((uv_stream_t*)tcp_client->conn.session);
        uv_close((uv_handle_t*)tcp_client->conn.session, on_close_connection);
    }
}