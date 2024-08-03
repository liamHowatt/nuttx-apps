#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>


int testing_main(int argc, char *argv[])
{
  int res;
  sigset_t set;

  puts("This is the testing app");

  sigemptyset(&set);
  sigaddset(&set, SIGUSR1);

  res = sigprocmask(SIG_BLOCK, &set, NULL);
  assert(res != -1);

  while(1) {
    puts("not waiting");
    sleep(3);
    puts("waiting");
    res = sigwaitinfo(&set, NULL);
    assert(res != -1);
  }

  return 0;
}
