#include <stdio.h>
#include <sched.h>

int custom_hello_main(int argc, char *argv[])
{
  printf("Hello, Custom World!! I <3 LVGL\n");

  int this_policy = sched_getscheduler(0);
  printf("%d %d\n", sched_get_priority_min(this_policy), sched_get_priority_max(this_policy));
  
  return 0;
}
