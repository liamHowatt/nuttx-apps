#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <nuttx/ioexpander/gpio.h>
#include <string.h>
#include <stdlib.h>

#include "mod.h"

#ifdef NDEBUG
  #error assert is used a lot here. it is recommended to undef NDEBUG
#endif

struct ctx {
  int pin_fds[2];
  struct mod_command *command;
  bool delay_has_elapsed;
};

struct all {
  struct mod mod;
  struct ctx ctx;
  struct mod_command command;
};

static void gpio_write(void *ctx, enum mod_Gpio pin, bool value) {
  struct ctx *ctx2 = ctx;

  int fd = pin == mod_GpioClk ? ctx2->pin_fds[0] : ctx2->pin_fds[1];
  enum gpio_pintype_e pin_type = value ? GPIO_INPUT_PIN : GPIO_OUTPUT_PIN;

  int ret = ioctl(fd, GPIOC_SETPINTYPE, pin_type);
  assert(ret == 0);
}
static bool gpio_read(void *ctx, enum mod_Gpio pin) {
  struct ctx *ctx2 = ctx;

  int fd = pin == mod_GpioClk ? ctx2->pin_fds[0] : ctx2->pin_fds[1];
  bool read_v;

  int ret = ioctl(fd, GPIOC_READ, &read_v);
  assert(ret == 0);

  return read_v;
}
static bool check_and_set_bootstrap(void *ctx) {
  return true;
}
static void report_command_is_complete(void *ctx) {
}
static bool delay_has_elapsed(void *ctx) {
  struct ctx *ctx2 = ctx;

  return ctx2->delay_has_elapsed;
}
static bool clk_is_high(void *ctx) {
  return gpio_read(ctx, mod_GpioClk);
}
static bool try_get_command(void *ctx, struct mod_command **command_dst) {
  struct ctx *ctx2 = ctx;

  if (!ctx2->command) return false;

  *command_dst = ctx2->command;
  ctx2->command = NULL;

  return true;
}
static void time_save(void *ctx) {
  struct ctx *ctx2 = ctx;

  ctx2->delay_has_elapsed = false;
}
static void unreachable(void *ctx) {
  assert(0);
}

static const struct mod_platform_interface interface = {
  .gpio_write=                  gpio_write,
  .gpio_read=                   gpio_read,
  .check_and_set_bootstrap=     check_and_set_bootstrap,
  .report_command_is_complete=  report_command_is_complete,
  .delay_has_elapsed=           delay_has_elapsed,
  .clk_is_high=                 clk_is_high,
  .try_get_command=             try_get_command,
  .time_save=                   time_save,
  .unreachable=                 unreachable,
};

int mcp_main(int argc, char *argv[])
{
  puts("This is the MCP app");

  struct all *all = malloc(sizeof(struct all));
  assert(all);

  int fd_clk = open("/dev/mcp0_clk", O_RDWR);
  assert(fd_clk != -1);
  int fd_dat = open("/dev/mcp0_dat", O_RDWR);
  assert(fd_dat != -1);

  all->ctx.pin_fds[0] = fd_clk;
  all->ctx.pin_fds[1] = fd_dat;
  all->ctx.command = NULL;

  mod_init(&all->mod, &interface, &all->ctx);

  all->command.command = CommandDeclare;
  strncpy(all->command.u.declare.declaration, "NuttX ESP32-S3", DECLARATION_LEN);

  all->ctx.command = &all->command;
  while (1) {
    mod_update(&all->mod);

    int blocked_by = mod_blocked_by(&all->mod);
    switch (blocked_by) {
      case mod_ModBlockOnDelay:
        usleep(MOD_BASE_PIN_HALF_CLK_PERIOD_US);
        all->ctx.delay_has_elapsed = true;
        break;
      case mod_ModBlockOnNeedClkHigh:
        usleep(MOD_BASE_PIN_HALF_CLK_PERIOD_US);
        break;
      case mod_ModBlockOnNeedCommand:
        break;
      default:
        assert(0);
    }
    if (blocked_by == mod_ModBlockOnNeedCommand) {
      break;
    }
  }

  printf(
    "location: %d, is hor: %d\n",
    (int) all->command.u.declare.location_response.location,
    (int) all->command.u.declare.location_response.is_horizontal
  );

  free(all);
  puts("done");
  return 0;
}
