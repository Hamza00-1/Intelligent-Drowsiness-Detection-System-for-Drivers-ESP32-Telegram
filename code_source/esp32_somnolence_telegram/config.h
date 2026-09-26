#ifndef CONFIG_H
#define CONFIG_H

// ==============================================================================
// 🚗 SYSTÈME DE SÉCURITÉ CONDUCTEUR - FICHIER DE CONFIGURATION
// ==============================================================================
// Architecture Matérielle (Fusion Tri-Capteurs) :
//  - Microcontrôleur : ESP32
//  - Capteur 1 : Capteur Infrarouge d'Yeux / Clignement (GPIO 34)
//  - Capteur 2 : MPU6050 (Micro-mouvements et dynamique du volant via I2C)
//  - Capteur 3 : MAX30100 (Oxymètre de pouls, rythme cardiaque et présence des mains via I2C)
//  - Moteur d'IA : Indice de Fatigue TinyML prédictif en temps réel (Yeux + Volant + VRC)
//  - Actionneurs : Buzzer actif + Double LED (Jaune Pré-Alerte, Rouge Critique)
//  - Cloud : API Telegram Bot via Wi-Fi sécurisé
// ==============================================================================

// 1. Identifiants Wi-Fi (Box Internet ou Partage de Connexion 4G/5G)
#define WIFI_SSID                   "VOTRE_NOM_WIFI"
#define WIFI_PASSWORD               "VOTRE_MOT_DE_PASSE_WIFI"

// 2. Configuration du Bot Telegram
// Obtenez le BOT_TOKEN auprès de @BotFather, et le CHAT_ID auprès de @userinfobot sur Telegram
#define BOT_TOKEN                   "VOTRE_TELEGRAM_BOT_TOKEN"
#define CHAT_ID                     "VOTRE_TELEGRAM_CHAT_ID"

// 3. Broches Matérielles (Brochage ESP32)
// Capteur Optique Oculaire
#define PIN_EYE_SENSOR              34     // GPIO 34 (Entrée analogique/numérique pour capteur d'yeux)

// Bus I2C (Partagé par le MPU6050 et le MAX30100)
#define PIN_I2C_SDA                 21     // GPIO 21 -> Ligne de données SDA des deux capteurs
#define PIN_I2C_SCL                 22     // GPIO 22 -> Ligne d'horloge SCL des deux capteurs

// Actionneurs d'Alerte
#define PIN_BUZZER                  25     // GPIO 25 -> Buzzer actif (+)
#define PIN_LED_RED_CRITICAL        26     // GPIO 26 -> LED Rouge (Alarme Critique de Somnolence)
#define PIN_LED_YELLOW_PREALERT     27     // GPIO 27 -> LED Jaune (Avertissement de Pré-Alerte)
#define PIN_LED_STATUS              2      // GPIO 2  -> LED Bleue intégrée (Statut Wi-Fi et système)

// 4. Seuils Temporels (en millisecondes)
#define STABILIZATION_TIME_MS       8000   // 8 secondes de calibration et préchauffage au démarrage
#define EYE_DROWSINESS_MS           1500   // 1,5 seconde d'yeux fermés en continu = Alerte Immédiate Micro-Sommeil !
#define PRE_ALERT_TIME_MS           4500   // 4,5 secondes sans main/mouvement -> Pré-alerte douce
#define CRITICAL_ALERT_TIME_MS      8000   // 8,0 secondes sans main/mouvement -> Alarme critique + Telegram
#define TELEGRAM_COOLDOWN_MS        25000  // 25 secondes d'attente entre deux alertes cloud (anti-spam)
#define BOT_CHECK_INTERVAL_MS       2000   // Vérification des commandes Telegram toutes les 2 secondes

// 5. Seuils de Détection des Capteurs
// Polarité du capteur oculaire (LOW = œil fermé pour la majorité des modules IR)
#define EYE_CLOSED_STATE            LOW
#define STEERING_MOTION_THRESHOLD   18.0f  // Vitesse angulaire minimale (deg/s) pour valider un mouvement du volant
#define HAND_PRESENCE_IR_THRESHOLD  22000  // Seuil infrarouge minimum confirmant la main sur le volant

// 6. 🧠 PARAMÈTRES DE L'IA ET DE LA FATIGUE PRÉDICTIVE
#define ENABLE_AI_PREDICTOR         true   // true = Activer le calcul de l'indice de fatigue TinyML embarqué
#define AI_FATIGUE_ALERT_SCORE      75     // Score (0-100%) déclenchant l'alerte prédictive d'endormissement
#define HRV_WINDOW_SIZE             12     // Taille de la fenêtre glissante des battements pour le calcul VRC (HRV)
#define STEERING_SAMPLE_WINDOW      20     // Fenêtre glissante pour l'analyse de la variance du volant

// 7. 🏥 LIMITES DES ANOMALIES BIOMÉTRIQUES & SANTÉ
#define MIN_SAFE_SPO2               90     // Seuil d'hypoxie : Oxygène sanguin inférieur à 90%
#define MIN_SAFE_BPM                45     // Seuil de bradycardie sévère (Pulsations par minute)
#define MAX_SAFE_BPM                140    // Seuil de tachycardie sévère au volant (Pulsations par minute)

#endif // CONFIG_H
