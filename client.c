#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <jansson.h>
#include <stdbool.h>
#include <sys/time.h>

#define CMD_LIST_ALL_POI   "apoi"
#define CMD_LIST_CLOSE_POI "cpoi"
#define CMD_SEARCH_POI     "search"
#define CMD_SHOW_POI       "show"
#define CMD_RATE           "rate"

static const unsigned int CLIENT_DEFAULT_BUFSIZ = 1024;
static double client_longitude;
static double client_latitude;
static bool first_command;

typedef enum {
  CLIENT_TYPE_TCP,
  CLIENT_TYPE_UDP
} client_type_t;

static double
time_difference(struct timeval  after, struct timeval  before) {
  double time_in_mill = 
    (after.tv_sec-before.tv_sec) * 1000.0 + (after.tv_usec-before.tv_usec) / 1000.0 ; // convert tv_sec & tv_usec to millisecond
  return time_in_mill;
}

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
set_position() {
  double lat = 1, lon = 1;

  printf("Set your position (latitude,longitude):\n");
  scanf("%lf,%lf", &lat, &lon);

  client_latitude = lat;
  client_longitude = lon;
}

static void
write_position(json_t* root) {
  json_t *lat_json = json_real(client_latitude);
  json_t *long_json = json_real(client_longitude);
  json_object_set(root, "latitude", lat_json);
  json_object_set(root, "longitude", long_json);
  json_decref(lat_json);
  json_decref(long_json);
}

static json_t*
read_command_args(char option) {
  json_t* args = json_array();
  json_t* aux = NULL;
  char name[CLIENT_DEFAULT_BUFSIZ/2];
  double rate;

  switch (option) {
    case 'f':
      printf("Name of category:\n");
      scanf(" %[^\n]s", name);
      aux = json_string(name);
      json_array_append(args, aux);
      json_decref (aux);
      break;
    case 's':
      printf("Name of the POI:\n");
      scanf(" %[^\n]s", name);
      aux = json_string(name);
      json_array_append(args, aux);
      json_decref (aux);
      break;
    case 'r':
      printf("Name of the POI to be rated:\n");
      scanf(" %[^\n]s", name);
      aux = json_string(name);
      json_array_append(args, aux);
      json_decref (aux);
      printf("Desired rate:\n");
      scanf(" %lf", &rate);
      aux = json_real(rate);
      json_array_append(args, aux);
      json_decref (aux);
      break;
  }

  return args;
}

static json_t*
basic_command_json(const client_type_t type, char option) {
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

  json_t* root = json_object();

  json_object_set(root, "command", command);
  json_decref(command);

  json_t* args = read_command_args(option);
  json_object_set(root, "args", args);
  json_decref(args);

  if (first_command) {
    first_command = false;
    needs_position = true;
  }

  if (needs_position) {
    write_position(root);
  }

  return root;
}

static void
print_command_list() {
  printf("Available commands:\n\tp - change your position\n\ta - list all poi\n\tc - list close poi\n");
  printf("\tf - search poi\n\ts - show poi\n\tr - rate poi\n\tq - quit client\n\n");
}

static void
client_loop(int socket, struct sockaddr * servaddr, client_type_t type) {
  char buf[CLIENT_DEFAULT_BUFSIZ];
  char* outbuf;
  char option = 'X';
  json_t* command;
  int n;
  struct timeval  before;
  struct timeval  after;

  set_position();
  while (option != 'q') {
    print_command_list();
    scanf(" %c",&option);
    if (option == 'q') {
      break;
    }
    if (option == 'p') {
      set_position();
      continue;
    }
    command = basic_command_json(type,option);
    if (!command) {
      printf("Option not recognized.\n");
      continue;
    }
    outbuf = json_dumps(command,JSON_COMPACT);
    gettimeofday(&before, NULL);
    sendto(socket,outbuf,strlen(outbuf)+1,0,servaddr,sizeof(servaddr));
    n = recvfrom(socket,buf,sizeof(buf)-2,0,NULL,NULL);
    gettimeofday(&after, NULL); 
    buf[n] = '\0';
    fprintf(stdout, "%s", buf);
    fprintf(stdout,"\nApproximated time elapsed since request: %.3f ms\n",time_difference(after,before));
    free(outbuf);
    json_decref(command);
  }
}

int
main(int argc, char* argv[]) {
  client_type_t server_type;
  int port, s;
  struct hostent* host;
  struct sockaddr_in sin;
  first_command = true;

  read_args(argc, argv, &host, &port, &server_type);
  sin = build_address_structure(host, port);
  s = create_socket_connection((struct sockaddr*)&sin, server_type);
  client_loop(s,(struct sockaddr*)&sin,server_type);

  return 0;
}
