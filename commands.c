#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>

#include <jansson.h>

#include "db.h"
#include "commands.h"

#define CMD_LIST_ALL_POI   "apoi"
#define CMD_LIST_CLOSE_POI "cpoi"
#define CMD_SEARCH_POI     "search"
#define CMD_SHOW_POI       "show"
#define CMD_RATE           "rate"

static char *cmd_list_all_poi(void);
static char *cmd_show_poi(json_t *args);
static char *cmd_rate_poi(json_t *args);
static char *cmd_list_close_poi(client_t *client, json_t *args);
static char *cmd_search_poi(client_t *client, json_t *args);

static int make_list_from_cols(void *userdata, int argc, char **argv, char **cols);
static int make_object_from_cols(void *userdata, int argc, char **argv, char **cols);
static int make_array_of_object_from_cols(void *userdata, int argc, char **argv, char **cols);

char *
process_commands(client_t *client, const char *request) {
    const char *command;
    json_t *args = NULL;
    char *retval = NULL;
    double latitude = 0, longitude = 0;

    json_t *jreq = json_loads(request, 0, NULL);
    if (!request) {
        retval = strdup("bad request. 1");
        goto bad_request;
    }

    int ok;
    ok = json_unpack(jreq, "{s?:f, s?:f, s:s, s:o}",
            "latitude", &latitude, "longitude", &longitude,
            "command", &command, "args", &args);
    if (ok < 0) {
        retval = strdup("bad request. 2");
        goto bad_request;
    }

    if (latitude || longitude) {
        if (!client_set_position(client, latitude, longitude)) {
            retval = strdup("no coverage for your position.");
            goto bad_request;
        }
    }

    if (!client_get_position(client, &latitude, &longitude)) {
        retval = strdup("client has no position.");
        goto bad_request;
    }

    if (!strcmp(command, CMD_LIST_ALL_POI)) {
        retval = cmd_list_all_poi();
    } else if (!strcmp(command, CMD_SHOW_POI)) {
        retval = cmd_show_poi(args);
    } else if (!strcmp(command, CMD_RATE)) {
        retval = cmd_rate_poi(args);
    } else if (!strcmp(command, CMD_LIST_CLOSE_POI)) {
        retval = cmd_list_close_poi(client, args);
    } else if (!strcmp(command, CMD_SEARCH_POI)) {
        retval = cmd_search_poi(client, args);
    } else {
        retval = strdup("invalid command");
    }

bad_request:
    // no need to decref args, as we the 'o' format
    // specifier doesn't incref
    if (jreq) json_decref(jreq);

    return retval;
}

static int
make_list_from_cols(void *userdata, int argc, char **argv, char **cols) {
    assert(argc == 1);

    json_t *result = (json_t *) userdata;
    json_t *coltext = json_string(argv[0]);

    json_array_append(result, coltext);
    json_decref(coltext);
    return 0;
}

static char *
cmd_list_all_poi(void) {
    json_t *list = json_array();
    db_run("select name from places;", make_list_from_cols, list);

    char *retval = json_dumps(list, JSON_INDENT(2));
    json_decref(list);
    return retval;
}

static int
make_object_from_cols(void *userdata, int argc, char **argv, char **cols) {
    json_t *result = (json_t *) userdata;

    for (int i = 0; i < argc; i++) {
        json_t *value = json_string(argv[i]);
        json_object_set(result, cols[i], value);
        json_decref(value);
    }

    return 0;
}

static char *
cmd_show_poi(json_t *args) {
    const char *poi;
    if (json_unpack(args, "[s]", &poi) < 0) {
        return strdup("bad arguments.");
    }

    json_t *object = json_object();
    db_run("select * from places where name = %Q",
            make_object_from_cols, object, poi);

    char *retval = json_dumps(object, JSON_INDENT(2));
    json_decref(object);
    return retval;
}

static char *
cmd_rate_poi(json_t *args) {
    const char *poi;
    double rating;
    char *rating_str = NULL;

    if (json_unpack(args, "[s, f]", &poi, &rating) < 0) {
        return strdup("bad arguments.");
    }

    // the db expects rating to be a string, so normalize it now
    // that we know for sure it's a double
    asprintf(&rating_str, "%.2f", rating);
    db_run("update places set rating = %Q where name = %Q", NULL, NULL,
            rating_str, poi);
    free(rating_str);

    // remove rating so we only send the poi to cmd_show_poi
    json_array_remove(args, 1);
    return cmd_show_poi(args);
}

static int
make_array_of_object_from_cols(void *userdata, int argc, char **argv, char **cols) {
    json_t *object = json_object();
    make_object_from_cols(object, argc, argv, cols);

    json_t *array = (json_t *) userdata;
    json_array_append(array, object);
    json_decref(object);
    return 0;
}

static char *
cmd_list_close_poi(client_t *client, json_t *args) {
    json_t *array = json_array();
    double latitude, longitude;
    char *latitude_str, *longitude_str;

    if (!client_get_position(client, &latitude, &longitude)) {
        return strdup("client has no position.");
    }

    asprintf(&latitude_str, "%.2f", latitude);
    asprintf(&longitude_str, "%.2f", latitude);

    db_run("select * from places where "
           "POINT_IN_CIRCLE(%Q, %Q, %Q, latitude, longitude) = 1;",
           make_array_of_object_from_cols, array, latitude_str, longitude_str,
           "100");

    free(latitude_str);
    free(longitude_str);

    char *retval = json_dumps(array, JSON_INDENT(2));
    json_decref(array);
    return retval;
}

static char *
cmd_search_poi(client_t *client, json_t *args) {
    json_t *array = json_array();
    double latitude, longitude;
    char *latitude_str, *longitude_str;
    const char *category;

    if (!client_get_position(client, &latitude, &longitude)) {
        return strdup("client has no position.");
    }

    if (json_unpack(args, "[s]", &category) < 0) {
        return strdup("bad arguments.");
    }

    asprintf(&latitude_str, "%.2f", latitude);
    asprintf(&longitude_str, "%.2f", latitude);

    db_run("select * from places where "
           "POINT_IN_CIRCLE(%Q, %Q, %Q, latitude, longitude) = 1 and "
           "category = %Q;",
           make_array_of_object_from_cols, array, latitude_str, longitude_str,
           "100", category);

    free(latitude_str);
    free(longitude_str);

    char *retval = json_dumps(array, JSON_INDENT(2));
    json_decref(array);
    return retval;
}
