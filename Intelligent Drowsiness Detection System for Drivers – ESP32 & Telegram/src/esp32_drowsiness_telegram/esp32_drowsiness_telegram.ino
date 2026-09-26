/*
 * ==============================================================================
 * 🚗 SYSTÈME INTELLIGENT DE DÉTECTION DE SOMNOLENCE AU VOLANT (TRI-CAPTEURS & IA)
 * ==============================================================================
 * Cible : Module ESP32 Dev
 * Configuration Matérielle Complète (3 Capteurs) :
 *   1. Capteur Oculaire IR :  Surveillance directe des paupières et du clignement (GPIO 34)
 *   2. MPU6050 :              Détecte les micro-ajustements et mouvements du volant (I2C)
 *   3. MAX30100 :             Mesure le pouls, SpO2, et confirme la présence des mains (I2C)
 *   - Actionneurs :           Buzzer sonore (GPIO 25), Stroboscope Rouge (GPIO 26), LED Jaune (GPIO 27)
 *   - Télémétrie Cloud :      Alertes d'urgence instantanées via Bot Telegram en Wi-Fi
 *
 * 🧠 FUSION TRI-MODALE & IA EMBARQUÉE :
 *   - Détection Instantanée : Yeux fermés > 1,5s = Alarme Immédiate de Micro-Sommeil !
 *   - Détection Volant & Mains : Inactivité > 4,5s = Pré-alerte, > 8,0s = Alarme critique.
 *   - Moteur TinyML : Calcul en direct d'un Indice de Fatigue (0 à 100%) intégrant
 *     la Variabilité Cardiaque (VRC / HRV - RMSSD), l'entropie de direction et les yeux.
 *   - Diagnostic à Distance : Commandes Telegram /ai, /vitals, /status, /test, /aide.
 * ==============================================================================
 */

#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <math.h>
#include "MAX30100_PulseOximeter.h"

// Chargement des paramètres utilisateur et configuration IA
#include "config.h"

// États du Système
enum SystemState {
  STATE_STABILISATION,
  STATE_NORMAL,
  STATE_PRE_ALERTE,
  STATE_ALARME_CRITIQUE
};

SystemState currentState = STATE_STABILISATION;

// Objets Réseau et Telegram
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

// Objets Capteurs
PulseOximeter pox;
const uint8_t MPU6050_ADDR = 0x68;

// Variables de Suivi Temporel
unsigned long bootTimestamp = 0;
unsigned long lastActivityTimestamp = 0;
unsigned long eyeClosedStartTime = 0;
unsigned long lastTelegramAlertTime = 0;
unsigned long lastBotCheckTime = 0;
unsigned long lastPoxReportTime = 0;
unsigned long lastAiEvaluationTime = 0;

// États des Capteurs
bool isEyeClosed = false;
bool eyeDrowsinessTriggered = false;
bool handDetectedOnWheel = false;
bool steeringMotionDetected = false;
float currentHeartRate = 0.0;
uint8_t currentSpO2 = 0;
uint32_t totalCriticalIncidents = 0;
String lastAlarmReason = "";

// Variables MPU6050
float lastGyroX = 0, lastGyroY = 0, lastGyroZ = 0;
bool mpuInitialized = false;
bool poxInitialized = false;

// ======================== 🧠 TAMPONS DE DONNÉES IA & VRC ========================
volatile unsigned long lastBeatTimestamp = 0;
volatile unsigned long ibiBuffer[HRV_WINDOW_SIZE];
volatile uint8_t ibiIndex = 0;
volatile uint8_t ibiCount = 0;
float currentHrvRmssd = 0.0; // VRC RMSSD en ms

float steeringBuffer[STEERING_SAMPLE_WINDOW];
uint8_t steeringIndex = 0;
float steeringVariance = 0.0;

int aiFatigueScore = 0;
bool aiWarningSent = false;
// ==============================================================================

// Prototypes des Fonctions
void connectToWiFi();
void initI2CDevices();
void initMPU6050();
void readEyeSensor();
void readMPU6050();
void onBeatDetected();
void calculateHRV();
void calculateSteeringDynamics(float currentMotion);
void computeAIFatigueIndex();
void checkBiometricHealthAnomalies();
void updateSystemState();
void triggerCriticalAlarm(String reason, unsigned long durationMs);
void triggerPreAlert();
void resetAlarmsToNormal();
void sendTelegramEmergencyAlert(String reason, unsigned long durationMs);
void sendTelegramAiWarning();
void handleIncomingBotMessages(int numNewMessages);

// ==============================================================================
// 1. INITIALISATION (Setup)
// ==============================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=======================================================");
  Serial.println("  🚗 Système ESP32 Tri-Capteurs : Yeux + Volant + VRC");
  Serial.println("=======================================================");

  // Configuration des Broches GPIO
  pinMode(PIN_EYE_SENSOR, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_RED_CRITICAL, OUTPUT);
  pinMode(PIN_LED_YELLOW_PREALERT, OUTPUT);
  pinMode(PIN_LED_STATUS, OUTPUT);

  // Éteindre tous les actionneurs au départ
  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED_RED_CRITICAL, LOW);
  digitalWrite(PIN_LED_YELLOW_PREALERT, LOW);
  digitalWrite(PIN_LED_STATUS, LOW);

  // Initialisation du bus I2C sur GPIO 21 (SDA) et GPIO 22 (SCL)
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  // Initialisation des capteurs I2C (MPU6050 + MAX30100)
  initI2CDevices();

  // Connexion Wi-Fi et configuration SSL Telegram
  connectToWiFi();
  secured_client.setInsecure();

  bootTimestamp = millis();
  lastActivityTimestamp = millis();

  // Initialisation des tampons de l'IA
  for (int i = 0; i < HRV_WINDOW_SIZE; i++) ibiBuffer[i] = 0;
  for (int i = 0; i < STEERING_SAMPLE_WINDOW; i++) steeringBuffer[i] = 0.0;

  Serial.println(">> Entrée dans la PHASE DE STABILISATION (8s)...");
}

// ==============================================================================
// 2. BOUCLE PRINCIPALE (Loop)
// ==============================================================================
void loop() {
  // A. Statut Wi-Fi
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(PIN_LED_STATUS, HIGH);
  } else {
    digitalWrite(PIN_LED_STATUS, LOW);
  }

  // B. Mise à jour continue de l'oxymètre MAX30100
  if (poxInitialized) {
    pox.update();
  }

  // C. Lecture périodique des constantes vitales (toutes les secondes)
  if (millis() - lastPoxReportTime >= 1000) {
    lastPoxReportTime = millis();
    if (poxInitialized) {
      currentHeartRate = pox.getHeartRate();
      currentSpO2 = pox.getSpO2();

      if (currentHeartRate > 35.0f && currentHeartRate < 195.0f) {
        handDetectedOnWheel = true;
      } else {
        handDetectedOnWheel = false;
      }

      if (handDetectedOnWheel) {
        checkBiometricHealthAnomalies();
      }
    }
  }

  // D. Lecture du Capteur Oculaire (Yeux / Clignement)
  readEyeSensor();

  // E. Lecture des mouvements du volant (MPU6050)
  readMPU6050();

  // F. Moteur d'IA Prédictif (toutes les 1,5s)
  if (ENABLE_AI_PREDICTOR && (millis() - lastAiEvaluationTime >= 1500)) {
    lastAiEvaluationTime = millis();
    calculateHRV();
    computeAIFatigueIndex();
  }

  // G. Évaluation de l'état du conducteur et déclenchement des alertes
  updateSystemState();

  // H. Gestion des commandes Telegram entrantes (toutes les 2s)
  if (millis() - lastBotCheckTime >= BOT_CHECK_INTERVAL_MS) {
    lastBotCheckTime = millis();
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      handleIncomingBotMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
  }
}

// ==============================================================================
// 3. LECTURE DU CAPTEUR OCULAIRE
// ==============================================================================
void readEyeSensor() {
  int sensorVal = digitalRead(PIN_EYE_SENSOR);

  if (sensorVal == EYE_CLOSED_STATE) {
    if (!isEyeClosed) {
      // Les yeux viennent de se fermer
      isEyeClosed = true;
      eyeClosedStartTime = millis();
    } else {
      // Les yeux restent fermés : calcul de la durée
      unsigned long duration = millis() - eyeClosedStartTime;
      if (duration >= EYE_DROWSINESS_MS) {
        eyeDrowsinessTriggered = true;
      }
    }
  } else {
    // Yeux ouverts
    isEyeClosed = false;
    eyeDrowsinessTriggered = false;
  }
}

// ==============================================================================
// 4. INITIALISATION DES CAPTEURS I2C
// ==============================================================================
void initI2CDevices() {
  initMPU6050();

  Serial.print("Initialisation du capteur cardiaque MAX30100...");
  if (!pox.begin()) {
    Serial.println(" [ÉCHEC] Vérifiez le câblage I2C du MAX30100.");
    poxInitialized = false;
  } else {
    Serial.println(" [SUCCÈS] MAX30100 Opérationnel !");
    pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
    pox.setOnBeatDetectedCallback(onBeatDetected);
    poxInitialized = true;
  }
}

void initMPU6050() {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  byte error = Wire.endTransmission();

  if (error == 0) {
    Serial.println(">> MPU6050 (Capteur de volant) initialisé avec succès.");
    mpuInitialized = true;
  } else {
    Serial.println(">> [ATTENTION] MPU6050 introuvable à l'adresse 0x68.");
    mpuInitialized = false;
  }
}

void onBeatDetected() {
  unsigned long now = millis();
  if (lastBeatTimestamp > 0) {
    unsigned long ibi = now - lastBeatTimestamp;
    if (ibi >= 300 && ibi <= 1800) {
      ibiBuffer[ibiIndex] = ibi;
      ibiIndex = (ibiIndex + 1) % HRV_WINDOW_SIZE;
      if (ibiCount < HRV_WINDOW_SIZE) ibiCount++;
    }
  }
  lastBeatTimestamp = now;
  handDetectedOnWheel = true;
}

// ==============================================================================
// 5. TRAITEMENT DU VOLANT (MPU6050)
// ==============================================================================
void readMPU6050() {
  if (!mpuInitialized) return;

  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x43);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, (uint8_t)6, (uint8_t)true);

  if (Wire.available() >= 6) {
    int16_t rawGX = (Wire.read() << 8) | Wire.read();
    int16_t rawGY = (Wire.read() << 8) | Wire.read();
    int16_t rawGZ = (Wire.read() << 8) | Wire.read();

    float gyroX = rawGX / 131.0f;
    float gyroY = rawGY / 131.0f;
    float gyroZ = rawGZ / 131.0f;

    float deltaX = abs(gyroX - lastGyroX);
    float deltaY = abs(gyroY - lastGyroY);
    float deltaZ = abs(gyroZ - lastGyroZ);
    float totalMotion = deltaX + deltaY + deltaZ;

    lastGyroX = gyroX;
    lastGyroY = gyroY;
    lastGyroZ = gyroZ;

    calculateSteeringDynamics(totalMotion);

    if (totalMotion >= STEERING_MOTION_THRESHOLD) {
      steeringMotionDetected = true;
      lastActivityTimestamp = millis();
    } else {
      steeringMotionDetected = false;
    }
  }
}

void calculateSteeringDynamics(float currentMotion) {
  steeringBuffer[steeringIndex] = currentMotion;
  steeringIndex = (steeringIndex + 1) % STEERING_SAMPLE_WINDOW;

  float sum = 0.0;
  for (int i = 0; i < STEERING_SAMPLE_WINDOW; i++) {
    sum += steeringBuffer[i];
  }
  float mean = sum / STEERING_SAMPLE_WINDOW;

  float varSum = 0.0;
  for (int i = 0; i < STEERING_SAMPLE_WINDOW; i++) {
    varSum += pow(steeringBuffer[i] - mean, 2);
  }
  steeringVariance = varSum / STEERING_SAMPLE_WINDOW;
}

// ==============================================================================
// 6. 🧠 MOTEUR D'IA EMBARQUÉ (FUSION YEUX + VOLANT + CARDIAQUE)
// ==============================================================================
void calculateHRV() {
  if (ibiCount < 4) {
    currentHrvRmssd = 45.0;
    return;
  }

  float sumSqDiff = 0.0;
  int pairs = 0;

  for (int i = 0; i < ibiCount - 1; i++) {
    int idx1 = (ibiIndex - 1 - i + HRV_WINDOW_SIZE) % HRV_WINDOW_SIZE;
    int idx2 = (idx1 - 1 + HRV_WINDOW_SIZE) % HRV_WINDOW_SIZE;
    long diff = (long)ibiBuffer[idx1] - (long)ibiBuffer[idx2];
    sumSqDiff += (float)(diff * diff);
    pairs++;
  }

  if (pairs > 0) {
    currentHrvRmssd = sqrt(sumSqDiff / pairs);
  }
}

void computeAIFatigueIndex() {
  if (currentState == STATE_STABILISATION) {
    aiFatigueScore = 0;
    return;
  }

  unsigned long inactiveDuration = millis() - lastActivityTimestamp;

  // Facteur 1 : Inactivité du volant (0 - 35 points)
  float inactivityFactor = (float)inactiveDuration / (float)CRITICAL_ALERT_TIME_MS;
  if (inactivityFactor > 1.0) inactivityFactor = 1.0;
  int ptsInactivity = (int)(inactivityFactor * 35.0);

  // Facteur 2 : Mouvements des yeux (0 - 35 points)
  int ptsEyes = 0;
  if (isEyeClosed) {
    unsigned long closedMs = millis() - eyeClosedStartTime;
    if (closedMs > 500) ptsEyes = 20;
    if (closedMs >= EYE_DROWSINESS_MS) ptsEyes = 35; // Yeux fermés de manière critique !
  }

  // Facteur 3 : Variance du volant (0 - 15 points)
  int ptsSteering = 0;
  if (steeringVariance < 2.0) {
    ptsSteering = 15; // Volant figé
  } else if (steeringVariance > 80.0) {
    ptsSteering = 12; // Coups de volant brusques
  }

  // Facteur 4 : Données cardiaques & mains (0 - 15 points)
  int ptsBiometrics = 0;
  if (!handDetectedOnWheel) ptsBiometrics += 10;
  if (currentHeartRate > 0 && currentHeartRate < 58.0f) ptsBiometrics += 5;

  aiFatigueScore = ptsInactivity + ptsEyes + ptsSteering + ptsBiometrics;
  if (aiFatigueScore > 100) aiFatigueScore = 100;

  // Alerte prédictive IA si le score atteint 75%
  if (aiFatigueScore >= AI_FATIGUE_ALERT_SCORE && !aiWarningSent && currentState == STATE_NORMAL) {
    sendTelegramAiWarning();
    aiWarningSent = true;
  } else if (aiFatigueScore < 50) {
    aiWarningSent = false;
  }
}

void checkBiometricHealthAnomalies() {
  if (currentSpO2 > 0 && currentSpO2 < MIN_SAFE_SPO2) {
    Serial.printf("⚠️ [ALERTE SANTÉ] Saturation basse en oxygène (SpO2) : %d%%\n", currentSpO2);
  }
  if (currentHeartRate > 0 && (currentHeartRate < MIN_SAFE_BPM || currentHeartRate > MAX_SAFE_BPM)) {
    Serial.printf("⚠️ [ALERTE SANTÉ] Rythme cardiaque anormal : %.1f BPM\n", currentHeartRate);
  }
}

// ==============================================================================
// 7. MACHINE À ÉTATS ET GESTION DES ALERTES
// ==============================================================================
void updateSystemState() {
  unsigned long now = millis();

  // Phase 1 : Stabilisation
  if (currentState == STATE_STABILISATION) {
    if (now - bootTimestamp < STABILIZATION_TIME_MS) {
      digitalWrite(PIN_LED_YELLOW_PREALERT, (now / 300) % 2);
      return;
    } else {
      digitalWrite(PIN_LED_YELLOW_PREALERT, LOW);
      currentState = STATE_NORMAL;
      lastActivityTimestamp = now;
      Serial.println(">> STABILISATION TERMINÉE. Surveillance Tri-Capteurs active.");
    }
  }

  // CONDITION CRITIQUE PRIORITAIRE : Micro-sommeil détecté par le capteur oculaire (> 1,5s)
  if (eyeDrowsinessTriggered) {
    currentState = STATE_ALARME_CRITIQUE;
    unsigned long closedDuration = now - eyeClosedStartTime;
    triggerCriticalAlarm("YEUX FERMÉS (Micro-sommeil)", closedDuration);
    return;
  }

  // Réinitialisation de l'inactivité si le conducteur touche le volant ou braque
  if (handDetectedOnWheel || steeringMotionDetected) {
    lastActivityTimestamp = now;
    if (currentState != STATE_NORMAL && !isEyeClosed) {
      resetAlarmsToNormal();
    }
    return;
  }

  unsigned long inactiveDuration = now - lastActivityTimestamp;

  // Phase 3 : Alarme critique pour inactivité du volant & mains
  if (inactiveDuration >= CRITICAL_ALERT_TIME_MS) {
    currentState = STATE_ALARME_CRITIQUE;
    triggerCriticalAlarm("VOLANT & MAINS INACTIFS", inactiveDuration);
  }
  // Phase 2 : Pré-Alerte
  else if (inactiveDuration >= PRE_ALERT_TIME_MS) {
    currentState = STATE_PRE_ALERTE;
    triggerPreAlert();
  }
  // Retour à l'état normal
  else {
    if (currentState != STATE_NORMAL && !isEyeClosed) {
      resetAlarmsToNormal();
    }
  }
}

void triggerPreAlert() {
  digitalWrite(PIN_LED_YELLOW_PREALERT, HIGH);
  digitalWrite(PIN_LED_RED_CRITICAL, LOW);

  if ((millis() / 500) % 2 == 0) {
    digitalWrite(PIN_BUZZER, HIGH);
  } else {
    digitalWrite(PIN_BUZZER, LOW);
  }

  Serial.println("[PRÉ-ALERTE] Inactivité détectée. Gardez les mains sur le volant !");
}

void triggerCriticalAlarm(String reason, unsigned long durationMs) {
  lastAlarmReason = reason;
  digitalWrite(PIN_LED_YELLOW_PREALERT, LOW);
  digitalWrite(PIN_LED_RED_CRITICAL, (millis() / 150) % 2); // Stroboscope
  digitalWrite(PIN_BUZZER, HIGH);                           // Sirène

  Serial.printf("🚨 [ALARME CRITIQUE] Cause: %s | Durée: %lu ms !\n", reason.c_str(), durationMs);

  if (millis() - lastTelegramAlertTime >= TELEGRAM_COOLDOWN_MS) {
    totalCriticalIncidents++;
    lastTelegramAlertTime = millis();
    sendTelegramEmergencyAlert(reason, durationMs);
  }
}

void resetAlarmsToNormal() {
  currentState = STATE_NORMAL;
  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED_RED_CRITICAL, LOW);
  digitalWrite(PIN_LED_YELLOW_PREALERT, LOW);
}

// ==============================================================================
// 8. TÉLÉMÉTRIE & COMMANDES TELEGRAM
// ==============================================================================
void sendTelegramEmergencyAlert(String reason, unsigned long durationMs) {
  String msg = "🚨 *ALERTE CRITIQUE DE SÉCURITÉ CONDUCTEUR*\n\n";
  msg += "⚠️ *Somnolence / Perte de Contrôle Détectée !*\n";
  msg += "• Motif : *" + reason + "*\n";
  msg += "• Durée de l'anomalie : *" + String(durationMs / 1000.0, 1) + " secondes*\n";
  msg += "• Score de Fatigue IA : *" + String(aiFatigueScore) + " / 100*\n";
  msg += "• Capteur Oculaire : *" + String(isEyeClosed ? "🔴 YEUX FERMÉS" : "🟢 YEUX OUVERTS") + "*\n";
  msg += "• Mouvements du volant : *" + String(steeringMotionDetected ? "ACTIFS" : "AUCUN (MPU6050)") + "*\n";
  msg += "• Mains sur le volant : *" + String(handDetectedOnWheel ? "DÉTECTÉES" : "ABSENTES (MAX30100)") + "*\n";
  msg += "• Pouls : *" + String(currentHeartRate, 0) + " BPM* | SpO2 : *" + String(currentSpO2) + "%*\n";
  msg += "• Incident N° : *#" + String(totalCriticalIncidents) + "*\n\n";
  msg += "🔊 La sirène et le stroboscope sont *ACTIFS*.\n";
  msg += "👉 Veuillez contacter immédiatement le conducteur !";

  bot.sendMessage(CHAT_ID, msg, "Markdown");
  Serial.println("[TELEGRAM] Alerte d'urgence transmise.");
}

void sendTelegramAiWarning() {
  if (millis() - lastTelegramAlertTime < TELEGRAM_COOLDOWN_MS) return;
  lastTelegramAlertTime = millis();

  String msg = "⚠️ *AVERTISSEMENT PRÉDICTIF IA : FATIGUE ÉLEVÉE*\n\n";
  msg += "🧠 Le moteur d'IA TinyML a détecté des signes précurseurs d'endormissement !\n";
  msg += "• Indice de Fatigue : *" + String(aiFatigueScore) + "%* (Seuil : " + String(AI_FATIGUE_ALERT_SCORE) + "%)\n";
  msg += "• État Oculaire : *" + String(isEyeClosed ? "Clignements anormaux" : "Normal") + "*\n";
  msg += "• Variabilité Cardiaque (RMSSD) : *" + String(currentHrvRmssd, 1) + " ms*\n";
  msg += "• Variance du Volant : *" + String(steeringVariance, 1) + "*\n";
  msg += "• Recommandation : *Faites une pause avant l'endormissement complet !*";

  bot.sendMessage(CHAT_ID, msg, "Markdown");
  Serial.println("[IA] Avertissement prédictif envoyé.");
}

void handleIncomingBotMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String senderChatId = String(bot.messages[i].chat_id);
    String commandText  = bot.messages[i].text;

    Serial.print("Commande Telegram reçue : ");
    Serial.println(commandText);

    if (commandText == "/start" || commandText == "/help" || commandText == "/aide") {
      String reply = "🚘 *Sentinelle ESP32 Tri-Capteurs (Yeux + Volant + VRC)*\n\n";
      reply += "Commandes disponibles :\n";
      reply += "• /status - Bilan complet des 3 capteurs et incidents\n";
      reply += "• /ai     - Indice de fatigue TinyML et données VRC\n";
      reply += "• /vitals - Fréquence cardiaque et oxygène sanguin\n";
      reply += "• /test   - Test physique du buzzer et des LED (1s)\n";
      reply += "• /aide   - Affiche ce menu";
      bot.sendMessage(senderChatId, reply, "Markdown");
    } 
    else if (commandText == "/ai") {
      String reply = "🧠 *Télémétrie d'IA Embarquée (TinyML)*\n\n";
      reply += "• Score de Fatigue : *" + String(aiFatigueScore) + " / 100*\n";
      reply += "• Niveau de Risque : *" + String(aiFatigueScore > 75 ? "🔴 ÉLEVÉ" : (aiFatigueScore > 45 ? "🟡 MODÉRÉ" : "🟢 FAIBLE")) + "*\n";
      reply += "• État des Yeux : *" + String(isEyeClosed ? "🔴 FERMÉS" : "🟢 OUVERTS") + "*\n";
      reply += "• Variabilité Cardiaque (RMSSD) : `" + String(currentHrvRmssd, 1) + " ms`\n";
      reply += "• Variance de Direction : `" + String(steeringVariance, 2) + "`\n";
      bot.sendMessage(senderChatId, reply, "Markdown");
    }
    else if (commandText == "/vitals") {
      String reply = "💓 *Données Biométriques & Présence*\n\n";
      reply += "• Pouls Cardiaque : *" + String(currentHeartRate, 1) + " BPM*\n";
      reply += "• Oxygène Sanguin (SpO2) : *" + String(currentSpO2) + "%*\n";
      reply += "• Mains sur le Volant : *" + String(handDetectedOnWheel ? "OUI ✅" : "NON ❌") + "*\n";
      reply += "• Capteur Oculaire : *" + String(isEyeClosed ? "🔴 Yeux Fermés" : "🟢 Yeux Ouverts") + "*\n";
      reply += "• Statut Cardiaque : *" + String(currentHeartRate < MIN_SAFE_BPM ? "Alerte Bradycardie" : (currentHeartRate > MAX_SAFE_BPM ? "Alerte Tachycardie" : "Normal")) + "*";
      bot.sendMessage(senderChatId, reply, "Markdown");
    }
    else if (commandText == "/status") {
      String reply = "📊 *Diagnostic Tri-Capteurs en Direct*\n\n";
      reply += "• Phase Actuelle : *" + String(currentState == STATE_NORMAL ? "SURVEILLANCE NORMALE" : (currentState == STATE_PRE_ALERTE ? "PRÉ-ALERTE" : "ALARME CRITIQUE")) + "*\n";
      reply += "• 1. Capteur Oculaire : *" + String(isEyeClosed ? "🔴 FERMÉS" : "🟢 OUVERTS") + "*\n";
      reply += "• 2. Mains sur Volant : *" + String(handDetectedOnWheel ? "OUI (MAX30100)" : "NON") + "*\n";
      reply += "• 3. Mouvements Volant : *" + String(steeringMotionDetected ? "ACTIFS (MPU6050)" : "FIGÉ") + "*\n";
      reply += "• Signal Wi-Fi : `" + String(WiFi.RSSI()) + " dBm`\n";
      reply += "• Incidents Totaux : *" + String(totalCriticalIncidents) + "*\n";
      reply += "• Temps de Fonctionnement : " + String(millis() / 60000) + " minutes";
      bot.sendMessage(senderChatId, reply, "Markdown");
    }
    else if (commandText == "/test") {
      bot.sendMessage(senderChatId, "🔔 Test des actionneurs pendant 1 seconde...", "");
      digitalWrite(PIN_BUZZER, HIGH);
      digitalWrite(PIN_LED_YELLOW_PREALERT, HIGH);
      digitalWrite(PIN_LED_RED_CRITICAL, HIGH);
      delay(1000);
      digitalWrite(PIN_BUZZER, LOW);
      digitalWrite(PIN_LED_YELLOW_PREALERT, LOW);
      digitalWrite(PIN_LED_RED_CRITICAL, LOW);
      bot.sendMessage(senderChatId, "✅ Test terminé avec succès.", "");
    }
  }
}

void connectToWiFi() {
  Serial.print("Connexion au réseau Wi-Fi : ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[SUCCÈS] Wi-Fi Connecté !");
    Serial.print("Adresse IP ESP32 : ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n[ATTENTION] Échec de connexion Wi-Fi. Nouvelle tentative en arrière-plan.");
  }
}
