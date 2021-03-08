
#include "uv_wrapper.h"
#include "zlog.h"

static void close_cb(uv_handle_t* handle)
{
}

static void walk_cb(uv_handle_t* handle, void* arg)
{
    dzlog_debug("handle %p :flags: %d", handle, handle->flags);
    if (uv_is_closing(handle))
    {
        dzlog_info("handle is already closing");
        return;
    }

    uv_close(handle, close_cb);
}

void uv_tear_down(uv_loop_t* loop)
{
#if 0
            uv_print_active_handles(loop, stderr);
            dzlog_debug("=================");
            uv_print_all_handles(loop, stderr);
#endif
    dzlog_debug("uv_tear_down");
    if (UV_EBUSY == uv_loop_close(loop))
    {
        uv_walk(loop, walk_cb, NULL);
        dzlog_debug("uv_run 2 ++ ");
        uv_run(loop, UV_RUN_DEFAULT);
        dzlog_debug("uv_run 2 -- ");
        if (UV_EBUSY == uv_loop_close(loop))
        {
            dzlog_error("uv_tear_down failed");
        }
        else
        {
            dzlog_info("uv_tear_down success 2");
        }
    }
    else
    {
        dzlog_info("uv_tear_down success 1");
    }
}