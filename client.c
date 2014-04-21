#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <jansson.h>
#include <stdbool.h>

#define CMD_LIST_ALL_POI   "apoi"
#define CMD_LIST_CLOSE_POI "cpoi"
#define CMD_SEARCH_POI     "search"
#define CMD_SHOW_POI       "show"
#define CMD_RATE           "rate"

static const unsigned int CLIENT_DEFAULT_BUFSIZ = 1024;
static double client_longitude;
static double client_latitude;
static bool position_set;

typedef enum {
  CLIENT_TYPE_TCP,
  CLIENT_TYPE_UDP
} client_type_t;

static void
read_args(const int argc, char * const argv[],
    struct hostent** ip, int* port, client_type_t* type) {
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
    *type = CLIENT_TYPE_UDP;
  }
  else {
    *type = CLIENT_TYPE_TCP;
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
create_socket_connection (struct sockaddr* sin, const client_type_t type) {
  int s, sock_type;
  if (type == CLIENT_TYPE_UDP) {
    sock_type = SOCK_DGRAM;
  }
  else {
    sock_type = SOCK_STREAM;
  }

  if ((s = socket(PF_INET, sock_type, 0)) < 0) {
    perror("client: socket");
    exit(1);
  }

  if (type == CLIENT_TYPE_TCP) {
    if (connect(s, sin, sizeof(*sin)) < 0) {
      perror("client: connect");
      close(s);
      exit(1);
    }
  }

  return s;
}

static void
command_to_json(char buf[], int bufsiz) {
  char json[bufsiz];
  //char *pch;
  strcpy(json,buf);
  //pch = strtok (buf," ,");

}

static void
set_position(double latitude, double longitude) {
  client_latitude = latitude;
  client_longitude = longitude;
  position_set = true;
}

static bool
write_position(json_t* root) {
  if (!position_set) return false;

  json_t *lat_json = json_real(client_latitude);
  json_t *long_json = json_real(client_longitude);
  json_object_set(root, "latitude", lat_json);
  json_object_set(root, "longitude", long_json);
  json_decref(lat_json);
  json_decref(long_json); 

  return true;
}

static json_t*
basic_command_json(const client_type_t type, char option) {
  json_t* root = json_object();
  bool needs_position = (type == CLIENT_TYPE_UDP);

  json_t* command = NULL;
  switch (option) {
    case 'a':
      command = json_string(CMD_LIST_ALL_POI);
      break;
    case 'c':
      command = json_string(CMD_LIST_CLOSE_POI);
      needs_position = true;
      break;
    case 'f':
      command = json_string(CMD_SEARCH_POI);
      needs_position = true;
      break;
    case 's':
      command = json_string(CMD_SHOW_POI);
      break;
    case 'r':
      command = json_string(CMD_RATE);
      break;
  }
  if (!command) {
    return NULL;
  }

  if (needs_position) {
    if(!write_position(root)) {
      return NULL;
    }
  }

  return root;
}

static void
client_loop(int socket, struct sockaddr * servaddr) {
  char buf[CLIENT_DEFAULT_BUFSIZ];
  int n;
  int elapsed;
  while (fgets(buf, sizeof(buf), stdin)) {
    elapsed = (int)time(NULL);
    command_to_json(buf,sizeof(buf));
    sendto(socket,buf,strlen(buf),0,servaddr,sizeof(servaddr));
    n = recvfrom(socket,buf,sizeof(buf),0,NULL,NULL);
    elapsed = (int)time(NULL)-elapsed;
    buf[n] = '\n';
    fputs(buf,stdout);
    fprintf(stdout,"\tTime elapsed since request: %d\n",elapsed);
  } 
}

int
main(int argc, char* argv[]) {
  client_type_t server_type;
  int port, s;
  struct hostent* host;
  struct sockaddr_in sin;
  position_set = false;

  read_args(argc, argv, &host, &port, &server_type);
  sin = build_address_structure(host, port);
  s = create_socket_connection((struct sockaddr*)&sin, server_type);
  client_loop(s,(struct sockaddr*)&sin);

  return 0;
}
