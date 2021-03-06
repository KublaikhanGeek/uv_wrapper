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

    dzlog_debug("[0x%lx] on_send_cb", uv_thread_self());
    if (udp_handle->async_send_flag)
    {
        uv_sem_post(&(udp_handle->sem));
    }
}

static void close_cb(uv_handle_t* handle)
{
    if (handle)
    {
        free(handle);
    }
}

static void close_handle(udp_socket_t* udp_handle)
{
    if (!udp_handle)
    {
        return;
    }

    uv_mutex_destroy(&(udp_handle->mutex));
    uv_sem_destroy(&(udp_handle->sem));
    uv_close((uv_handle_t*)(udp_handle->async_send), close_cb);
    uv_close((uv_handle_t*)(udp_handle->async_close), close_cb);

    uv_udp_recv_stop(udp_handle->udp);
    if (!uv_is_closing((uv_handle_t*)(udp_handle->udp)))
    {
        uv_close((uv_handle_t*)udp_handle->udp, close_cb);
    }

    dzlog_debug("free udp handle");
    free(udp_handle);
}

static void async_close_cb(uv_async_t* handle)
{
    udp_socket_t* udp_handle = handle->data;
    if (!udp_handle)
    {
        return;
    }

    close_handle(udp_handle);
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
    uv_udp_send(send_req, handle->udp, handle->data1, 1, handle->data2, on_send_cb);
}

static void async_send_cb(uv_async_t* handle)
{
    udp_socket_t* udp_handle = (udp_socket_t*)handle->data;
    send_data(udp_handle);
}

/*******************************************************************************
 * function name : udp_handle_run
 * description	 :
 * param[in]     :
 * param[out] 	 :
 * return 		 : udp_socket_t*
 *******************************************************************************/
udp_socket_t* udp_handle_run(const char* addr, int port, uv_loop_t* loop, udp_handle_on_data_func_t on_data,
                             udp_handle_on_send_func_t on_send, udp_handle_on_error_func_t on_error)
{
    if (!loop)
    {
        return NULL;
    }

    int r                    = 0;
    udp_socket_t* udp_handle = (udp_socket_t*)calloc(1, sizeof(udp_socket_t));
    if (udp_handle)
    {
        memset(udp_handle, 0, sizeof(udp_socket_t));
    }
    else
    {
        dzlog_error("udp_handle_run error: udp_handle malloc failed");
        return NULL;
    }

    udp_handle->udp = (uv_udp_t*)calloc(1, sizeof(uv_udp_t));
    if (udp_handle->udp == NULL)
    {
        free(udp_handle);
        return NULL;
    }

    udp_handle->async_send = (uv_async_t*)calloc(1, sizeof(uv_async_t));
    if (udp_handle->async_send == NULL)
    {
        free(udp_handle->udp);
        free(udp_handle);
        return NULL;
    }

    udp_handle->async_close = (uv_async_t*)calloc(1, sizeof(uv_async_t));
    if (udp_handle->async_close == NULL)
    {
        free(udp_handle->udp);
        free(udp_handle->async_send);
        free(udp_handle);
        return NULL;
    }

    uv_mutex_init(&(udp_handle->mutex));
    uv_sem_init(&(udp_handle->sem), 0);
    uv_udp_init(loop, udp_handle->udp);
    uv_async_init(loop, udp_handle->async_close, async_close_cb);
    uv_async_init(loop, udp_handle->async_send, async_send_cb);
    udp_handle->on_data           = on_data;
    udp_handle->on_send           = on_send;
    udp_handle->on_error          = on_error;
    udp_handle->uvloop            = loop;
    udp_handle->async_close->data = udp_handle;
    udp_handle->async_send->data  = udp_handle;
    udp_handle->udp->data         = udp_handle;
    udp_handle->thread_id         = uv_thread_self();

    if (addr != NULL)
    {
        struct sockaddr bind_addr;
        if (strchr(addr, ':'))
        {
            uv_ip6_addr(addr, port, (struct sockaddr_in6*)&bind_addr);
        }
        else
        {
            uv_ip4_addr(addr, port, (struct sockaddr_in*)&bind_addr);
        }
        uv_udp_bind(udp_handle->udp, (const struct sockaddr*)&bind_addr, 0);
    }

    r = uv_udp_recv_start(udp_handle->udp, alloc_buffer, on_read_cb);
    if (r)
    {
        dzlog_error("start udp failed:%d", r);
        udp_handle_close(udp_handle);
        return NULL;
    }

    return udp_handle;
}

/*******************************************************************************
 * function name : udp_send_data
 * description	 :
 * param[in]     : udp_handle;data;size;addr
 * param[out] 	 :
 * return 		 : none
 *******************************************************************************/
void udp_send_data(udp_socket_t* udp_handle, char* data, size_t size, struct sockaddr* addr)
{
    if (!udp_handle || !data || !addr)
    {
        return;
    }

    if (udp_handle->thread_id == uv_thread_self())
    {
        dzlog_debug("[0x%lx] udp_send_data sync", uv_thread_self());
        udp_handle->async_send_flag = false;
        uv_buf_t buf                = uv_buf_init(data, (unsigned int)size);
        udp_handle->data1           = &buf;
        udp_handle->data2           = addr;
        send_data(udp_handle);
    }
    else
    {
        uv_mutex_lock(&(udp_handle->mutex));
        dzlog_debug("[0x%lx] udp_send_data async start", uv_thread_self());
        udp_handle->async_send_flag  = true;
        uv_buf_t buf                 = uv_buf_init(data, (unsigned int)size);
        udp_handle->data1            = &buf;
        udp_handle->data2            = addr;
        udp_handle->async_send->data = udp_handle;
        uv_async_send(udp_handle->async_send);
        uv_sem_wait(&(udp_handle->sem));
        dzlog_debug("[0x%lx] udp_send_data async end", uv_thread_self());
        uv_mutex_unlock(&(udp_handle->mutex));
    }
}

/*******************************************************************************
 * function name : udp_send_data_ip
 * description	 :
 * param[in]     : udp_handle;data;ip;port
 * param[out] 	 :
 * return 		 : none
 *******************************************************************************/
void udp_send_data_ip(udp_socket_t* udp_handle, char* data, size_t size, const char* ip, int port)
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

/*******************************************************************************
 * function name : udp_handle_close
 * description	 :
 * param[in]     : udp_handle
 * param[out] 	 :
 * return 		 : none
 *******************************************************************************/
void udp_handle_close(udp_socket_t* udp_handle)
{
    if (!udp_handle)
    {
        return;
    }

    if (udp_handle->thread_id == uv_thread_self())
    {
        close_handle(udp_handle);
    }
    else
    {
        uv_async_send(udp_handle->async_close);
    }
}

//??????????????????
int udp_set_broadcast(udp_socket_t* udp_handle, bool on)
{
    return uv_udp_set_broadcast(udp_handle->udp, on);
}

//????????????
int udp_set_membership(udp_socket_t* udp_handle, const char* multicast_addr, const char* interface_addr, bool on)
{
    return uv_udp_set_membership(udp_handle->udp, multicast_addr, interface_addr, on);
}

//????????????????????????
int udp_set_multicast_loop(udp_socket_t* udp_handle, bool on)
{
    return uv_udp_set_multicast_loop(udp_handle->udp, on);
}

//????????????TTL
int udp_set_multicast_ttl(udp_socket_t* udp_handle, int ttl)
{
    return uv_udp_set_multicast_ttl(udp_handle->udp, ttl);
}

//???????????????????????????????????????
int udp_set_multicast_interface(udp_socket_t* udp_handle, const char* interface_addr)
{
    return uv_udp_set_multicast_interface(udp_handle->udp, interface_addr);
}