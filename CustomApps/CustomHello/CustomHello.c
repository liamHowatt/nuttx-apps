#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <olm/olm.h>

static int compar(const void * left, const void * right)
{
    const char * const * s1 = left;
    const char * const * s2 = right;
    return strcmp(*s1, *s2);
}

static void show(const char * const * data) {
    for(int i = 0; i < 5; i++) {
        puts(data[i]);
    }
}

int custom_hello_main(int argc, char *argv[])
{
    printf("pointers are %d bytes\n", (int) sizeof(void *));
    puts("");

    const char *data[5] = {"one", "two", "three", "four", "five"};

    show(data);
    puts("");
    qsort(data, 5, sizeof(const char *), compar);
    show(data);
    puts("");

    uint8_t major = 0, minor = 0, patch = 0;
    olm_get_library_version(&major, &minor, &patch);
    printf("version: %d.%d.%d\n", (int)major, (int)minor, (int)patch);

    return 0;
}
