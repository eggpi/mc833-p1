#pragma once

#include <stdbool.h>

typedef struct client client_t;

client_t *client_new(void);
void client_destroy(client_t *client);
bool client_set_position(client_t *client, double latitude, double longitude);
