#include "beeper_private.h"
#include <lvgl/lvgl.h>
#include "beeper_task.h"

typedef struct {
    beeper_task_t * task;
} beeper_ui_t;

/* beeper_ui_util.c */
lv_obj_t * beeper_ui_base_obj_create(void);

/* beeper_ui_login.c */
void beeper_ui_login(lv_obj_t * base_obj);

/* beeper_ui_verify.c */
void beeper_ui_verify(lv_obj_t * base_obj);

/* beeper_ui_networks.c */
void beeper_ui_networks(lv_obj_t * base_obj);
