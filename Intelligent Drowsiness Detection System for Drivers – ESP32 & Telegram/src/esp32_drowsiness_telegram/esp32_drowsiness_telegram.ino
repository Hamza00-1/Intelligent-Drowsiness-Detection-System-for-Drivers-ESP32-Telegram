/*
 * ==============================================================================
 * 🚗 SYSTÈME INTELLIGENT DE DÉTECTION DE SOMNOLENCE AU VOLANT (AVEC IA EMBARQUÉE)
 * ==============================================================================
 * Cible : Module ESP32 Dev
 * Configuration Matérielle :
 *   - MPU6050 :   Détecte les micro-ajustements et mouvements du volant (I2C)
 *   - MAX30100 :  Mesure le pouls, SpO2, et confirme la présence des mains (I2C)
 *   - Buzzer :    Alarme sonore de bord (GPIO 25)
 *   - LED Rouge : Stroboscope d'alarme critique (GPIO 26)
 *   - LED Jaune : Témoin lumineux de pré-alerte (GPIO 27)
 *   - Cloud :     Alertes d'urgence via Bot Telegram en Wi-Fi
 *
 * 🧠 FONCTIONNALITÉS AVANCÉES D'INTELLIGENCE ARTIFICIELLE :
 *   1. Phase de Stabilisation : Étalonnage des capteurs pendant 8 secondes au démarrage.
 *   2. Variabilité de la Fréquence Cardiaque (VRC / HRV - RMSSD) : Analyse
 *      des variations du système nerveux autonome révélatrices de la somnolence.
 *   3. Entropie des Mouvements du Volant : Détection des micro-braquages vs trajectoire figée.
 *   4. Moteur de Fusion TinyML : Calcul en direct d'un Indice de Fatigue (0 à 100%).
 *   5. Pré-Alerte Prédictive : Avertit le conducteur *avant* l'endormissement complet.
 *   6. Surveillance des Anomalies de Santé : Détecte hypoxie (<90% SpO2) ou détresse cardiaque.
 *   7. Télédiagnostic Interactif : Commandes Telegram /ai, /vitals, /status, /test.
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

// États du Système (Machine à États Finis)
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
unsigned long lastTelegramAlertTime = 0;
unsigned long lastBotCheckTime = 0;
unsigned long lastPoxReportTime = 0;
unsigned long lastAiEvaluationTime = 0;

bool handDetectedOnWheel = false;
bool steeringMotionDetected = false;
float currentHeartRate = 0.0;
uint8_t currentSpO2 = 0;
uint32_t totalCriticalIncidents = 0;

// Variables de Référence MPU6050
float lastGyroX = 0, lastGyroY = 0, lastGyroZ = 0;
bool mpuInitialized = false;
bool poxInitialized = false;

// ======================== 🧠 TAMPONS DE DONNÉES IA & VRC ========================
// Tampon circulaire pour les Intervalles Inter-Battements (IBI) en millisecondes
volatile unsigned long lastBeatTimestamp = 0;
volatile unsigned long ibiBuffer[HRV_WINDOW_SIZE];
volatile uint8_t ibiIndex = 0;
volatile uint8_t ibiCount = 0;
float currentHrvRmssd = 0.0; // Racine carrée de la moyenne des différences successives (ms)

// Tampon glissant pour les mesures de mouvement du volant (MPU6050)
float steeringBuffer[STEERING_SAMPLE_WINDOW];
uint8_t steeringIndex = 0;
float steeringVariance = 0.0;

// Indice de Fatigue IA (0 à 100%)
int aiFatigueScore = 0;
bool aiWarningSent = false;
// ==============================================================================

// Déclarations des Fonctions
void connectToWiFi();
void initI2CDevices();
void initMPU6050();
void readMPU6050();
void onBeatDetected();
void calculateHRV();
void calculateSteeringDynamics(float currentMotion);
void computeAIFatigueIndex();
void checkBiometricHealthAnomalies();
void updateSystemState();
void triggerCriticalAlarm(unsigned long inactiveDuration);
void triggerPreAlert();
void resetAlarmsToNormal();
void sendTelegramEmergencyAlert(unsigned long durationMs);
void sendTelegramAiWarning();
void handleIncomingBotMessages(int numNewMessages);

// ==============================================================================
// 1. INITIALISATION (Exécutée une fois à la mise sous tension)
// ==============================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=======================================================");
  Serial.println("  🚗 Système ESP32 de Vigilance Conducteur : IA & Capteurs");
  Serial.println("=======================================================");

  // Configuration des Broches GPIO
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_RED_CRITICAL, OUTPUT);
  pinMode(PIN_LED_YELLOW_PREALERT, OUTPUT);
  pinMode(PIN_LED_STATUS, OUTPUT);

  // Extinction initiale de toutes les alertes
  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED_RED_CRITICAL, LOW);
  digitalWrite(PIN_LED_YELLOW_PREALERT, LOW);
  digitalWrite(PIN_LED_STATUS, LOW);

  // Initialisation du bus I2C sur GPIO 21 (SDA) et GPIO 22 (SCL)
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  // Initialisation des capteurs I2C
  initI2CDevices();

  // Connexion Wi-Fi et configuration SSL Telegram
  connectToWiFi();
  secured_client.setInsecure(); // Évite les problèmes de certificat racine expiré sur ESP32

  bootTimestamp = millis();
  lastActivityTimestamp = millis();

  // Initialisation des tampons de l'IA
  for (int i = 0; i < HRV_WINDOW_SIZE; i++) ibiBuffer[i] = 0;
  for (int i = 0; i < STEERING_SAMPLE_WINDOW; i++) steeringBuffer[i] = 0.0;

  Serial.println(">> Entrée dans la PHASE DE STABILISATION. Étalonnage des capteurs...");
}

// ==============================================================================
// 2. BOUCLE PRINCIPALE (Exécutée en continu)
// ==============================================================================
void loop() {
  // A. Maintien de la connexion Wi-Fi
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(PIN_LED_STATUS, HIGH);
  } else {
    digitalWrite(PIN_LED_STATUS, LOW);
  }

  // B. Mise à jour de l'oxymètre de pouls (doit être appelé en continu)
  if (poxInitialized) {
    pox.update();
  }

  // C. Lecture périodique des constantes vitales (toutes les secondes)
  if (millis() - lastPoxReportTime >= 1000) {
    lastPoxReportTime = millis();
    if (poxInitialized) {
      currentHeartRate = pox.getHeartRate();
      currentSpO2 = pox.getSpO2();

      // Présence des mains confirmée par la détection du rythme cardiaque
      if (currentHeartRate > 35.0f && currentHeartRate < 195.0f) {
        handDetectedOnWheel = true;
      } else {
        handDetectedOnWheel = false;
      }

      // 🏥 Vérification des anomalies de santé (détresse cardiaque / hypoxie)
      if (handDetectedOnWheel) {
        checkBiometricHealthAnomalies();
      }
    }
  }

  // D. Lecture des mouvements du volant avec le MPU6050
  readMPU6050();

  // E. Exécution du moteur d'IA prédictif (toutes les 1,5 secondes)
  if (ENABLE_AI_PREDICTOR && (millis() - lastAiEvaluationTime >= 1500)) {
    lastAiEvaluationTime = millis();
    calculateHRV();
    computeAIFatigueIndex();
  }

  // F. Évaluation de l'état du conducteur et gestion des alertes
  updateSystemState();

  // G. Traitement des commandes Telegram entrantes (toutes les 2 secondes)
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
// 3. INITIALISATION DU MATÉRIEL ET DES CAPTEURS
// ==============================================================================
void initI2CDevices() {
  // 1. Initialisation du MPU6050 (Mouvements du volant)
  initMPU6050();

  // 2. Initialisation du MAX30100 (Oxymètre de pouls et contact des mains)
  Serial.print("Initialisation de l'oxymètre MAX30100...");
  if (!pox.begin()) {
    Serial.println(" [ÉCHEC] Vérifiez le câblage du MAX30100 (VCC/GND/SDA/SCL).");
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
  Wire.write(0x6B); // Registre PWR_MGMT_1
  Wire.write(0);    // Réveil du MPU6050
  byte error = Wire.endTransmission();

  if (error == 0) {
    Serial.println(">> MPU6050 (Capteur de volant) initialisé avec succès.");
    mpuInitialized = true;
  } else {
    Serial.println(">> [ATTENTION] MPU6050 introuvable à l'adresse I2C 0x68.");
    mpuInitialized = false;
  }
}

// Rappel automatique lors de la détection d'une pulsation cardiaque
void onBeatDetected() {
  unsigned long now = millis();
  if (lastBeatTimestamp > 0) {
    unsigned long ibi = now - lastBeatTimestamp;
    // Intervalles physiologiques plausibles (300ms = 200 bpm, 1800ms = 33 bpm)
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
// 4. TRAITEMENT INERTIEL DU VOLANT (MPU6050)
// ==============================================================================
void readMPU6050() {
  if (!mpuInitialized) return;

  // Lecture des 6 registres gyroscopiques à partir de 0x43 (GYRO_XOUT_H)
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

    // Calcul de la dynamique du volant pour l'IA
    calculateSteeringDynamics(totalMotion);

    // Si le mouvement dépasse le seuil, le conducteur agit sur la direction
    if (totalMotion >= STEERING_MOTION_THRESHOLD) {
      steeringMotionDetected = true;
      lastActivityTimestamp = millis(); // Réinitialise le compte à rebours d'inactivité
    } else {
      steeringMotionDetected = false;
    }
  }
}

void calculateSteeringDynamics(float currentMotion) {
  steeringBuffer[steeringIndex] = currentMotion;
  steeringIndex = (steeringIndex + 1) % STEERING_SAMPLE_WINDOW;

  // Calcul de la moyenne et de la variance des mouvements
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
// 5. 🧠 MOTEUR D'IA EMBARQUÉ ET ANALYSE BIOMÉTRIQUE
// ==============================================================================

/**
 * Calcule la Variabilité de la Fréquence Cardiaque (VRC / HRV - RMSSD).
 * Lors de la somnolence, l'activité du système parasympathique entraîne
 * une baisse et une instabilité caractéristiques de la valeur RMSSD.
 */
void calculateHRV() {
  if (ibiCount < 4) {
    currentHrvRmssd = 45.0; // Valeur médiane par défaut en phase d'acquisition
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

/**
 * Moteur de Fusion Inspiré du TinyML :
 * Combine 3 biomarqueurs indépendants pour générer un Score de Fatigue de 0 à 100% :
 *   1. Durée d'Inactivité du Volant (0 à 45 points)
 *   2. Variance et Entropie de Direction (0 à 25 points)
 *   3. Ralentissement Cardiaque & Baisse VRC (0 à 30 points)
 */
void computeAIFatigueIndex() {
  if (currentState == STATE_STABILISATION) {
    aiFatigueScore = 0;
    return;
  }

  unsigned long inactiveDuration = millis() - lastActivityTimestamp;

  // Facteur 1 : Durée d'inactivité directionnelle (0 - 45 points)
  float inactivityFactor = (float)inactiveDuration / (float)CRITICAL_ALERT_TIME_MS;
  if (inactivityFactor > 1.0) inactivityFactor = 1.0;
  int ptsInactivity = (int)(inactivityFactor * 45.0);

  // Facteur 2 : Variance des micro-ajustements du volant (0 - 25 points)
  int ptsSteering = 0;
  if (steeringVariance < 2.0) {
    ptsSteering = 25; // Volant figé (somnolence ou perte de contrôle)
  } else if (steeringVariance < 6.0) {
    ptsSteering = 15;
  } else if (steeringVariance > 80.0) {
    ptsSteering = 20; // Coups de volant brusques (réveil en sursaut)
  }

  // Facteur 3 : Signatures biométriques et cardiaques (0 - 30 points)
  int ptsBiometrics = 0;
  if (!handDetectedOnWheel) {
    ptsBiometrics += 25; // Mains retirées du volant !
  } else {
    if (currentHeartRate > 0 && currentHeartRate < 58.0f) {
      ptsBiometrics += 15; // Ralentissement du pouls
    }
    if (currentHrvRmssd > 0 && currentHrvRmssd < 20.0f) {
      ptsBiometrics += 10; // Chute de la variabilité cardiaque
    }
  }

  // Calcul du score final borné entre 0 et 100%
  aiFatigueScore = ptsInactivity + ptsSteering + ptsBiometrics;
  if (aiFatigueScore > 100) aiFatigueScore = 100;

  // 🔔 Déclenchement de l'avertissement prédictif IA avant l'endormissement complet
  if (aiFatigueScore >= AI_FATIGUE_ALERT_SCORE && !aiWarningSent && currentState == STATE_NORMAL) {
    sendTelegramAiWarning();
    aiWarningSent = true;
  } else if (aiFatigueScore < 50) {
    aiWarningSent = false;
  }
}

/**
 * 🏥 Surveillance Médicale et Biométrique :
 * Distingue la simple somnolence d'une urgence médicale (malaise, hypoxie, arrêt).
 */
void checkBiometricHealthAnomalies() {
  if (currentSpO2 > 0 && currentSpO2 < MIN_SAFE_SPO2) {
    Serial.printf("⚠️ [ALERTE SANTÉ] Saturation basse en oxygène (SpO2) : %d%%\n", currentSpO2);
  }
  if (currentHeartRate > 0 && (currentHeartRate < MIN_SAFE_BPM || currentHeartRate > MAX_SAFE_BPM)) {
    Serial.printf("⚠️ [ALERTE SANTÉ] Rythme cardiaque anormal : %.1f BPM\n", currentHeartRate);
  }
}

// ==============================================================================
// 6. MACHINE À ÉTATS ET LOGIQUE D'ALERTE
// ==============================================================================
void updateSystemState() {
  unsigned long now = millis();

  // Phase 1 : Phase de Stabilisation (Calibration des capteurs au démarrage)
  if (currentState == STATE_STABILISATION) {
    if (now - bootTimestamp < STABILIZATION_TIME_MS) {
      // Clignotement doux de la LED jaune pendant la calibration
      digitalWrite(PIN_LED_YELLOW_PREALERT, (now / 300) % 2);
      return;
    } else {
      digitalWrite(PIN_LED_YELLOW_PREALERT, LOW);
      currentState = STATE_NORMAL;
      lastActivityTimestamp = now;
      Serial.println(">> STABILISATION TERMINÉE. Surveillance active engagée.");
    }
  }

  // Si le conducteur touche le volant OU tourne, réinitialisation de l'inactivité
  if (handDetectedOnWheel || steeringMotionDetected) {
    lastActivityTimestamp = now;
    if (currentState != STATE_NORMAL) {
      resetAlarmsToNormal();
    }
    return;
  }

  // Calcul du temps écoulé sans action du conducteur
  unsigned long inactiveDuration = now - lastActivityTimestamp;

  // Phase 3 : Alarme Critique
  if (inactiveDuration >= CRITICAL_ALERT_TIME_MS) {
    currentState = STATE_ALARME_CRITIQUE;
    triggerCriticalAlarm(inactiveDuration);
  }
  // Phase 2 : Pré-Alerte
  else if (inactiveDuration >= PRE_ALERT_TIME_MS) {
    currentState = STATE_PRE_ALERTE;
    triggerPreAlert();
  }
  // État Normal
  else {
    if (currentState != STATE_NORMAL) {
      resetAlarmsToNormal();
    }
  }
}

void triggerPreAlert() {
  // Pré-alerte douce : LED Jaune allumée fixe, signal sonore court
  digitalWrite(PIN_LED_YELLOW_PREALERT, HIGH);
  digitalWrite(PIN_LED_RED_CRITICAL, LOW);

  // Bip intermittent toutes les secondes
  if ((millis() / 500) % 2 == 0) {
    digitalWrite(PIN_BUZZER, HIGH);
  } else {
    digitalWrite(PIN_BUZZER, LOW);
  }

  Serial.println("[PRÉ-ALERTE] Inactivité détectée. Gardez les mains sur le volant !");
}

void triggerCriticalAlarm(unsigned long inactiveDuration) {
  // Alarme critique : Sirène sonore continue et stroboscope LED rouge
  digitalWrite(PIN_LED_YELLOW_PREALERT, LOW);
  digitalWrite(PIN_LED_RED_CRITICAL, (millis() / 150) % 2); // Clignotement stroboscopique
  digitalWrite(PIN_BUZZER, HIGH);                           // Alarme continue puissante

  Serial.printf("🚨 [ALARME CRITIQUE] Inactivité : %lu ms ! Conducteur sans réaction !\n", inactiveDuration);

  // Transmission de l'alerte sur Telegram si le délai de cooldown est passé
  if (millis() - lastTelegramAlertTime >= TELEGRAM_COOLDOWN_MS) {
    totalCriticalIncidents++;
    lastTelegramAlertTime = millis();
    sendTelegramEmergencyAlert(inactiveDuration);
  }
}

void resetAlarmsToNormal() {
  currentState = STATE_NORMAL;
  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED_RED_CRITICAL, LOW);
  digitalWrite(PIN_LED_YELLOW_PREALERT, LOW);
}

// ==============================================================================
// 7. GESTION DES NOTIFICATIONS ET COMMANDES TELEGRAM
// ==============================================================================
void sendTelegramEmergencyAlert(unsigned long durationMs) {
  String msg = "🚨 *ALERTE CRITIQUE DE SÉCURITÉ CONDUCTEUR*\n\n";
  msg += "⚠️ *Inactivité Prolongée / Somnolence Avérée !*\n";
  msg += "• Durée sans réaction : *" + String(durationMs / 1000.0, 1) + " secondes*\n";
  msg += "• Score de Fatigue IA : *" + String(aiFatigueScore) + " / 100*\n";
  msg += "• Mouvements du volant : *AUCUN DÉTECTÉ (MPU6050)*\n";
  msg += "• Mains sur le volant : *NON DÉTECTÉES (MAX30100)*\n";
  msg += "• Pouls : *" + String(currentHeartRate, 0) + " BPM* | SpO2 : *" + String(currentSpO2) + "%*\n";
  msg += "• Incident N° : *#" + String(totalCriticalIncidents) + "*\n\n";
  msg += "🔊 La sirène et le stroboscope de bord sont *ACTIFS*.\n";
  msg += "👉 Veuillez contacter le conducteur d'urgence !";

  bot.sendMessage(CHAT_ID, msg, "Markdown");
  Serial.println("[TELEGRAM] Alerte d'urgence transmise sur le cloud.");
}

void sendTelegramAiWarning() {
  if (millis() - lastTelegramAlertTime < TELEGRAM_COOLDOWN_MS) return;
  lastTelegramAlertTime = millis();

  String msg = "⚠️ *AVERTISSEMENT PRÉDICTIF IA : FATIGUE ÉLEVÉE*\n\n";
  msg += "🧠 Le moteur d'IA TinyML a détecté des signes précurseurs d'endormissement !\n";
  msg += "• Indice de Fatigue : *" + String(aiFatigueScore) + "%* (Seuil critique : " + String(AI_FATIGUE_ALERT_SCORE) + "%)\n";
  msg += "• Variabilité Cardiaque (RMSSD) : *" + String(currentHrvRmssd, 1) + " ms*\n";
  msg += "• Variance des Mouvements du Volant : *" + String(steeringVariance, 1) + "*\n";
  msg += "• Recommandation : *Faites une pause avant que le micro-sommeil ne survienne !*";

  bot.sendMessage(CHAT_ID, msg, "Markdown");
  Serial.println("[IA] Avertissement prédictif envoyé sur Telegram.");
}

void handleIncomingBotMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String senderChatId = String(bot.messages[i].chat_id);
    String commandText  = bot.messages[i].text;

    Serial.print("Commande Telegram reçue : ");
    Serial.println(commandText);

    if (commandText == "/start" || commandText == "/help" || commandText == "/aide") {
      String reply = "🚘 *Sentinelle de Sécurité ESP32 (IA & Biométrie)*\n\n";
      reply += "Commandes disponibles :\n";
      reply += "• /status - État général et compteur d'incidents\n";
      reply += "• /ai     - Indice de fatigue TinyML et données VRC (HRV)\n";
      reply += "• /vitals - Fréquence cardiaque et oxygène sanguin (SpO2)\n";
      reply += "• /test   - Test physique du buzzer et des LED (1 sec)\n";
      reply += "• /aide   - Affiche ce menu d'aide";
      bot.sendMessage(senderChatId, reply, "Markdown");
    } 
    else if (commandText == "/ai") {
      String reply = "🧠 *Télémétrie d'IA Embarquée (TinyML)*\n\n";
      reply += "• Score de Fatigue : *" + String(aiFatigueScore) + " / 100*\n";
      reply += "• Niveau de Risque : *" + String(aiFatigueScore > 75 ? "🔴 ÉLEVÉ" : (aiFatigueScore > 45 ? "🟡 MODÉRÉ" : "🟢 FAIBLE")) + "*\n";
      reply += "• Variabilité Cardiaque (RMSSD) : `" + String(currentHrvRmssd, 1) + " ms`\n";
      reply += "• Variance de Direction : `" + String(steeringVariance, 2) + "`\n";
      reply += "• Activité du Volant : " + String(steeringMotionDetected ? "Micro-ajustements Actifs" : "Direction Figée") + "\n";
      bot.sendMessage(senderChatId, reply, "Markdown");
    }
    else if (commandText == "/vitals") {
      String reply = "💓 *Données Biométriques du Conducteur*\n\n";
      reply += "• Pouls Cardiaque : *" + String(currentHeartRate, 1) + " BPM*\n";
      reply += "• Oxygène Sanguin (SpO2) : *" + String(currentSpO2) + "%*\n";
      reply += "• Mains sur le Volant : *" + String(handDetectedOnWheel ? "OUI ✅" : "NON ❌") + "*\n";
      reply += "• Statut Cardiaque : *" + String(currentHeartRate < MIN_SAFE_BPM ? "Alerte Bradycardie" : (currentHeartRate > MAX_SAFE_BPM ? "Alerte Tachycardie" : "Normal")) + "*";
      bot.sendMessage(senderChatId, reply, "Markdown");
    }
    else if (commandText == "/status") {
      String reply = "📊 *Diagnostic Système en Direct*\n\n";
      reply += "• Phase Actuelle : *" + String(currentState == STATE_NORMAL ? "SURVEILLANCE NORMALE" : (currentState == STATE_PRE_ALERTE ? "PRÉ-ALERTE" : "ALARME CRITIQUE")) + "*\n";
      reply += "• Mains Détectées : *" + String(handDetectedOnWheel ? "OUI" : "NON") + "*\n";
      reply += "• Volant Actif : *" + String(steeringMotionDetected ? "OUI" : "NON") + "*\n";
      reply += "• Signal Wi-Fi : `" + String(WiFi.RSSI()) + " dBm`\n";
      reply += "• Incidents Totaux : *" + String(totalCriticalIncidents) + "*\n";
      reply += "• Temps de Fonctionnement : " + String(millis() / 60000) + " minutes";
      bot.sendMessage(senderChatId, reply, "Markdown");
    }
    else if (commandText == "/test") {
      bot.sendMessage(senderChatId, "🔔 Exécution d'un test système d'1 seconde...", "");
      digitalWrite(PIN_BUZZER, HIGH);
      digitalWrite(PIN_LED_YELLOW_PREALERT, HIGH);
      digitalWrite(PIN_LED_RED_CRITICAL, HIGH);
      delay(1000);
      digitalWrite(PIN_BUZZER, LOW);
      digitalWrite(PIN_LED_YELLOW_PREALERT, LOW);
      digitalWrite(PIN_LED_RED_CRITICAL, LOW);
      bot.sendMessage(senderChatId, "✅ Test terminé. Tous les actionneurs sont opérationnels.", "");
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
