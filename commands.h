#pragma once

#include "client.h"

char *process_commands(client_t *client, const char *request, size_t len);
