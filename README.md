# 🏭 Smart Industrial Safety & Monitoring System
&#x20;

 📌 Project Overview

&#x20;

This system simulates a real industrial safety controller built around the \*\*ESP32-C3 Mini\*\* microcontroller. It continuously reads multiple environmental sensors and compares values against predefined safety thresholds. When an unsafe condition is detected, it automatically:

&#x20;

\- ⚡ \*\*Cuts machine power\*\* via relay

\- 🔔 \*\*Activates audible alarm\*\* via buzzer

\- 📟 \*\*Displays warning\*\* on OLED screen

\- 🌐 \*\*Updates live web dashboard\*\* over WiFi in real time

> \*\*Developed as an academic embedded systems project\*\*

  

&#x20;

\---

&#x20;

\## ✨ Features

&#x20;

| Feature | Description |

|---------|-------------|

| 🌡 Temperature Monitoring | DHT11 sensor — alerts above 40°C |

| 💧 Humidity Monitoring | Real-time humidity reading and display |

| ☁ Gas / Air Quality | MQ series analog sensor — alerts above ADC threshold 1500 |

| 👁 Motion Detection | PIR sensor — triggers danger on unauthorized presence |

| ⚡ Auto Machine Shutdown | Active-LOW relay cuts power on any danger trigger |

| 🔔 Audible Alarm | Buzzer beeps in pattern during danger state |

| 📟 OLED Display | SH1106 1.3" — shows live readings, SAFE / DANGER screen |

| 🌐 WiFi Web Server | Real-time dashboard accessible from any browser |

| 📊 Live Data Feed | JSON API endpoint `/data` polled every 2 seconds |

| 📋 Event Log | Web dashboard logs all state changes with timestamps |

| ⏱ System Uptime | Tracks and displays system running time |

| 🔢 Alert Counter | Counts total danger events since boot |

&#x20;

\---

&#x20;

\## 🧰 Hardware

&#x20;

\### Components List

&#x20;

| Component | Model | Quantity |

|-----------|-------|----------|

| Microcontroller | ESP32-C3 Mini | 1 |

| OLED Display | SH1106 1.3" I2C (128×64) | 1 |

| Temperature \& Humidity | DHT11 | 1 |

| Gas Sensor | MQ-2 / MQ-135 (any MQ series) | 1 |

| Motion Sensor | HC-SR501 PIR | 1 |

| Relay Module | 5V Single Channel (Active LOW) | 1 |

| Buzzer | Active Buzzer 5V | 1 |

| Power Supply | Buck Converter / USB 5V | 1 |

| Miscellaneous | Breadboard, Jumper Wires | — |

&#x20;

\### ⚠ Relay Important Note

&#x20;

This project uses an \*\*Active LOW relay module\*\* (common blue relay boards):

&#x20;

| ESP32 GPIO Output | Relay Coil | Motor/Machine |

|-------------------|-----------|---------------|

| `LOW` | ON ✅ | \*\*RUNS\*\* |

| `HIGH` | OFF ❌ | \*\*STOPPED\*\* |

&#x20;

The code defines `RELAY\_ON LOW` and `RELAY\_OFF HIGH` to handle this correctly.

&#x20;

\---

&#x20;

\## 🔌 Pin Configuration

&#x20;

```

ESP32-C3 Mini

┌─────────────────────────────────────┐

│  GPIO 8  ──────── OLED SDA          │

│  GPIO 9  ──────── OLED SCL          │

│  GPIO 2  ──────── DHT11 Data        │

│  GPIO 3  ──────── PIR Signal        │

│  GPIO 0  ──────── Gas Sensor (AO)   │  ← Analog

│  GPIO 4  ──────── Relay IN          │  ← Active LOW

│  GPIO 5  ──────── Buzzer (+)        │

└─────────────────────────────────────┘

```

&#x20;

| Component | ESP32-C3 Pin | Type |

|-----------|-------------|------|

| OLED SDA | GPIO 8 | I2C Data |

| OLED SCL | GPIO 9 | I2C Clock |

| DHT11 | GPIO 2 | Digital IN |

| PIR Sensor | GPIO 3 | Digital IN |

| Gas Sensor (AO) | GPIO 0 | Analog IN (ADC) |

| Relay Module | GPIO 4 | Digital OUT |

| Buzzer | GPIO 5 | Digital OUT |

&#x20;

> ⚠ \*\*ESP32-C3 Mini only has GPIO 0–10 available.\*\* GPIO 11–21 are used internally for flash and must NOT be connected.

&#x20;

\---

&#x20;

\## 📚 Libraries

&#x20;

Install all libraries via \*\*Arduino IDE → Tools → Manage Libraries\*\*:

&#x20;

| Library | Author | Purpose |

|---------|--------|---------|

| `U8g2` | oliver | SH1106 OLED display driver |

| `DHT sensor library` | Adafruit | DHT11 temperature \& humidity |

| `Adafruit Unified Sensor` | Adafruit | Required by DHT library |

&#x20;

> `WiFi.h` and `WebServer.h` are \*\*built-in\*\* to the ESP32 Arduino core — no installation needed.

&#x20;

\---

&#x20;

\## ⚙ Setup \& Installation

&#x20;

\### 1. Install ESP32 Board Support

&#x20;

In Arduino IDE go to \*\*File → Preferences\*\* and add this URL to "Additional Board Manager URLs":

&#x20;

```

https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package\_esp32\_index.json

```

&#x20;

Then go to \*\*Tools → Board → Boards Manager\*\*, search for `esp32` and install \*\*esp32 by Espressif Systems\*\*.

&#x20;

\### 2. Select the Correct Board

&#x20;

```

Tools → Board    → ESP32C3 Dev Module

Tools → Port     → (your COM port)

Tools → USB CDC on Boot → Enabled   ← important for Serial Monitor

```

&#x20;

\### 3. Configure WiFi Credentials

&#x20;

Open the `.ino` file and update these two lines at the top:

&#x20;

```cpp

const char\* WIFI\_SSID     = "YOUR\_WIFI\_NAME";

const char\* WIFI\_PASSWORD = "YOUR\_WIFI\_PASSWORD";

```

&#x20;

> \*\*Important:\*\* ESP32-C3 only supports \*\*2.4GHz WiFi\*\*. Make sure your network is 2.4GHz.

&#x20;

\### 4. Adjust Safety Thresholds (Optional)

&#x20;

```cpp

\#define GAS\_THRESHOLD   1500   // ADC raw value (0–4095)

\#define TEMP\_THRESHOLD  40.0   // °C

```

&#x20;

\### 5. Upload

&#x20;

Connect ESP32-C3 via USB, select the correct port, and click \*\*Upload\*\*.

&#x20;

\### 6. Access Web Dashboard

&#x20;

After upload, open Serial Monitor at \*\*115200 baud\*\*. The IP address will be printed:

&#x20;

```

✔ IP: 192.168.x.x

Open: http://192.168.x.x

```

&#x20;

The OLED will also show the IP. Open that address in any browser \*\*on the same WiFi network\*\*.

&#x20;

\---

&#x20;

\## 🌐 Web Dashboard

&#x20;

The built-in web server hosts a real-time dashboard with:

&#x20;

\- \*\*Live status banner\*\* — glows green (SAFE) or pulses red (DANGER)

\- \*\*4 sensor cards\*\* — Temperature, Humidity, Gas, Motion with animated gauge bars

\- \*\*Machine \& Alarm state\*\* — shows RUNNING / STOPPED in real time

\- \*\*System info tiles\*\* — IP address, uptime, total alert count, last updated time

\- \*\*Event log\*\* — records every state change with timestamp

\- \*\*Auto-refresh\*\* every 2 seconds

\### API Endpoint

&#x20;

The ESP32 exposes a JSON endpoint at `/data`:

&#x20;

```

GET http://192.168.x.x/data

```

&#x20;

\*\*Example Response:\*\*

```json

{

&#x20; "temperature": 28.5,

&#x20; "humidity": 65.0,

&#x20; "gas": 420,

&#x20; "motion": false,

&#x20; "danger": false,

&#x20; "dangerGas": false,

&#x20; "dangerTemp": false,

&#x20; "dangerMotion": false,

&#x20; "ip": "192.168.179.194",

&#x20; "uptime": 3600,

&#x20; "alertCount": 2

}

```

&#x20;

\---

&#x20;

\## 🧠 Working Principle

&#x20;

```

┌────────────────────────────────────────────────────┐

│              SENSOR READING (every 1s)             │

│   DHT11 → Temp \& Humidity                         │

│   MQ Gas → Analog ADC value                       │

│   PIR → Motion HIGH/LOW                           │

└─────────────────────┬──────────────────────────────┘

&#x20;                     │

&#x20;                     ▼

┌────────────────────────────────────────────────────┐

│              SAFETY EVALUATION                     │

│   Gas  >= 1500   → dangerGas    = true            │

│   Temp >= 40°C   → dangerTemp   = true            │

│   PIR  == HIGH   → dangerMotion = true            │

└─────────────────────┬──────────────────────────────┘

&#x20;                     │

&#x20;         ┌───────────┴───────────┐

&#x20;         │ ANY danger = true?    │

&#x20;         └───────────┬───────────┘

&#x20;                YES  │  NO

&#x20;         ┌──────────┘  └──────────┐

&#x20;         ▼                        ▼

&#x20;  ┌─────────────┐         ┌─────────────┐

&#x20;  │  DANGER     │         │   SAFE      │

&#x20;  │ Relay → OFF │         │ Relay → ON  │

&#x20;  │ Buzzer → ON │         │ Buzzer → OFF│

&#x20;  │ OLED DANGER │         │ OLED SAFE   │

&#x20;  │ Web → RED   │         │ Web → GREEN │

&#x20;  └─────────────┘         └─────────────┘

```

&#x20;

\---

&#x20;

\## 🚨 Safety Trigger Table

&#x20;

| Sensor | Condition | Action |

|--------|-----------|--------|

| Gas (MQ) | ADC reading ≥ 1500 | ⛔ Shutdown + Alarm |

| Temperature | ≥ 40°C | ⛔ Shutdown + Alarm |

| PIR Motion | Motion detected | ⛔ Shutdown + Alarm |

| All normal | All below threshold | ✅ Machine runs |

&#x20;

\---

&#x20;

\## 🏭 Industrial Applications

&#x20;

\- Factory floor safety monitoring

\- Gas leakage detection systems

\- Unauthorized access detection

\- Server room temperature monitoring

\- Warehouse environmental control

\- Machine protection systems

\- Smart automation controllers

\---

&#x20;

\## 🚀 Future Improvements

&#x20;

\- \[ ] MQTT protocol integration (Node-RED / Home Assistant)

\- \[ ] Blynk IoT mobile app alerts

\- \[ ] Push notifications via Telegram bot

\- \[ ] Data logging to SD card or Firebase

\- \[ ] Multiple sensor zones with zone ID

\- \[ ] Historical graph on web dashboard

\- \[ ] OTA (Over-The-Air) firmware updates

\- \[ ] PCB design for production-ready board

\---

&#x20;

\## 📂 Repository Structure

&#x20;

```

Smart-Industrial-Safety-System/

│

├── SmartIndustrialSafety\_v3\_3.ino    ← Main Arduino source code

├── README.md                          ← This file

├── LICENSE                            ← MIT License

│

└── images/

&#x20;   ├── circuit\_diagram.jpg            ← Wiring diagram

&#x20;   ├── oled\_safe.jpg                  ← OLED SAFE screen

&#x20;   ├── oled\_danger.jpg                ← OLED DANGER screen

&#x20;   └── web\_dashboard.jpg              ← Web server screenshot

```

&#x20;

\---

&#x20;

\## 👨‍💻 Author

&#x20;

\*\*Nimna\*\*  

Electronic \& Telecommunication Engineering Student  

Institute of Technology, University of Moratuwa (ITUM)  

Sri Lanka 🇱🇰

&#x20;

\[!\[GitHub](https://img.shields.io/badge/GitHub-Follow-181717?style=flat\&logo=github)](https://github.com/YOUR\_USERNAME)

\[!\[LinkedIn](https://img.shields.io/badge/LinkedIn-Connect-0077B5?style=flat\&logo=linkedin)](https://linkedin.com/in/YOUR\_PROFILE)

&#x20;

\---

&#x20;

\## 📄 License

&#x20;

This project is licensed under the \*\*MIT License\*\* — see the \[LICENSE](LICENSE) file for details.

&#x20;

\---

&#x20;

<div align="center">

⭐ \*\*If this project helped you, please give it a star!\*\* ⭐

&#x20;

\*Built with ❤ for embedded systems engineering\*

&#x20;

</div>

&#x20;

