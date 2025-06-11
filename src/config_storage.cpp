// config_storage.cpp — загрузка и сохранение пользовательской конфигурации кнопок (бинарный формат)
#include "config_storage.h"
#include "config.h"
#include "FS.h"
#include "LittleFS.h" //https://randomnerdtutorials.com/esp8266-nodemcu-vs-code-platformio-littlefs/

#define CONFIG_PATH "/config.bin"
#define CONFIG_WIFI_PATH "/wifi.bin"
#define CONFIG_COLOR_PATH "/color.bin"

ButtonLogic buttonLogic[MAX_LAYERS][NUM_DEFAULT_KEYS];
uint32_t buttonColors[MAX_LAYERS][NUM_DEFAULT_KEYS];
bool configLoaded = false;
WiFiConfig wifiConfig;


void clear_config()
{
#if DEBUG
  Serial.println("[CFG] Config cleared");
#endif
  for (size_t i = 0; i < MAX_LAYERS; i++)
  {
    for (size_t j = 0; j < NUM_DEFAULT_KEYS; j++)
    {
      buttonLogic[i][j] = {};
      buttonColors[i][j] = 0x000000;
    }
  }
}

void load_default_config()
{
  clear_config();
#if DEBUG
  Serial.println("[CFG] Loading default config");
#endif
  for (size_t i = 0; i < MAX_LAYERS; i++)
  {
    for (size_t j = 0; j < NUM_DEFAULT_KEYS; j++)
    {
      buttonLogic[i][j].click.type = ACTION_NONE;
      buttonLogic[i][j].dblclick.type = ACTION_NONE;
      buttonLogic[i][j].hold.type = ACTION_NONE;
      buttonLogic[i][j].holdRepeat.type = ACTION_NONE;
      buttonLogic[i][j].holdRelease.type = ACTION_NONE;
      buttonLogic[i][j].release.type = ACTION_NONE;

      buttonColors[i][j] = LED_COLOR_DEFAULT;
    }
  }
}

bool load_button()
{

  load_default_config();
  if (!LittleFS.begin(true))
  {
#if DEBUG
    Serial.println("[CFG] Mount LittleFS failed");
#endif
    return false;
  }

  File file = LittleFS.open(CONFIG_PATH, FILE_READ);
  if (!file)
  {
#if DEBUG
    Serial.println("[CFG] File open failed, using default config");
#endif
    file.close();
    return false;
  }

  size_t expected = sizeof(buttonLogic);
  if (file.size() != expected)
  {
#if DEBUG
    Serial.printf("[CFG] Wrong file size: %d, expected: %d\n", (int)file.size(), (int)expected);
#endif
    file.close();
    return false;
  }

  file.read((uint8_t *)buttonLogic, sizeof(buttonLogic));
  file.close();

  configLoaded = true;
#if DEBUG
  Serial.println("[CFG] Config loaded successfully");
#endif
  return true;
}

bool load_color()
{
  if (!LittleFS.begin(true))
  {
#if DEBUG
    Serial.println("[CFG] Mount LittleFS failed");
#endif
    return false;
  }

  File file = LittleFS.open(CONFIG_COLOR_PATH, FILE_READ);
  if (!file)
  {
#if DEBUG
    Serial.println("[CFG] File open failed, using default config");
#endif
    file.close();
    return false;
  }

  size_t expected = sizeof(buttonColors);
  if (file.size() != expected)
  {
#if DEBUG
    Serial.printf("[CFG] Wrong file size: %d, expected: %d\n", (int)file.size(), (int)expected);
#endif
    file.close();
    return false;
  }

  file.read((uint8_t *)buttonColors, sizeof(buttonColors));
  file.close();
#if DEBUG
  Serial.println("[CFG] Color config loaded successfully");
#endif
  return true;
}

void print_config()
{
#if DEBUG
  Serial.println("[CFG] Config:");
  for (size_t i = 0; i < MAX_LAYERS; i++)
  {
    for (size_t j = 0; j < NUM_DEFAULT_KEYS; j++)
    {
      const ButtonAction &act = buttonLogic[i][j].click;
      Serial.printf("[CFG] Layer %d Key %d: type=%d code=%d color=%06X\n", i, j, act.type, act.code, buttonColors[i][j]);
    }
  }
#endif
}

bool load_wifi_config()
{
  wifiConfig.mode = true; // AP mode by default
  wifiConfig.ssid = WIFI_SSID;
  wifiConfig.password = WIFI_PASSWORD;
  if (!LittleFS.begin(true))
  {
#if DEBUG
    Serial.println("[CFG] Mount LittleFS failed");
#endif
    return false;
  }

  File file = LittleFS.open(CONFIG_WIFI_PATH, FILE_READ);
  if (!file)
  {
#if DEBUG
    Serial.println("[CFG] File open failed, using default config");
#endif

    file.close();
    return false;
  }

  size_t expected = sizeof(WiFiConfig);
  if (file.size() != expected)
  {
#if DEBUG
    Serial.printf("[CFG] Wrong file size: %d, expected: %d\n", (int)file.size(), (int)expected);
#endif
    file.close();
    return false;
  }

  file.read((uint8_t *)&wifiConfig, sizeof(WiFiConfig));
  file.close();
#if DEBUG
  Serial.println("[CFG] Wi-Fi config loaded successfully");
  Serial.printf("[CFG] Wi-Fi SSID: %s, Password: %s, Mode: %s\n", wifiConfig.ssid, wifiConfig.password, wifiConfig.mode ? "AP" : "STA");
#endif
  return true;
}

bool load_config()
{
  load_button();
  load_color();
  load_wifi_config();
  return configLoaded;
}

const ButtonLogic (&get_button_logic())[MAX_LAYERS][NUM_DEFAULT_KEYS]
{
  return buttonLogic;
}

const ButtonAction get_button_action(byte layer, byte key, ButtonActionKind kind)
{
  if (layer >= MAX_LAYERS || key >= NUM_DEFAULT_KEYS)
    return {ACTION_NONE, 0};
  switch (kind)
  {
  case BUTTON_CLICK:
    return buttonLogic[layer][key].click;
  case BUTTON_DOUBLE:
    return buttonLogic[layer][key].dblclick;
  case BUTTON_HOLD_START:
    return buttonLogic[layer][key].hold;
  case BUTTON_HOLD_REPEAT:
    return buttonLogic[layer][key].holdRepeat;
  case BUTTON_HOLD_RELEASE:
    return buttonLogic[layer][key].holdRelease;
  case BUTTON_RELEASE:
    return buttonLogic[layer][key].release;
  }
  return {ACTION_NONE, 0};
}

void set_button_logic(const ButtonLogic (&newLogic)[MAX_LAYERS][NUM_DEFAULT_KEYS])
{
  for (size_t i = 0; i < MAX_LAYERS; i++)
  {
    for (size_t j = 0; j < NUM_DEFAULT_KEYS; j++)
    {
      buttonLogic[i][j] = newLogic[i][j];
    }
  }
}

void set_button_action(uint8_t layer, uint8_t key, ButtonActionType type, int16_t code, int16_t sub_code, ButtonActionKind kind)
{
  if (layer >= MAX_LAYERS || key >= NUM_DEFAULT_KEYS)
    return;
  switch (kind)
  {
  case BUTTON_CLICK:
    buttonLogic[layer][key].click = {type, code, sub_code};
    break;
  case BUTTON_DOUBLE:
    buttonLogic[layer][key].dblclick = {type, code, sub_code};
    break;
  case BUTTON_HOLD_START:
    buttonLogic[layer][key].hold = {type, code, sub_code};
    break;
  case BUTTON_HOLD_REPEAT:
    buttonLogic[layer][key].holdRepeat = {type, code, sub_code};
    break;
  case BUTTON_HOLD_RELEASE:
    buttonLogic[layer][key].holdRelease = {type, code, sub_code};
    break;
  case BUTTON_RELEASE:
    buttonLogic[layer][key].release = {type, code, sub_code};
    break;
  }
}

void set_button_color(uint8_t layer, uint8_t key, uint32_t color)
{
  if (layer >= MAX_LAYERS || key >= NUM_DEFAULT_KEYS)
    return;
  buttonColors[layer][key] = color;
#if DEBUG
  Serial.printf("[CFG] Set color: layer=%zu, key=%zu, color=%u\n", layer, key, buttonColors[layer][key]);
#endif
}

bool save_full_button_config()
{
  if (!LittleFS.begin(true))
  {
#if DEBUG
    Serial.println("[CFG] Mount LittleFS failed");
#endif
    return false;
  }

  File file = LittleFS.open(CONFIG_PATH, FILE_WRITE);
  if (!file)
  {
#if DEBUG
    Serial.println("[CFG] File open failed");
#endif
    return false;
  }

  size_t written = 0;
  written += file.write((uint8_t *)buttonLogic, sizeof(buttonLogic));
  file.close();

#if DEBUG
  Serial.printf("[CFG] Saved button logic (%d bytes)\n", (int)written);
#endif

  return written == sizeof(buttonLogic);
}

const uint32_t get_button_colors(byte layer, byte key)
{
  if (layer >= MAX_LAYERS || key >= NUM_DEFAULT_KEYS)
    return 0;
  return buttonColors[layer][key];
}

const HardwareKeyConfig (&get_keys_config())[NUM_DEFAULT_KEYS]
{
  return hardwareKeys;
}

String get_wifi_ssid()
{
  if (wifiConfig.ssid.length() == 0)
  {
    return WIFI_SSID;
  }
  return wifiConfig.ssid;
}

String get_wifi_password()
{
  if (wifiConfig.ssid.length() == 0)
  {
    return WIFI_PASSWORD;
  }
  return wifiConfig.password;
}

bool get_wifi_mode()
{
  if (wifiConfig.ssid.length() == 0)
  {
    return true; // AP mode
  }
  return wifiConfig.mode;
}

bool save_wifi_config(const String &ssid, const String &password, bool mode)
{
  wifiConfig.ssid = ssid;
  wifiConfig.password = password;
  wifiConfig.mode = mode;

  if (!LittleFS.begin(true))
  {
#if DEBUG
    Serial.println("[CFG] Mount LittleFS failed");
#endif
    return false;
  }

  File file = LittleFS.open(CONFIG_WIFI_PATH, FILE_WRITE);
  if (!file)
  {
#if DEBUG
    Serial.println("[CFG] File open failed");
#endif
    return false;
  }

  file.write((uint8_t *)&wifiConfig, sizeof(WiFiConfig));
  file.close();
#if DEBUG

  Serial.println("[CFG] Wi-Fi config saved successfully");
  Serial.printf("[CFG] Wi-Fi SSID: %s, Password: %s, Mode: %s\n", wifiConfig.ssid, wifiConfig.password, wifiConfig.mode ? "AP" : "STA");
#endif
  return true;
}

bool save_color_config()
{
  if (!LittleFS.begin(true))
  {
#if DEBUG
    Serial.println("[CFG] Mount LittleFS failed");
#endif
    return false;
  }

  File file = LittleFS.open(CONFIG_COLOR_PATH, FILE_WRITE);
  if (!file)
  {
#if DEBUG
    Serial.println("[CFG] File open failed");
#endif
    return false;
  }

  file.write((uint8_t *)buttonColors, sizeof(buttonColors));
  file.close();
#if DEBUG
  Serial.printf("[CFG] Color config saved successfully (%d bytes)\n", (int)sizeof(buttonColors));
#endif
  return true;
}

const int8_t get_button_colors_index(byte key)
{
  if (key >= NUM_DEFAULT_KEYS)
    return -1;
  return hardwareKeys[key].ledIndex;
}
