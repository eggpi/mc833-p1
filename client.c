#include <stdlib.h>

#include "client.h"

struct client {
    double latitude, longitude;
};

client_t *
client_new(void) {
    return calloc(1, sizeof(client_t));
}

void
client_destroy(client_t *client) {
    free(client);
}

bool
client_set_position(client_t *client, double latitude, double longitude) {
    if (latitude && longitude) {
        client->latitude = latitude;
        client->longitude = longitude;
        return true;
    }

    return false;
}
