include $(APPDIR)/Make.defs

# LVGL App built-in application info

PROGNAME = $(CONFIG_CUSTOM_APPS_LVGL_APP_PROGNAME)
PRIORITY = $(CONFIG_CUSTOM_APPS_LVGL_APP_PRIORITY)
STACKSIZE = $(CONFIG_CUSTOM_APPS_LVGL_APP_STACKSIZE)
MODULE = $(CONFIG_CUSTOM_APPS_LVGL_APP)

# LVGL App

MAINSRC = LvglApp.c
CSRCS = mcp_platform.c \
		mcp_app.c
ifeq ($(CONFIG_CUSTOM_APPS_LVGL_APP_GB_APP),y)
CSRCS += apps/gb/gb_app.c
endif
ifeq ($(CONFIG_CUSTOM_APPS_LVGL_APP_XKCD_APP),y)
CSRCS += apps/xkcd/xkcd_app.c \
		 apps/xkcd/pngle/pngle.c \
		 apps/xkcd/pngle/miniz.c
endif

include $(APPDIR)/Application.mk
