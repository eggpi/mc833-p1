#include <stdio.h>
#include <sys/socket.h>

#include "server.h"

static void
on_client_accepted(int fd, const struct sockaddr *peer) {
    printf("connected!\n");
}

static void
on_incoming_data(int fd, const char *data, size_t len) {
    printf("%.*s", (int) len, data);
    send(fd, data, len, 0);
}

static void
on_client_closed(int fd, const struct sockaddr *peer) {
    printf("disconnected!\n");
}

int
main(void) {
    server_t *server = server_new(SERVER_TYPE_TCP, 8989);
    server_set_accept_callback(server, on_client_accepted);
    server_set_data_callback(server, on_incoming_data);
    server_set_close_callback(server, on_client_closed);
    server_loop(server);
    return 0;
}
