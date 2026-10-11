#pragma once
#include <FastAccelStepper.h>

// One FastAccelStepperEngine shared by all steppers (pull motor + dispenser).
// The engine is initialised on first use, so the init order in setup() doesn't matter.
FastAccelStepperEngine& stepper_engine();
