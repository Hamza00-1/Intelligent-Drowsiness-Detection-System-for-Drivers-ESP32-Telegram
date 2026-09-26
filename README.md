# 🚘 Système Intelligent de Détection de Somnolence au Volant (ESP32, Capteur Oculaire, MPU6050, MAX30100 & Telegram)
### 🧠 Fusion Tri-Capteurs & IA Embarquée TinyML pour la Sécurité Routière

Un système complet de sécurité routière IoT et IA développé sur le microcontrôleur **ESP32**. Il combine la puissance de **trois capteurs physiques et physiologiques** pour une fiabilité maximale contre l'endormissement :
1. **👁️ Capteur Oculaire Infrarouge (GPIO 34)** : Détecte directement la fermeture prolongée des yeux et les micro-sommeils instantanés.
2. **🔄 MPU6050 (Centrale inertielle 6 axes)** : Surveille en continu les micro-ajustements angulaires du volant, la variance des mouvements et les à-coups directionnels.
3. **💓 MAX30100 (Oxymètre de pouls)** : Mesure en temps réel le rythme cardiaque, la saturation en oxygène (SpO2), la présence effective des mains sur le volant et les intervalles inter-battements (IBI).
4. **Moteur d'IA Embarqué (Edge-AI TinyML)** : Calcule en continu la **Variabilité de la Fréquence Cardiaque (VRC / HRV - RMSSD)**, l'état d'ouverture oculaire et l'entropie de direction pour générer un **Indice de Fatigue de 0 à 100%**.
5. **Télémétrie Cloud** : Envoi automatique d'alertes chiffrées via l'API Telegram Bot avec constantes vitales en direct, motif précis de l'alarme et télédiagnostic interactif (`/ai`, `/vitals`, `/status`, `/test`, `/aide`).

---

## 📁 Arborescence du Projet

```text
anti/
│
├── 📁 src/
│   └── 📁 esp32_drowsiness_telegram/
│       ├── config.h                       <-- ⚙️ SEUL FICHIER À MODIFIER ! (Wi-Fi, Telegram, Seuils IA)
│       └── esp32_drowsiness_telegram.ino  <-- 🚀 Programme Arduino ESP32 (Fusion Tri-Capteurs + IA)
│
├── 📁 ai_companion/
│   ├── drowsiness_ai_detector.py          <-- 👁️ Vision par ordinateur Face Mesh (Yeux & Bâillements)
│   ├── ml_data_logger_and_trainer.py      <-- 🔬 Entraînement de Modèle ML (Random Forest)
│   ├── requirements.txt                   <-- 📦 Dépendances Python
│   └── run_ai_detector.bat                <-- ⚡ Lanceur en un clic pour Windows
│
├── QUICKSTART.md                          <-- 🚀 Guide de démarrage rapide et câblage
└── README.md                              <-- 📖 Manuel technique complet du système (Ce fichier)
```

---

## 🧠 Le Moteur d'IA Prédictif TinyML (Fusion Tri-Modale)

Notre firmware calcule un **Indice de Fatigue continu de 0 à 100%** en temps réel directement sur le processeur de l'ESP32 :

$$\text{Indice de Fatigue} = w_1 \cdot \text{Facteur Oculaire} + w_2 \cdot \text{Inactivité Volant} + w_3 \cdot \text{Entropie Direction} + w_4 \cdot \text{Chute VRC / Rythme}$$

### 1. Surveillance Oculaire Directe (Capteur IR)
* Un clignement normal dure entre 100 et 400 millisecondes.
* Dès que les yeux restent fermés **$\ge 1,5$ seconde**, le système déclenche immédiatement l'alarme critique de bord pour éviter l'accident imminent.

### 2. Variabilité de la Fréquence Cardiaque (VRC / HRV - RMSSD)
* L'algorithme calcule la racine carrée de la moyenne des différences successives (RMSSD) entre les battements via l'interruption du capteur MAX30100.
* Lors de l'endormissement, le relâchement du système nerveux autonome provoque une baisse et une instabilité caractéristiques de la valeur RMSSD.

### 3. Dynamique et Entropie du Volant (MPU6050)
* **Conducteur attentif** : Fréquence élevée de micro-corrections douces (variance saine).
* **Conducteur somnolent** : Trajectoire figée (variance proche de zéro), suivie de coups de volant brusques pour rattraper la trajectoire.

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
 │                   PHASE 2 : SURVEILLANCE ACTIVE TRI-CAPTEURS           │
 │  • Yeux ouverts + Mains sur volant (MAX30100) + Micro-mouvements (MPU).│
 │  • Calcul continu de l'indice de fatigue et de la VRC. Alarmes COUPEES.│
 └──────────────────────────────────┬─────────────────────────────────────┘
                                    │
          ┌─────────────────────────┴────────────────────────┐
          │ (Yeux fermés > 1,5s)                             │ (Mains & Volant inactifs > 4,5s)
          ▼                                                  ▼
 ┌──────────────────────────────────┐      ┌──────────────────────────────────┐
 │      ALARME CRITIQUE IMMÉDIATE   │      │   PHASE 3 : PRÉ-ALERTE DOUCE     │
 │ • Sirène continue & stroboscope  │      │ • LED Jaune fixe + bip discret   │
 │ • Envoi alerte Telegram immédiate│      │ • Rappel de reprendre le volant  │
 └──────────────────────────────────┘      └─────────────────┬────────────────┘
                                                             │ (Inactivité continue > 8,0s)
                                                             ▼
                                           ┌──────────────────────────────────┐
                                           │  PHASE 4 : ALARME CRITIQUE VOLANT│
                                           │ • Alarme sonore continue         │
                                           │ • Alerte Telegram avec télémétrie│
                                           └──────────────────────────────────┘
```

---

## 🔌 Câblage Électrique Complet

```text
                        +----------------------------------+
                        |           CARTE ESP32            |
                        |                                  |
 [CAPTEUR OCULAIRE IR]  |                                  |
 VCC -----------------> | 3V3                              |
 GND -----------------> | GND                              |
 OUT -----------------> | GPIO 34 (Entrée Dédiée)          |
                        |                                  |
 [MPU6050 GYRO/ACCEL]   |                                  |
 VCC -----------------> | 3V3                              |
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

| Commande | Action |
| :--- | :--- |
| `/status` | Renvoie l'état détaillé des **3 capteurs** (Yeux, Volant, Mains), le signal Wi-Fi et les incidents. |
| `/ai` | Renvoie le **Score de Fatigue TinyML (0–100%)**, l'état des yeux, la VRC (RMSSD en ms) et la variance de direction. |
| `/vitals` | Renvoie en direct la **Fréquence Cardiaque (BPM)**, l'**Oxygène Sanguin (SpO2 %)** et la présence des mains. |
| `/test` | Déclenche un test physique d'une seconde du buzzer, de la LED jaune et de la LED rouge. |
| `/aide` | Affiche le menu des commandes disponibles. |

---

## 🛠️ Instructions pour l'IDE Arduino

### 1. Installation des Bibliothèques Requises
Dans l'IDE Arduino (**Croquis > Inclure une bibliothèque > Gérer les bibliothèques...**) :
1. **`MAX30100lib`** (par OXullo Intervent) — Gestion de l'oxymètre de pouls et des battements cardiaques.
2. **`UniversalTelegramBot`** (par Brian Lough) — Communication HTTPS sécurisée avec Telegram.
3. **`ArduinoJson`** (par Benoit Blanchon) — **Version 6.x impérative**.

### 2. Configuration des Identifiants
Ouvrez [`src/esp32_drowsiness_telegram/config.h`](file:///d:/anti/src/esp32_drowsiness_telegram/config.h) et renseignez vos identifiants Wi-Fi et Telegram.

### 3. Téléversement
* Connectez votre ESP32 en USB.
* Choisissez **Outils > Type de carte > ESP32 Dev Module** et le bon port COM.
* Cliquez sur **Téléverser (➡️)**.

---


Compétences : ESP32, Internet des Objets (IoT), TinyML, Capteurs Infrarouges, MPU6050, MAX30100, C++ Embarqué, I2C, Biométrie, Sécurité Routière, API Telegram Bot.
```
