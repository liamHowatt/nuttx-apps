include $(APPDIR)/Make.defs

# Mcp built-in application info

PROGNAME = $(CONFIG_CUSTOM_APPS_MCP_PROGNAME)
PRIORITY = $(CONFIG_CUSTOM_APPS_MCP_PRIORITY)
STACKSIZE = $(CONFIG_CUSTOM_APPS_MCP_STACKSIZE)
MODULE = $(CONFIG_CUSTOM_APPS_MCP)

# Mcp

MAINSRC = Mcp.c
CSRCS   = mcp_core/module/mod.c mcp_core/common/types/types.c mcp_core/common/serialize/ser.c
CFLAGS += -Imcp_core/module
CFLAGS += -Imcp_core/common/mcpasync
CFLAGS += -Imcp_core/common/serialize
CFLAGS += -Imcp_core/common/types
CFLAGS += -Imcp_core/common/util

include $(APPDIR)/Application.mk
