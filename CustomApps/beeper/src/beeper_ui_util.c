#include "beeper_ui_private.h"

static void base_obj_delete_cb(lv_event_t * e)
{
    lv_obj_t * base_obj = lv_event_get_target_obj(e);
    beeper_ui_t * c = lv_obj_get_user_data(base_obj);
    beeper_task_destroy(c->task);
    free(c);
    lv_obj_set_user_data(base_obj, NULL);
}

lv_obj_t * beeper_ui_base_obj_create(void)
{
    lv_obj_t * base_obj = lv_obj_create(lv_screen_active());
    beeper_ui_t * c = calloc(1, sizeof(beeper_ui_t));
    assert(c);
    lv_obj_set_user_data(base_obj, c);
    lv_obj_add_event_cb(base_obj, base_obj_delete_cb, LV_EVENT_DELETE, NULL);
    return base_obj;
}
