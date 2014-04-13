#include <stdio.h>
#include <stdarg.h>

#include <sqlite3.h>

#include "db.h"

static sqlite3 *db;

static void sqlite_point_in_circle(sqlite3_context *context, int argc, sqlite3_value **argv);

bool
db_init(void) {
    if (!db) {
        if (sqlite3_open("places.sqlite3", &db) != SQLITE_OK) {
            return false;
        }

        return sqlite3_create_function(db, "POINT_IN_CIRCLE", 5, SQLITE_UTF8,
            NULL, &sqlite_point_in_circle, NULL, NULL);
    }

    return true;
}

void
db_close(void) {
    sqlite3_close(db);
    db = NULL;
    return;
}

void
db_run(const char *stmt, db_callback_t callback, void *userdata, ...) {
    va_list vargs;
    va_start(vargs, userdata);

    char *query = sqlite3_vmprintf(stmt, vargs);
    sqlite3_exec(db, query, callback, userdata, NULL);

    sqlite3_free(query);
    va_end(vargs);
    return;
}

static void
sqlite_point_in_circle(sqlite3_context *context, int argc, sqlite3_value **argv) {
    double cx = sqlite3_value_double(argv[0]);
    double cy = sqlite3_value_double(argv[1]);
    double r = sqlite3_value_double(argv[2]);
    double x = sqlite3_value_double(argv[3]);
    double y = sqlite3_value_double(argv[4]);

    int inside = (x - cx)*(x - cx) + (y - cy)*(y - cy) <= r*r;
    sqlite3_result_int64(context, inside);
    return;
}
