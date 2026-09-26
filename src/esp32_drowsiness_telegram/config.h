#ifndef CONFIG_H
#define CONFIG_H

// ==============================================================================
// 🚗 DRIVER SAFETY SYSTEM - USER CONFIGURATION FILE
// ==============================================================================
// Hardware Architecture:
//  - Microcontroller: ESP32
//  - Sensor 1: MPU6050 (Steering Wheel Movement & Micro-adjustments via I2C)
//  - Sensor 2: MAX30100 (Pulse Oximeter, Vitals & Hand-on-Wheel Detection via I2C)
//  - AI Engine: Real-Time TinyML-inspired Fatigue Index & Biometric Anomaly Detection
//  - Actuators: Active Buzzer + Dual LEDs (Yellow Pre-Alert, Red Critical)
//  - Cloud: Telegram Bot API over Wi-Fi
// ==============================================================================

// 1. Wi-Fi Credentials
#define WIFI_SSID                   "YOUR_WIFI_NAME"
#define WIFI_PASSWORD               "YOUR_WIFI_PASSWORD"

// 2. Telegram Bot Configuration
// Get BOT_TOKEN from @BotFather, and CHAT_ID from @userinfobot on Telegram
#define BOT_TOKEN                   "YOUR_TELEGRAM_BOT_TOKEN"
#define CHAT_ID                     "YOUR_TELEGRAM_CHAT_ID"

// 3. Hardware Pinout (ESP32)
// I2C Bus Pins (Shared by both MPU6050 & MAX30100)
#define PIN_I2C_SDA                 21     // GPIO 21 -> SDA on both sensors
#define PIN_I2C_SCL                 22     // GPIO 22 -> SCL on both sensors

// Alert Actuators
#define PIN_BUZZER                  25     // GPIO 25 -> Active Buzzer (+)
#define PIN_LED_RED_CRITICAL        26     // GPIO 26 -> Red LED (Critical Alarm)
#define PIN_LED_YELLOW_PREALERT     27     // GPIO 27 -> Yellow/Amber LED (Pre-Alert Warning)
#define PIN_LED_STATUS              2      // GPIO 2  -> Onboard Blue LED (System & Wi-Fi Status)

// 4. Timing & State Thresholds (in milliseconds)
#define STABILIZATION_TIME_MS       8000   // 8 seconds stabilization/calibration at boot
#define PRE_ALERT_TIME_MS           4500   // 4.5 seconds -> Triggers soft pre-alert (LED / Beep)
#define CRITICAL_ALERT_TIME_MS      8000   // 8.0 seconds -> Triggers full alarm + Telegram alert
#define TELEGRAM_COOLDOWN_MS        25000  // 25 seconds cooldown between cloud alert messages
#define BOT_CHECK_INTERVAL_MS       2000   // 2.0 seconds polling for incoming Telegram commands

// 5. Sensor Detection Sensitivity Thresholds
#define STEERING_MOTION_THRESHOLD   18.0f  // Minimum angular rate delta (deg/s) for active steering
#define HAND_PRESENCE_IR_THRESHOLD  22000  // Minimum raw reflection for hand presence

// 6. 🧠 AI & PREDICTIVE FATIGUE SETTINGS
#define ENABLE_AI_PREDICTOR         true   // Set to true to enable on-device TinyML fatigue scoring
#define AI_FATIGUE_ALERT_SCORE      75     // Score (0-100%) above which AI Early Warning triggers
#define HRV_WINDOW_SIZE             12     // Rolling window of heartbeat intervals for HRV calculation
#define STEERING_SAMPLE_WINDOW      20     // Rolling window of steering motion samples

// 7. 🏥 BIOMETRIC HEALTH ANOMALY LIMITS
#define MIN_SAFE_SPO2               90     // Blood oxygen below 90% triggers hypoxia warning
#define MIN_SAFE_BPM                45     // Extreme bradycardia threshold (BPM)
#define MAX_SAFE_BPM                140    // Extreme tachycardia threshold while driving (BPM)

#endif // CONFIG_H
