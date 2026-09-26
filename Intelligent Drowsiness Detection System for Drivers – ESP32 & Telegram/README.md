# 🚘 Intelligent Drowsiness Detection System for Drivers (ESP32, MPU6050, MAX30100 & Telegram)
### 🧠 Now Enhanced with Real-Time TinyML Biometric & Kinematic Fatigue Inference

An end-to-end IoT and AI-driven Driver Safety Monitoring System developed on the **ESP32** microcontroller. Instead of relying solely on cameras or simple timers, this system detects driver alertness through multi-modal physical and physiological channels:
1. **MPU6050 (6-Axis IMU)**: Monitors steering wheel micro-adjustments, motion variance, and steering jerk.
2. **MAX30100 (Pulse Oximeter)**: Measures continuous heart rate, blood oxygen (SpO2), hand presence on the wheel, and Inter-Beat-Intervals (IBI).
3. **On-Device AI Engine**: Computes real-time **Heart Rate Variability (HRV - RMSSD)**, steering entropy, and an on-chip **0–100% Fatigue Index** to predict driver micro-sleep **before** an accident happens.
4. **Cloud Telemetry**: Encrypted Telegram Bot reporting with live vitals, health anomaly detection, and two-way commands (`/ai`, `/vitals`, `/status`, `/test`).

---

## 📁 Repository Structure

```text
d:\anti/
│
├── 📁 src/
│   └── 📁 esp32_drowsiness_telegram/
│       ├── config.h                       <-- ⚙️ EDIT THIS FILE ONLY! (Wi-Fi, Telegram, AI thresholds)
│       └── esp32_drowsiness_telegram.ino  <-- 🚀 Main ESP32 Firmware with on-device AI Engine
│
├── 📁 ai_companion/
│   ├── drowsiness_ai_detector.py          <-- 👁️ Computer Vision Face Mesh (EAR + MAR)
│   ├── ml_data_logger_and_trainer.py      <-- 🔬 Machine Learning Dataset Trainer (Random Forest)
│   ├── requirements.txt                   <-- 📦 Python libraries
│   └── run_ai_detector.bat                <-- ⚡ One-click launcher for Windows
│
├── README.md                              <-- 📖 Master System Manual (This file)
└── QUICKSTART.md                          <-- 🚀 Quick setup & deployment guide
```

---

## 🧠 The AI Predictive Fatigue Engine (How It Works)

Traditional systems only sound an alarm after the driver has already passed out. This system calculates a **continuous 0–100% Fatigue Index** in real-time on the ESP32:

$$\text{Fatigue Index} = w_1 \cdot \text{Inactivity Factor} + w_2 \cdot \text{Steering Entropy} + w_3 \cdot \text{HRV / Cardiac Drop}$$

### 1. Heart Rate Variability (HRV - RMSSD)
* The firmware calculates the **Root Mean Square of Successive Differences (RMSSD)** between heartbeats via the MAX30100 interrupt.
* During fatigue and transition to drowsiness, autonomic nervous system tone shifts, causing a recognizable drop and instability in RMSSD.

### 2. Steering Dynamics & Entropy
* **Alert Driver**: Steady micro-corrections (healthy variance).
* **Drowsy Driver**: Long periods of flatline zero-motion, followed by sudden, abrupt jerky corrections (overshoots).

### 3. Early Warning Dispatch
* If the AI Fatigue Index reaches **$\ge 75\%$**, the ESP32 sends a predictive warning:
  > *"⚠️ AI PREDICTIVE FATIGUE WARNING: Fused Fatigue Index is 82%. Heart Rate Variability has dropped to 18.4 ms. Driver is advised to rest before microsleep occurs!"*

---

## ⚙️ System Phases

```text
 ┌────────────────────────────────────────────────────────────────────────┐
 │                     PHASE 1: STABILIZATION (8s)                        │
 │  • Calibrates baseline sensor drift on boot.                           │
 │  • Yellow LED pulses gently while the driver settles into the seat.    │
 └──────────────────────────────────┬─────────────────────────────────────┘
                                    │
                                    ▼
 ┌────────────────────────────────────────────────────────────────────────┐
 │                     PHASE 2: NORMAL AI MONITORING                      │
 │  • Hand on wheel (MAX30100) + Steering micro-motion (MPU6050).         │
 │  • Continuous HRV & steering variance updates. Alarms remain OFF.      │
 └──────────────────────────────────┬─────────────────────────────────────┘
                                    │ (Inactivity > 4.5s OR High AI Fatigue)
                                    ▼
 ┌────────────────────────────────────────────────────────────────────────┐
 │                     PHASE 3: PRE-ALERT WARNING                         │
 │  • Yellow LED turns solid ON + gentle periodic reminder beep.          │
 │  • Prompts the driver to place hands on the wheel or correct steering. │
 └──────────────────────────────────┬─────────────────────────────────────┘
                                    │ (Inactivity > 8.0s: Driver unresponsive)
                                    ▼
 ┌────────────────────────────────────────────────────────────────────────┐
 │                     PHASE 4: CRITICAL ALARM & TELEGRAM                 │
 │  • Continuous high-decibel siren sounds.                               │
 │  • Rapid Red strobe flashes.                                           │
 │  • Real-time emergency Telegram alert with live vitals:                │
 │    "🚨 Critical Alert: Driver Inactive! HR: 74 BPM | SpO2: 98%"        │
 └────────────────────────────────────────────────────────────────────────┘
```

---

## 🔌 Hardware Wiring & Pinout (Shared I2C Bus)

Both the **MPU6050** and the **MAX30100** share **GPIO 21 (SDA)** and **GPIO 22 (SCL)**:

```text
                        +----------------------------------+
                        |           ESP32 BOARD            |
                        |                                  |
 [MPU6050 IMU]          |                                  |
 VCC -----------------> | 3V3 / VIN                        |
 GND -----------------> | GND                              |
 SDA -----------------> | GPIO 21 (I2C SDA) <──────────────┼────+
 SCL -----------------> | GPIO 22 (I2C SCL) <──────────────┼──+ │
                        |                                  |  │ │
 [MAX30100 OXIMETER]    |                                  |  │ │
 VCC -----------------> | 3V3 (or VIN depending on module) |  │ │
 GND -----------------> | GND                              |  │ │
 SDA ---------------------------------------------------------+ │
 SCL -----------------------------------------------------------+
                        |                                  |
 [ACTIVE BUZZER]        |                                  |
 POS (+) -------------> | GPIO 25                          |
 NEG (-) -------------> | GND                              |
                        |                                  |
 [RED CRITICAL LED]     |                                  |
 Anode (+) -----------> | GPIO 26 ─── [ 220Ω Resistor ] ───+
 Cathode (-) ---------> | GND                              |
                        |                                  |
 [YELLOW PRE-ALERT LED] |                                  |
 Anode (+) -----------> | GPIO 27 ─── [ 220Ω Resistor ] ───+
 Cathode (-) ---------> | GND                              |
                        +----------------------------------+
```

---

## 📱 Telegram Interactive Commands

Users and fleet supervisors can message the bot anytime to inspect the system in real time:

| Command | Action |
| :--- | :--- |
| `/ai` | Returns the **TinyML Fatigue Index (0–100%)**, risk category, HRV (RMSSD in ms), and steering variance. |
| `/vitals` | Returns real-time **Heart Rate (BPM)**, **SpO2 (%)**, and hand-on-wheel status. |
| `/status` | Returns overall system state, Wi-Fi RSSI signal strength, and total incident counter. |
| `/test` | Triggers a 1-second physical test of the buzzer, yellow LED, and red LED. |
| `/help` | Displays the list of available commands. |

---

## 🛠️ Step-by-Step Setup in Arduino IDE

### 1. Install Required Arduino Libraries
Open Arduino IDE > **Sketch** > **Include Library** > **Manage Libraries...** and install:
1. **`MAX30100lib`** (by OXullo Intervent) — For MAX30100 pulse oximeter & beat detection.
2. **`UniversalTelegramBot`** (by Brian Lough) — For Telegram HTTPS Bot communication.
3. **`ArduinoJson`** (by Benoit Blanchon) — **Select version 6.x**!

*(Note: MPU6050 motion processing uses native direct I2C registers in the firmware, requiring zero external Adafruit dependencies!)*

### 2. Configure Your Settings in `config.h`
Open [`src/esp32_drowsiness_telegram/config.h`](file:///d:/anti/src/esp32_drowsiness_telegram/config.h) and set:
```cpp
#define WIFI_SSID     "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define BOT_TOKEN     "YOUR_TELEGRAM_BOT_TOKEN"   // From @BotFather on Telegram
#define CHAT_ID       "YOUR_TELEGRAM_CHAT_ID"     // From @userinfobot on Telegram
```

### 3. Upload & Run
* Connect ESP32 via USB.
* Select **Tools > Board > ESP32 Dev Module** and choose your **COM Port**.
* Click **Upload (➡️)** and open the **Serial Monitor (115200 baud)**.

---

## 📝 Updated LinkedIn Description (Copy & Paste Ready)

```text
Title: Intelligent Drowsiness Detection System for Drivers – ESP32, MPU6050, MAX30100 & Telegram

Description:
Developed an IoT and Edge-AI Driver Safety and Drowsiness Detection System on an ESP32 microcontroller designed to prevent road accidents through multi-sensor kinematic and biometric fusion.

Key Engineering Highlights:
• Multi-Sensor Kinematic & Biometric Fusion: Integrated an MPU6050 (6-axis IMU) to monitor continuous steering wheel micro-adjustments and a MAX30100 pulse oximeter on the steering grip to measure heart rate, blood oxygen (SpO2), and confirm hand presence on the wheel.
• On-Device Predictive AI Engine: Engineered an embedded TinyML-inspired inference algorithm calculating real-time Heart Rate Variability (HRV - RMSSD) and steering motion entropy to generate a continuous 0–100% Fatigue Index, predicting driver microsleep prior to total disengagement.
• Phased Alert Architecture: Engineered a finite-state machine incorporating an 8-second sensor stabilization/warm-up phase, a gentle pre-alert phase (yellow LED and intermittent acoustic warning), and a full critical alarm phase (rapid red strobe and siren).
• Cloud Emergency Dispatch: Configured the Telegram Bot API over secure TLS/SSL (WiFiClientSecure) to transmit instant encrypted alerts to fleet managers and emergency contacts with real-time biometric vitals and duration telemetry when prolonged inactivity is detected.
• Remote Telemetry & Diagnostics: Implemented two-way non-blocking Telegram commands (/ai, /vitals, /status, /test) for real-time remote diagnostics of sensor health, RSSI, and incident counters.

Skills: ESP32, Internet of Things (IoT), Edge AI, TinyML, Embedded C++, I2C Protocol, MPU6050, MAX30100, Biometric Sensing, Telegram Bot API, Microcontrollers.
```
