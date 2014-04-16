#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include "server.h"
#include "client_class.h"

static client_t client;
static struct  hostent *host;

static void
read_args(const int argc, char * const argv[],
    struct hostent** ip, int* port, server_type_t* type) {
  if (argc > 1) {
    *ip = gethostbyname(argv[1]);
  }
  else {
    *ip = gethostbyname("localhost");
  }
  if (!*ip) {
    fprintf(stderr, "client: unknown host: %s\n", argv[1]);
    exit(1);
  }

  if (argc > 2) {
    *port = atoi(argv[2]);
  }
  else {
    *port = 8989;
  }

  if (argc > 3 && argv[3][0] == 'u') {
    *type = SERVER_TYPE_UDP;
  }
  else {
    *type = SERVER_TYPE_TCP;
  }
}

