#pragma once

#include <stdbool.h>

typedef int (*db_callback_t)(void *userdata, int argc, char **argv, char **cols);

bool db_init(void);
void db_close(void);
void db_run(const char *stmt, db_callback_t callback, void *userdata, ...);
