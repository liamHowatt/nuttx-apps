#pragma once
#include <nuttx/config.h>

typedef void (*app_init_cb)(void);

#ifdef CONFIG_CUSTOM_APPS_LVGL_APP_GB_APP
void gb_app(void);
#endif
#ifdef CONFIG_CUSTOM_APPS_LVGL_APP_XKCD_APP
void xkcd_app(void);
#endif
