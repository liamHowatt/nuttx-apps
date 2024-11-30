#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
// #include <termios.h>
#include <poll.h>
#include <assert.h>
#include <string.h>
#include <nuttx/fs/userfs.h>

static int fdr, fdw;

static void xwrite(const void * src, int len)
{
  usleep(100000);
  int written = write(fdw, src, len);
  assert(written == len);
}

static void xread(void * dst, int len)
{
  while (len) {
    int nread;
    while (1) {
      nread = read(fdr, dst, len);
      if(nread != -1) {
        break;
      }
      usleep(1000);
    }
    dst += nread;
    len -= nread;
  }
}

static int mopen(FAR void *volinfo, FAR const char *relpath,
                        int oflags, mode_t mode, FAR void **openinfo)
{
  xwrite("o", 1);
  xwrite(oflags & O_WRONLY ? "w" : "r", 1);
  int len = strlen(relpath);
  xwrite(&len, 4);
  xwrite(relpath, len);
  char dst;
  xread(&dst, 1);
  assert(dst == 'x');
  return OK;
}

static int mclose(FAR void *volinfo, FAR void *openinfo)
{
  xwrite("c", 1);
  char dst;
  xread(&dst, 1);
  assert(dst == 'x');
  return OK;
}

static ssize_t mread(FAR void *volinfo, FAR void *openinfo,
                            FAR char *buffer, size_t buflen) {
  xwrite("r", 1);
  int total_read = 0;
  int readamt = 0;
  while(buflen) {
    readamt = buflen > 256 ? 256 : buflen;
    xwrite(&readamt, 4);
    int readact;
    xread(&readact, 4);
    xread(buffer, readact);
    total_read += readact;
    if(readact < readamt) break;
    buffer += readact;
    buflen -= readact;
  }
  readamt = 0;
  xwrite(&readamt, 4);
  return total_read;
}

static ssize_t mwrite(FAR void *volinfo, FAR void *openinfo,
                             FAR const char *buffer, size_t buflen)
{
  xwrite("w", 1);
  int orig = buflen;
  int writeamt;
  while(buflen)
  {
    writeamt = buflen > 256 ? 256 : buflen;
    xwrite(&writeamt, 4);
    xwrite(buffer, writeamt);
    buffer += writeamt;
    buflen -= writeamt;
    char dst;
    xread(&dst, 1);
    assert(dst == 'x');
  }
  writeamt = 0;
  xwrite(&writeamt, 4);
  return orig;
}

static int munlink(FAR void *volinfo, FAR const char *relpath)
{
  xwrite("d", 1);
  int len = strlen(relpath);
  xwrite(&len, 4);
  xwrite(relpath, len);
  char dst;
  xread(&dst, 1);
  assert(dst == 'x');
  return OK;
}

static int mopendir(FAR void *volinfo, FAR const char *relpath,
                           FAR void **dir)
{
  if (!relpath || relpath[0] == '\0')
    {
      xwrite("l", 1);
      return OK;
    }

  return -ENOENT;
}

static int mclosedir(FAR void *volinfo, FAR void *dir)
{
  return OK;
}

static int mreaddir(FAR void *volinfo, FAR void *dir,
                           FAR struct dirent *entry)
{
  int pathlen;
  xread(&pathlen, 4);
  if(pathlen == 0) return -ENOENT;
  entry->d_type = DTYPE_FILE;
  xread(entry->d_name, pathlen);
  entry->d_name[pathlen] = '\0';
  return OK;
}

static int mstat(FAR void *volinfo, FAR const char *relpath,
                        FAR struct stat *buf)
{
  if (*relpath == '\0')
  {
    buf->st_mode = S_IFDIR;
    return OK;
  }
  return -ENOSYS;
}

static const struct userfs_operations_s ops =
{
  mopen,
  mclose,
  mread,
  mwrite,
  NULL, // ufstest_seek,
  NULL, // ufstest_ioctl,
  NULL, // ufstest_sync,
  NULL, // ufstest_dup,
  NULL, // ufstest_fstat,
  NULL, // ufstest_truncate,
  mopendir,
  mclosedir,
  mreaddir,
  NULL, // ufstest_rewinddir,
  NULL, // ufstest_statfs,
  munlink,
  NULL, // ufstest_mkdir,
  NULL, // ufstest_rmdir,
  NULL, // ufstest_rename,
  mstat,
  NULL, // ufstest_destroy,
  NULL, // ufstest_fchstat,
  NULL, // ufstest_chstat
};

int custom_hello_main(int argc, char *argv[])
{
  printf("Hello, Custom World!! I <3 LVGL\n");

  fdw = open("/host/ttyACM0", O_WRONLY);
  printf("fdw: %d\n", fdw);
  if(fdw == -1) {
    return 1;
  }

  fdr = open("/host/reader", O_RDONLY);
  printf("fdr: %d\n", fdr);
  if(fdr == -1) {
    return 1;
  }

  xwrite("p", 1);
  char dst;
  xread(&dst, 1);
  assert(dst == 'p');
  puts("OK");

  int ret;

  ret = userfs_run("/stm", &ops, NULL, 50000);

  fprintf(stderr, "ERROR: userfs_run() returned: %d\n", ret);
  return EXIT_FAILURE;
}
