#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
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

static char **strsplit(const char *str, int len, char sep, int maxparts);
static void strlist_free(char **strlist);
static int make_list_from_cols(void *userdata, int argc, char **argv, char **cols);

static char **
strsplit(const char *str, int len, char sep, int maxparts) {
    int parts = 1;
    for (int i = 0; i < len && (maxparts < 0 || parts < maxparts); i++) {
        parts += (str[i] == sep);
    }

    const char *s = str;
    char **ret = calloc(parts + 1, sizeof(char *));
    for (int i = 0; i < parts; i++) {
        const char *end = strchr(s, sep);
        if (!end || i == maxparts - 1) end = str + len;
        ret[i] = strndup(s, end - s);
        s = end + 1;
    }

    return ret;
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
process_commands(client_t *client, const char *request, size_t len) {
    char **request_parts = strsplit(request, len, ' ', 2);
    char *command = request_parts[0];
    char *args = request_parts[1];

    char **argv = NULL;
    if (args) argv = strsplit(args, strlen(args), ',', -1);

    if (!strcmp(command, CMD_POSITION)) {
        if (!client_set_position(client, atof(argv[0]), atof(argv[1]))) {
            return strdup("no coverage for your position!");
        }
    } else if (!strcmp(command, CMD_LIST_ALL_POI)) {
        return cmd_list_all_poi();
    } else if (!strcmp(command, CMD_SHOW_POI)) {
        if (!argv) return strdup("missing argument");
        return cmd_show_poi(argv[0]);
    }

    strlist_free(request_parts);
    if (argv) strlist_free(argv);
    return strdup("ack");
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
    db_run("select name, latitude, longitude from places where name = %Q",
            make_object_from_cols, object, poi);

    char *retval = json_dumps(object, 0);
    json_decref(object);
    return retval;
}
