#include "../include/stepper_engine.h"

FastAccelStepperEngine& stepper_engine() {
    static FastAccelStepperEngine engine;
    static bool initialized = false;
    if (!initialized) {
        engine.init();
        initialized = true;
    }
    return engine;
}
