#include "component.h"
#include "component.h"
#include "event_loop_thread.h"
#include "udp.h"
#include "msg_queue.h"
#include "utils.h"

#define LOCAL_GCS_UDP_PORT 7088
static pthread_t event_thread;
static struct sockaddr peer_addr;
static udp_socket_t* udp_handle = NULL;
static msg_queue_t* msg_queue   = NULL;
static int init_peer            = 0;

static void on_data(udp_socket_t* handle, const struct sockaddr* peer, void* data, size_t len);
static void* event_thread_func(void* arg);
static void on_msg(void* data, size_t len);

static void* event_thread_func(void* arg)
{
    args_t* args = (args_t*)arg;
    udp_handle   = udp_handle_run("0.0.0.0", LOCAL_GCS_UDP_PORT, args->loop, on_data, NULL, NULL);
    return NULL;
}

void component_init()
{
    msg_queue = msg_queue_create(COMPONENT, on_msg);
    event_thread_create(&event_thread, event_thread_func, NULL);
}

void component_uninit()
{
    msg_queue_destroy(msg_queue);
    udp_handle_close(udp_handle);
    event_thread_destroy(event_thread);
}

int get_component_msg_id()
{
    if (msg_queue)
    {
        return msg_queue->msg_id;
    }

    return -1;
}

static void on_data(udp_socket_t* handle, const struct sockaddr* peer, void* data, size_t len)
{
    dzlog_debug("gcs receive msg: ");
    print_in_hex(data, len);
    if (0 == init_peer)
    {
        memcpy(&peer_addr, peer, sizeof(struct sockaddr));
        init_peer = 1;
    }
    msg_send(get_component_msg_id(), data, len, 0);
}

static void on_msg(void* data, size_t len)
{
    dzlog_debug("gcs message queue received msg:");
    print_in_hex(data, len);
    if (init_peer)
    {
        udp_send_data(udp_handle, (char*)&data, len, &peer_addr);
    }
}