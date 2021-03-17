/******************************************************************************
 * Copyright (c) 2007-2019, ZeroTech Co., Ltd.
 * All rights reserved.
 *******************************************************************************
 * File name     : tcp_server.c
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
#include "tcp_server.h"

static void on_new_connection(uv_stream_t* server, int status);
static void on_conn_read(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf);
static void on_conn_write(uv_write_t* req, int status);
static void on_close_connection(uv_handle_t* handle);
static void on_close_server(uv_handle_t* handle);
static void async_close_cb(uv_async_t* handle);
static void async_send_cb(uv_async_t* handle);
static void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff);
static void tcp_server_conn_close(tcp_connection_t* conn);
static void close(tcp_server_t* tcp_server);

static void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff)
{
    tcp_connection_t* conn   = (tcp_connection_t*)handle->data;
    tcp_server_t* tcp_server = (tcp_server_t*)conn->data;
    buff->base               = tcp_server->buf;
    buff->len                = MAX_BUF_LEN;
}

static void on_conn_write(uv_write_t* req, int status)
{
    tcp_connection_t* conn   = (tcp_connection_t*)req->data;
    tcp_server_t* tcp_server = (tcp_server_t*)conn->data;
    if (status < 0)
    {
        dzlog_error("Write error!");
    }
    if (req)
    {
        free(req);
    }

    dzlog_debug("[0x%x] on_conn_write", uv_thread_self());
    if (conn->async_send_flag)
    {
        uv_sem_post(&(tcp_server->sem));
    }
}

static void on_close_connection(uv_handle_t* handle)
{
    tcp_connection_t* conn   = (tcp_connection_t*)handle->data;
    tcp_server_t* tcp_server = (tcp_server_t*)conn->data;
    if (tcp_server->on_conn_close)
    {
        tcp_server->on_conn_close(conn);
    }
    QUEUE_REMOVE(&(conn->node));
    tcp_server->conn_count--;

    if (handle)
    {
        free(handle);
    }

    if (conn)
    {
        free(conn);
    }

    dzlog_debug("tcp conn count: %d", tcp_server->conn_count);
    if ((tcp_server->is_closing == true) && QUEUE_EMPTY(&(tcp_server->sessions)))
    {
        dzlog_debug("free tcp_server");
        free(tcp_server);
    }
}

static void on_conn_read(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf)
{
    tcp_connection_t* conn   = (tcp_connection_t*)client->data;
    tcp_server_t* tcp_server = (tcp_server_t*)conn->data;
    if (nread > 0)
    {
        if (tcp_server->on_read)
        {
            tcp_server->on_read(conn, buf->base, (size_t)nread);
        }
    }
    else if (nread == 0)
    {
        // nothing recv
    }
    else
    {
        if (tcp_server->on_conn_error)
        {
            tcp_server->on_conn_error(conn, "error on_conn_read");
        }
        tcp_server_conn_close(conn);
    }
}

static void on_close_server(uv_handle_t* handle)
{
    dzlog_info("on_close_server");
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

    int result       = uv_accept(server, (uv_stream_t*)client);
    bool conn_status = false;
    if (result == 0)
    {
        tcp_connection_t* conn = calloc(1, sizeof(tcp_connection_t));
        if (conn != NULL)
        {
            conn_status = true;

            conn->data          = tcp_server;
            conn->session       = client;
            conn->session->data = conn;
            QUEUE_INIT(&(conn->node));
            QUEUE_INSERT_TAIL(&(tcp_server->sessions), &(conn->node));
            tcp_server->conn_count++; //服务连接数
            if (tcp_server->on_conn_open)
            {
                tcp_server->on_conn_open(conn);
            }
            uv_read_start((uv_stream_t*)client, alloc_buffer, on_conn_read);
        }
    }

    if (!conn_status)
    {
        dzlog_error("accept a new connection error");
        if (tcp_server->on_conn_error)
        {
            tcp_server->on_conn_error(NULL, "accept connction failed.");
        }

        if (!uv_is_closing((uv_handle_t*)client))
        {
            uv_close((uv_handle_t*)client, NULL);
        }

        if (client)
        {
            free(client);
        }
    }
}

/*******************************************************************************
 * function name : tcp_server_conn_close
 * description	 : close a tcp connection
 * param[in]     : conn
 * param[out] 	 : none
 * return 		 : none
 *******************************************************************************/
static void tcp_server_conn_close(tcp_connection_t* conn)
{
    if (conn)
    {
        uv_read_stop((uv_stream_t*)conn->session);
        if (!uv_is_closing((uv_handle_t*)conn->session))
        {
            char addr[128] = { 0 };
            dzlog_info("tcp server close connection : %s", tcp_server_get_client_addr(conn, addr, 127));
            uv_close((uv_handle_t*)conn->session, on_close_connection);
        }
    }
}

static void close(tcp_server_t* tcp_server)
{
    if (!tcp_server)
    {
        return;
    }

    dzlog_debug("tcp_server_close");

    if (!uv_is_closing((uv_handle_t*)&(tcp_server->server)))
    {
        uv_close((uv_handle_t*)(&(tcp_server->server)), on_close_server);
    }

    uv_mutex_destroy(&(tcp_server->mutex));
    uv_sem_destroy(&(tcp_server->sem));
    uv_close((uv_handle_t*)&(tcp_server->async_send), NULL);
    uv_close((uv_handle_t*)&(tcp_server->async_close), NULL);

    if (QUEUE_EMPTY(&(tcp_server->sessions)))
    {
        dzlog_debug("free tcp_server");
        free(tcp_server);
        return;
    }

    QUEUE* q = QUEUE_HEAD(&(tcp_server->sessions));
    QUEUE_FOREACH(q, &(tcp_server->sessions))
    {
        dzlog_debug("tcp_server_close connection close");
        tcp_connection_t* data = QUEUE_DATA(q, tcp_connection_t, node);
        tcp_server_conn_close(data);
    }
}

static void async_close_cb(uv_async_t* handle)
{
    tcp_server_t* tcp_server = handle->data;
    if (!tcp_server)
    {
        return;
    }
    close(tcp_server);
}

static void send_data(tcp_connection_t* conn)
{
    tcp_server_t* tcp_server = (tcp_server_t*)conn->data;
    uv_write_t* write_req    = (uv_write_t*)malloc(sizeof(uv_write_t));
    if (write_req == NULL)
    {
        if (tcp_server->on_conn_error)
        {
            tcp_server->on_conn_error(conn, "allocate memory failed");
        }
        dzlog_error("allocate  memory failed");
        if (conn->async_send_flag)
        {
            uv_sem_post(&(tcp_server->sem));
        }
        return;
    }

    write_req->data = conn;
    uv_write(write_req, (uv_stream_t*)conn->session, conn->data2, 1, on_conn_write);
}

static void async_send_cb(uv_async_t* handle)
{
    tcp_connection_t* conn = (tcp_connection_t*)handle->data;
    send_data(conn);
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
    if (!loop)
    {
        dzlog_error("tcp_server_run error: loop is NULL");
        return NULL;
    }

    tcp_server_t* tcp_server = (tcp_server_t*)calloc(1, sizeof(tcp_server_t));
    if (tcp_server)
    {
        memset(tcp_server, 0, sizeof(tcp_server_t));
    }
    else
    {
        dzlog_error("tcp_server_run error: tcp_server malloc failed");
        return NULL;
    }

    uv_sem_init(&(tcp_server->sem), 0);
    uv_mutex_init(&(tcp_server->mutex));
    uv_async_init(loop, &(tcp_server->async_close), async_close_cb);
    uv_async_init(loop, &(tcp_server->async_send), async_send_cb);
    tcp_server->async_close.data = tcp_server;
    tcp_server->async_send.data  = tcp_server;
    tcp_server->on_conn_open     = on_open;
    tcp_server->on_conn_close    = on_close;
    tcp_server->on_conn_error    = on_error;
    tcp_server->on_read          = on_data;
    tcp_server->on_write         = on_send;
    tcp_server->uvloop           = loop;
    tcp_server->is_closing       = false;
    tcp_server->thread_id        = uv_thread_self();
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

    int ret = uv_tcp_bind(server, (const struct sockaddr*)&bind_addr, 0);
    if (ret)
    {
        dzlog_error("tcp server bind %d failed:%d\n", server_port, ret);
        tcp_server_close(tcp_server);
        return NULL;
    }

    server->data = tcp_server;
    ret          = uv_listen((uv_stream_t*)server, 5, on_new_connection);
    if (ret)
    {
        dzlog_error("listen tcp server failed:%d\n", ret);
        tcp_server_close(tcp_server);
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
    if (!tcp_server)
    {
        return;
    }

    tcp_server->is_closing = true;
    if (tcp_server->thread_id == uv_thread_self())
    {
        close(tcp_server);
    }
    else
    {
        uv_async_send(&(tcp_server->async_close));
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
    if (!tcp_server)
    {
        return;
    }

    QUEUE* q = QUEUE_HEAD(&(tcp_server->sessions));
    QUEUE_FOREACH(q, &(tcp_server->sessions))
    {
        char addr[128]         = { 0 };
        tcp_connection_t* data = QUEUE_DATA(q, tcp_connection_t, node);
        dzlog_info("connections peer address: %s", tcp_server_get_client_addr(data, addr, 127));
    }
}

void tcp_server_send_data_2_all(tcp_server_t* tcp_server, char* data, size_t size)
{
    if (!tcp_server)
    {
        return;
    }

    QUEUE* q = QUEUE_HEAD(&(tcp_server->sessions));
    QUEUE_FOREACH(q, &(tcp_server->sessions))
    {
        tcp_connection_t* conn = QUEUE_DATA(q, tcp_connection_t, node);
        tcp_server_send_data(conn, data, size);
    }
}

/*******************************************************************************
 * function name : tcp_server_send_data
 * description	 : send data
 * param[in]     : conn, data, size
 * param[out] 	 : none
 * return 		 : none
 *******************************************************************************/
void tcp_server_send_data(tcp_connection_t* conn, char* data, size_t size)
{
    if (!conn || !data)
    {
        return;
    }

    tcp_server_t* tcp_server = (tcp_server_t*)conn->data;
    if (tcp_server->thread_id == uv_thread_self())
    {
        dzlog_debug("[0x%x] send data sync", uv_thread_self());
        conn->async_send_flag = false;
        uv_buf_t buf          = uv_buf_init(data, (unsigned int)size);
        conn->data2           = &buf;
        send_data(conn);
    }
    else
    {
        uv_mutex_lock(&(tcp_server->mutex));
        dzlog_debug("[0x%x] send data async start", uv_thread_self());
        conn->async_send_flag       = true;
        uv_buf_t buf                = uv_buf_init(data, (unsigned int)size);
        conn->data2                 = &buf;
        tcp_server->async_send.data = conn;
        uv_async_send(&(tcp_server->async_send));
        uv_sem_wait(&(tcp_server->sem));
        dzlog_debug("[0x%x] send data async end", uv_thread_self());
        uv_mutex_unlock(&(tcp_server->mutex));
    }
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
    if (!client || !dst)
    {
        return NULL;
    }

    struct sockaddr_storage peername;
    int namelen = sizeof(peername);
    int r       = uv_tcp_getpeername((uv_tcp_t*)(client->session), (struct sockaddr*)&peername, &namelen);
    if (r == 0)
    {
        if (peername.ss_family == AF_INET)
        {
            uv_ip4_name((const struct sockaddr_in*)&peername, dst, len);
            sprintf(dst, "%s:%d", dst, ntohs(((const struct sockaddr_in*)&peername)->sin_port));
        }
        else
        {
            uv_ip6_name((const struct sockaddr_in6*)&peername, dst, len);
            sprintf(dst, "%s:%d", dst, ntohs(((const struct sockaddr_in6*)&peername)->sin6_port));
        }
    }
    return dst;
}

void tcp_server_set_no_delay(tcp_server_t* tcp_server, bool enable)
{
    if (!tcp_server)
    {
        return;
    }

    int ret = uv_tcp_nodelay(&(tcp_server->server), enable);
    if (ret)
    {
        dzlog_error("tcp_server_set_no_delay failed!");
    }
}

void tcp_server_set_keepalive(tcp_server_t* tcp_server, bool enable, unsigned int delay)
{
    if (!tcp_server)
    {
        return;
    }

    int ret = uv_tcp_keepalive(&(tcp_server->server), enable, delay);
    if (ret)
    {
        dzlog_error("tcp_server_set_keepalive failed!");
    }
}