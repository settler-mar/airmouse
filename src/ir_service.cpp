// ir_service.cpp
#include "ir_service.h"
#include "config.h"
#include <LittleFS.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>
#include <ESPAsyncWebServer.h>
#include "led_service.h"

static IRrecv irrecv(IR_RECV_PIN);
static IRsend irsend(IR_SEND_PIN);
static decode_results results;
static uint16_t ir_rawbuf[MAX_IR_BUFFER];
static size_t ir_rawlen = 0;
static String ir_name;
static int ir_freq = 38;
static bool wait_for_ir = false;   // ожидание захвата
static int ir_slot = 0;            // номер последнего слота
static unsigned long ir_timer = 0; // таймер захвата
static byte last_ir_status = 0;    // статус последнего захвата
static byte ir_send_cnt = 0;       // счетчик отправок
// 0 = не захвачен, 1 = захвачен, 2 = timeout, 3 = no empty slots, 4 = error, 5 = save error

void ir_clear_all()
{
  // очистка старых файлов
  File root = LittleFS.open("/ir");
  if (!root || !root.isDirectory())
    return;
  File file = root.openNextFile();
  while (file)
  {
    if (!file.isDirectory())
    {
      String filename = String(file.name());
      if (filename.endsWith(".ir"))
      {
#if DEBUG
        Serial.printf("[IR] Deleting old file: %s\n", filename.c_str());
#endif
        file.close();
        LittleFS.remove("/ir/" + filename);
      }
    }
    file = root.openNextFile();
  }
}

void ir_setup()
{
  LittleFS.begin();
  irrecv.enableIRIn();
  irsend.begin();
  if (!LittleFS.exists("/ir"))
  {
    LittleFS.mkdir("/ir");
  }
}

bool ir_find_free_slot(int &slot)
{
#if DEBUG
  Serial.println("[IR] Searching for free slot...");
#endif
  for (int i = 1; i <= MAX_IR_CODES; ++i)
  {
    String path = "/ir/" + String(i) + ".ir";
    if (!LittleFS.exists(path))
    {
      slot = i;
      return true;
    }
  }
  return false;
}

bool ir_save(uint16_t *rawbuf, size_t rawlen, const String &name, int freq, int slot)
{
  Serial.printf("[IR] Saving signal: %s, freq=%d, slot=%d\n", name.c_str(), freq, slot);
  String path = "/ir/" + String(slot) + ".ir";
  File f = LittleFS.open(path, "w");
  if (!f)
    return false;

  f.println(name);
  f.println(String(freq));
  f.println(String(rawlen));
  f.write((uint8_t *)rawbuf, rawlen * sizeof(uint16_t));
  f.close();
#if DEBUG
  Serial.printf("[IR] Signal saved to %s\n", path.c_str());
  // печата в лог сигнал
  Serial.printf("[IR] Signal: ");
  for (size_t i = 0; i < rawlen; i++)
  {
    Serial.printf("%d ", rawbuf[i]);
  }
  Serial.println();
#endif
  return true;
}

bool ir_load(int slot, String &name, int &freq, uint16_t *rawbuf, size_t &rawlen)
{
#if DEBUG
  Serial.printf("[IR] Loading signal from slot %d\n", slot);
#endif
  if (ir_slot == slot)
  {
#if DEBUG
    Serial.printf("[IR] Signal already loaded: %s, freq=%d, rawlen=%d\n", name.c_str(), freq, rawlen);
#endif
    return true;
  }
  String path = "/ir/" + String(slot) + ".ir";
  File f = LittleFS.open(path, "r");
  if (!f)
  {
#if DEBUG
    Serial.printf("[IR] Error: file not found %s\n", path.c_str());
#endif
    return false;
  }
  ir_slot = 0;
  name = f.readStringUntil('\n');
  name.trim();
  String freqStr = f.readStringUntil('\n');
  freq = freqStr.toInt();

  String rawlenStr = f.readStringUntil('\n');
  rawlenStr.trim();
  rawlen = rawlenStr.toInt();
  if (rawlen <= 0 || rawlen > MAX_IR_BUFFER)
  {
#if DEBUG
    Serial.printf("[IR] Error: rawlen=%d, maxLen=%d\n", rawlen, MAX_IR_BUFFER);
#endif
    return false;
  }

  int readBytes = f.read((uint8_t *)rawbuf, rawlen * sizeof(uint16_t));
  if (readBytes != rawlen * sizeof(uint16_t))
  {
#if DEBUG
    Serial.println("[IR] Incomplete read");
#endif
    return false;
  }
  f.close();

#if DEBUG
  Serial.println("[IR] Signal loaded:");
  Serial.printf("[IR] Signal loaded: %s, freq=%d, rawlen=%d\n", name.c_str(), freq, rawlen);
#endif
  if (rawlen > MAX_IR_BUFFER || rawlen <= 0)
  {
#if DEBUG
    Serial.printf("[IR] Error: rawlen=%d, maxLen=%d\n", rawlen, MAX_IR_BUFFER);
#endif
    return false;
  }
#if DEBUG
  Serial.printf("[IR] Signal: ");
  for (size_t i = 0; i < rawlen; i++)
  {
    Serial.printf("%d ", rawbuf[i]);
  }
  Serial.println();
#endif
  ir_slot = slot;

  return true;
}

bool ir_remove(int slot)
{
  String path = "/ir/" + String(slot) + ".ir";
  return LittleFS.remove(path);
}

void ir_send()
{
  if (ir_rawlen > 0 && ir_slot > 0)
  {
    ir_send_cnt--;
    irsend.sendRaw(ir_rawbuf, ir_rawlen, ir_freq * 1000);
#if DEBUG
    Serial.printf("[IR] Sending signal: %s, freq=%d, rawlen=%d\n", ir_name.c_str(), ir_freq, ir_rawlen);
#endif
  }
}

bool ir_start_send(int slot)
{
  ir_slot = 0;
  if (ir_load(slot, ir_name, ir_freq, ir_rawbuf, ir_rawlen))
  {
    ir_slot = slot;
    ir_send_cnt = IR_SEND_CNT;
#if DEBUG
    Serial.printf("[IR] Sending signal: %s, freq=%d, rawlen=%d\n", ir_name.c_str(), ir_freq, ir_rawlen);
#endif
    ir_timer = millis() + IR_SEND_TIMEOUT;
    ir_send();
    return true;
  }
  else
  {
#if DEBUG
    Serial.printf("[IR] Error loading signal from slot %d\n", slot);
#endif
    return false;
  }
}

void ir_loop()
{
  if (wait_for_ir)
  {
    if (irrecv.decode(&results))
    {
      ir_rawlen = getCorrectedRawLength(&results);
      uint16_t *raw_array = resultToRawArray(&results);
      memcpy(ir_rawbuf, raw_array, ir_rawlen * sizeof(uint16_t)); // обязательно
      irrecv.resume();

      irrecv.resume();
      last_ir_status = 1;
      // сохраняем сигнал
      if (ir_save(ir_rawbuf, ir_rawlen, ir_name, ir_freq, ir_slot))
      {
#if DEBUG
        Serial.printf("[IR] Signal saved: %s, freq=%d, slot=%d\n", ir_name.c_str(), ir_freq, ir_slot);
#endif
        // успешно сохранено
        last_ir_status = 1;
        wait_for_ir = false;
        LED_STATUS_IR_LEARN_END;
      }
      else
      {
#if DEBUG
        Serial.println("[IR] Error saving IR signal");
#endif
        // ошибка сохранения
        last_ir_status = 5;
        wait_for_ir = false;
        LED_STATUS_IR_LEARN_END;
      }
    }
    else if (millis() > ir_timer)
    {
      // таймаут
      last_ir_status = 2;
      wait_for_ir = false;
      LED_STATUS_IR_LEARN_END;
    }
    else if (ir_slot == 0 && !ir_find_free_slot(ir_slot))
    {
      // нет свободных слотов
      last_ir_status = 3;
      wait_for_ir = false;
      LED_STATUS_IR_LEARN_END;
    }
    else if (ir_rawlen > MAX_IR_BUFFER)
    {
      // ошибка захвата
      last_ir_status = 4;
      wait_for_ir = false;
      LED_STATUS_IR_LEARN_END;
    }
  }

  if (ir_send_cnt > 0)
  {
    if (millis() > ir_timer)
    {
      ir_send();
      ir_timer = millis() + IR_SEND_TIMEOUT;
    }
  }
}

void register_ir_api(AsyncWebServer &server)
{
  // API: захват ИК-сигнала
  server.on("/api/ir/capture", HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t, size_t)
            {
              String body = String((char *)data);
              int timeout = 3000;
              int freq = 38;
              String name = "Без названия";
              int tIdx = body.indexOf("\"timeout\":");
              if (tIdx != -1)
                timeout = body.substring(tIdx + 10).toInt();

              int fIdx = body.indexOf("\"freq\":");
              if (fIdx != -1)
                freq = body.substring(fIdx + 7).toInt();

              int nIdx = body.indexOf("\"name\":");
              if (nIdx != -1)
              {
                int start = body.indexOf('"', nIdx + 7);
                int end = body.indexOf('"', start + 1);
                if (start != -1 && end != -1)
                  name = body.substring(start + 1, end);
              }
              int fSlot = body.indexOf("\"slot\":");
              if (fSlot != -1)
              {
                ir_slot = body.substring(fSlot + 7).toInt();
                if (ir_slot < 0 || ir_slot > MAX_IR_CODES)
                {
                  request->send(400, "application/json", "{\"status\":\"invalid_slot\"}");
                  return;
                }
              }
              else
              {
                if (!ir_find_free_slot(ir_slot))
                {
                  request->send(400, "application/json", "{\"status\":\"no_empty_slots\"}");
                  return;
                }
              }

              name.replace("\"", "'");    // защита от кавычек
              name.replace("\\", "\\\\"); // защита от обратной косой черты
              name.replace("\n", "");     // защита от перевода строки
              name.replace("\r", "");     // защита от возврата каретки
              name.replace("\t", " ");    // защита от табуляции
              name.trim();
              if (name.length() > 20)
              {
                request->send(400, "application/json", "{\"status\":\"name_too_long\"}");
                return;
              }
              ir_name= name;
              ir_freq = freq;
#if DEBUG
              Serial.printf("[IR] Capture: timeout=%d, freq=%d, name=%s\n", timeout, freq, name.c_str());
#endif
              if (wait_for_ir)
              {
                request->send(400, "application/json", "{\"status\":\"already_capturing\"}");
                return;
              }
              ir_timer = millis() + timeout;
              wait_for_ir = true;
              LED_STATUS_IR_LEARN;
              request->send(200, "application/json", "{\"status\":\"capturing\"}"); });

  // API: статус захвата ИК-сигнала
  server.on("/api/ir/status", HTTP_GET, [](AsyncWebServerRequest *request)
            {
      String json = "{\"status\":";
      if (wait_for_ir)
      {
        json += "\"process\"";
      }
      else if(last_ir_status == 1)
      {
        json += "\"captured\",\"rawlen\":" + String(ir_rawlen) + ",\"raw\":\"";
        for (size_t i = 0; i < ir_rawlen && i < MAX_IR_BUFFER; i++)
        {
          String part = String(ir_rawbuf[i], HEX);
          while (part.length() < 4 && part!="")
          {
            part = "0" + part;
          }
          json += part;
        }
        json += "\"";
      }
      else if (last_ir_status == 2)
      {
        json += "\"timeout\"";
      }
      else if (last_ir_status == 3)
      {
        json += "\"no_empty_slots\"";
      }
      else if (last_ir_status == 4)
      {
        json += "\"error\"";
      }
      else
      {
        json += "\"not_captured\"";
      }
      json += ",\"slot\":" + String(ir_slot) + ",\"freq\":" + String(ir_freq) + ",\"name\":\"" + ir_name + "\"}";
      request->send(200, "application/json", json); });

  // API: отправка ИК-сигнала
  server.on("/api/ir/send", HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t, size_t)
            {
      String body = String((char*)data);
      int slot = body.toInt();
#if DEBUG
      Serial.printf("[IR] Send: slot=%d\n", slot);
#endif
      String name; int freq; size_t rawlen;
      if (ir_start_send(slot)){
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      } else {
        request->send(404, "application/json", "{\"status\":\"not_found\"}");
      } });

  // API: удаление ИК-сигнала
  server.on("/api/ir/delete", HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t, size_t)
            {
      int slot = String((char*)data).toInt();
      if (ir_remove(slot)) {
        request->send(200, "application/json", "{\"status\":\"deleted\"}");
      } else {
        request->send(404, "application/json", "{\"status\":\"not_found\"}");
      } });

  // API: получение списка ИК-сигналов
  server.on("/api/ir/list", HTTP_GET, [](AsyncWebServerRequest *request)
            {
#if DEBUG
    Serial.println("[IR] List IR codes");
#endif
    File root = LittleFS.open("/ir");
    if (!root || !root.isDirectory()){
      request->send(200, "application/json", "[]");
      return;
            }
    File file = root.openNextFile();
    String json = "[";
    while (file)
    {
      if (!file.isDirectory())
      {
        String filename = String(file.name());
        if (!filename.endsWith(".ir"))
          continue;
        int slot = filename.substring(filename.lastIndexOf("/") + 1).toInt();
        String name;
         int freq;
         size_t rawlen;
         ir_load(slot, name, freq, ir_rawbuf, rawlen);
         json += "{\"slot\":" + String(slot) + ",\"name\":\"" + name + "\",\"freq\":" + String(freq);
          json += ",\"rawlen\":" + String(rawlen) + ",\"raw\":\"";
          for (size_t i = 0; i < rawlen && i < MAX_IR_BUFFER; i++)
          {
            String part = String(ir_rawbuf[i], HEX);
            while (part.length() < 4 && part!="")
            {
              part = "0" + part;
            }
            json += part;
          }
          json += "\"}";
         if (file.openNextFile())
           json += ",";
          else
           break;
      }
    } 
    json += "]";
    request->send(200, "application/json", json); });

  // API: скачать ИК-сигнал
  server.on("/api/ir/download/*", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              String path = request->url();
              int index = path.indexOf("/download/");
              if (index == -1)
              {
                request->send(400, "application/json", "{\"error\":\"Invalid path\"}");
                return;
              }
              String filename = "/ir/" +path.substring(index + 10)+ ".ir"; // 10 = "/download/".length()

      File file = LittleFS.open(filename, "r");
      if (!file)
      {
        request->send(404, "application/json", "{\"status\":\"not_found\"}");
        return;
      }
      request->send(file, filename, "application/octet-stream"); });
}