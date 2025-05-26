// button_service.cpp — обработка всех кнопок (MCP + GPIO)
#include "button_service.h"
#include "config.h"
#include "mcp_handler.h"
#include "led_service.h"
#include "config_storage.h"
#include "button_fsm.h"
#include <BleCombo.h>

static uint8_t pressedCodes[NUM_DEFAULT_KEYS]; // буфер для нажатых кнопок
static bool hidConnected = false;              // текущее состояние подключения HID

// === Установить состояние HID-подключения ===
void set_hid_connected(bool connected)
{
  if (hidConnected == connected)
    return; // состояние не изменилось
  hidConnected = connected;
#if DEBUG
  Serial.print("[BTN] HID connect: ");
  Serial.println(hidConnected ? "active" : "NOT active");
#endif
}

// === Инициализация GPIO-кнопок ===
void setup_button_service()
{
#if DEBUG
  Serial.println("[BTN] GPIO buttons setup");
#endif
  const auto &hw = get_keys_config();
  for (size_t i = 0; i < NUM_DEFAULT_KEYS; i++)
  {
    pressedCodes[i] = 0; // сбрасываем состояние кнопки

    if (hw[i].source == HARDWARE_KEY_SOURCE_GPIO)
    {
      if (hw[i].pin == 0)
        pinMode(hw[i].pin, INPUT); // GPIO 0 (BOOT) не может быть INPUT_PULLUP
      else
        pinMode(hw[i].pin, INPUT_PULLUP);
#if DEBUG && DEBUG_PRINT_BTN_CONFIG
      Serial.print("[BTN] GPIO ");
      Serial.print(hw[i].pin);
      Serial.println(" configured as INPUT_PULLUP");
#endif
    }
  }
#if DEBUG && DEBUG_PRINT_BTN_CONFIG
  for (size_t i = 0; i < NUM_DEFAULT_KEYS; i++)
  {
    Serial.print("[BTN] Button ");
    Serial.print(i);
    Serial.print(" (");
    Serial.print(hw[i].source == HARDWARE_KEY_SOURCE_MCP ? "MCP" : "GPIO");
    Serial.print("_");
    Serial.print(hw[i].sourceIndex);
    Serial.print(" - ");
    Serial.println(hw[i].pin);
  }
#endif
}

// === Основной цикл проверки всех кнопок ===
void update_buttons()
{
  // сбрасываем состояние нажатых кнопок
  memset(pressedCodes, 0, sizeof(pressedCodes));

  // считываем состояние кнопок MCP
  Side side = get_mcp_active_side();
  const auto &hw = get_keys_config();
#if DEBUG && DEBUG_BUTTON_STATE
  Serial.print("[BTN] State: ");
#endif
  for (size_t i = 0; i < NUM_DEFAULT_KEYS; i++)
  {
    if (!(hw[i].side == side || hw[i].side == SIDE_BOTH))
    {
      pressedCodes[i] = 0;
#if DEBUG && DEBUG_BUTTON_STATE
      Serial.print("x ");
#endif
      continue;
    }

    // GPIO кнопки
    if (hw[i].source == HARDWARE_KEY_SOURCE_GPIO)
      pressedCodes[i] = digitalRead(hw[i].pin) == LOW;

    // MCP кнопки
    if (hw[i].source == HARDWARE_KEY_SOURCE_MCP)
      pressedCodes[i] = get_pin_state(hw[i].sourceIndex, hw[i].pin);

#if DEBUG && DEBUG_BUTTON_STATE
    Serial.print(pressedCodes[i] ? "1 " : "0 ");
#endif
  }
#if DEBUG && DEBUG_BUTTON_STATE
  Serial.println();
#endif

  update_button_fsm(pressedCodes);
}

bool is_hid_connected()
{
  return hidConnected;
}
