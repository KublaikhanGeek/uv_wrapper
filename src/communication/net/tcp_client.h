/******************************************************************************
 * Copyright (c) 2007-2019, ZeroTech Co., Ltd.
 * All rights reserved.
 *******************************************************************************
 * File name     : tcp_client.h
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
#ifndef TCP_CLIENT_H_
#define TCP_CLIENT_H_

#include "defines.h"

extern tcp_client_t* tcp_client_run(const char* server_addr, int server_port, uv_loop_t* loop, tcp_conn_on_open_func_t,
                                    tcp_conn_on_close_func_t, tcp_conn_on_error_func_t, tcp_conn_on_read_func_t,
                                    tcp_conn_on_write_func_t);
extern void tcp_client_close(tcp_client_t* tcp_client);
extern void tcp_client_send_data(tcp_connection_t* conn, char* data, size_t size);

#endif