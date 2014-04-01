#include <stdio.h>
#include <sqlite3.h>

#include "db.h"

static sqlite3 *db;

bool
db_init(void) {
    return db || sqlite3_open("places.sqlite3", &db) == SQLITE_OK;
}

void
db_close(void) {
    sqlite3_close(db);
    return;
}

void
db_run(const char *stmt, db_callback_t callback, void *userdata) {
    sqlite3_exec(db, stmt, callback, userdata, NULL);
    return;
}
