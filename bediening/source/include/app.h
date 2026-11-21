#pragma once

// Top-level application: initialises subsystems and runs one non-blocking
// pass of all periodic work per app_update() call.

void app_init();
void app_update();
