#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "commands.h"

#define CMD_POSITION       "position"
#define CMD_LIST_ALL_POI   "apoi"
#define CMD_LIST_CLOSE_POI "cpoi"
#define CMD_SEARCH_POI     "search"
#define CMD_SHOW_POI       "show"
#define CMD_RATE           "rate"

static char **
strsplit(const char *str, int len, char sep) {
    int parts = 1;
    for (int i = 0; i < len; i++) parts += (str[i] == sep);

    const char *s = str;
    char **ret = calloc(parts + 1, sizeof(char *));
    for (int i = 0; i < parts; i++) {
        const char *end = strchr(s, sep);
        if (!end) end = str + len - 1;
        ret[i] = strndup(s, end - s);
        s = end + 1;
    }

    return ret;
}

void
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
    char **request_parts = strsplit(request, len, ' ');
    char *command = request_parts[0];
    char *args = request_parts[1];

    char **argv = NULL;
    if (args) argv = strsplit(args, strlen(args), ',');

    if (!strcmp(command, CMD_POSITION)) {
        if (!client_set_position(client, atof(argv[0]), atof(argv[1]))) {
            return strdup("no coverage for your position!");
        }
    }

    strlist_free(request_parts);
    if (argv) strlist_free(argv);
    return strdup("ack");
}
