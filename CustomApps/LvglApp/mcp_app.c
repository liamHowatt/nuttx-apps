#include "mcp_app.h"
#include "apps.h"

#include "lvgl/lvgl.h"

#include "lvgl/demos/lv_demos.h"
#include "lvgl/examples/lv_examples.h"

static void make_enc_kpd_group(void)
{
    lv_group_t * g = lv_group_create();
    lv_group_set_default(g);

    lv_indev_t * indev = NULL;
    for(;;) {
        indev = lv_indev_get_next(indev);
        if(!indev) {
            break;
        }

        lv_indev_type_t indev_type = lv_indev_get_type(indev);
        if(indev_type == LV_INDEV_TYPE_KEYPAD) {
            lv_indev_set_group(indev, g);
        }

        if(indev_type == LV_INDEV_TYPE_ENCODER) {
            lv_indev_set_group(indev, g);
        }
    }
}

static void choice_cb(lv_event_t * e)
{
    lv_obj_clean(lv_screen_active());

    app_init_cb app_init = lv_event_get_user_data(e);
    app_init();
}

void mcp_app(void) {
    // lv_demo_keypad_encoder();



    make_enc_kpd_group();

    lv_obj_t * scr = lv_screen_active();

    lv_obj_t * list = lv_list_create(scr);
    lv_obj_set_size(list, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_radius(list, 0, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_add_event_cb(lv_list_add_button(list, NULL, "gb"), choice_cb, LV_EVENT_CLICKED, gb_app);
    lv_obj_add_event_cb(lv_list_add_button(list, NULL, "xkcd"), choice_cb, LV_EVENT_CLICKED, xkcd_app);
    lv_list_add_button(list, NULL, "123");
    lv_list_add_button(list, NULL, "123");
    lv_list_add_button(list, NULL, "123");
    lv_list_add_button(list, NULL, "123");



    // lv_obj_t * scr = lv_screen_active();
    // lv_obj_t * canv = lv_canvas_create(scr);
    // lv_canvas_set_buffer()
    // lv_canvas_set_px
    // lv_canvas_set_palette(canv, 0, (lv_color32_t){0, 0, 0, 255});
    // lv_canvas_set_palette(canv, 1, (lv_color32_t){85, 85, 85, 255});
    // lv_canvas_set_palette(canv, 2, (lv_color32_t){170, 170, 170, 255});
    // lv_canvas_set_palette(canv, 3, (lv_color32_t){255, 255, 255, 255});

}
