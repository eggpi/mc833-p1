#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include "server.h"
#include "client_class.h"

static const unsigned int CLIENT_DEFAULT_BUFSIZ = 1024;

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

static struct sockaddr_in
build_address_structure(struct hostent *hp, const int port) {
  struct sockaddr_in sinaddr;
  memset((char *)&sinaddr,0, sizeof(sinaddr));
  sinaddr.sin_family = AF_INET;
  memcpy(hp->h_addr, (char *)&sinaddr.sin_addr, hp->h_length);
  sinaddr.sin_port = htons(port);
  return sinaddr;
}

static int
create_socket_connection (struct sockaddr* sin, const server_type_t type) {
  int s, sock_type;
  if (type == SERVER_TYPE_UDP) {
    sock_type = SOCK_DGRAM;
  }
  else {
    sock_type = SOCK_STREAM;
  }

  if ((s = socket(PF_INET, sock_type, 0)) < 0) {
    perror("client: socket");
    exit(1);
  }

  if (type == SERVER_TYPE_TCP) {
    if (connect(s, sin, sizeof(sin)) < 0) {
      perror("client: connect");
      close(s);
      exit(1);
    }
  }

  return s;
}

static void
client_loop(int socket, struct sockaddr * servaddr) {
  char buf[CLIENT_DEFAULT_BUFSIZ];
  int n;
  int elapsed;
  while (fgets(buf, sizeof(buf), stdin)) {
    elapsed = (int)time(NULL);
    sendto(socket,buf,strlen(buf),0,servaddr,sizeof(servaddr));
    n = recvfrom(socket,buf,sizeof(buf),0,NULL,NULL);
    elapsed = (int)time(NULL)-elapsed;
    fprintf(stdout,"\tTime elapsed since request: %d",elapsed);
    buf[n] = 0;
    fputs(buf,stdout);
  } 
}

int
main(int argc, char* argv[]) {
  server_type_t server_type;
  int port, s;
  struct hostent* host;
  struct sockaddr_in sin;

  read_args(argc, argv, &host, &port, &server_type);
  sin = build_address_structure(host, port);
  s = create_socket_connection((struct sockaddr*)&sin, server_type);
  client_loop(s,(struct sockaddr*)&sin);

  return 0;
}
