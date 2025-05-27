#pragma once
#include <Arduino.h>

#define FIRMWARE_VERSION "1.0.0" // Версия прошивки

// === Типы сторон ===
enum Side
{
  SIDE_KEYBOARD,
  SIDE_MOUSE,
  SIDE_BOTH
};

// === Назначение действия при нажатии кнопки ===
enum TargetType
{
  TARGET_NONE = 0,
  TARGET_KEYBOARD,
  TARGET_MOUSE,
  TARGET_IR,
  TARGET_SCREEN,
  TARGET_FUNCTION,
  TARGET_WS_TEACH
};

// === Структура хардовой части кнопки ===
enum HardwareKeyIndex
{
  HARDWARE_KEY_SOURCE_MCP = 0,
  HARDWARE_KEY_SOURCE_GPIO = 1,
};

struct HardwareKeyConfig
{
  HardwareKeyIndex source; // MCP, GPIO
  uint8_t sourceIndex;     // Индекс MCP (0–1) или GPIO ()
  uint8_t pin;             // Номер пина (0–15 для MCP, или GPIO номер)
  Side side;               // SIDE_KEYBOARD / SIDE_MOUSE / SIDE_BOTH
  int8_t ledIndex;         // Индекс светодиода (или -1 если не используется)
};

// === Настраиваемая часть кнопки ===
struct UserKeyConfig
{
  uint8_t code;      // HID-код (или код IR)
  TargetType target; // TARGET_KEYBOARD / MOUSE / IR / ...
};

// === Типы действий кнопки ===
enum ButtonActionType : uint8_t
{
  ACTION_NONE = 0, // oтсутствует действие или пауза
  ACTION_KEYBOARD = 1,
  ACTION_MEDIA = 10,
  ACTION_MOUSE_CLICK = 2,
  ACTION_DELAY = 3,
  ACTION_MOUSE_MOVE = 4,
  ACTION_IR = 6,
  ACTION_LAYER_SWITCH = 7,
  ACTION_ACTION = 8,
  ACTION_SCRIPT = 9
};

struct WiFiConfig
{
  String ssid;
  String password;
  bool mode; // true - AP, false - STA
};

enum LedSource : int8_t
{
  LED_SOURCE_NONE = -1,
  LED_SOURCE_BATTERY = 0,
  LED_SOURCE_BLE,
  LED_SOURCE_WIFI,
  LED_SOURCE_IR
};

#define LED_SOURCE_COUNT 4