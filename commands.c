#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>

#include <jansson.h>

#include "db.h"
#include "commands.h"

#define CMD_POSITION       "position"
#define CMD_LIST_ALL_POI   "apoi"
#define CMD_LIST_CLOSE_POI "cpoi"
#define CMD_SEARCH_POI     "search"
#define CMD_SHOW_POI       "show"
#define CMD_RATE           "rate"

static char *cmd_list_all_poi(void);
static char *cmd_show_poi(const char *poi);
static char *cmd_rate_poi(const char *poi, const char *rating);

static char **strsplit(const char *str, char sep, int maxparts);
static char *strstrip(const char *str, int len);
static void strlist_free(char **strlist);
static int make_list_from_cols(void *userdata, int argc, char **argv, char **cols);

// XXX this also strips each part of the string after splitting
static char **
strsplit(const char *str, char sep, int maxparts) {
    int parts = 1;
    for (const char *s = str; *s && (maxparts < 0 || parts < maxparts); s++) {
        parts += (*s == sep);
    }

    const char *s = str;
    char **ret = calloc(parts + 1, sizeof(char *));
    for (int i = 0; i < parts; i++) {
        const char *end = strchr(s, sep);
        if (!end || i == maxparts - 1) end = s + strlen(s);
        ret[i] = strstrip(s, end - s);
        s = end + 1;
    }

    return ret;
}

static char *
strstrip(const char *str, int len) {
    const char *start = str;
    const char *end = str + len - 1;

    while (isspace(*start)) start++;
    while (end > start && isspace(*end)) end--;

    return strndup(start, end - start + 1);
}

static void
strlist_free(char **strlist) {
    int i = 0;
    while (true) {
       if (strlist[i]) {
           free(strlist[i]);
       } else break;

       i++;
    }
}

char *
process_commands(client_t *client, const char *request) {
    char *retval = NULL;
    char **request_parts = strsplit(request, ' ', 2);
    char *command = request_parts[0];
    char *args = request_parts[1];

    char **argv = NULL;
    if (args) argv = strsplit(args, ',', -1);

    if (!strcmp(command, CMD_POSITION)) {
        if (!client_set_position(client, atof(argv[0]), atof(argv[1]))) {
            return strdup("no coverage for your position!");
        }
    } else if (!strcmp(command, CMD_LIST_ALL_POI)) {
        retval = cmd_list_all_poi();
    } else if (!strcmp(command, CMD_SHOW_POI)) {
        if (!argv) retval = strdup("missing argument");
        else retval = cmd_show_poi(argv[0]);
    } else if (!strcmp(command, CMD_RATE)) {
        if (!argv || !argv[1]) retval = strdup("missing argument");
        else retval = cmd_rate_poi(argv[0], argv[1]);
    } else {
        retval = strdup("invalid command");
    }

    strlist_free(request_parts);
    if (argv) strlist_free(argv);
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

    char *retval = json_dumps(list, 0);
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
cmd_show_poi(const char *poi) {
    json_t *object = json_object();
    db_run("select * from places where name = %Q",
            make_object_from_cols, object, poi);

    char *retval = json_dumps(object, 0);
    json_decref(object);
    return retval;
}

static char *
cmd_rate_poi(const char *poi, const char *rating) {
    db_run("update places set rating = %Q where name = %Q", NULL, NULL, rating,
            poi);
    return cmd_show_poi(poi);
}
