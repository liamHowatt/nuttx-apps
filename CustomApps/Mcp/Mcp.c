#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <nuttx/ioexpander/gpio.h>
#include <string.h>
#include <stdlib.h>

#include "mod.h"
#include "util.h"

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
  Request requests[12];
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
  static bool boostrapped;
  bool ret = !boostrapped;
  boostrapped = true;
  return ret;
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

static uint8_t decode_pin_id(const char *s) {
  if (
    strlen(s) != 2
    || s[0] < 'A'
    || s[0] > 'L'
    || s[1] < '0'
    || s[1] > '3'
  ) {
    return 255;
  }
  int slot = s[0] - 'A';
  int pin = s[1] - '0';
  return slot * 4 + pin;
}

int mcp_main(int argc, char *argv[])
{
  int returncode = 1;
  static struct all all;

  if(argc < 2) {
    goto out;
  }

  if (strcmp(argv[1], "declare") == 0) {
    if (argc != 3 || strlen(argv[2]) > DECLARATION_LEN) {
      goto out;
    }
    all.command.command = CommandDeclare;
    strncpy(all.command.u.declare.declaration, argv[2], DECLARATION_LEN);
  }
  else if (strcmp(argv[1], "request") == 0) {
    if (argc != 2) {
      goto out;
    }
    all.command.command = CommandRequest;
    all.command.u.request.requests_dst = all.requests;
  }
  else if (strcmp(argv[1], "route") == 0) {
    all.command.command = CommandRoute;
    if (argc < 3) {
      goto out;
    }
    all.command.u.route.route_request.input_pin = 254;
    all.command.u.route.route_request.output_pin = 254;
    if (strcmp(argv[2], "set") == 0) {
      all.command.u.route.route_request.op = RouteOpSetOne;
      if (argc != 4) {
        goto out;
      }
      all.command.u.route.route_request.output_pin = decode_pin_id(argv[3]);
    }
    else if (strcmp(argv[2], "clear") == 0) {
      all.command.u.route.route_request.op = RouteOpClearOne;
      if (argc != 4) {
        goto out;
      }
      all.command.u.route.route_request.output_pin = decode_pin_id(argv[3]);
    }
    else if (strcmp(argv[2], "route") == 0) {
      all.command.u.route.route_request.op = RouteOpRouteIo;
      if (argc != 5) {
        goto out;
      }
      all.command.u.route.route_request.input_pin = decode_pin_id(argv[3]);
      all.command.u.route.route_request.output_pin = decode_pin_id(argv[4]);
    }
    // else if (strcmp(argv[2], "unroute") == 0) {
    //   all.command.u.route.route_request.op = RouteOpUnrouteIo;
    // }
    else {
      goto out;
    }
    if (
      all.command.u.route.route_request.input_pin == 255
      || all.command.u.route.route_request.output_pin == 255
    ) {
      goto out;
    }
  }
  else {
    goto out;
  }

  int fd_clk = open("/dev/mcp0_clk", O_RDWR);
  assert(fd_clk != -1);
  int fd_dat = open("/dev/mcp0_dat", O_RDWR);
  assert(fd_dat != -1);

  all.ctx.pin_fds[0] = fd_clk;
  all.ctx.pin_fds[1] = fd_dat;
  all.ctx.command = NULL;
  all.ctx.delay_has_elapsed = false;

  mod_init(&all.mod, &interface, &all.ctx);

  all.ctx.command = &all.command;
  while (1) {
    mod_update(&all.mod);

    int blocked_by = mod_blocked_by(&all.mod);
    switch (blocked_by) {
      case mod_ModBlockOnDelay:
        usleep(MOD_BASE_PIN_HALF_CLK_PERIOD_US);
        all.ctx.delay_has_elapsed = true;
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

  if (all.command.command == CommandDeclare) {
    printf(
      "%d %d\n",
      (int) all.command.u.declare.location_response.location,
      (int) all.command.u.declare.location_response.is_horizontal
    );
  }
  else if (all.command.command == CommandRequest) {
    for (int i=0; i<all.command.u.request.n_requests_result; i++) {
      assert(memchr(all.requests[i].declaration, '\n', DECLARATION_LEN) == NULL);
      printf(
        "%d %d %." STRINGIFY(DECLARATION_LEN) "s\n",
        (int) all.requests[i].location,
        (int) all.requests[i].is_horizontal,
        all.requests[i].declaration
      );
    }
  }
  else if (all.command.command == CommandRoute) {
  }
  else {
    assert(0);
  }

  int res = close(fd_clk);
  assert(res == 0);
  res = close(fd_dat);
  assert(res == 0);

  returncode = 0;
out:
  return returncode;
}
