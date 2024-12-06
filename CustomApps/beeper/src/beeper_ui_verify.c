#include "beeper_ui_private.h"

static void is_verified_cb(bool is_verified, void * user_data)
{
    if(is_verified) {
        lv_obj_t * base_obj = user_data;
        lv_obj_clean(base_obj);
        beeper_ui_networks(base_obj);
    }
    else {
        // TODO SAS
    }
}

void beeper_ui_verify(lv_obj_t * base_obj)
{
    beeper_ui_t * c = lv_obj_get_user_data(base_obj);

    lv_obj_remove_style_all(base_obj);
    lv_obj_set_size(base_obj, LV_PCT(100), LV_PCT(100));

    lv_obj_t * bg_cont = lv_obj_create(base_obj);
    lv_obj_remove_style_all(bg_cont);
    lv_obj_set_size(bg_cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(bg_cont, LV_FLEX_FLOW_COLUMN);

    lv_obj_t * verify_label = lv_label_create(bg_cont);
    lv_label_set_text_static(verify_label, "Verify");

    lv_obj_t * body = lv_obj_create(bg_cont);
    lv_obj_remove_style_all(body);
    lv_obj_set_width(body, LV_PCT(100));
    lv_obj_set_flex_grow(body, 1);

    lv_obj_t * spinner = lv_spinner_create(body);
    lv_obj_center(spinner);
    lv_obj_set_width(spinner, LV_PCT(20));

    beeper_task_await_is_verified(c->task, is_verified_cb, base_obj);
}
