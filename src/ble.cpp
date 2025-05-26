#include "config.h"
#include "config_storage.h"
#include <button_service.h>

#include <BleKeyboard.h>
#include <BleMouse.h>

// BLE HID
#include "BLEDevice.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "led_service.h"

bool ble_is_init = false;
float ble_battery_level = 100.0f;
bool is_connected = false;

void ble_start()
{
  if (ble_is_init)
    return;
// === BLE HID ===
#if DEBUG
  Serial.println("[BLE HID] Starting...");
#endif
  // === Инициализация BLE ===
  bleDevice.setName(DEVICE_NAME);               // call before any of the begin functions to change the device name.
  bleDevice.setBatteryLevel(ble_battery_level); // change the battery level to
  Keyboard.begin();
  ble_is_init = true;
#if DEBUG
  Serial.println("[BLE HID] Started");
#endif
  LED_STATUS_NO_BLE_CONNECTION; // Устанавливаем статус без подключения BLE
}

void set_battery_level(float level)
{
  if (level < 0.0f)
    level = 0.0f;
  if (level > 100.0f)
    level = 100.0f;
  ble_battery_level = level;
  if (ble_is_init)
    bleDevice.setBatteryLevel(ble_battery_level);
}

void ble_stop()
{
  if (!ble_is_init)
    return;
#if DEBUG
  Serial.println("[BLE HID] Stopping...");
#endif
  // === Остановка BLE HID ===
  Keyboard.end();
  Mouse.end();

  // === Остановка BLE ===
  BLEDevice::deinit(true); // true = освобождает весь стек и память

  // Дополнительно (если BLE использовался):
  esp_bluedroid_disable();
  esp_bluedroid_deinit();
  esp_bt_controller_disable();
  esp_bt_controller_deinit();

  // Если не используешь Bluetooth Classic:
  esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
#if DEBUG
  Serial.println("[BLE HID] Stopped");
#endif
  delay(500);
  ble_is_init = false;
}

void ble_loop()
{
  if (!ble_is_init)
    return;
  // === Проверка подключения ===
  if (bleDevice.isConnected() != is_connected)
  {
    is_connected = !is_connected;
    // === Подключение ===
    set_hid_connected(is_connected);
    if (!is_connected)
    {
      LED_STATUS_NO_BLE_CONNECTION; // Устанавливаем статус без подключения BLE
    }
    else
    {
      LED_STATUS_BLE_CONNECTED; // Устанавливаем статус подключения BLE
    }
#if DEBUG
    Serial.printf("[BLE HID] %s\n", is_connected ? "Connected" : "Disconnected");
#endif
  }
}
