// action_runner.h — интерфейс запуска действий по событию кнопки
#pragma once

#include <Arduino.h>
#include "config.h"
#include "button_fsm.h"
#include "config_storage.h"
#include <ESPAsyncWebServer.h>

// Основные функции
void init_action_runner();

// Запуск действия по кнопке
void run_button_action(uint8_t index, ButtonActionKind kind);

// Запуск действия напрямую (для скриптов и других вызовов)
void run_action(uint8_t type, int32_t code, int32_t sub_code);

// Выполнение бинарного скрипта из файла /scripts/{id}.bin
void run_script_binary(uint8_t script_id);

// Получить текущий активный слой
byte get_active_layer();

// Список доступных встроенных действий (для UI)
String get_action_list();

// --- Web API функции для работы со скриптами ---

// Получить список скриптов с именами
String list_scripts_json();

// Удалить скрипт по имени файла (например, "3.bin")
bool delete_script(const String &filename);

// Скачать скрипт через API
String get_action_file(const String &filename);

// Список доступных медиа-клавиш (для UI)
String get_media_key_list();

void register_action_api(AsyncWebServer &server);

const char KEYCODES_JSON[] PROGMEM = R"rawliteral(
[
  {"id":128,"name":"Left Ctrl","group":"mod"},
  {"id":129,"name":"Left Shift","group":"mod"},
  {"id":130,"name":"Left Alt","group":"mod"},
  {"id":131,"name":"Left GUI","group":"mod"},
  {"id":132,"name":"Right Ctrl","group":"mod"},
  {"id":133,"name":"Right Shift","group":"mod"},
  {"id":134,"name":"Right Alt","group":"mod"},
  {"id":135,"name":"Right GUI","group":"mod"},
  {"id":140,"name":"A","group":"alpha"},
  {"id":141,"name":"B","group":"alpha"},
  {"id":142,"name":"C","group":"alpha"},
  {"id":143,"name":"D","group":"alpha"},
  {"id":144,"name":"E","group":"alpha"},
  {"id":145,"name":"F","group":"alpha"},
  {"id":146,"name":"G","group":"alpha"},
  {"id":147,"name":"H","group":"alpha"},
  {"id":148,"name":"I","group":"alpha"},
  {"id":149,"name":"J","group":"alpha"},
  {"id":150,"name":"K","group":"alpha"},
  {"id":151,"name":"L","group":"alpha"},
  {"id":152,"name":"M","group":"alpha"},
  {"id":153,"name":"N","group":"alpha"},
  {"id":154,"name":"O","group":"alpha"},
  {"id":155,"name":"P","group":"alpha"},
  {"id":156,"name":"Q","group":"alpha"},
  {"id":157,"name":"R","group":"alpha"},
  {"id":158,"name":"S","group":"alpha"},
  {"id":159,"name":"T","group":"alpha"},
  {"id":160,"name":"U","group":"alpha"},
  {"id":161,"name":"V","group":"alpha"},
  {"id":162,"name":"W","group":"alpha"},
  {"id":163,"name":"X","group":"alpha"},
  {"id":164,"name":"Y","group":"alpha"},
  {"id":165,"name":"Z","group":"alpha"},
  {"id":166,"name":"1","group":"num"},
  {"id":167,"name":"2","group":"num"},
  {"id":168,"name":"3","group":"num"},
  {"id":169,"name":"4","group":"num"},
  {"id":170,"name":"5","group":"num"},
  {"id":171,"name":"6","group":"num"},
  {"id":172,"name":"7","group":"num"},
  {"id":173,"name":"8","group":"num"},
  {"id":174,"name":"9","group":"num"},
  {"id":175,"name":"0","group":"num"},
  {"id":176,"name":"Enter","group":"ctrl"},
  {"id":177,"name":"Esc","group":"ctrl"},
  {"id":178,"name":"Backspace","group":"ctrl"},
  {"id":179,"name":"Tab","group":"ctrl"},
  {"id":180,"name":"Space","group":"ctrl"},
  {"id":193,"name":"Caps Lock","group":"ctrl"},
  {"id":181,"name":"-","group":"sym"},
  {"id":182,"name":"=","group":"sym"},
  {"id":183,"name":"[","group":"sym"},
  {"id":184,"name":"]","group":"sym"},
  {"id":185,"name":"\\","group":"sym"},
  {"id":187,"name":";","group":"sym"},
  {"id":188,"name":"'","group":"sym"},
  {"id":189,"name":"~","group":"sym"},
  {"id":190,"name":",","group":"sym"},
  {"id":191,"name":".","group":"sym"},
  {"id":192,"name":"/","group":"sym"},
  {"id":194,"name":"F1","group":"func"},
  {"id":195,"name":"F2","group":"func"},
  {"id":196,"name":"F3","group":"func"},
  {"id":197,"name":"F4","group":"func"},
  {"id":198,"name":"F5","group":"func"},
  {"id":199,"name":"F6","group":"func"},
  {"id":200,"name":"F7","group":"func"},
  {"id":201,"name":"F8","group":"func"},
  {"id":202,"name":"F9","group":"func"},
  {"id":203,"name":"F10","group":"func"},
  {"id":204,"name":"F11","group":"func"},
  {"id":205,"name":"F12","group":"func"},
  {"id":206,"name":"Print Screen","group":"func"},
  {"id":207,"name":"Scroll Lock","group":"func"},
  {"id":208,"name":"Pause","group":"func"},
  {"id":237,"name":"Application","group":"func"},
  {"id":240,"name":"F13","group":"func"},
  {"id":241,"name":"F14","group":"func"},
  {"id":242,"name":"F15","group":"func"},
  {"id":243,"name":"F16","group":"func"},
  {"id":244,"name":"F17","group":"func"},
  {"id":245,"name":"F18","group":"func"},
  {"id":246,"name":"F19","group":"func"},
  {"id":247,"name":"F20","group":"func"},
  {"id":248,"name":"F21","group":"func"},
  {"id":249,"name":"F22","group":"func"},
  {"id":250,"name":"F23","group":"func"},
  {"id":251,"name":"F24","group":"func"},
  {"id":209,"name":"Insert","group":"nav"},
  {"id":210,"name":"Home","group":"nav"},
  {"id":211,"name":"Page Up","group":"nav"},
  {"id":212,"name":"Delete","group":"nav"},
  {"id":213,"name":"End","group":"nav"},
  {"id":214,"name":"Page Down","group":"nav"},
  {"id":215,"name":"→","group":"nav"},
  {"id":216,"name":"←","group":"nav"},
  {"id":217,"name":"↓","group":"nav"},
  {"id":218,"name":"↑","group":"nav"},
  {"id":219,"name":"Num Lock","group":"num"},
  {"id":220,"name":"Num /","group":"num"},
  {"id":221,"name":"Num *","group":"num"},
  {"id":222,"name":"Num -","group":"num"},
  {"id":223,"name":"Num +","group":"num"},
  {"id":224,"name":"Num Enter","group":"num"},
  {"id":225,"name":"Num 1","group":"num"},
  {"id":226,"name":"Num 2","group":"num"},
  {"id":227,"name":"Num 3","group":"num"},
  {"id":228,"name":"Num 4","group":"num"},
  {"id":229,"name":"Num 5","group":"num"},
  {"id":230,"name":"Num 6","group":"num"},
  {"id":231,"name":"Num 7","group":"num"},
  {"id":232,"name":"Num 8","group":"num"},
  {"id":233,"name":"Num 9","group":"num"},
  {"id":234,"name":"Num 0","group":"num"},
  {"id":235,"name":"Num .","group":"num"}
]
)rawliteral";

#pragma once
