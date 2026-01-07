#pragma once
#include <stdint.h>

// Initialize horn hardware
void horn_init();

// Turn horn on or off (true = on)
void horn_set(bool on);

// Call periodically to handle auto-off and internal tasks
void horn_update();

// Return true if horn currently on
bool horn_is_on();
