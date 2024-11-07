#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>


int testing_main(int argc, char *argv[])
{
  while (1) {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint32_t tick = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

    printf("%d\n", tick);
  }

  return 0;
}
