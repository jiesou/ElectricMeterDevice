#pragma once
#include "pins.h"

// Forward declaration for MultiButton
typedef struct _Button Button;

namespace button
{
    // Event handler: returns true to stop propagation
    using EventHandler = bool (*)(Button *, uint8_t, uint8_t);

    void init(void);
    void update(void);
    void addHandler(EventHandler handler);
}