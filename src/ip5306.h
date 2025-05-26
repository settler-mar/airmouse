#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

// ================== Функции управления ==================
void ip5306_init();
void ip5306_poll();
void ip5306_update_status();
void ip5306_loop();

void power_off();
void setSleepEnabled(bool enabled);
void handle_power_status_api(AsyncWebServer &server);
