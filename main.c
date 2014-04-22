#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "db.h"
#include "server.h"
#include "client_class.h"
#include "commands.h"

static client_t *client;

static void
on_client_accepted(int fd, const struct sockaddr *peer) {
  db_init();
  client = client_new();
}

static void
on_incoming_data(int fd, const char *data, size_t len) {
  char *request = strndup(data, len);
  char *response = process_commands(client, request);
  send(fd, response, strlen(response), 0);
  free(request);
  free(response);
}

static void
on_client_closed(int fd, const struct sockaddr *peer) {
  db_close();
  client_destroy(client);
  client = NULL;
}

static void
read_args(const int argc, char * const argv[], int* port, server_type_t* type) {
  if (argc == 2) {
    *type = SERVER_TYPE_TCP;
    *port = atoi(argv[1]);
  }
  else if (argc == 3) {
    if (argv[2][0] == 'u') {
      *type = SERVER_TYPE_UDP;
    }
    else {
      *type = SERVER_TYPE_TCP;
    }
    *port = atoi(argv[1]);
  }
  else {
    *type = SERVER_TYPE_TCP;
    *port = 8989;
  }
}

int
main(int argc, char *argv[]) {
  server_type_t server_type;
  int port;
  read_args(argc, argv, &port, &server_type);
  server_t *server = server_new(server_type, port);
  server_set_accept_callback(server, on_client_accepted);
  server_set_data_callback(server, on_incoming_data);
  server_set_close_callback(server, on_client_closed);
  server_loop(server);
  server_destroy(server);
  return 0;
}
