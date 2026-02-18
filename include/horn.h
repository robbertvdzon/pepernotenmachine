#pragma once
#include <stdint.h>

// Initialize horn hardware
void horn_init();

// Turn horn on or off (true = on)
void horn_set(bool on);

// Return true if horn currently on
bool horn_is_on();
