// config.h — глобальные настройки устройства
#pragma once
#include "base_struct.h"

#include <Arduino.h>

#define DEVICE_NAME "Air Mouse+"

// === WIFI ===
#define WIFI_SSID "AirMouse"          // WIFI SSID точки доступа
#define WIFI_PASSWORD "123456789"     // WIFI пароль точки доступа
#define WIFI_MODE_RUN_PIN 17          // GPIO пин для выбора режима работы WIFI or BLE
#define WIFI_CONNECT_TIMEOUT_MS 30000 // Таймаут подключения для перехода в режим AP (в мс)

// === Отладка ===
#define DEBUG 1                  // 0 - отключить отладку, 1 - включить отладку
#define DEBUG_BUTTON_STATE 0     // 0 - отключить отладку состояния кнопок, 1 - включить отладку состояния кнопок
#define DEBUG_MOUSE_STATE 0      // 0 - отключить отладку состояния мыши, 1 - включить отладку состояния мыши
#define DEBUG_MCP_STATE 0        // 0 - отключить отладку состояния MCP, 1 - включить отладку состояния MCP
#define DEBUG_PRINT_BTN_CONFIG 0 // 0 - отключить вывод конфигурации кнопок, 1 - включить вывод конфигурации кнопок
#define DEBUG_WAKE_PIN 0         // 0 - отключить отладку WAKE_PIN, 1 - включить отладку WAKE_PIN

// === I2C ===
#define I2C_SDA_PIN 5
#define I2C_SCL_PIN 4

// === params ===
#define MAX_LAYERS 4 // Максимальное число слоев для кнопок + подсветки

// === GPIO ===
#define PIN_WS_LED 12 // WS2812 лента

// === MCP ===
#define MCP_ADDR {0x21, 0x20} // MCP23017

// === LED ===
#define NUM_WS_LEDS 31             // Общее число светодиодов в ленте
#define LED_BRIGHTNESS 32          // Яркость (0–255)
#define LED_STATUS_INDEX 0         // Индекс статусного светодиода (-1 если не используется)
#define LED_COLOR_DEFAULT 0x00FF00 // Зеленый

// ----- statuses -----
// LedStatus(color, blink_ms, timeout_ms, is_fade, source)
#define LED_STATUS_BATTERY_CHARGING LED_STATUS(0x0000FF, 2000, 0, true, LED_SOURCE_BATTERY)
#define LED_STATUS_BATTERY_FULL LED_STATUS(0x00FF00, 0, 5000, false, LED_SOURCE_BATTERY)
#define LED_STATUS_BATTERY_NEEDS_CHARGE LED_STATUS(0xFFFF00, 500, 30000, false, LED_SOURCE_BATTERY)
#define LED_STATUS_BATTERY_LOW LED_STATUS(0xFF0000, 500, 0, false, LED_SOURCE_BATTERY)
#define LED_STATUS_NO_BLE_CONNECTION LED_STATUS(0x3300ee, 1000, 0, true, LED_SOURCE_BLE)
#define LED_STATUS_BLE_CONNECTED LED_STATUS(0x4400ff, 0, 5000, false, LED_SOURCE_BLE)
#define LED_STATUS_WIFI_AP_START LED_STATUS(0xFFA500, 0, 3000, false, LED_SOURCE_WIFI)
#define LED_STATUS_WIFI_CONNECTING LED_STATUS(0xFFFF00, 1000, 0, true, LED_SOURCE_WIFI)
#define LED_STATUS_WIFI_CONNECTED LED_STATUS(0x00FFFF, 0, 5000, false, LED_SOURCE_WIFI)
#define LED_STATUS_IR_LEARN LED_STATUS(0xFF00FF, 200, 0, false, LED_SOURCE_IR)
#define LED_STATUS_IR_LEARN_END LED_STATUS(0xFF00FF, 0, 5000, false, LED_SOURCE_IR)

// === IR ===
#define MAX_IR_CODES 99            // Максимальное число IR-кодов
#define MAX_IR_LEARN_TIMEOUT 10000 // Таймаут обучения IR-кода (в мс)
#define IR_RECV_PIN 15             // GPIO пин для приема IR
#define IR_SEND_PIN 14             // GPIO пин для передачи IR
#define MAX_IR_BUFFER 512          // Максимальный размер буфера IR (в байтах)
#define IR_SEND_TIMEOUT 100        // Таймаут передачи IR (в мс)
#define IR_SEND_CNT 1              // Количество повторов передачи IR

// === Состояние сна ===
#define PIN_TILT_WAKE 36                       // Бинарный датчик наклона (для пробуждения)
#define SLEEP_BACKLIGHT_TIMEOUT 10 * 1000      // Время  выключения подсветки (в мс)
#define SLEEP_STATUS_LED_TIMEOUT 30 * 1000     // Время выключения статусного светодиода (в мс)
#define SLEEP_TIMEOUT 60 * 1000                // Время до перехода в light sleep (в мс)
#define SLEEP_POWER_OFF_TIMEOUT 60 * 60 * 1000 // Время до перехода в deep sleep (в мс)

// === Управление зарядкой ===
#define IP5306_ADDR 0x75              // Адрес контроллера питания IP5306
#define BATTERY_LEVEL_LOW 5           // Минимальный уровень заряда батареи (в процентах)
#define BATTERY_LEVEL_NEEDS_CHARGE 20 // Уровень заряда батареи, при котором нужно начать зарядку (в процентах)
#define IP5306_POLL_INTERVAL 5000     // Интервал опроса IP5306 (в мс)
#define IP5306_CHARGE_VOLTAGE_SEL 0   // Напреяжение заряда (0 - 4.2V, 1 - 4.3В, 2 - 4.35В, 3 - 4.4В)
#define IP5306_CHARGE_CURRENT_MA 500  // Ток заряда (в мА, от 100 до 2000 мА. Округляется до ближайших 100 мА в меньшую сторону)
#define IP5306_AUTO_POWER_ON 1        // Автоматическое включение питания при подключении USB (0 - выключено, 1 - включено)
#define IP5306_AUTO_CHARGE_CONTROL 1  // Автоматическое управление зарядкой (0 - выключено, 1 - включено)

#define NUM_DEFAULT_KEYS 30
extern const UserKeyConfig defaultUserKeys[NUM_DEFAULT_KEYS];
// extern const HardwareKeyConfig hardwareKeys[NUM_DEFAULT_KEYS];
const HardwareKeyConfig hardwareKeys[NUM_DEFAULT_KEYS] = {
    // MCP1 (клавиатура)
    {HARDWARE_KEY_SOURCE_MCP, 0, 0, SIDE_KEYBOARD, 6},  // Num4
    {HARDWARE_KEY_SOURCE_MCP, 0, 1, SIDE_KEYBOARD, 9},  // Num1
    {HARDWARE_KEY_SOURCE_MCP, 0, 2, SIDE_KEYBOARD, 4},  // Num8
    {HARDWARE_KEY_SOURCE_MCP, 0, 3, SIDE_KEYBOARD, 5},  // Num9
    {HARDWARE_KEY_SOURCE_MCP, 0, 4, SIDE_KEYBOARD, 7},  // Num5
    {HARDWARE_KEY_SOURCE_MCP, 0, 5, SIDE_KEYBOARD, 10}, // Num2
    {HARDWARE_KEY_SOURCE_MCP, 0, 6, SIDE_KEYBOARD, 11}, // Num3
    {HARDWARE_KEY_SOURCE_MCP, 0, 7, SIDE_KEYBOARD, 8},  // Num6
    {HARDWARE_KEY_SOURCE_MCP, 0, 8, SIDE_KEYBOARD, 12}, // Num0
    {HARDWARE_KEY_SOURCE_MCP, 0, 9, SIDE_KEYBOARD, 2},  // Enter
    {HARDWARE_KEY_SOURCE_MCP, 0, 10, SIDE_KEYBOARD, 0}, // Space
    {HARDWARE_KEY_SOURCE_MCP, 0, 11, SIDE_KEYBOARD, 1}, // Esc
    {HARDWARE_KEY_SOURCE_MCP, 0, 12, SIDE_KEYBOARD, 3}, // Num7

    // MCP2 (мышь)
    {HARDWARE_KEY_SOURCE_MCP, 1, 0, SIDE_BOTH, 13},   // Volume-
    {HARDWARE_KEY_SOURCE_MCP, 1, 1, SIDE_BOTH, 14},   // Volume+
    {HARDWARE_KEY_SOURCE_MCP, 1, 2, SIDE_MOUSE, 22},  // Right
    {HARDWARE_KEY_SOURCE_MCP, 1, 3, SIDE_MOUSE, 20},  // Down
    {HARDWARE_KEY_SOURCE_MCP, 1, 4, SIDE_MOUSE, 16},  // OK (Enter)
    {HARDWARE_KEY_SOURCE_MCP, 1, 5, SIDE_MOUSE, 19},  // Up
    {HARDWARE_KEY_SOURCE_MCP, 1, 6, SIDE_MOUSE, 17},  // Power (IR)
    {HARDWARE_KEY_SOURCE_MCP, 1, 7, SIDE_MOUSE, 21},  // Left
    {HARDWARE_KEY_SOURCE_MCP, 1, 8, SIDE_MOUSE, 24},  // Back
    {HARDWARE_KEY_SOURCE_MCP, 1, 9, SIDE_MOUSE, 23},  // Home
    {HARDWARE_KEY_SOURCE_MCP, 1, 10, SIDE_MOUSE, 25}, // Play/Pause
    {HARDWARE_KEY_SOURCE_MCP, 1, 11, SIDE_MOUSE, 15}, // Right Click
    {HARDWARE_KEY_SOURCE_MCP, 1, 12, SIDE_BOTH, 18},  // Mute

    // GPIO
    {HARDWARE_KEY_SOURCE_GPIO, 0, 17, SIDE_KEYBOARD, 26}, // Tab
    {HARDWARE_KEY_SOURCE_GPIO, 0, 34, SIDE_KEYBOARD, 27}, // PageUp
    {HARDWARE_KEY_SOURCE_GPIO, 0, 35, SIDE_KEYBOARD, 28}, // PageDown
    {HARDWARE_KEY_SOURCE_GPIO, 0, 0, SIDE_BOTH, 29}       // Fn (особый)
};

// === Параметры управления мышью ===
#define MOUSE_DEADZONE 0.5            // мертвая зона (от 0 до 1)
#define MOUSE_SENSITIVITY 2.5         // чувствительность (от 0 до 5)
#define MOUSE_ACCEL_THRESHOLD 1.2     // порог ускорения (от 0 до 1)
#define MOUSE_ACCEL_MULTIPLIER 2.0    // коэффициент ускорения (от 0 до 1)
#define MOUSE_PRECISION_THRESHOLD 0.6 // порог точности (от 0 до 1)
#define MOUSE_PRECISION_SCALE 0.4     // коэффициент точности (от 0 до 1)
#define MOUSE_SIDE_ZONE 0.8           // мертвая зона по оси Z (от 0 до 1)
// #define MOUSE_SIDE_INVERT             // инвертировать ось Z (если включено, то при наклоне вниз будет работать мышь, а при наклоне вверх — клавиатура)

#if DEBUG
#pragma message("[DEBUG ENABLED] config.h loaded with HID + mouse + LED + power")
#endif
