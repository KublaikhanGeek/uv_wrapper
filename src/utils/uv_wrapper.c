#include "uv_wrapper.h"
#include "zlog.h"

static void close_cb(uv_handle_t* handle)
{
}

static void walk_cb(uv_handle_t* handle, void* arg)
{
    uv_close(handle, close_cb);
}

void uv_tear_down(uv_loop_t* loop)
{
    if (UV_EBUSY == uv_loop_close(loop))
    {
        uv_walk(loop, walk_cb, NULL);
        uv_run(loop, UV_RUN_DEFAULT);
        if (UV_EBUSY == uv_loop_close(loop))
        {
            dzlog_error("uv_tear_down failed");
        }
        else
        {
            dzlog_info("uv_tear_down success");
        }
    }
    else
    {
        dzlog_info("uv_tear_down success");
    }
}