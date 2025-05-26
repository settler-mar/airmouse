#include "led_service.h"
#include "config.h"
#include "config_storage.h"

#include <Adafruit_NeoPixel.h>
#include <ESPAsyncWebServer.h>

#include "action_runner.h"
#include "mcp_handler.h"
#include "helpers.h"
#include "base_struct.h"
#include "sleep_manager.h"

Adafruit_NeoPixel leds(NUM_WS_LEDS, PIN_WS_LED, NEO_GRB + NEO_KHZ800);
static uint32_t lastColor[NUM_WS_LEDS];

static LedStatus sources[LED_SOURCE_COUNT];
static int8_t currentSource = LED_SOURCE_NONE;
static bool isSleeping = false;
static bool isBacklightSleeping = false;
static uint32_t startTimeSource = 0;

// ----- Вспомогательные -----
void set_led_color(int index, uint32_t color)
{
  if (index < 0 || index >= NUM_WS_LEDS)
    return;
  if (lastColor[index] != color)
  {
    leds.setPixelColor(index, color);
    lastColor[index] = color;
  }
}

void leds_show()
{
  leds.show();
}

void clear_all_leds()
{
  for (int i = 0; i < NUM_WS_LEDS; ++i)
  {
    leds.setPixelColor(i, 0);
    lastColor[i] = 0;
  }
  leds.show();
}

// ----- Режим сна -----
void set_led_sleep(bool sleep)
{
  if (isSleeping == sleep)
    return;
  isSleeping = sleep;

  if (sleep)
  {
    clear_all_leds();
  }
  else
  {
    if (currentSource != LED_SOURCE_NONE)
    {
      if (LED_STATUS_INDEX >= 0)
      {
        set_led_color(LED_STATUS_INDEX, sources[currentSource].color);
        leds_show();
      }
      apply_led_layer_colors();
    }
  }
}

// ----- Подсветка в режиме сна -----
void set_led_backlight_sleep(bool sleep)
{
  if (isBacklightSleeping == sleep)
    return;
  isBacklightSleeping = sleep;

  if (sleep)
  {
    for (int i = 0; i < NUM_WS_LEDS; ++i)
    {
      if (i == LED_STATUS_INDEX)
        continue;
      leds.setPixelColor(i, 0);
      lastColor[i] = 0;
    }
    leds.show();
  }
  else
  {
    apply_led_layer_colors();
  }
}

// ----- Поиск следующего активного источника -----
void find_next_active_source()
{
  int8_t next = LED_SOURCE_NONE;

  // Сначала ищем первый с таймаутом
  for (uint8_t s = 0; s < LED_SOURCE_COUNT; ++s)
  {
    if (sources[s].color != 0 && sources[s].timeout_ms > 0)
    {
      next = s;
      resetSleepTimer();
      break;
    }
  }

  // Если не нашли — берём первый без таймаута
  if (next == LED_SOURCE_NONE)
  {
    for (uint8_t s = 0; s < LED_SOURCE_COUNT; ++s)
    {
      if (sources[s].color != 0 && sources[s].timeout_ms == 0)
      {
        next = s;
        break;
      }
    }
  }

  currentSource = next;
  startTimeSource = millis();

  if (next != LED_SOURCE_NONE)
  {
#if DEBUG
    Serial.printf("[LED] New active source: %d\n", currentSource);
#endif
  }
  else
  {
    clear_all_leds();
  }
}

// ----- Основной API -----
void set_led_status(uint32_t color, uint16_t blink_ms, uint32_t timeout_ms, bool fade, LedSource source)
{
  if (source < 0 || source >= LED_SOURCE_COUNT)
    return;

  sources[source].color = color;
  sources[source].blink_ms = blink_ms;
  sources[source].timeout_ms = timeout_ms;
  sources[source].fade = fade;
  sources[source].source = (LedSource)source;

#if DEBUG
  Serial.printf("[LED] Source %d updated: color=#%06X, blink=%d, timeout=%d, fade=%d\n",
                source, color, blink_ms, timeout_ms, fade);
#endif

  if (currentSource == LED_SOURCE_NONE)
  {
    currentSource = source;
    resetSleepTimer();
    startTimeSource = millis();
#if DEBUG
    Serial.printf("[LED] New active source: %d\n", currentSource);
#endif
  }
  else if (currentSource == source)
  {
    if (timeout_ms > 0)
    {
      startTimeSource = millis();
    }
    else
    {
      find_next_active_source();
    }
  }
  else if (sources[currentSource].timeout_ms == 0 && timeout_ms > 0)
  {
    currentSource = source;
    startTimeSource = millis();
    resetSleepTimer();
#if DEBUG
    Serial.printf("[LED] New active source: %d (prev had no timeout)\n", currentSource);
#endif
  }
}

// ----- Основной цикл -----
void led_service_loop()
{
  if (isSleeping)
    return;
  if (currentSource == LED_SOURCE_NONE)
    return;

  LedStatus &active = sources[currentSource];
  uint32_t now = millis();

  if (active.timeout_ms > 0 && now - startTimeSource >= active.timeout_ms)
  {
    find_next_active_source();
    return;
  }

  if (LED_STATUS_INDEX >= 0)
  {
    if (active.blink_ms == 0)
    {
      set_led_color(LED_STATUS_INDEX, active.color);
      leds_show();
    }
    else if (active.fade)
    {
      float phase = (now % active.blink_ms) / (float)active.blink_ms;
      float brightness = 0.5 * (1.0 - cos(phase * 2 * PI));
      uint8_t r = ((active.color >> 16) & 0xFF) * brightness;
      uint8_t g = ((active.color >> 8) & 0xFF) * brightness;
      uint8_t b = (active.color & 0xFF) * brightness;
      leds.setPixelColor(LED_STATUS_INDEX, leds.Color(r, g, b));
      leds.show();
    }
    else
    {
      static uint32_t lastToggle = 0;
      static bool ledOn = true;
      if (now - lastToggle >= active.blink_ms)
      {
        lastToggle = now;
        ledOn = !ledOn;
        leds.setPixelColor(LED_STATUS_INDEX, ledOn ? active.color : 0);
        leds.show();
      }
    }
  }

  if (!isBacklightSleeping)
  {
    apply_led_layer_colors();
  }
}

// ----- Применить цвета для слоя -----
void apply_led_layer_colors()
{
  if (isBacklightSleeping)
    return;

  byte layer = get_active_layer();
  const auto *hw = get_keys_config();
  Side side = get_mcp_active_side();
  for (size_t i = 0; i < NUM_DEFAULT_KEYS; i++)
  {
    if (hw[i].source != HARDWARE_KEY_SOURCE_MCP)
      continue;
    if (hw[i].side != side && hw[i].side != SIDE_BOTH)
      continue;

    int8_t ledIndex = get_button_colors_index(i);
    if (ledIndex < 0)
      continue;

    uint32_t color = get_button_colors(layer, i);
    set_led_color(ledIndex, color);
  }
  leds.show();
}

// ----- Инициализация -----
void setup_led_service()
{
  leds.begin();
  leds.setBrightness(LED_BRIGHTNESS);
  clear_all_leds();
}

// ----- Web API -----
void handle_led_status_api(AsyncWebServer &server)
{
  Serial.println("[LED] Registering API");

  server.on("/api/led/set", HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t, size_t)
            {
    int index = -1;
    char colorStr[10] = "#000000";
    Serial.println("[LED] Set color");

    String body = String((const char*)data).substring(0, len);
    int idxPos = body.indexOf("index=");
    int colPos = body.indexOf("color=");
    if (idxPos != -1)
    {
      int amp = body.indexOf('&', idxPos);
      String val = body.substring(idxPos + 6, amp == -1 ? len : amp);
      index = val.toInt();
    }
    if (colPos != -1)
    {
      int amp = body.indexOf('&', colPos);
      String val = body.substring(colPos + 6, amp == -1 ? len : amp);
      val.replace("%23", "#");
      strncpy(colorStr, val.c_str(), sizeof(colorStr) - 1);
      colorStr[sizeof(colorStr) - 1] = 0;
    }
    Serial.printf("[LED] Set color: index=%d, color=%s\n", index, colorStr);

    if (index < 0 || index >= NUM_WS_LEDS)
    {
      request->send(400, "text/plain", "Invalid index");
      return;
    }

    uint32_t color = parse_hex_color(colorStr);
    set_led_color(index, color);
    leds.show();

    request->send(200, "text/plain", "OK"); });

  server.on("/api/led/clear", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    clear_all_leds();
    request->send(200, "application/json", "{\"status\":\"cleared\"}"); });
}
