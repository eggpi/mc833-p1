#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "db.h"
#include "server.h"
#include "client.h"
#include "commands.h"

static client_t *client;

static void
on_client_accepted(int fd, const struct sockaddr *peer) {
    db_init();
    client = client_new();
}

static void
on_incoming_data(int fd, const char *data, size_t len) {
    char *response = process_commands(client, data, len);
    send(fd, response, strlen(response), 0);
    free(response);
}

static void
on_client_closed(int fd, const struct sockaddr *peer) {
    db_close();
    client_destroy(client);
    client = NULL;
}

int
main(void) {
    server_t *server = server_new(SERVER_TYPE_TCP, 8989);
    server_set_accept_callback(server, on_client_accepted);
    server_set_data_callback(server, on_incoming_data);
    server_set_close_callback(server, on_client_closed);
    server_loop(server);
    server_destroy(server);
    return 0;
}
