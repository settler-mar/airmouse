// mouse_control.h — заголовок для гироскопического управления
#pragma once

// Инициализация MPU6050 и подготовка к управлению мышью
void setup_mouse_control();

// Вызов при каждом цикле — обновление движения мыши
void update_mouse_control();

// Отдельная функция обработки движения на основе углов
void processMouseMove();

void mouse_control_enable();
void mouse_control_disable();
void mouse_control_toggle();