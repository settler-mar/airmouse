// button_service.h — заголовок для обработки кнопок
#pragma once

#include <Arduino.h>

// Инициализация GPIO-кнопок из конфигурации
void setup_button_service();

// Основной цикл опроса MCP и GPIO, обработка действий
void update_buttons();

// Установить флаг HID-подключения (вызывается из main)
void set_hid_connected(bool connected);

// Получить текущее состояние HID-подключения
bool is_hid_connected();
