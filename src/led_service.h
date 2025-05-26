#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "base_struct.h"
#include "config.h"

// Макрос для определения статуса
#define LED_STATUS(color, blink, timeout, fade, priority) set_led_status(color, blink, timeout, fade, priority)

// ----- Структура статуса -----
struct LedStatus
{
  uint32_t color;
  uint16_t blink_ms = 0;
  uint32_t timeout_ms = 0;
  bool fade = false;
  LedSource source;
};

// ----- Основное управление -----
void setup_led_service();
void led_service_loop();

// ----- Управление сном -----
void set_led_sleep(bool sleep);
void set_led_backlight_sleep(bool sleep);

// ----- Работа со статусами -----
void set_led_status(uint32_t color, uint16_t blink_ms = 0, uint32_t timeout_ms = 0, bool fade = false, LedSource source = LED_SOURCE_NONE);

// ----- Работа с лентой -----
void clear_all_leds();
void leds_show();
void set_led_color(int index, uint32_t color);
void apply_led_layer_colors();

// ----- API для Web -----
void handle_led_status_api(AsyncWebServer &server);
