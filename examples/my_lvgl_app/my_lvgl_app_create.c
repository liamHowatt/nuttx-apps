#include "my_lvgl_app_create.h"

#include "lvgl/lvgl.h"
#include "lvgl/examples/lv_examples.h"

static void anim_image_cb(void * var, int32_t v)
{
    lv_obj_t * image = var;

    lv_obj_remove_flag(image, LV_OBJ_FLAG_HIDDEN);
    lv_image_set_scale(image, v);
}

static void anim_label_cb(void * var, int32_t v)
{
    lv_obj_t * label = var;

    lv_obj_set_style_opa(label, v, 0);
}

static void screen_clicked_cb(lv_event_t * e)
{
    lv_obj_t * new_screen = lv_obj_create(NULL);
    lv_screen_load(new_screen);
    lv_example_scroll_6();
}

void my_lvgl_app_create(void)
{
    lv_obj_t * screen = lv_screen_active();
    lv_obj_add_event_cb(screen, screen_clicked_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);

    LV_IMAGE_DECLARE(NuttX_Orig);
    lv_obj_t * image = lv_image_create(screen);
    lv_image_set_src(image, &NuttX_Orig);
    lv_obj_center(image);
    lv_obj_add_flag(image, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(image, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, image);
    lv_anim_set_values(&a, 1, 256);
    lv_anim_set_delay(&a, 500);
    lv_anim_set_duration(&a, 1000);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_set_exec_cb(&a, anim_image_cb);
    lv_anim_start(&a);

    lv_obj_t * label = lv_label_create(screen);
    lv_label_set_text_static(label, "NuttX + LVGL");
    lv_obj_align_to(label, image, LV_ALIGN_OUT_BOTTOM_MID, 0, 40);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_opa(label, 0, 0);
    lv_obj_add_flag(label, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_anim_init(&a);
    lv_anim_set_var(&a, label);
    lv_anim_set_values(&a, 1, 255);
    lv_anim_set_delay(&a, 2000);
    lv_anim_set_duration(&a, 1000);
    lv_anim_set_exec_cb(&a, anim_label_cb);
    lv_anim_start(&a);
}
