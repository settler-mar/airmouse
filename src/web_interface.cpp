// web_interface.cpp - реализация веб-интерфейса с ESPAsyncWebServer
#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include "web_interface.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "config.h"
#include "config_storage.h"
#include <LittleFS.h>
#include "ble.h"
#include "action_runner.h"
#include "mode_manager.h"
#include "ir_service.h"
#include "helpers.h"
#include "led_service.h"
#include "ip5306.h"

AsyncWebServer server(80);
bool wifi_enabled = false;
bool wifi_connected = false;
bool wifi_waiting_connect = false;

void wifi_start()
{
  if (wifi_enabled)
    return;

  ble_stop();
  if (!get_wifi_mode())
  {
    WiFi.mode(WIFI_STA);
    WiFi.begin(get_wifi_ssid().c_str(), get_wifi_password().c_str());
#if DEBUG
    Serial.printf("[WIFI] STA start: SSID=%s\n", get_wifi_ssid().c_str());
#endif
    wifi_waiting_connect = true;
    wifi_connected = false;
    LED_STATUS_WIFI_CONNECTING;
  }
  else
  {
    LED_STATUS_WIFI_AP_START;
    WiFi.mode(WIFI_AP);
    WiFi.softAP(get_wifi_ssid().c_str(), get_wifi_password().c_str());
#if DEBUG
    Serial.printf("[WIFI] AP started: SSID=%s IP=%s\n", get_wifi_ssid().c_str(), WiFi.softAPIP().toString().c_str());
#endif
  }

  wifi_enabled = true;
  setup_web_interface();
}

void wifi_stop()
{
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  server.end();
  ble_start();
  wifi_enabled = false;
#if DEBUG
  Serial.println("[WIFI] Wi-Fi OFF");
#endif
}

void wifi_target()
{
  if (wifi_enabled)
  {
    wifi_stop();
  }
  else
  {
    wifi_start();
  }
}

void serveCompressed(AsyncWebServerRequest *request, const char *path, const char *mime)
{
  String gzPath = String(path) + ".gz";
  if (LittleFS.exists(gzPath))
  {
    AsyncWebServerResponse *response = request->beginResponse(LittleFS, gzPath, mime);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  }
  else
  {
    request->send(LittleFS, path, mime);
  }
}

void setup_web_interface()
{
#if DEBUG
  Serial.println("[WEB] Starting web server");
#endif

  if (!LittleFS.begin(true))
  {
#if DEBUG
    Serial.println("[WEB] Failed to mount LittleFS");
#endif
    return;
  }

  // Главная страница
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { serveCompressed(request, "/index.html", "text/html"); });

  // Captive portal support
  server.onNotFound([](AsyncWebServerRequest *request)
                    { request->redirect("/"); });
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->redirect("/"); });

  // API: информация о контроллере
  server.on("/api/info", HTTP_GET, [](AsyncWebServerRequest *request)
            {
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  String json = "{";
  json += "\"layers\":" + String(MAX_LAYERS) + ",";
  json += "\"keys\":" + String(NUM_DEFAULT_KEYS) + ",";
  json += "\"ir_slots\":" + String(MAX_IR_CODES) + ",";
  json += "\"chip\":\"ESP32\",";
  json += "\"chip_model\":\"" + String((chip_info.model == CHIP_ESP32 ? "ESP32" :
                                        chip_info.model == CHIP_ESP32S2 ? "ESP32-S2" :
                                        chip_info.model == CHIP_ESP32S3 ? "ESP32-S3" :
                                        chip_info.model == CHIP_ESP32C3 ? "ESP32-C3" :
                                        "UNKNOWN")) + "\",";
  json += "\"cores\":" + String(chip_info.cores) + ",";
  json += "\"revision\":" + String(chip_info.revision) + ",";
  json += "\"flash_size\":" + String(spi_flash_get_chip_size() / 1024) + ",";
  // json += "\"flash_speed\":" + String(spi_flash_get_speed() / 1000000) + ",";
  json += "\"heap_free\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"heap_total\":" + String(ESP.getHeapSize()) + ",";
  json += "\"sketch_size\":" + String(ESP.getSketchSize()) + ",";
  json += "\"sketch_free_space\":" + String(ESP.getFreeSketchSpace()) + ",";
  json += "\"build\":\"" __DATE__ " " __TIME__ "\"";
  json += ",\"leds_count\":" + String(NUM_WS_LEDS) + ",";
  json += "\"leds_brightness\":" + String(LED_BRIGHTNESS) + ",";
  json += "\"status_led_index\":\"" + String(LED_STATUS_INDEX) + "\"";
  json += "}";

  request->send(200, "application/json", json); });

  // API: получить Wi-Fi конфиг
  server.on("/api/wifi", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String json = "{";
    json += "\"ssid\":\"" + get_wifi_ssid() + "\"";
    json += ",\"password\":\"" + get_wifi_password() + "\"";
    json += ",\"mode\":" + String(get_wifi_mode());
    json += "}";
    request->send(200, "application/json", json); });

  // API: сохранить Wi-Fi конфиг
  server.on("/api/wifi", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    if (!request->hasParam("ssid", true) || !request->hasParam("password", true) || !request->hasParam("mode", true)) {
      request->send(400, "application/json", "{\"error\":\"Missing parameters\"}");
      return;
    }
    String ssid = request->getParam("ssid", true)->value();
    String password = request->getParam("password", true)->value();
    bool mode = request->getParam("mode", true)->value().toInt();
    save_wifi_config(ssid, password, mode);
#if DEBUG
    Serial.printf("[WEB] Saved Wi-Fi config: SSID=%s, Mode=%d\n", ssid.c_str(), mode);
#endif
    request->send(200, "application/json", "{\"status\":\"ok\"}");
    delay(500);
    ESP.restart(); });

  // API: получить конфигурацию кнопок
  server.on("/api/buttons", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String json = "[";
    const auto &logic = get_button_logic();
    for (size_t l = 0; l < MAX_LAYERS; l++) {
      json += "[";
      for (size_t k = 0; k < NUM_DEFAULT_KEYS; k++) {
        const auto &b = logic[l][k];
        json += "{\"click\":{\"type\":" + String(b.click.type) + ",\"code\":" + String(b.click.code) + ",\"sub_code\":" + String(b.click.sub_code) + "},";
        json += "\"double\":{\"type\":" + String(b.dblclick.type) + ",\"code\":" + String(b.dblclick.code) + ",\"sub_code\":" + String(b.click.sub_code) + "},";
        json += "\"hold\":{\"type\":" + String(b.hold.type) + ",\"code\":" + String(b.hold.code) + ",\"sub_code\":" + String(b.click.sub_code) + "},";
        json += "\"holdRepeat\":{\"type\":" + String(b.holdRepeat.type) + ",\"code\":" + String(b.holdRepeat.code) + ",\"sub_code\":" + String(b.click.sub_code) + "},";
        json += "\"holdRelease\":{\"type\":" + String(b.holdRelease.type) + ",\"code\":" + String(b.holdRelease.code) + ",\"sub_code\":" + String(b.click.sub_code) + "},";
        json += "\"oneClickHold\":{\"type\":" + String(b.oneClickHold.type) + ",\"code\":" + String(b.oneClickHold.code) + ",\"sub_code\":" + String(b.click.sub_code) + "},";
        json += "\"release\":{\"type\":" + String(b.release.type) + ",\"code\":" + String(b.release.code) + ",\"sub_code\":" + String(b.click.sub_code) + "},";
        json += "\"color\":" + String(get_button_colors(l,k)) + "}";
        if (k + 1 < NUM_DEFAULT_KEYS) json += ",";
      }
      json += "]";
      if (l + 1 < MAX_LAYERS) json += ",";
    }
    json += "]";
    request->send(200, "application/json", json); });

  // API: сохранить конфигурацию кнопок
  server.on("/api/buttons", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    if (!request->hasParam("body", true))
    {
      Serial.println("[WEB] Missing body");
      request->send(400, "application/json", "{\"error\":\"Missing body\"}");
      return;
    }

    String body = request->getParam("body", true)->value();
    // key:<layout>:<bnt id>;color:<int color>;click:<type>:<code>;double:<type>:<code>|key:<layout>:<bnt id>;color:<int color>;click:<type>:<code>
    char *json = (char *)body.c_str();

    size_t layer = 0;
    size_t key = 0;

    while (*json)
    {
      if (strncmp(json, "key:", 4) == 0)
      {
        json += 4;
        layer = get_int_from_char(&json, ':');
        key = get_int_from_char(&json, ';');
        if (layer >= MAX_LAYERS || key >= NUM_DEFAULT_KEYS)
        {
          Serial.printf("[WEB] Invalid key: layer=%zu, key=%zu\n", layer, key);
          break;
        }
#if DEBUG
        Serial.printf("[WEB] Key: layer=%zu, key=%zu\n", layer, key);
#endif
      continue;
      }
      else if (strncmp(json, "color:", 6) == 0)
      {
        json += 6;
        uint32_t color = 0;
        color = get_uint32_from_char(&json, ';');
        set_button_color(layer, key, color);
#if DEBUG
        Serial.printf("[WEB]     Set color: layer=%zu, key=%zu, color=%u\n", layer, key, color);
#endif
      continue;
      }else{
        uint32_t end_pos = strcspn(json, ":");
        String action_name = String(json).substring(0, end_pos);
        ButtonActionKind action_type;
        bool is_action = true;
        if (action_name == "click")
        {
          action_type = BUTTON_CLICK;
        }
        else if (action_name == "double")
        {
          action_type = BUTTON_DOUBLE;
        }
        else if (action_name == "hold")
        {
          action_type = BUTTON_HOLD_START;
        }
        else if (action_name == "holdRepeat")
        {
          action_type = BUTTON_HOLD_REPEAT;
        }
        else if (action_name == "holdRelease")
        {
          action_type = BUTTON_HOLD_RELEASE;
        }
        else if (action_name == "oneClickHold")
        {
          action_type = BUTTON_ONE_CLICK_HOLD;
        }
        else if (action_name == "release")
        {
          action_type = BUTTON_RELEASE;
        }else{
          is_action= false;
        }

        if (is_action){
          json += end_pos;
          json += 1; // пропускаем ':'
          ButtonActionType type;
          type = (ButtonActionType)get_int_from_char(&json, ':');
        int16_t code = get_int_from_char(&json, ':');
        int16_t sub_code = get_int_from_char(&json, '|');
        set_button_action(layer, key, type, code, sub_code, action_type);
#if DEBUG
        Serial.printf("[WEB]     Set %d: layer=%zu, key=%zu, type=%hhu, code=%hd\n", action_type,layer, key, type, code);
#endif
        continue;
        }
      }
      
      
      if (strncmp(json, "|", 1) == 0)
      {
        json++;
        continue;
      }
      else if (strncmp(json, ";", 1) == 0)
      {
        json++;
        continue;
      }
      else
      {
        request->send(400, "application/json", "{\"error\":\"Invalid format\"}");
        Serial.printf("[WEB] Unknown command: %s\n", json);
        break;
      }
    }
    save_full_button_config();
    save_color_config();
    print_config();
    request->send(200, "application/json", "{\"status\":\"saved\"}"); });

  // API: получить список событий
  server.on("/api/actions", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "application/json", get_action_list()); });

  // API: получить список медиа-клавиш
  server.on("/api/media", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "application/json", get_media_key_list()); });

  // API: получить список клавиш
  server.on("/api/keycodes", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "application/json", KEYCODES_JSON); });

  server.on("/api/layout", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              if (LittleFS.exists("/layout.data"))
              {
                request->send(LittleFS, "/layout.data", "application/json");
              }
              else
              {
                request->send(200, "application/json", "");
              } });

  server.on("/api/layout", HTTP_POST, [](AsyncWebServerRequest *request)
            {
              if (request->hasParam("body", true))
              {
                String body = request->getParam("body", true)->value();
                File file = LittleFS.open("/layout.data", FILE_WRITE);
                if (file)
                {
                  file.print(body);
                  file.close();
                  request->send(200, "application/json", "{\"status\":\"saved\"}");
                }
                else
                {
                  request->send(500, "application/json", "{\"error\":\"Failed to save layout\"}");
                }
              }
              else
              {
                request->send(400, "application/json", "{\"error\":\"Missing body\"}");
              } });

  // Скачать бинарные конфиги
  server.on("/wifi.bin", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/wifi.bin", "application/octet-stream", true); });
  server.on("/config.bin", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/config.bin", "application/octet-stream", true); });
  server.on("/color.bin", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/color.bin", "application/octet-stream", true); });
  server.on("/api/restart_in_ble", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    request->send(200, "application/json", "{\"status\":\"ok\"}");
    delay(500);
    ESP.restart(); });
  server.on("/api/restart_in_wifi", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    request->send(200, "application/json", "{\"status\":\"ok\"}");
    delay(500);
    ModeManager::setMode(MODE_WIFI);
    ESP.restart(); });

  register_ir_api(server);
  register_action_api(server);
  handle_led_status_api(server);
  handle_power_status_api(server);

  server.begin();
#if DEBUG
  Serial.println("[WEB] Server started");
#endif
}

void web_loop()
{
  if (wifi_waiting_connect)
  {
    bool is_connected = WiFi.status() == WL_CONNECTED;
    if (is_connected != wifi_connected)
    {
#if DEBUG
      Serial.printf("[WIFI] Connection status changed: %s\n", is_connected ? "Connected" : "Disconnected");
#endif
      if (is_connected)
      {
        wifi_connected = true;
        LED_STATUS_WIFI_CONNECTED;
      }
      else
      {
        wifi_connected = false;
        LED_STATUS_WIFI_CONNECTING;
        // Переподключаемся
        WiFi.disconnect();
        WiFi.begin(get_wifi_ssid().c_str(), get_wifi_password().c_str());
      }
    }
  }
}
#endif // WEB_INTERFACE_H
