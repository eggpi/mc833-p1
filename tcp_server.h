#pragma once

#include "server.h"

void tcp_server_setup_socket(server_t *server, int port);
void tcp_server_loop(server_t *server);
