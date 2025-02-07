#pragma once

#include <beeper/beeper.h>

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

/* beeper_util.c */
typedef struct {
    size_t item_size;
    size_t len;
    size_t cap;
    void * data;
} beeper_queue_t;
char * beeper_read_text_file(const char * path);
void beeper_queue_init(beeper_queue_t * bq, size_t item_size);
void beeper_queue_destroy(beeper_queue_t * bq);
void beeper_queue_push(beeper_queue_t * bq, const void * to_push);
bool beeper_queue_pop(beeper_queue_t * bq, void * pop_dst);
bool beeper_queue_is_empty(beeper_queue_t * bq);
