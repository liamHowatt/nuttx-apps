#include <stdio.h>
#include <unistd.h>
#include <nuttx/spi/spi_transfer.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#include "mcp.h"

#ifdef NDEBUG
  #error assert is used a lot here. it is recommended to undef NDEBUG
#endif

#define GPIO_BACKLIGHT 0
#define GPIO_DC        1
#define GPIO_RESET     2

#define COMMAND        false
#define DATA           true

#define N_BYTES        (240 * 320 * 2)

static void gpio_write(int gpio, bool level)
{
  static bool ran_once;
  static uint8_t state_and_ctr[2];

  if(!ran_once) {
    ran_once = true;
    mcp_memory(0, true, 10, 0, 2, state_and_ctr);
  }

  uint8_t new_state;
  if(level) {
    new_state = state_and_ctr[0] | 1 << gpio;
  } else {
    new_state = state_and_ctr[0] & ~(1 << gpio);
  }

  if(new_state == state_and_ctr[0]) {
    return;
  }

  state_and_ctr[0] = new_state;
  state_and_ctr[1]++;

  mcp_memory(0, false, 10, 0, 2, state_and_ctr);

  while(1) {
    uint8_t mctr;
    mcp_memory(0, true, 10, 2, 1, &mctr);
    if(mctr == state_and_ctr[1]) {
      break;
    }
  }
}

static void send_spi_data(uint8_t * data, size_t n, bool deselect, uint32_t speed)
{
  static int fd = -1;
  if(fd == -1) {
    fd = open("/dev/spi2", O_RDONLY);
    assert(fd != -1);
  }

  struct spi_trans_s trans = {0};
  struct spi_sequence_s seq = {0};

  seq.dev = 2;
  seq.mode = 0;
  seq.nbits = 8;
  seq.frequency = speed;
  seq.ntrans = 1;
  seq.trans = &trans;

  trans.deselect = deselect;
  // trans.cmd = false;
  trans.delay = 0;
  trans.nwords = n;
  trans.txbuffer = data;
  trans.rxbuffer = NULL;

  int ret = ioctl(fd, SPIIOC_TRANSFER, (unsigned long)((uintptr_t)&seq));
  assert(-1 != ret);
}

static void send_spi_byte(uint8_t byte)
{
  send_spi_data(&byte, 1, true, 1000000);
}

int mcpd_main(int argc, char **argv)
{
  uint8_t loc;
  bool is_horizontal;
  puts("declaring a...");
  mcp_declare(0, "cpu3a", &loc, &is_horizontal);
  puts("declaring b...");
  mcp_declare(1, "cpu3b", &loc, &is_horizontal);

  puts("routing cs...");
  mcp_route(0, MCP_ROUTE_E_ROUTE, 7, 3, 10, 2); // cs
  puts("routing sck...");
  mcp_route(0, MCP_ROUTE_E_ROUTE, 7, 0, 10, 3); // sck
  puts("routing mosi...");
  mcp_route(0, MCP_ROUTE_E_ROUTE, 7, 1, 10, 1); // mosi

  // for(int i=0; i<3; i++) {
  //   gpio_write(0, (i + 1) % 2);
  // }

  gpio_write(GPIO_BACKLIGHT, true);

  gpio_write(GPIO_RESET, false);
  usleep(500 * 1000);
  gpio_write(GPIO_RESET, true);
  usleep(500 * 1000);

  gpio_write(GPIO_DC, COMMAND);
  send_spi_byte(0x01);
  usleep(150 * 1000);

  gpio_write(GPIO_DC, COMMAND);
  send_spi_byte(0x11);
  usleep(500 * 1000);

  gpio_write(GPIO_DC, COMMAND);
  send_spi_byte(0x3A);
  gpio_write(GPIO_DC, DATA);
  send_spi_byte(0x55);
  usleep(10 * 1000);

  gpio_write(GPIO_DC, COMMAND);
  send_spi_byte(0x36);
  gpio_write(GPIO_DC, DATA);
  send_spi_byte(0x08);

  gpio_write(GPIO_DC, COMMAND);
  send_spi_byte(0x21);
  usleep(10 * 1000);

  gpio_write(GPIO_DC, COMMAND);
  send_spi_byte(0x13);
  usleep(10 * 1000);

  gpio_write(GPIO_DC, COMMAND);
  send_spi_byte(0x36);
  gpio_write(GPIO_DC, DATA);
  send_spi_byte(0xC0);

  gpio_write(GPIO_DC, COMMAND);
  send_spi_byte(0x29);
  usleep(500 * 1000);

  gpio_write(GPIO_DC, COMMAND);
  send_spi_byte(0x2C);

  gpio_write(GPIO_DC, DATA);

  puts("done");
  // sleep(1);

  bool flip = true;
  while(1) {
    static uint8_t fb[N_BYTES] __attribute__((aligned(64)));

    send_spi_data(fb, N_BYTES, false, 1000000);
  
    // int nbytes = N_BYTES;
    // while (nbytes > 0) {
    //   send_spi_data(fb, nbytes < 20000 ? nbytes : 20000, false, 1000000);
    //   nbytes -= 20000;
    // }
  
    memset(fb, flip ? 255 : 0, N_BYTES);
    flip = !flip;
    sleep(1);
  }

  return 0;
}
