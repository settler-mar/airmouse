// config_storage.h — интерфейс хранения и загрузки конфигурации
#pragma once

#include <Arduino.h>
#include "config.h"

// Структура действия кнопки
struct ButtonAction
{
  ButtonActionType type;
  int16_t code;
  int16_t sub_code;
};

// Полная логика одной кнопки
struct ButtonLogic
{
  ButtonAction click;
  ButtonAction dblclick;
  ButtonAction hold;
  ButtonAction holdRepeat;
  ButtonAction holdRelease;
  ButtonAction oneClickHold;
  ButtonAction release;
};

enum ButtonActionKind : uint8_t
{
  BUTTON_CLICK = 0,
  BUTTON_DOUBLE = 1,
  BUTTON_HOLD_START = 2,
  BUTTON_HOLD_REPEAT = 3,
  BUTTON_HOLD_RELEASE = 4,
  BUTTON_ONE_CLICK_HOLD = 5,
  BUTTON_RELEASE = 6,
};

// === Глобальные массивы конфигурации ===
extern const HardwareKeyConfig hardwareKeys[NUM_DEFAULT_KEYS];
extern const UserKeyConfig defaultUserKeys[NUM_DEFAULT_KEYS]; // используется для генерации дефолтной логики

// Основная логика по слоям и цветам
extern ButtonLogic buttonLogic[MAX_LAYERS][NUM_DEFAULT_KEYS];
extern uint32_t buttonColors[MAX_LAYERS][NUM_DEFAULT_KEYS];
extern bool configLoaded;

// === Интерфейс ===
bool load_config(); // загрузить из файла или использовать дефолт
void load_default_config();
void clear_config(); // сбросить логику и цвета
void print_config(); // отладочный вывод в консоль

// Доступ к текущей логике и цветам
const ButtonLogic (&get_button_logic())[MAX_LAYERS][NUM_DEFAULT_KEYS];
const uint32_t get_button_colors(byte layer, byte key);
const int8_t get_button_colors_index(byte key);
const ButtonAction get_button_action(byte layer, byte key, ButtonActionKind kind);

// Получить конфигурацию кнопок
const HardwareKeyConfig (&get_keys_config())[NUM_DEFAULT_KEYS];
void set_button_action(uint8_t layer, uint8_t key, ButtonActionType type, int16_t code, int16_t sub_code, ButtonActionKind kind);
void set_button_color(uint8_t layer, uint8_t key, uint32_t color);
bool save_full_button_config();
bool save_color_config();

// === Wi-Fi конфигурация ===
String get_wifi_ssid();
String get_wifi_password();
bool get_wifi_mode();
bool save_wifi_config(const String &ssid, const String &password, bool mode);