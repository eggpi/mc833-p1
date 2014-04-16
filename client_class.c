#include <stdlib.h>

#include "client_class.h"

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
    if (0 < latitude && latitude < 1000 &&
        0 < longitude && longitude < 1000) {

        client->latitude = latitude;
        client->longitude = longitude;
        return true;
    }

    return false;
}

bool
client_get_position(client_t *client, double *latitude, double *longitude) {
    if (client->latitude && client->longitude) {
        *latitude = client->latitude;
        *longitude = client->longitude;
        return true;
    }

    return false;
}
