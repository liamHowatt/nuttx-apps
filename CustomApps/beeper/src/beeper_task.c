#include "beeper_task_private.h"

// static char * fmt_device_id_path(const char * upath)
// {
//     char * device_id_path;
//     res = asprintf(&device_id_path, "%sdevice_id", upath);
//     assert(res != -1);
//     return device_id_path;
// }

// static void device_id_cb(const char * device_id, void * user_data)
// {
//     char * device_id_path = fmt_device_id_path(c->upath);
//     int fd = open(device_id_path, O_WRONLY | O_EXCL | O_CREAT, 0644);
//     assert(fd != -1);
//     free(device_id_path);

//     ssize_t device_id_len = strlen(device_id)
//     ssize_t bw = write(fd, device_id, device_id_len);
//     assert(bw == device_id_len);

//     res = close(fd);
//     assert(res == 0);
// }

beeper_task_t * beeper_task_create(const char * path, const char * username, const char * password)
{
    int res;

    beeper_task_t * t = malloc(sizeof(beeper_task_t));
    assert(t);

    res = asprintf(&t->upath, "%s%s/", path, username);
    assert(res != -1);
    res = mkdir(t->upath, 0755);
    assert(res == 0 || errno == EEXIST);

    return t;
}

void beeper_task_destroy(beeper_task_t * t)
{
    if(!t) return;
    free(t->upath);
    free(t);
}

void beeper_task_await_is_verified(beeper_task_t * t, beeper_task_await_is_verified_cb_t cb, void * user_data)
{
    cb(true, user_data);
}
