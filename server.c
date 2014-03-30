#include <unistd.h>
#include <stdlib.h>

#include "server_internal.h"

static const unsigned int SERVER_DEFAULT_BUFSIZ = 1024;

#define GEN_SET_CALLBACK_FUNCTION(event)                              \
void                                                                  \
server_set_##event##_callback(server_t *server,                       \
                              server_##event##_callback_t callback) { \
   server->on_##event = callback;                                     \
}                                                                     \

GEN_SET_CALLBACK_FUNCTION(accept)
GEN_SET_CALLBACK_FUNCTION(data)
GEN_SET_CALLBACK_FUNCTION(close)

#define DISPATCH_METHOD_TO_SERVER(method, ...)  \
    (server->type == SERVER_TYPE_TCP) ?                 \
        tcp_server_##method(__VA_ARGS__) :    \
        udp_server_##method(__VA_ARGS__)      \

server_t *
server_new(server_type_t type, int port) {
    server_t *server = calloc(1, sizeof(server_t));
    server->type = type;
    server->bufsiz = SERVER_DEFAULT_BUFSIZ;

    DISPATCH_METHOD_TO_SERVER(setup_socket, server, port);
    return server;
}

void
server_destroy(server_t *server) {
    close(server->listen_fd);
    free(server);
}

void
server_loop(server_t *server) {
    DISPATCH_METHOD_TO_SERVER(loop, server);
    return;
}
