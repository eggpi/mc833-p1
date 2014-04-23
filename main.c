#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "db.h"
#include "server.h"
#include "client_class.h"
#include "commands.h"

#define REQUEST_BUFFER_SIZE 1024

static client_t *client;
static char *rbuf;
static unsigned int rbufsize;
static char request_buffer[REQUEST_BUFFER_SIZE];

static void
on_client_accepted(int fd, const struct sockaddr *peer) {
  db_init();
  client = client_new();

  memset(request_buffer, '\0', REQUEST_BUFFER_SIZE);
  rbuf = request_buffer;
  rbufsize = REQUEST_BUFFER_SIZE;
}

static int
on_incoming_data(int fd, const char *data, size_t len) {
  size_t to_copy = (len > rbufsize) ? rbufsize : len;
  memcpy(rbuf, data, to_copy);
  rbuf += to_copy;
  rbufsize -= to_copy;

  const char *request_end;
  for (request_end = request_buffer; request_end < rbuf; request_end++) {
      if (*request_end == '\0') break;
  }

  if (*request_end != '\0') {
    // drop the connection if we filled the buffer but
    // haven't seen a request, and keep going otherwise.
    return rbufsize > 0;
  }

  char *request = strndup(request_buffer, request_end - request_buffer);
  rbuf = request_buffer;
  rbufsize = REQUEST_BUFFER_SIZE;

  struct timeval  before;
  struct timeval  after;
  gettimeofday(&before, NULL);
  char *response = process_commands(client, request);
  gettimeofday(&after, NULL);

  double time_in_mill = 
    (after.tv_sec-before.tv_sec) * 1000.0 + (after.tv_usec-before.tv_usec) / 1000.0 ;
  fprintf(stdout,"Time spent with request '%s': %.3f ms\n",request,time_in_mill);

  send(fd, response, strlen(response) + 1 /* send \0 as well */, 0);
  free(request);
  free(response);
  return 1;
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
