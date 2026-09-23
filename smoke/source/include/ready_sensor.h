#ifndef READY_SENSOR_H
#define READY_SENSOR_H

using ReadySensorCallback = void (*)(bool);

void ready_sensor_setup(ReadySensorCallback callback);

#endif
