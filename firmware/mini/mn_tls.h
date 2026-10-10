// Only one HTTPS download at a time (each one needs about 45 KB of the board's
// memory): scores, logos and updates take turns.
#pragma once
#include <Arduino.h>
bool mnTlsTake(uint32_t waitMs);
void mnTlsGive();
