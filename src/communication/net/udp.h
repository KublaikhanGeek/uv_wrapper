/******************************************************************************
 * Copyright (c) 2007-2019, ZeroTech Co., Ltd.
 * All rights reserved.
 *******************************************************************************
 * File name     : udp.h
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
#ifndef UDP_H_
#define UDP_H_

#include "defines.h"

typedef struct udp_data_s udp_data_t;
struct udp_data_s
{
    uv_buf_t* buf;
    struct sockaddr* addr;
    void* data;
};

extern udp_socket_t* udp_handle_run(const char* addr, int port, uv_loop_t* loop, udp_handle_on_data_func_t on_data,
                                    udp_handle_on_send_func_t on_send, udp_handle_on_error_func_t on_error);

extern void udp_handle_close(udp_socket_t* udp_handle);

//发送数据到特定地址，地址用struct sockaddr 表示
extern void udp_send_data(udp_socket_t* handle, char* data, size_t size, struct sockaddr* addr);
//发送数据到特定ip和端口 如127.0.0. 8000
extern void udp_send_data_ip(udp_socket_t* udp_handle, char* data, size_t size, const char* ip, int port);

//设置广播开关
extern int udp_set_broadcast(udp_socket_t* udp_handle, bool on);

//加入组播
extern int udp_set_membership(udp_socket_t* udp_handle, const char* multicast_addr, const char* interface_addr,
                              bool on);
//设置组播回环标志
extern int udp_set_multicast_loop(udp_socket_t* udp_handle, bool on);
//设置组播TTL
extern int udp_set_multicast_ttl(udp_socket_t* udp_handle, int ttl);
//设置组播发送和接收数据接口
extern int udp_set_multicast_interface(udp_socket_t* udp_handle, const char* interface_addr);

#endif