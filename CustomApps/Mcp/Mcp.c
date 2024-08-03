#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <nuttx/ioexpander/gpio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <nuttx/timers/timer.h>

#include "mod.h"
#include "util.h"

#include "mcp.h"

#ifdef NDEBUG
  #error assert is used a lot here. it is recommended to undef NDEBUG
#endif

#if MCP_DECLARATION_LEN != DECLARATION_LEN
  #error MCP_DECLARATION_LEN should be equal to DECLARATION_LEN
#endif

#define TIM_SIGNO SIGUSR1

struct ctx {
  int pin_fds[2];
  struct mod_command *command;
  bool delay_has_elapsed;
  uint8_t slot_idx;
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
  struct ctx *ctx2 = ctx;

  static bool boostrapped[2];
  bool ret = !boostrapped[ctx2->slot_idx];
  boostrapped[ctx2->slot_idx] = true;
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

static void decode_pin_id(const char *s, uint8_t *mod, uint8_t *pin) {
  if (
    strlen(s) != 2
    || s[0] < 'A'
    || s[0] > 'L'
    || s[1] < '0'
    || s[1] > '3'
  ) {
    return;
  }
  *mod = s[0] - 'A';
  *pin = s[1] - '0';
}

static struct mod_command g_command;

mcp_request_s mcp_g_requests[MCP_MAX_REQUESTS];

static void send_command(uint8_t idx)
{
  int res;
  static struct {
    struct mod mod;
    struct ctx ctx;
  } all;

  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, TIM_SIGNO);
  res = sigprocmask(SIG_BLOCK, &set, NULL);
  assert(res == 0);
  int tim_fd = open("/dev/timer0", O_RDONLY);
  assert(tim_fd != -1);
  res = ioctl(tim_fd, TCIOC_SETTIMEOUT, MOD_BASE_PIN_HALF_CLK_PERIOD_US);
  assert(res == 0);
  struct timer_notify_s notify;
  notify.pid      = getpid();
  notify.periodic = false;
  notify.event.sigev_notify = SIGEV_SIGNAL;
  notify.event.sigev_signo  = TIM_SIGNO;
  notify.event.sigev_value.sival_ptr = NULL;
  // res = ioctl(tim_fd, TCIOC_NOTIFICATION, (unsigned long)((uintptr_t)&notify));
  // assert(res == 0);

  char fname_buf[14];
  snprintf(fname_buf, sizeof(fname_buf), "/dev/mcp%c_clk", idx + '0');
  int fd_clk = open(fname_buf, O_RDWR);
  assert(fd_clk != -1);
  snprintf(fname_buf, sizeof(fname_buf), "/dev/mcp%c_dat", idx + '0');
  int fd_dat = open(fname_buf, O_RDWR);
  assert(fd_dat != -1);

  all.ctx.pin_fds[0] = fd_clk;
  all.ctx.pin_fds[1] = fd_dat;
  all.ctx.command = NULL;
  all.ctx.delay_has_elapsed = false;
  all.ctx.slot_idx = idx;

  mod_init(&all.mod, &interface, &all.ctx);

  all.ctx.command = &g_command;
  while (1) {
    mod_update(&all.mod);

    int blocked_by = mod_blocked_by(&all.mod);
    switch (blocked_by) {
      case mod_ModBlockOnDelay:
        // puts("1");
        res = ioctl(tim_fd, TCIOC_NOTIFICATION, (unsigned long)((uintptr_t)&notify));
        assert(res == 0);
        res = ioctl(tim_fd, TCIOC_START, 0);
        assert(res == 0);
        res = sigwaitinfo(&set, NULL);
        assert(res != -1);
        // puts("2");

        all.ctx.delay_has_elapsed = true;
        break;
      case mod_ModBlockOnNeedClkHigh:
        // puts("3");
        res = ioctl(tim_fd, TCIOC_NOTIFICATION, (unsigned long)((uintptr_t)&notify));
        assert(res == 0);
        res = ioctl(tim_fd, TCIOC_START, 0);
        assert(res == 0);
        res = sigwaitinfo(&set, NULL);
        assert(res != -1);
        // puts("4");

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

  res = close(fd_clk);
  assert(res == 0);
  res = close(fd_dat);
  assert(res == 0);
  res = close(tim_fd);
  assert(res == 0);
}

void mcp_declare(uint8_t idx, const char * declaration, uint8_t *location, bool *is_horizontal)
{
  g_command.command = CommandDeclare;
  strncpy(g_command.u.declare.declaration, declaration, DECLARATION_LEN);
  send_command(idx);
  *location = g_command.u.declare.location_response.location;
  *is_horizontal = g_command.u.declare.location_response.is_horizontal;
}

int mcp_request(uint8_t idx)
{
  static Request requests[MCP_MAX_REQUESTS];
  g_command.command = CommandRequest;
  g_command.u.request.requests_dst = requests;
  send_command(idx);
  for (int i=0; i<g_command.u.request.n_requests_result; i++) {
    mcp_g_requests[i].location = requests[i].location;
    mcp_g_requests[i].is_horizontal = requests[i].is_horizontal;
    strncpy(mcp_g_requests[i].declaration, requests[i].declaration, DECLARATION_LEN);
  }
  return g_command.u.request.n_requests_result;
}

void mcp_route(uint8_t idx, mcp_route_e op, uint8_t in_mod, uint8_t in_pin, uint8_t out_mod, uint8_t out_pin)
{
  if(op == MCP_ROUTE_E_SET) {
    g_command.u.route.route_request.op = RouteOpSetOne;
  } else if(op == MCP_ROUTE_E_CLEAR) {
    g_command.u.route.route_request.op = RouteOpClearOne;
  } else if(op == MCP_ROUTE_E_ROUTE) {
    g_command.u.route.route_request.op = RouteOpRouteIo;
  } else {
    return;
  }
  g_command.command = CommandRoute;
  g_command.u.route.route_request.input_pin = in_mod * 4 + in_pin;
  g_command.u.route.route_request.output_pin = out_mod * 4 + out_pin;
  send_command(idx);
}

void mcp_memory(uint8_t idx, bool is_read, uint8_t mod_n, uint8_t offset, uint8_t length, uint8_t *srcdst)
{
  g_command.command = CommandMemory;
  g_command.u.memory.memory_request.is_read = is_read;
  g_command.u.memory.memory_request.mod_n = mod_n;
  g_command.u.memory.memory_request.offset = offset;
  g_command.u.memory.memory_request.length = length;
  g_command.u.memory.mem_srcdst = srcdst;
  send_command(idx);
}

int mcp_main(int argc, char **argv)
{
  if(argc < 3) {
    return 1;
  }

  if(!((argv[1][0] == '0' || argv[1][0] == '1') && argv[1][1] == '\0')) {
    return 1;
  }
  uint8_t idx = argv[1][0] - '0';

  argc--;
  argv++;

  if (strcmp(argv[1], "declare") == 0) {
    if (argc != 3 || strlen(argv[2]) > DECLARATION_LEN) {
      return 1;
    }
    uint8_t location;
    bool is_horizontal;
    mcp_declare(idx, argv[2], &location, &is_horizontal);
    printf("%d %d\n", (int) location, (int) is_horizontal);
  }
  else if (strcmp(argv[1], "request") == 0) {
    if (argc != 2) {
      return 1;
    }
    int n_result = mcp_request(idx);
    for (int i=0; i<n_result; i++) {
      printf(
        "%d %d %." STRINGIFY(DECLARATION_LEN) "s\n",
        (int) mcp_g_requests[i].location,
        (int) mcp_g_requests[i].is_horizontal,
        mcp_g_requests[i].declaration
      );
    }
  }
  else if (strcmp(argv[1], "route") == 0) {
    if (argc < 3) {
      return 1;
    }
    uint8_t mod_in = 255;
    uint8_t pin_in = 255;
    uint8_t mod_out = 255;
    uint8_t pin_out = 255;
    mcp_route_e op;
    if (strcmp(argv[2], "set") == 0) {
      if (argc != 4) {
        return 1;
      }
      op = MCP_ROUTE_E_SET;
      mod_in = 0;
      pin_in = 0;
      decode_pin_id(argv[3], &mod_out, &pin_out);
    }
    else if (strcmp(argv[2], "clear") == 0) {
      if (argc != 4) {
        return 1;
      }
      op = MCP_ROUTE_E_CLEAR;
      mod_in = 0;
      pin_in = 0;
      decode_pin_id(argv[3], &mod_out, &pin_out);
    }
    else if (strcmp(argv[2], "route") == 0) {
      if (argc != 5) {
        return 1;
      }
      op = MCP_ROUTE_E_ROUTE;
      decode_pin_id(argv[3], &mod_in, &pin_in);
      decode_pin_id(argv[4], &mod_out, &pin_out);
    }
    else {
      return 1;
    }

    if(mod_in == 255 || pin_in == 255 || mod_out == 255 || pin_out == 255) {
      return 1;
    }

    mcp_route(idx, op, mod_in, pin_in, mod_out, pin_out);
  }
  else {
    return 1;
  }

  return 0;
}
