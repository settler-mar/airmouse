#ifndef SLEEP_MANAGER_H
#define SLEEP_MANAGER_H

#include <Arduino.h>

#pragma once

/**
 * @brief Инициализация sleep менеджера: сброс таймеров, настройка прерывания WAKE_PIN.
 */
void sleepManagerBegin();

/**
 * @brief Сброс таймера сна и состояний (подсветка и статусный LED снова включены).
 */
void resetSleepTimer();

/**
 * @brief Главный цикл sleep менеджера: управляет выключением подсветки, статусного LED и входом в light sleep.
 */
void sleepManagerLoop();

/**
 * @brief Установка разрешения sleep-таймеров (true — разрешить, false — сбросить и выключить sleep).
 * @param enabled Состояние разрешения sleep.
 */
void setSleepEnabled(bool enabled);

#endif
