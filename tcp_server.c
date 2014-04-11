#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>

#include "server_internal.h"

static void
on_child_exit(int signal) {
    wait(NULL);
}

void
tcp_server_setup_socket(server_t *server, int port) {
    int s;
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        return;
    }

    struct sockaddr_in sin = {0};
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);

    if (bind(s, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
        return;
    }

    if (listen(s, SOMAXCONN) < 0) {
        return;
    }

    struct sigaction action = {0};
    action.sa_handler = on_child_exit;
    action.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &action, NULL);

    server->listen_fd = s;
    return;
}

void
tcp_server_loop(server_t *server) {
    while (true) {
        int conn_fd;
        struct sockaddr_in peer = {0};
        socklen_t peer_len = sizeof(peer);

        conn_fd = accept(server->listen_fd, (struct sockaddr *) &peer,
                         &peer_len);
        if (conn_fd < 0) {
            return;
        }

        if (fork() == 0) {
            // child
            size_t rd;
            char buf[server->bufsiz];

            server->on_accept(conn_fd, (struct sockaddr *) &peer);
            while ((rd = recv(conn_fd, buf, sizeof(buf), 0)) > 0) {
                server->on_data(conn_fd, buf, rd);
            }
            server->on_close(conn_fd, (struct sockaddr *) &peer);

            close(conn_fd);
            server_destroy(server);
            exit(0);
        }

        // parent
        close(conn_fd);
    }
}
