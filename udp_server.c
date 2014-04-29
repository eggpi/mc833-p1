#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "server_internal.h"

void
udp_server_setup_socket(server_t *server, int port) {
    int s;
    if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        return;
    }

    struct sockaddr_in sin = {0};
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);

    if (bind(s, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
        return;
    }

    server->listen_fd = s;
    return;
}

void
udp_server_loop(server_t *server) {
    while (true) {
        size_t rd;
        char buf[server->bufsiz];
        struct sockaddr_in peer = {0};
        socklen_t peer_len = sizeof(peer);

        rd = recvfrom(server->listen_fd, buf, server->bufsiz, 0,
                      (struct sockaddr *) &peer, &peer_len);
        connect(server->listen_fd, (struct sockaddr *) &peer, peer_len);
        server->on_accept(server->listen_fd, (struct sockaddr *) &peer);
        server->on_data(server->listen_fd, buf, rd);
        server->on_close(server->listen_fd, (struct sockaddr *) &peer);

        memset(&peer, 0, sizeof(peer));
        peer.sin_family = AF_UNSPEC;
        connect(server->listen_fd, (struct sockaddr *) &peer, peer_len);
    }
}
