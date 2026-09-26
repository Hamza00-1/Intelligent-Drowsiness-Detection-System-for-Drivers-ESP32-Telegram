# 🚀 Quick Start & Deployment Guide

Welcome! Here is everything you need to quickly set up, configure, and deploy the **AI-Enhanced ESP32 Driver Safety System** based on **MPU6050 + MAX30100 + Telegram**.

---

## 🚘 System Overview

This system monitors driver alertness directly from the vehicle **steering wheel** using multi-modal sensing and **predictive AI**:
1. **MPU6050 (6-Axis IMU)**: Detects continuous steering wheel micro-adjustments and angular motion.
2. **MAX30100 (Pulse Oximeter)**: Measures continuous **heart rate**, **blood oxygen (SpO2)**, and confirms **hand presence on the wheel**.
3. **🧠 On-Device TinyML Engine**: Computes real-time **Heart Rate Variability (HRV - RMSSD)** and steering motion variance to compute a **0–100% Fatigue Index** directly on the ESP32. It warns the driver **before** microsleep occurs!
4. **Stabilization Phase (8s)**: At boot, calibrates sensor baseline drift and warm-up (Yellow LED pulses gently).
5. **Pre-Alert Phase (after 4.5s of inactivity)**: Yellow LED turns solid and a soft periodic beep warns the driver to place hands on the wheel.
6. **Critical Alarm Phase (after 8.0s of inactivity)**: Red strobe flashes, loud continuous buzzer activates, and an emergency **Telegram alert** is dispatched over Wi-Fi with live biometric vitals.

---

## 📱 Interactive Telegram Commands

The vehicle unit can be queried remotely at any time via Telegram:
* `/ai` $\rightarrow$ Live TinyML Fatigue Score (0–100%), HRV in ms, and steering variance.
* `/vitals` $\rightarrow$ Live Heart Rate (BPM), SpO2 (%), and hand-on-wheel status.
* `/status` $\rightarrow$ System state, Wi-Fi RSSI signal strength, and total incident counter.
* `/test` $\rightarrow$ Runs a 1-second alarm and LED diagnostic test.
* `/help` $\rightarrow$ Lists all available commands.

---

## 🛠️ Deployment Checklist

### 1. Hardware Required:
* [ ] 1x ESP32 Dev Board (ESP32-WROOM-32 or NodeMCU ESP32) + Micro-USB cable
* [ ] 1x MPU6050 6-Axis IMU Module
* [ ] 1x MAX30100 Pulse Oximeter Module
* [ ] 1x Active Buzzer (5V / 3.3V)
* [ ] 1x Red LED + 1x Yellow LED + 2x 220Ω resistors
* [ ] Breadboard & Jumper wires

### 2. Wiring (Shared I2C Bus on GPIO 21 & 22):
Both sensors share the same I2C lines:
* **MPU6050 SDA** AND **MAX30100 SDA** $\rightarrow$ **ESP32 GPIO 21**
* **MPU6050 SCL** AND **MAX30100 SCL** $\rightarrow$ **ESP32 GPIO 22**
* **MPU6050 VCC** & **MAX30100 VCC** $\rightarrow$ **3.3V (or VIN)**
* **GND** $\rightarrow$ **GND** (Common Ground)
* **Buzzer (+)** $\rightarrow$ **GPIO 25**
* **Red LED (+)** $\rightarrow$ **GPIO 26** (through 220Ω resistor)
* **Yellow LED (+)** $\rightarrow$ **GPIO 27** (through 220Ω resistor)

### 3. Configure `config.h`:
Open [`src/esp32_drowsiness_telegram/config.h`](file:///d:/anti/src/esp32_drowsiness_telegram/config.h) and set:
* `WIFI_SSID` & `WIFI_PASSWORD` (Your local Wi-Fi or mobile hotspot)
* `BOT_TOKEN` (Generated via `@BotFather` on Telegram)
* `CHAT_ID` (Your Telegram numeric user/chat ID from `@userinfobot`)

### 4. Flash Firmware via Arduino IDE:
1. In Arduino IDE Library Manager (**Sketch > Include Library > Manage Libraries...**), install:
   * **`MAX30100lib`** (by OXullo Intervent)
   * **`UniversalTelegramBot`** (by Brian Lough)
   * **`ArduinoJson`** (by Benoit Blanchon — **v6.x**)
2. Open [`esp32_drowsiness_telegram.ino`](file:///d:/anti/src/esp32_drowsiness_telegram/esp32_drowsiness_telegram.ino), select your ESP32 board and COM port, and click **Upload (➡️)**!

---

📖 *For comprehensive documentation, wiring schematics, and ML training scripts, refer to the main [README.md](file:///d:/anti/README.md).*
