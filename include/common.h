#ifndef COMMON_H_
#define COMMON_H_

#define MSG_BUF_SIZE            4096
#define MSG_PRINT_SIZE          1024
#define MAX_PRINT_BUFSIZE       (MSG_PRINT_SIZE * 3)
typedef void (*component_init_t)();
typedef void (*component_uninit_t)();

typedef enum
{
    true  = 1,
    false = 0
} bool;

typedef enum
{
    COMPONENT,
    COMPONENT_UNKNOW
} component_type;

typedef struct app_task_s
{
    component_type component;
    component_init_t task_init;
    component_uninit_t task_uninit;
} app_task_t;

#endif