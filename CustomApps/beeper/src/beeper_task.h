#pragma once

#include "beeper_private.h"

typedef struct beeper_task_t beeper_task_t;

typedef void (*beeper_task_await_is_verified_cb_t)(bool is_verified, void * user_data);

beeper_task_t * beeper_task_create(const char * path, const char * username, const char * password);
void beeper_task_destroy(beeper_task_t * t);
void beeper_task_await_is_verified(beeper_task_t * t, beeper_task_await_is_verified_cb_t cb, void * user_data);
