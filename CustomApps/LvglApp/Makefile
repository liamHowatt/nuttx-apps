include $(APPDIR)/Make.defs

# LVGL App built-in application info

PROGNAME = $(CONFIG_CUSTOM_APPS_LVGL_APP_PROGNAME)
PRIORITY = $(CONFIG_CUSTOM_APPS_LVGL_APP_PRIORITY)
STACKSIZE = $(CONFIG_CUSTOM_APPS_LVGL_APP_STACKSIZE)
MODULE = $(CONFIG_CUSTOM_APPS_LVGL_APP)

# LVGL App

MAINSRC = LvglApp.c
CSRCS = mcp_platform.c \
		mcp_app.c \
		apps/gb/gb_app.c \
		apps/xkcd/xkcd_app.c \
		apps/xkcd/pngle/pngle.c \
		apps/xkcd/pngle/miniz.c

include $(APPDIR)/Application.mk
