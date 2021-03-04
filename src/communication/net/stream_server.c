/******************************************************************************
 * Copyright (c) 2007-2019, ZeroTech Co., Ltd.
 * All rights reserved.
 *******************************************************************************
 * File name     : stream_server.c
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
#include "stream_server.h"

static void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff)
{
    tcp_connection_t* tcp_client = (tcp_connection_t*)handle->data;
    tcp_server_t* tcp_server     = (tcp_server_t*)tcp_client->data;
    buff->base                   = tcp_server->buf;
    buff->len                    = MAX_BUF_LEN;
}

static void on_client_write(uv_write_t* req, int status)
{
    if (status < 0)
    {
        dzlog_error("Write error!");
    }
    free(req);
}

static void on_close_connection(uv_handle_t* handle)
{
    tcp_connection_t* tcp_client = (tcp_connection_t*)handle->data;
    tcp_server_t* tcp_server     = (tcp_server_t*)tcp_client->data;
    if (tcp_server->on_conn_close)
    {
        tcp_server->on_conn_close(tcp_client);
    }
    QUEUE_REMOVE(&(tcp_client->node));
    tcp_server->conn_count--;

    if (tcp_client)
    {
        free(tcp_client);
    }
    if (handle)
    {
        free(handle);
    }
}

static void on_client_read(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf)
{
    tcp_connection_t* tcp_client = (tcp_connection_t*)client->data;
    tcp_server_t* tcp_server     = (tcp_server_t*)tcp_client->data;
    if (nread > 0)
    {
        if (tcp_server->on_read)
        {
            tcp_server->on_read(tcp_client, buf->base, (size_t)nread);
        }
    }
    else if (nread == 0)
    {
        // nothing recv
    }
    else
    {
        uv_read_stop(client);
        if (tcp_server->on_conn_error)
        {
            tcp_server->on_conn_error(tcp_client, "error on_client_read");
        }
        uv_close((uv_handle_t*)client, on_close_connection);
    }
}

static void on_close_server(uv_handle_t* handle)
{
    tcp_server_t* tcp_server = (tcp_server_t*)handle->data;
    if (tcp_server)
    {
        free(tcp_server);
    }
}

static void on_new_connection(uv_stream_t* server, int status)
{
    tcp_server_t* tcp_server = (tcp_server_t*)server->data;
    if (status < 0)
    {
        dzlog_error("error on new connection");
        if (tcp_server->on_conn_error)
        {
            tcp_server->on_conn_error(NULL, "error on_new_connection");
        }
        return;
    }

    uv_tcp_t* client = (uv_tcp_t*)calloc(1, sizeof(uv_tcp_t));
    if (client == NULL)
    {
        dzlog_error("malloc client error while establishing a new connection");
        if (tcp_server->on_conn_error)
        {
            tcp_server->on_conn_error(NULL, "error on_new_connection");
        }
        return;
    }

    uv_tcp_init(tcp_server->uvloop, client);

    int result = uv_accept(server, (uv_stream_t*)client);
    bool conn  = false;
    if (result == 0)
    {
        tcp_connection_t* tcp_client = calloc(1, sizeof(tcp_connection_t));
        if (tcp_client != NULL)
        {
            conn = true;
        }

        tcp_client->data    = tcp_server;
        tcp_client->session = client;
        QUEUE_INIT(&(tcp_client->node));
        QUEUE_INSERT_TAIL(&(tcp_server->sessions), &(tcp_client->node));
        client->data = tcp_client;
        tcp_server->conn_count++; //服务连接数
        if (tcp_server->on_conn_open)
        {
            tcp_server->on_conn_open(tcp_client);
        }
        uv_read_start((uv_stream_t*)client, alloc_buffer, on_client_read);
    }

    if (!conn)
    {
        dzlog_error("accept a new connection error");
        if (tcp_server->on_conn_error)
        {
            tcp_server->on_conn_error(NULL, "accept connction failed.");
        }
        uv_close((uv_handle_t*)client, NULL);
        if (client)
        {
            free(client);
        }
    }
}

/*******************************************************************************
 * function name : tcp_server_run
 * description	 : start a tcp server
 * param[in]     :
 * param[out] 	 : none
 * return 		 : tcp_server_t handle
 *******************************************************************************/
tcp_server_t* tcp_server_run(const char* server_addr, int server_port, uv_loop_t* loop, tcp_conn_on_open_func_t on_open,
                             tcp_conn_on_close_func_t on_close, tcp_conn_on_error_func_t on_error,
                             tcp_conn_on_read_func_t on_data, tcp_conn_on_write_func_t on_send)
{
    int r                    = -1;
    tcp_server_t* tcp_server = (tcp_server_t*)calloc(1, sizeof(tcp_server_t));
    if (tcp_server)
    {
        memset(tcp_server, 0, sizeof(tcp_server_t));
    }
    else
    {
        dzlog_error("tcp_server_create error: tcp_server malloc error");
        return NULL;
    }

    tcp_server->on_conn_open  = on_open;
    tcp_server->on_conn_close = on_close;
    tcp_server->on_conn_error = on_error;
    tcp_server->on_read       = on_data;
    tcp_server->on_write      = on_send;
    tcp_server->uvloop        = loop;
    QUEUE_INIT(&(tcp_server->sessions));

    uv_tcp_t* server = &(tcp_server->server);
    uv_tcp_init(loop, server);
    struct sockaddr bind_addr = { 0 };
    if (strchr(server_addr, ':'))
    {
        dzlog_debug("server address is ipv6");
        uv_ip6_addr(server_addr, server_port, (struct sockaddr_in6*)&bind_addr);
    }
    else
    {
        dzlog_debug("server address is ipv4");
        uv_ip4_addr(server_addr, server_port, (struct sockaddr_in*)&bind_addr);
    }

    uv_tcp_bind(server, (const struct sockaddr*)&bind_addr, 0);
    server->data = tcp_server;
    r            = uv_listen((uv_stream_t*)server, 5, on_new_connection);
    if (r)
    {
        dzlog_error("listen tcp server failed:%d\n", r);
        free(tcp_server);
        return NULL;
    }

    return tcp_server;
}

/*******************************************************************************
 * function name : tcp_server_close
 * description	 : close a tcp server
 * param[in]     : tcp_server
 * param[out] 	 : none
 * return 		 : none
 *******************************************************************************/
void tcp_server_close(tcp_server_t* tcp_server)
{
    QUEUE* q = QUEUE_HEAD(&(tcp_server->sessions));
    QUEUE_FOREACH(q, &(tcp_server->sessions))
    {
        tcp_connection_t* data = QUEUE_DATA(q, tcp_connection_t, node);
        tcp_server_conn_close(data);
    }
    uv_close((uv_handle_t*)(&(tcp_server->server)), on_close_server);
}

/*******************************************************************************
 * function name : tcp_server_conn_close
 * description	 : close a tcp connection
 * param[in]     : conn
 * param[out] 	 : none
 * return 		 : none
 *******************************************************************************/
void tcp_server_conn_close(tcp_connection_t* conn)
{
    if (conn)
    {
        uv_read_stop((uv_stream_t*)conn->session);
        uv_close((uv_handle_t*)conn->session, on_close_connection);
    }
}

/*******************************************************************************
 * function name : tcp_server_print_all_conn
 * description	 : print all tcp connection
 * param[in]     : tcp_server
 * param[out] 	 : none
 * return 		 : none
 *******************************************************************************/
void tcp_server_print_all_conn(tcp_server_t* tcp_server)
{
    QUEUE* q = QUEUE_HEAD(&(tcp_server->sessions));
    QUEUE_FOREACH(q, &(tcp_server->sessions))
    {
        char addr[128]         = { 0 };
        tcp_connection_t* data = QUEUE_DATA(q, tcp_connection_t, node);
        dzlog_info("connections peer address: %s", tcp_server_get_client_addr(data, addr, 127));
    }
}

/*******************************************************************************
 * function name : tcp_server_send_data
 * description	 : send data
 * param[in]     : tcp_client, data, size
 * param[out] 	 : none
 * return 		 : none
 *******************************************************************************/
void tcp_server_send_data(tcp_connection_t* tcp_client, char* data, size_t size)
{
    tcp_server_t* tcp_server = (tcp_server_t*)tcp_client->data;
    uv_write_t* write_req    = (uv_write_t*)malloc(sizeof(uv_write_t));
    if (write_req == NULL)
    {
        if (tcp_server->on_conn_error)
        {
            tcp_server->on_conn_error(tcp_client, "allocate memory failed");
        }
        dzlog_error("allocate  memory failed");
        return;
    }

    uv_buf_t buf = uv_buf_init(data, (unsigned int)size);
    uv_write(write_req, (uv_stream_t*)tcp_client->session, &buf, 1, on_client_write);
}

/*******************************************************************************
 * function name : tcp_server_get_client_addr
 * description	 : get tcp client address
 * param[in]     : client
 * param[out] 	 : dst, len
 * return 		 : dst
 *******************************************************************************/
char* tcp_server_get_client_addr(tcp_connection_t* client, char* dst, size_t len)
{
    struct sockaddr_storage peername;
    int namelen = sizeof(peername);
    int r       = uv_tcp_getpeername((uv_tcp_t*)(client->session), (struct sockaddr*)&peername, &namelen);
    if (r == 0)
    {
        if (peername.ss_family == AF_INET)
        {
            uv_ip4_name((const struct sockaddr_in*)&peername, dst, len);
        }
        else
        {
            uv_ip6_name((const struct sockaddr_in6*)&peername, dst, len);
        }
    }
    return dst;
}
