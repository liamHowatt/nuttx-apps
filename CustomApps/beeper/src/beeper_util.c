#include "beeper_private.h"

char * beeper_read_text_file(const char * path)
{
    int fd = open(path, O_RDONLY);
    if(fd == -1) {
        int en = errno;
        assert(en == ENOENT
               || en == EBADF); /* host_fs sets this instead */
        return NULL;
    }
    struct stat statbuf;
    int res = fstat(fd, &statbuf);
    assert(res == 0);
    ssize_t sz = statbuf.st_size;
    char * buf = malloc(sz + 1);
    assert(buf);
    buf[sz] = '\0';
    ssize_t br = read(fd, buf, sz);
    assert(br == sz);
    res = close(fd);
    assert(res == 0);
    return buf;
}
