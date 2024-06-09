#include "my_lvgl_app_create.h"

#include <lvgl/lvgl.h>

void my_lvgl_app_create(void)
{
    lv_roller_create(lv_screen_active());
}
