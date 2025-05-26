// action_runner.cpp — выполнение действий по кнопке и типу события
#include "button_service.h"
#include "action_runner.h"
#include "config.h"
#include "config_storage.h"
#include "web_interface.h"
#include <BleKeyboard.h>
#include <BleMouse.h>
#include <ir_service.h>
#include <led_service.h>
#include <mouse_control.h>
#include <LittleFS.h>
#include "mode_manager.h"
#include "helpers.h"
#include <ESPAsyncWebServer.h>
#include "sleep_manager.h"

byte active_layer = 0; // активный слой (по умолчанию 0)

void init_action_runner()
{
#if DEBUG
  Serial.println("[ACT] Initializing action runner");
#endif
  // инициализация модуля
  active_layer = 0;
  // инициализация файловой системы
  if (!LittleFS.exists("/scripts"))
  {
    LittleFS.mkdir("/scripts");
  }
}
void run_script_binary(uint8_t script_id);

void run_action(uint8_t type, int32_t code, int32_t sub_code, bool is_script = false)
{
#if DEBUG
  Serial.printf("[ACT] Exec: type=%d code=%d sub_code=%d\n", type, code, sub_code);
#endif

  if (type == ACTION_NONE)
  {
    if (is_script && code >= 0)
    {
      delay(code);
    }
    return;
  }

  if (type == ACTION_KEYBOARD && is_hid_connected())
  {
    Keyboard.press(code);
    delay(5);
    Keyboard.release(code);
    return;
  }
  if (type == ACTION_MEDIA && is_hid_connected())
  {
    // Media keys
    switch (code)
    {
    case 0:
      Keyboard.write(KEY_MEDIA_NEXT_TRACK);
      break;
    case 1:
      Keyboard.write(KEY_MEDIA_PREVIOUS_TRACK);
      break;
    case 2:
      Keyboard.write(KEY_MEDIA_STOP);
      break;
    case 3:
      Keyboard.write(KEY_MEDIA_PLAY_PAUSE);
      break;
    case 4:
      Keyboard.write(KEY_MEDIA_MUTE);
      break;
    case 5:
      Keyboard.write(KEY_MEDIA_VOLUME_UP);
      break;
    case 6:
      Keyboard.write(KEY_MEDIA_VOLUME_DOWN);
      break;
    case 7:
      Keyboard.write(KEY_MEDIA_WWW_HOME);
      break;
    case 8:
      Keyboard.write(KEY_MEDIA_LOCAL_MACHINE_BROWSER);
      break;
    case 9:
      Keyboard.write(KEY_MEDIA_CALCULATOR);
      break;
    case 10:
      Keyboard.write(KEY_MEDIA_WWW_BOOKMARKS);
      break;
    case 11:
      Keyboard.write(KEY_MEDIA_WWW_SEARCH);
      break;
    case 12:
      Keyboard.write(KEY_MEDIA_WWW_STOP);
      break;
    case 13:
      Keyboard.write(KEY_MEDIA_WWW_BACK);
      break;
    case 14:
      Keyboard.write(KEY_MEDIA_CONSUMER_CONTROL_CONFIGURATION);
      break;
    case 15:
      Keyboard.write(KEY_MEDIA_EMAIL_READER);
      break;
    default:
      break;
    }
    return;
  }

  if (type == ACTION_MOUSE_CLICK && is_hid_connected())
  {
    Mouse.click(code);
    return;
  }

  if (type == ACTION_IR)
  {
    // send_ir_code(code);
    return;
  }

  if (type == ACTION_LAYER_SWITCH)
  {
    if (code < MAX_LAYERS)
    {
      active_layer = code;
#if DEBUG
      Serial.printf("[ACT] Layer switch: -> %d\n", code);
#endif
      apply_led_layer_colors();
    }
    return;
  }
  uint8_t mode = ModeManager::getMode();

  if (type == ACTION_ACTION)
  {
    switch (code)
    {
    case 0:
      ModeManager::setMode(MODE_WIFI);
      ESP.restart();
      break;
    case 1:
      ModeManager::setMode(MODE_BLE);
      ESP.restart();
      break;
    case 2:
      ModeManager::toggleMode();
      ESP.restart();
      break;
    case 3:
      // learn_ir_code();
      break;
    case 4:
      mouse_control_enable();
      break;
    case 5:
      mouse_control_disable();
      break;
    case 6:
      mouse_control_toggle();
      break;
    }
    return;
  }

  if (type == ACTION_MOUSE_MOVE && is_hid_connected())
  {
    Mouse.move(code, sub_code);
    return;
  }

  if (type == ACTION_SCRIPT && not is_script)
  {
    run_script_binary(code);
    return;
  }
}

void run_button_action(uint8_t index, ButtonActionKind kind)
{
  if (index >= NUM_DEFAULT_KEYS)
    return;
  resetSleepTimer();

#if DEBUG
  Serial.printf("[ACT] Button action: index=%d kind=%d\n", index, kind);
#endif

  const ButtonAction &action = get_button_action(active_layer, index, kind);
  run_action(action.type, action.code, action.sub_code, false);
}

void run_script_binary(uint8_t script_id)
{
  String path = "/scripts/" + String(script_id) + ".bin";
  File file = LittleFS.open(path, "r");
  if (!file)
  {
    Serial.printf("[SCRIPT] Failed to open: %s\n", path.c_str());
    return;
  }

  // читаем имя скрипта (до \n)
  String script_name = file.readStringUntil('\n');
  Serial.printf("[SCRIPT] Name: %s\n", script_name.c_str());

  while (file.available() >= 3)
  {
    // читаем действие
    // 1 байт - тип действия
    // 2 байта - код действия
    // 2 байта - доп код действия
    uint8_t action_id = file.read();
    int16_t param = 0;
    // читаем 2 байта
    file.readBytes((char *)&param, sizeof(param));
    // читаем 2 байта
    int16_t param2 = 0;
    run_action(action_id, param, param2, true);
  }

  file.close();
}

String get_media_key_list()
{
  String json = "[";
  json += "{\"id\":0,\"name\":\"Next track\"},";
  json += "{\"id\":1,\"name\":\"Previous track\"},";
  json += "{\"id\":2,\"name\":\"Stop\"},";
  json += "{\"id\":3,\"name\":\"Play/Pause\"},";
  json += "{\"id\":4,\"name\":\"Mute\"},";
  json += "{\"id\":5,\"name\":\"Volume up\"},";
  json += "{\"id\":6,\"name\":\"Volume down\"},";
  json += "{\"id\":7,\"name\":\"Home\"},";
  json += "{\"id\":8,\"name\":\"Local machine browser\"},";
  json += "{\"id\":9,\"name\":\"Calculator\"},";
  json += "{\"id\":10,\"name\":\"Bookmarks\"},";
  json += "{\"id\":11,\"name\":\"Search\"},";
  json += "{\"id\":12,\"name\":\"Stop\"},";
  json += "{\"id\":13,\"name\":\"Back\"},";
  json += "{\"id\":14,\"name\":\"Consumer control configuration\"},";
  json += "{\"id\":15,\"name\":\"Email reader\"}";
  json += "]";
  return json;
}

String get_action_list()
{
  String json = "[";
  json += "{\"id\":0,\"name\":\"START in WIFI mode\"},";
  json += "{\"id\":1,\"name\":\"START in BLE mode\"},";
  json += "{\"id\":2,\"name\":\"MODE toggle\"},";
  // json += "{\"id\":3,\"name\":\"IR learn\"},";
  json += "{\"id\":4,\"name\":\"Mouse control on\"},";
  json += "{\"id\":5,\"name\":\"Mouse control off\"},";
  json += "{\"id\":6,\"name\":\"Mouse control toggle\"}";
  json += "]";
  return json;
}

byte get_active_layer()
{
  return active_layer;
}

String list_scripts_json()
{
  String json = "[";
  File root = LittleFS.open("/scripts");
  if (!root || !root.isDirectory())
    return "[]";

  File file = root.openNextFile();
  while (file)
  {
    if (!file.isDirectory())
    {
      String filename = String(file.name());
      String id = filename.substring(filename.lastIndexOf("/") + 1);

      // читаем имя скрипта
      String name = file.readStringUntil('\n');
      name.replace("\"", "'");    // защита от кавычек
      name.replace("\\", "\\\\"); // защита от обратной косой черты
      name.replace("\n", "");     // защита от перевода строки
      name.replace("\r", "");     // защита от возврата каретки
      name.replace("\t", " ");    // защита от табуляции
      name.replace("\b", " ");    // защита от backspace

      json += "{\"id\":\"" + id + "\",\"name\":\"" + name + "\"},";
    }
    file = root.openNextFile();
  }

  if (json.endsWith(","))
    json.remove(json.length() - 1);
  json += "]";
  return json;
}

String get_action_file(const String &filename)
{
  String path = "/scripts/" + filename;
  File file = LittleFS.open(path, "r");
  if (!file)
    return "{\"error\":\"File not found\"}";
#if DEBUG
  Serial.printf("[SCRIPT] Download: %s\n", path.c_str());
#endif

  String json = "{";
  String name = file.readStringUntil('\n');
  name.replace("\"", "'");    // защита от кавычек
  name.replace("\\", "\\\\"); // защита от обратной косой черты
  name.replace("\n", "");     // защита от перевода строки
  name.replace("\r", "");     // защита от возврата каретки
  name.replace("\t", " ");    // защита от табуляции
  json += "\"name\":\"" + name + "\",\"file\":\"" + filename + "\",\"actions\":[";
  bool first = true;
  // читаем все действия
  while (file.available() >= 3)
  {
    uint8_t action_id = file.read();
    int16_t param = 0;
    file.readBytes((char *)&param, sizeof(param));
    int16_t param2 = 0;
    file.readBytes((char *)&param2, sizeof(param2));
    if (first)
    {
      first = false;
    }
    else
    {
      json += ",";
    }
    json += "{\"type\":" + String(action_id) + ",\"code\":" + String(param) + ",\"sub_code\":" + String(param2) + "}";
  }
  json += "]}";
  file.close();
#if DEBUG
  Serial.printf("[SCRIPT] Download: %s\n", json.c_str());
#endif
  return json;
}

bool delete_script(const String &filename)
{
  String path = "/scripts/" + filename;
  return LittleFS.remove(path);
}

void register_action_api(AsyncWebServer &server)
{
  server.on("/api/scripts/delete", HTTP_POST, [](AsyncWebServerRequest *request)
            {
  if (!request->hasParam("name", true)) {
    request->send(400, "text/plain", "Missing name");
    return;
  }
  
  String name = request->getParam("name", true)->value();
  bool ok = delete_script(name);
  request->send(ok ? 200 : 500, "application/json", "{\"ok\":" + String(ok ? "true" : "false") + "}"); });

  server.on("/api/scripts/upload", HTTP_POST, [](AsyncWebServerRequest *request)
            {
  if (not request->hasParam("name", true) || not request->hasParam("body", true) || not request->hasParam("id", true))
  {
    request->send(400, "application/json", "{\"error\":\"Missing parameters\"}");
  }
    String script_title = request->getParam("name", true)->value();
    uint16_t id = request->getParam("id", true)->value().toInt();
    String body = request->getParam("body", true)->value();

    String filename = "/scripts/" + String(id) + ".bin";
    
    File file = LittleFS.open(filename, FILE_WRITE);
    if (!file) {
      request->send(500, "application/json", "{\"error\":\"Failed to open file\"}");
      return;
    }

    // первая строка — название (без расширения)
    file.println(script_title); // добавляем \n автоматически
#if DEBUG
    Serial.printf("[SCRIPT] Upload: %s\n", filename.c_str());
#endif

    char *json = (char *)body.c_str();

    ButtonAction action;
    int cnt=0;
    while (*json) {
      cnt++;
      if(cnt > 1000)
      {
        break;
      }
      action.type = (ButtonActionType)get_int_from_char(&json, ':');
      action.code = get_int_from_char(&json, ':');
      action.sub_code = get_int_from_char(&json, ';');
      if (action.type == 0 && action.code == 0 && action.sub_code == 0)
      {
        continue;
      }
      file.write((uint8_t *)&action.type, sizeof(action.type));
      file.write((uint8_t *)&action.code, sizeof(action.code));
      file.write((uint8_t *)&action.sub_code, sizeof(action.sub_code));

#if DEBUG
      Serial.printf("[SCRIPT] Write: type=%d, code=%d\n", action.type, action.code);
#endif
      if (*json == '|')
      {
        json++;
      }
      else if (*json == ';')
      {
        json++;
      }
      else
      {
        continue;
      }
    }
#if DEBUG
    Serial.printf("[SCRIPT] Write end: %s\n", filename.c_str());
#endif
    file.close();
    request->send(200, "application/json", "{\"status\":\"saved\"}"); });

  // Скачать скрипт по имени. /api/scripts/download/{filename}
  server.on("/api/scripts/download/*", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              // split name from request. /api/scripts/download/{filename}
              String path = request->url();
              // api/scripts/download/3.bin
              int index = path.indexOf("/download/");
              if (index == -1)
              {
                request->send(400, "application/json", "{\"error\":\"Invalid path\"}");
                return;
              }
              String filename = path.substring(index + 10); // 10 = "/download/".length()
              String data = get_action_file(filename);
              request->send(200, "application/json", data); });

  server.on("/api/scripts", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "application/json", list_scripts_json()); });
}