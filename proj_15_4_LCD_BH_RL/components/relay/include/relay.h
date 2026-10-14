#ifndef RELAY_H
#define RELAY_H

#include <stdbool.h>

void relay_init(int pin);
void relay_set_status(int pin, bool status);

#endif