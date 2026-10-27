#pragma once

// BLE identity constants for the two remote devices.
// Service UUIDs and advertised names come from the launcher's include/config.h
// (SERVICE_UUID) and the switchboard README. Characteristic UUIDs are added in
// the connection stage, from the device sources, not reconstructed from suffixes.

constexpr const char* LAUNCHER_ADV_NAME = "Peppernut launcher";
constexpr const char* SWITCHBOARD_ADV_NAME = "IO Switchboard";

#define LAUNCHER_SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define SWITCHBOARD_SERVICE_UUID "3dc93769-33d3-4883-ae7b-877c14fd3eca"
