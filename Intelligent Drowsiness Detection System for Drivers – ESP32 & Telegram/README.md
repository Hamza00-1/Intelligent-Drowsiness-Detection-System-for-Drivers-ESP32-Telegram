# 🚘 Système Intelligent de Détection de Somnolence au Volant (ESP32, MPU6050, MAX30100 & Telegram)
### 🧠 Amélioré avec une IA Embarquée TinyML pour l'Inférence Prédictive de la Fatigue

Un système complet de sécurité routière IoT et IA développé sur le microcontrôleur **ESP32**. Contrairement aux solutions traditionnelles basées uniquement sur des caméras ou des minuteurs fixes, ce système analyse la vigilance du conducteur via deux canaux physiques et physiologiques complémentaires :
1. **MPU6050 (Centrale inertielle 6 axes)** : Surveille en continu les micro-ajustements angulaires du volant, la variance des mouvements et les à-coups directionnels.
2. **MAX30100 (Oxymètre de pouls)** : Mesure en temps réel le rythme cardiaque, la saturation en oxygène (SpO2), la présence effective des mains sur le volant et les intervalles inter-battements (IBI).
3. **Moteur d'IA Embarqué (Edge-AI TinyML)** : Calcule en continu la **Variabilité de la Fréquence Cardiaque (VRC / HRV - RMSSD)**, l'entropie de direction et un **Indice de Fatigue de 0 à 100%** pour anticiper le micro-sommeil **avant** que l'accident ne survienne.
4. **Télémétrie Cloud** : Envoi automatique d'alertes chiffrées via l'API Telegram Bot avec constantes vitales en direct, détection d'anomalies de santé et télédiagnostic interactif (`/ai`, `/vitals`, `/status`, `/test`).

---

## 📁 Arborescence du Projet

```text
anti/
│
├── 📁 src/
│   └── 📁 esp32_drowsiness_telegram/
│       ├── config.h                       <-- ⚙️ SEUL FICHIER À MODIFIER ! (Wi-Fi, Telegram, Seuils IA)
│       └── esp32_drowsiness_telegram.ino  <-- 🚀 Programme Arduino ESP32 avec Moteur d'IA Embarqué
│
├── 📁 ai_companion/
│   ├── drowsiness_ai_detector.py          <-- 👁️ Vision par ordinateur Face Mesh (Yeux & Bâillements)
│   ├── ml_data_logger_and_trainer.py      <-- 🔬 Entraînement de Modèle ML (Random Forest)
│   ├── requirements.txt                   <-- 📦 Dépendances Python
│   └── run_ai_detector.bat                <-- ⚡ Lanceur en un clic pour Windows
│
├── QUICKSTART.md                          <-- 🚀 Guide de démarrage rapide et câblage
├── README.md                              <-- 📖 Manuel technique complet du système (Ce fichier)
└── push_to_github.bat                     <-- 📤 Script de publication en un clic sur GitHub
```

---

## 🧠 Le Moteur d'IA Prédictif TinyML (Fonctionnement)

Les systèmes passifs attendent généralement que le conducteur s'endorme pour réagir. Notre firmware calcule un **Indice de Fatigue continu de 0 à 100%** en temps réel directement sur le processeur de l'ESP32 :

$$\text{Indice de Fatigue} = w_1 \cdot \text{Facteur d'Inactivité} + w_2 \cdot \text{Entropie du Volant} + w_3 \cdot \text{Chute VRC / Rythme Cardiaque}$$

### 1. Variabilité de la Fréquence Cardiaque (VRC / HRV - RMSSD)
* L'algorithme calcule la racine carrée de la moyenne des différences successives (RMSSD) entre les battements via l'interruption du capteur MAX30100.
* Lors de la phase de transition vers le sommeil, les modifications du système nerveux autonome provoquent une chute nette et une désynchronisation de la valeur RMSSD.

### 2. Dynamique et Entropie du Volant (MPU6050)
* **Conducteur attentif** : Fréquence élevée de micro-corrections douces (variance saine).
* **Conducteur somnolent** : Trajectoire figée (variance proche de zéro), suivie de coups de volant brusques pour rattraper la trajectoire (réveil en sursaut).

### 3. Avertissement Prédictif Immédiat
* Dès que l'indice de fatigue atteint **$\ge 75\%$**, l'ESP32 transmet un avertissement prédictif sur Telegram :
  > *"⚠️ AVERTISSEMENT PRÉDICTIF IA : Indice de fatigue à 82%. La variabilité cardiaque a chuté à 18,4 ms. Il est recommandé de faire une pause avant que l'endormissement ne survienne !"*

---

## ⚙️ Phases de Fonctionnement

```text
 ┌────────────────────────────────────────────────────────────────────────┐
 │                   PHASE 1 : STABILISATION (8 secondes)                 │
 │  • Étalonnage du gyroscope et compensation de la dérive au démarrage.  │
 │  • Clignotement doux de la LED jaune pendant l'installation.           │
 └──────────────────────────────────┬─────────────────────────────────────┘
                                    │
                                    ▼
 ┌────────────────────────────────────────────────────────────────────────┐
 │                   PHASE 2 : SURVEILLANCE ACTIVE PAR IA                 │
 │  • Mains sur le volant (MAX30100) + micro-mouvements (MPU6050).        │
 │  • Calcul continu de l'indice de fatigue et de la VRC. Alarmes COUPEES.│
 └──────────────────────────────────┬─────────────────────────────────────┘
                                    │ (Inactivité > 4,5s OU Fatigue IA Élevée)
                                    ▼
 ┌────────────────────────────────────────────────────────────────────────┐
 │                   PHASE 3 : PRÉ-ALERTE AVERTISSEMENT                   │
 │  • LED Jaune allumée fixe + bip sonore intermittent doux.              │
 │  • Incite le conducteur à replacer les mains et corriger la direction. │
 └──────────────────────────────────┬─────────────────────────────────────┘
                                    │ (Inactivité > 8,0s : Conducteur sans réaction)
                                    ▼
 ┌────────────────────────────────────────────────────────────────────────┐
 │                   PHASE 4 : ALARME CRITIQUE & TÉLÉGRAM                 │
 │  • Sirène continue à fort volume sonore.                               │
 │  • Stroboscope rapide de la LED Rouge.                                 │
 │  • Envoi d'une alerte Telegram d'urgence avec données physiologiques : │
 │    "🚨 Alerte Critique : Conducteur Inactif ! Pouls : 74 BPM | SpO2: 98%"│
 └────────────────────────────────────────────────────────────────────────┘
```

---

## 🔌 Câblage Électrique (Bus I2C Partagé)

Le **MPU6050** et le **MAX30100** partagent les lignes **GPIO 21 (SDA)** et **GPIO 22 (SCL)** de l'ESP32 :

```text
                        +----------------------------------+
                        |           CARTE ESP32            |
                        |                                  |
 [MPU6050 GYRO/ACCEL]   |                                  |
 VCC -----------------> | 3V3 / VIN                        |
 GND -----------------> | GND                              |
 SDA -----------------> | GPIO 21 (I2C SDA) <──────────────┼────+
 SCL -----------------> | GPIO 22 (I2C SCL) <──────────────┼──+ │
                        |                                  |  │ │
 [MAX30100 CARDIAQUE]   |                                  |  │ │
 VCC -----------------> | 3V3 (ou VIN selon le module)     |  │ │
 GND -----------------> | GND                              |  │ │
 SDA ---------------------------------------------------------+ │
 SCL -----------------------------------------------------------+
                        |                                  |
 [BUZZER ACTIF]         |                                  |
 POS (+) -------------> | GPIO 25                          |
 NEG (-) -------------> | GND                              |
                        |                                  |
 [LED ROUGE CRITIQUE]   |                                  |
 Anode (+) -----------> | GPIO 26 ─── [ Résistance 220Ω ] ─+
 Cathode (-) ---------> | GND                              |
                        |                                  |
 [LED JAUNE PRÉ-ALERTE] |                                  |
 Anode (+) -----------> | GPIO 27 ─── [ Résistance 220Ω ] ─+
 Cathode (-) ---------> | GND                              |
                        +----------------------------------+
```

---

## 📱 Commandes Interactives sur Telegram

Vous pouvez envoyer des commandes au bot Telegram à tout moment pour obtenir le diagnostic du véhicule :

| Commande | Action |
| :--- | :--- |
| `/ai` | Renvoie le **Score de Fatigue TinyML (0–100%)**, le niveau de risque, la VRC (RMSSD en ms) et la variance de direction. |
| `/vitals` | Renvoie en direct la **Fréquence Cardiaque (BPM)**, l'**Oxygène Sanguin (SpO2 %)** et l'état des mains sur le volant. |
| `/status` | Renvoie l'état global du système, la qualité du signal Wi-Fi (RSSI) et le compteur total d'incidents. |
| `/test` | Déclenche un test physique d'une seconde du buzzer, de la LED jaune et de la LED rouge. |
| `/aide` | Affiche le menu des commandes disponibles. |

---

## 🛠️ Instructions pour l'IDE Arduino

### 1. Installation des Bibliothèques Requises
Dans l'IDE Arduino, allez dans **Croquis > Inclure une bibliothèque > Gérer les bibliothèques...** et installez :
1. **`MAX30100lib`** (par OXullo Intervent) — Gestion de l'oxymètre de pouls et des battements cardiaques.
2. **`UniversalTelegramBot`** (par Brian Lough) — Communication HTTPS sécurisée avec Telegram.
3. **`ArduinoJson`** (par Benoit Blanchon) — **Choisir impérativement la version 6.x** !

### 2. Configuration des Identifiants
Ouvrez [`src/esp32_drowsiness_telegram/config.h`](file:///d:/anti/src/esp32_drowsiness_telegram/config.h) et complétez :
```cpp
#define WIFI_SSID     "VOTRE_NOM_WIFI"
#define WIFI_PASSWORD "VOTRE_MOT_DE_PASSE"
#define BOT_TOKEN     "VOTRE_TOKEN_BOT_TELEGRAM" // Obtenu via @BotFather sur Telegram
#define CHAT_ID       "VOTRE_CHAT_ID_NUMERIQUE"  // Obtenu via @userinfobot sur Telegram
```

### 3. Téléversement
* Connectez votre ESP32 en USB.
* Sélectionnez **Outils > Type de carte > ESP32 Dev Module** et le bon **Port COM**.
* Cliquez sur **Téléverser (➡️)** puis ouvrez le **Moniteur Série (115200 bauds)**.

---

## 📝 Description pour Profil LinkedIn (Prête à Copier-Coller)

```text
Titre : Système Intelligent de Détection de Somnolence au Volant – ESP32, MPU6050, MAX30100 & Telegram

Description :
Développement d'un système intelligent et connecté de sécurité routière basé sur microcontrôleur ESP32 et intelligence artificielle embarquée (TinyML), conçu pour prévenir les accidents causés par la fatigue et le micro-sommeil au volant grâce à une fusion de données biométriques et cinématiques.

Points Forts Techniques & Ingénierie :
• Fusion Multi-Capteurs Volant & Biométrie : Intégration d'une centrale inertielle 6 axes MPU6050 analysant les micro-ajustements permanents de la direction et d'un oxymètre de pouls MAX30100 surveillant en direct le rythme cardiaque, l'oxygénation sanguine (SpO2) et la présence physique des mains sur le volant.
• Moteur d'IA Prédictif Embarqué (Edge-AI) : Implémentation d'un algorithme d'inférence TinyML calculant en temps réel la Variabilité de la Fréquence Cardiaque (VRC / HRV - RMSSD) et l'entropie directionnelle pour générer un Indice de Fatigue (0 à 100%), alertant le conducteur avant la perte de conscience.
• Architecture d'Alerte Graduée : Conception d'une machine à états finis intégrant une phase de stabilisation de 8 secondes, une pré-alerte douce (LED jaune et signal sonore discret) et une alarme critique d'urgence (stroboscope rouge et sirène continue).
• Télémétrie Cloud & Sécurité Routière : Intégration de l'API Telegram Bot via protocole TLS/SSL chiffré (WiFiClientSecure) pour la transmission instantanée de rapports d'urgence incluant constantes vitales et télémétrie vers les gestionnaires de flotte et proches.
• Télédiagnostic Bidirectionnel : Implémentation de commandes asynchrones (/ai, /vitals, /status, /test) pour la surveillance à distance de l'état des capteurs, de la puissance réseau et des constantes du conducteur.

Compétences : ESP32, Internet des Objets (IoT), TinyML, Intelligence Artificielle Embarquée, C++ Embarqué, I2C, MPU6050, MAX30100, Télémétrie, API Telegram Bot.
```
