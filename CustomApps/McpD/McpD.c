#include <stdio.h>
#include <unistd.h>
#include <nuttx/spi/spi_transfer.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <lvgl/lvgl.h>
#include <termios.h>
#include <semaphore.h>
#include <pthread.h>

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
    mcp_memory(0, true, 9, 0, 2, state_and_ctr);
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

  mcp_memory(0, false, 9, 0, 2, state_and_ctr);

  while(1) {
    uint8_t mctr;
    mcp_memory(0, true, 9, 2, 1, &mctr);
    if(mctr == state_and_ctr[1]) {
      break;
    }
  }
}

static void send_spi_data(int fd, uint8_t * data, size_t n, bool deselect, uint32_t speed)
{
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

static void send_spi_byte(int fd, uint8_t byte)
{
  send_spi_data(fd, &byte, 1, true, 1000000);
}

// static struct {
//   bool is_init;
//   uint8_t * fb;
//   sem_t work_avail;
//   sem_t work_done;
//   sem_t mutex;
//   pthread_t thread;
// } ctx;

// static void * flush_thread(void * arg)
// {
//   int fd = open("/dev/spi2", O_RDONLY);
//   assert(fd != -1);

//   while (1) {
//     assert(sem_wait(&ctx.work_avail) == 0);
//     assert(sem_wait(&ctx.mutex) == 0);

//     lv_draw_sw_rgb565_swap(ctx.fb, N_BYTES / 2);
//     int nbytes = N_BYTES;
//     uint8_t * px = ctx.fb;
//     while (nbytes > 0) {
//       send_spi_data(fd, px, nbytes < 20000 ? nbytes : 20000, false, 48000000);
//       nbytes -= 20000;
//       px += 20000;
//     }
//     lv_draw_sw_rgb565_swap(ctx.fb, N_BYTES / 2);

//     assert(sem_post(&ctx.mutex) == 0);
//     assert(sem_post(&ctx.work_done) == 0);
//   }
// }

static void flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
  if(lv_display_flush_is_last(disp)) {
    // if(!ctx.is_init) {
    //   ctx.is_init = true;
    //   ctx.fb = px_map;

    //   assert(sem_init(&ctx.work_avail, 0, 1) == 0);
    //   assert(sem_init(&ctx.work_done, 0, 0) == 0);
    //   assert(sem_init(&ctx.mutex, 0, 1) == 0);

    //   struct sched_param param;
    //   int policy;
    //   assert(pthread_getschedparam(pthread_self(), &policy, &param) == 0);
    //   printf("%d %d %d\n", (int)SCHED_FIFO, policy, param.sched_priority);

    //   pthread_attr_t attr;
    //   assert(pthread_attr_init(&attr) == 0);
    //   assert(pthread_attr_setstacksize(&attr, 2048) == 0);
    //   assert(pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED) == 0);
    //   param.sched_priority = 101;
    //   assert(pthread_attr_setschedparam(&attr, &param) == 0);
    //   assert(pthread_create(&ctx.thread, &attr, flush_thread, NULL) == 0);
    //   assert(pthread_attr_destroy(&attr) == 0);
    // }
    // else {
    //   assert(sem_wait(&ctx.work_done) == 0);
    //   assert(sem_wait(&ctx.mutex) == 0);

    //   ctx.fb = px_map;

    //   assert(sem_post(&ctx.mutex) == 0);
    //   assert(sem_post(&ctx.work_avail) == 0);
    // }

    static int fd = -1;
    if(fd == -1) {
      fd = open("/dev/spi2", O_RDONLY);
      assert(fd != -1);
    }
    lv_draw_sw_rgb565_swap(px_map, N_BYTES / 2);
    int nbytes = N_BYTES;
    uint8_t * px = px_map;
    while (nbytes > 0) {
      send_spi_data(fd, px, nbytes < 20000 ? nbytes : 20000, false, 48000000);
      nbytes -= 20000;
      px += 20000;
    }
    lv_draw_sw_rgb565_swap(px_map, N_BYTES / 2);
  }
  lv_display_flush_ready(disp);
}

// bool mcp_platform_enter_key;

static void read_cb(lv_indev_t * indev, lv_indev_data_t * data)
{
  static int gp_fd = -1;
  if(gp_fd == -1) {
    gp_fd = open("/dev/ttyS1", O_RDONLY | O_NONBLOCK);
    assert(gp_fd != -1);
  }

  uint8_t byte;
  ssize_t res = read(gp_fd, &byte, 1);
  assert(res == 1 || res == 0 || (res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)));
  if(res == 1) {
    data->continue_reading = true;
    switch (byte & 0x7f) {
      case 0:
        data->key = LV_KEY_UP;
        break;
      case 1:
        data->key = LV_KEY_DOWN;
        break;
      case 2:
        data->key = LV_KEY_LEFT;
        break;
      case 3:
        data->key = LV_KEY_RIGHT;
        break;
      case 4:
        data->key = LV_KEY_NEXT;
        break;
      case 5:
        data->key = LV_KEY_ENTER;
        // mcp_platform_enter_key = !(byte & 0x80);
        break;
      default:
        return;
    }
    data->state = (byte & 0x80) ? LV_INDEV_STATE_RELEASED : LV_INDEV_STATE_PRESSED;
  }
}


int mcpd_main(int argc, char **argv)
{
  if(argc == 1) {
    puts("needs init");

    uint8_t loc;
    bool is_horizontal;
    puts("declaring a...");
    mcp_declare(0, "cpu3a", &loc, &is_horizontal);
    puts("declaring b...");
    mcp_declare(1, "cpu3b", &loc, &is_horizontal);

    puts("routing cs...");
    mcp_route(0, MCP_ROUTE_E_ROUTE, 0, 3, 9, 2); // cs
    puts("routing sck...");
    mcp_route(0, MCP_ROUTE_E_ROUTE, 0, 0, 9, 3); // sck
    puts("routing mosi...");
    mcp_route(0, MCP_ROUTE_E_ROUTE, 0, 1, 9, 1); // mosi

    puts("routing gamepad...");
    mcp_route(0, MCP_ROUTE_E_ROUTE, 6, 2, 2, 3);

    // for(int i=0; i<3; i++) {
    //   gpio_write(0, (i + 1) % 2);
    // }
  } else {
    puts("does not need init");
  }

  puts("preparing screen...");

  int spi_fd = open("/dev/spi2", O_RDONLY);
  assert(spi_fd != -1);

  gpio_write(GPIO_BACKLIGHT, true);

  gpio_write(GPIO_RESET, false);
  usleep(500 * 1000);
  gpio_write(GPIO_RESET, true);
  usleep(500 * 1000);

  gpio_write(GPIO_DC, COMMAND);
  send_spi_byte(spi_fd, 0x01);
  usleep(150 * 1000);

  gpio_write(GPIO_DC, COMMAND);
  send_spi_byte(spi_fd, 0x11);
  usleep(500 * 1000);

  gpio_write(GPIO_DC, COMMAND);
  send_spi_byte(spi_fd, 0x3A);
  gpio_write(GPIO_DC, DATA);
  send_spi_byte(spi_fd, 0x55);
  usleep(10 * 1000);

  gpio_write(GPIO_DC, COMMAND);
  send_spi_byte(spi_fd, 0x36);
  gpio_write(GPIO_DC, DATA);
  send_spi_byte(spi_fd, 0x08);

  gpio_write(GPIO_DC, COMMAND);
  send_spi_byte(spi_fd, 0x21);
  usleep(10 * 1000);

  gpio_write(GPIO_DC, COMMAND);
  send_spi_byte(spi_fd, 0x13);
  usleep(10 * 1000);

  gpio_write(GPIO_DC, COMMAND);
  send_spi_byte(spi_fd, 0x36);
  gpio_write(GPIO_DC, DATA);
  send_spi_byte(spi_fd, 0xC0);

  gpio_write(GPIO_DC, COMMAND);
  send_spi_byte(spi_fd, 0x29);
  usleep(500 * 1000);

  gpio_write(GPIO_DC, COMMAND);
  send_spi_byte(spi_fd, 0x2C);

  gpio_write(GPIO_DC, DATA);

  int res = close(spi_fd);
  assert(res != -1);


  puts("preparing gamepad...");

  int gp_fd = open("/dev/ttyS1", O_RDONLY);
  assert(gp_fd != -1);
  struct termios tio;
  res = tcgetattr(gp_fd, &tio);
  assert(res >= 0);
  res = cfsetspeed(&tio, B115200);
  assert(res >= 0);
  tio.c_cflag &= ~CSTOPB;
  tio.c_cflag &= ~(PARENB | PARODD);
  tio.c_cflag &= ~CSIZE;
  tio.c_cflag |= CS8;
  tio.c_cflag &= ~CCTS_OFLOW;
  tio.c_cflag &= ~CRTS_IFLOW;
  res = tcsetattr(gp_fd, TCSANOW, &tio);
  assert(res >= 0);
  res = close(gp_fd);
  assert(res != -1);

  puts("done");
  // sleep(1);

  static uint8_t fb[N_BYTES] __attribute__((aligned(4)));

  lv_init();

  lv_display_t * disp = lv_display_create(240, 320);
  assert(disp);
  lv_display_set_buffers(disp, fb, NULL, N_BYTES, LV_DISPLAY_RENDER_MODE_DIRECT);
  lv_display_set_flush_cb(disp, flush_cb);
  
  lv_indev_t * indev = lv_indev_create();
  assert(indev);
  lv_indev_set_type(indev, LV_INDEV_TYPE_KEYPAD);
  lv_indev_set_read_cb(indev, read_cb);

  return 0;
}
