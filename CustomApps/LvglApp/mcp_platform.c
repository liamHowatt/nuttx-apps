#include "mcp_platform.h"
#include <nuttx/config.h>

#ifdef CONFIG_ARCH_SIM

#include <stdlib.h>
#include <assert.h>
#define SLOW_RAM_SIZE (8 * 1024 * 1024)

void * mcp_platform_slow_ram_get_ptr(void) {
    static void * ptr;
    if(!ptr) {
        ptr = malloc(SLOW_RAM_SIZE);
        assert(ptr);
    }
    return ptr;
}

size_t mcp_platform_slow_ram_get_size(void) {
    return SLOW_RAM_SIZE;
}

#else
#error only the SIM is supported for now
#endif
