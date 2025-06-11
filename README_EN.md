# 🚀 ESP32 AirMouse

[🇷🇺 Русская версия](./README.MD)

ESP32 AirMouse — a project implementing a wireless air mouse device.

---

## ⚙️ Features

✅ Gyroscope-based mouse control  
✅ IR receiver and transmitter with learning mode  
✅ Many buttons (direct connection and MCP23017)  
✅ Customizable actions (multimedia, Android keys)  
✅ Supports: click, double-click, hold, click+hold  
✅ BLE HID mode (Bluetooth Low Energy)  
✅ Wi-Fi web interface (HTTP server)  
✅ WS2812 LED indicators  
✅ Power management with IP5306 (optional)  
✅ LittleFS (binary configs)  
✅ Sleep mode on inactivity  
✅ Layers for keys  
✅ Scripts (sequences of actions)

---

## 🛠️ Project Diagram

🔗 [AirMouse on EasyEDA](https://oshwlab.com/matuhinmax/air_mouse)

---

## 🧩 Main Components

| 🎛️ Component         | ⚡ Purpose                                                                          |
|-----------------------|------------------------------------------------------------------------------------|
| **ESP32**             | Main microcontroller: BLE, Wi-Fi, peripherals, user interaction                     |
| **MCP23017**          | I2C GPIO expander (2 chips, expandable up to 8)                                     |
| **IP5306**            | Power management: charging, battery monitoring (optional)                            |
| **VS1838B + IR LED**  | Receive and transmit infrared signals                                               |
| **WS2812**            | Addressable LEDs for status and button backlighting                                  |

---

## 🔌 Connection

### I2C
- SDA: `GPIO4`
- SCL: `GPIO5`

### 📡 IR
| Purpose       | Pin      | Comment         |
|---------------|----------|-----------------|
| IR receiver   | `GPIO15` | VS1838B         |
| IR LED        | `GPIO14` | Transmission    |

### 🖱️ Buttons (direct connection)
| Purpose       | Pin      | Side            |
|---------------|----------|-----------------|
| Page Down     | `GPIO35` | Keyboard        |
| Page Up       | `GPIO34` | Keyboard        |
| Tab           | `GPIO17` | Keyboard        |
| Fn            | `GPIO0`  | Both            |
| Tilt Wake     | `GPIO36` | Wake-up sensor and motion detection (mercury tilt sensor 102)  |

### 🌈 WS2812
| Purpose           | Pin      | Comment         |
|--------------------|----------|-----------------|
| LED strip          | `GPIO12` | 32 LEDs         |

---

## 🟨 MCP23017

1. Side **Keyboard** — `0x20`  
2. Side **Mouse** — `0x21`

---

## 💡 LED Status Table

| Status                      | Color      | Effect             | Timeout (ms) | Source   | Description            |
|-----------------------------|------------|---------------------|--------------|----------|------------------------|
| BATTERY_CHARGING            | 🔵 #0000FF | Fade (2 sec)       | 0            | BATTERY  | Charging               |
| BATTERY_FULL                | 🟢 #00FF00 | -                  | 5000         | BATTERY  | Fully charged          |
| BATTERY_NEADS_CHARGE        | 🟡 #FFFF00 | Blink (500 ms)     | 30000        | BATTERY  | Needs charging         |
| BATTERY_LOW                 | 🔴 #FF0000 | Blink (500 ms)     | 0            | BATTERY  | Low battery            |
| NO_BLE_CONNECTION           | 🔵 #3300EE | Fade (1 sec)       | 0            | BLE      | BLE not connected      |
| BLE_CONNECTED               | 🟣 #4400FF | -                  | 5000         | BLE      | BLE connected          |
| WIFI_AP_START               | 🟠 #FFA500 | -                  | 3000         | WIFI     | Wi-Fi AP mode          |
| WIFI_CONNECTING             | 🟡 #FFFF00 | Fade (1 sec)       | 0            | WIFI     | Connecting to Wi-Fi    |
| WIFI_CONNECTED              | 🔵 #00FFFF | -                  | 5000         | WIFI     | Wi-Fi connected        |
| IR_LEARN                    | 🟣 #FF00FF | Blink (200 ms)     | 0            | IR       | IR learning mode       |
| IR_LEARN_END                | 🟣 #FF00FF | -                  | 5000         | IR       | IR learning complete   |

---

### 🔧 Sources (`LED_SOURCE_*`)
Each status belongs to its **source**:

```cpp
#define LED_SOURCE_NONE    -1 (cannot be used as a source)
#define LED_SOURCE_BATTERY  0
#define LED_SOURCE_BLE      1
#define LED_SOURCE_WIFI     2
#define LED_SOURCE_IR       3
```

### Parameters

✅ Timeout — means that after this time, the status is automatically cleared and the next available one (by source priority) is selected.
✅ Fade — smooth brightness animation.
✅ Blinking — color toggles at an interval (blink).
✅ Priority is determined by logic: first status with timeout, then persistent.

## 🛠️ Installation

1. Clone:
```bash
git clone https://github.com/settler-mar/airmouse.git
```

2. Configure src/config.h

3. Flash firmware via PlatformIO.

---

## 📶 Wi-Fi Mode

**EN:**  
You can enter **Wi-Fi mode** either by assigning it as an action to a button or by using a dedicated "Wi-Fi mode" pin (`WIFI_MODE_RUN_PIN`, default: `GPIO17`).

- **Exit:**  
  To exit Wi-Fi mode, simply restart the device using the reset button.

- **AP mode:**  
  If network parameters are not configured, or if the device cannot connect to a saved network within 30 seconds, it will automatically start in **Access Point (AP) mode**.
