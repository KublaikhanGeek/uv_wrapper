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
#ifndef STREAM_SERVER_H_
#define STREAM_SERVER_H_

#include "defines.h"

extern tcp_server_t* tcp_server_run(const char* server_addr, int server_port, uv_loop_t* loop, tcp_conn_on_open_func_t,
                                    tcp_conn_on_close_func_t, tcp_conn_on_error_func_t, tcp_conn_on_read_func_t,
                                    tcp_conn_on_write_func_t);

extern void tcp_server_close(tcp_server_t* tcp_server);
extern void tcp_server_print_all_conn(tcp_server_t* tcp_server);
extern void tcp_server_set_no_delay(tcp_server_t* tcp_server, bool enable);
extern void tcp_server_set_keepalive(tcp_server_t* tcp_server, bool enable, unsigned int delay);
extern void tcp_server_send_data(tcp_connection_t* client, char* data, size_t size);
extern char* tcp_server_get_client_addr(tcp_connection_t* client, char* dst, size_t len);

#endif