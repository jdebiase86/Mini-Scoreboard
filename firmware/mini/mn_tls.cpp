#include "mn_tls.h"

static SemaphoreHandle_t lock = nullptr;
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

bool mnTlsTake(uint32_t waitMs) {
  portENTER_CRITICAL(&mux);
  if (!lock) lock = xSemaphoreCreateMutex();
  portEXIT_CRITICAL(&mux);
  return xSemaphoreTake(lock, pdMS_TO_TICKS(waitMs)) == pdTRUE;
}

void mnTlsGive() { xSemaphoreGive(lock); }
