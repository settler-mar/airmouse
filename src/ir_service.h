// ir_storage.h
#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

// Инициализация ИК
void ir_setup();

// Отправка последнего захваченного сигнала
void ir_start_send();

// Поиск свободного слота (1–99)
bool ir_find_free_slot(int &slot);

// Сохранение сигнала: имя, частота, данные
bool ir_save(uint16_t *rawbuf, size_t rawlen, const String &name, int freq, int slot);

// Загрузка сигнала: возвращает имя, частоту, данные
bool ir_load(int slot, String &name, int &freq, uint16_t *rawbuf, size_t &rawlen, size_t maxLen);

// Удаление сигнала по номеру
bool ir_remove(int slot);

// Регистрация API (capture, send, delete)
void register_ir_api(AsyncWebServer &server);

// Цикл обработки ИК
void ir_loop();
