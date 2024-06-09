#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <nuttx/ioexpander/gpio.h>
#include <string.h>

int mcp_main(int argc, char *argv[])
{
  // printf("This is the MCP app\n");

  if(argc != 2 || strnlen(argv[1], 2) != 1 || argv[1][0] < '0' || argv[1][0] > '9') {
    puts("wrong args");
    return 1;
  }
  const int sleep_len = argv[1][0] - '0';

  int fd = open("/dev/mcp0_clk", O_RDWR);
  assert(fd != -1);

  int ret;
  unsigned uret;

  for (int i = 0; i < 20; i++) {
    ret = ioctl(fd, GPIOC_SETPINTYPE, GPIO_OUTPUT_PIN);
    assert(ret == 0);

    if (sleep_len) {
      uret = sleep(sleep_len);
      assert(uret == 0);
    }

    ret = ioctl(fd, GPIOC_SETPINTYPE, GPIO_INPUT_PIN);
    assert(ret == 0);
  }

  ret = close(fd);
  assert(ret == 0);

  return 0;
}
