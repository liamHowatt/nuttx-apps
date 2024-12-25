#pragma once

#ifdef NDEBUG
  #error it is recommended to undef NDEBUG
#endif

#define BEEPER_ROOT_PATH "/appdata/beeper/"
#define BEEPER_DEVICE_DISPLAY_NAME "MCP"
#define BEEPER_RANDOM_PATH "/host/random"

void beeper_start(void);
