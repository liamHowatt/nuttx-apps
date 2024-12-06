#pragma once

#ifdef NDEBUG
  #error it is recommended to undef NDEBUG
#endif

#define BEEPER_ROOT_PATH "/appdata/beeper/"

void beeper_start(void);
