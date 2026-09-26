# 🚀 Guide de Démarrage Rapide et de Déploiement

Bienvenue ! Voici toutes les instructions pour configurer, câbler et déployer rapidement le **Système Intelligent de Détection de Somnolence pour Conducteurs (ESP32 + MPU6050 + MAX30100 + Telegram avec IA Embarquée)**.

---

## 🚘 Vue d'Ensemble du Système

Ce système surveille la vigilance du conducteur directement depuis le **volant du véhicule** grâce à une fusion multi-capteurs et une **IA prédictive** :
1. **MPU6050 (Centrale inertielle 6 axes)** : Détecte les micro-ajustements permanents et les mouvements angulaires du volant.
2. **MAX30100 (Oxymètre de pouls)** : Mesure la **fréquence cardiaque**, la **saturation en oxygène (SpO2)** et confirme la **présence des mains sur le volant**.
3. **🧠 Moteur d'IA TinyML Embarqué** : Calcule en temps réel la **Variabilité de la Fréquence Cardiaque (VRC / HRV - RMSSD)** et la variance des mouvements du volant pour établir un **Indice de Fatigue de 0 à 100%**. Il prévient le conducteur **avant** l'endormissement !
4. **Phase de Stabilisation (8s)** : Au démarrage, étalonne les capteurs et compense la dérive du gyroscope (la LED jaune clignote doucement).
5. **Phase de Pré-Alerte (après 4,5s d'inactivité)** : La LED jaune s'allume en continu et un bip intermittent avertit le conducteur de remettre les mains sur le volant.
6. **Phase d'Alarme Critique (après 8,0s d'inactivité)** : La LED rouge clignote rapidement en stroboscope, la sirène retentit en continu, et une **alerte Telegram instantanée** est envoyée en Wi-Fi avec les constantes vitales en direct.

---

## 📱 Commandes Interactives sur Telegram

Le boîtier dans le véhicule peut être interrogé à tout moment depuis votre téléphone :
* `/ai` $\rightarrow$ Score de fatigue IA TinyML (0–100%), VRC (RMSSD en ms) et régularité du volant.
* `/vitals` $\rightarrow$ Fréquence cardiaque (BPM), saturation SpO2 (%) et détection des mains.
* `/status` $\rightarrow$ État général du système, puissance Wi-Fi (RSSI) et compteur d'incidents.
* `/test` $\rightarrow$ Test physique d'une seconde du buzzer et des LED.
* `/help` ou `/aide` $\rightarrow$ Liste des commandes disponibles.

---

## 🛠️ Matériel Requis (Checklist)

### 1. Composants :
* [ ] 1x Carte de développement ESP32 (ESP32-WROOM-32 ou NodeMCU ESP32) + câble Micro-USB
* [ ] 1x Module MPU6050 (Accéléromètre / Gyroscope 6 axes)
* [ ] 1x Module MAX30100 (Oxymètre de pouls et capteur cardiaque)
* [ ] 1x Buzzer actif (5V ou 3.3V)
* [ ] 1x LED Rouge + 1x LED Jaune + 2x Résistances de 220Ω
* [ ] Plaque d'essai (Breadboard) & Fils de prototypage (jumper wires)

### 2. Câblage Électrique (Bus I2C partagé sur GPIO 21 & 22) :
Les deux capteurs partagent les mêmes broches de communication I2C :
* **MPU6050 SDA** ET **MAX30100 SDA** $\rightarrow$ **ESP32 GPIO 21**
* **MPU6050 SCL** ET **MAX30100 SCL** $\rightarrow$ **ESP32 GPIO 22**
* **MPU6050 VCC** & **MAX30100 VCC** $\rightarrow$ **3.3V (ou VIN)**
* **GND** $\rightarrow$ **GND** (Masse commune)
* **Buzzer (+)** $\rightarrow$ **GPIO 25**
* **LED Rouge (+)** $\rightarrow$ **GPIO 26** (avec résistance 220Ω)
* **LED Jaune (+)** $\rightarrow$ **GPIO 27** (avec résistance 220Ω)

### 3. Configuration dans `config.h` :
Ouvrez le fichier [`src/esp32_drowsiness_telegram/config.h`](file:///d:/anti/src/esp32_drowsiness_telegram/config.h) et renseignez :
* `WIFI_SSID` & `WIFI_PASSWORD` (Votre réseau Wi-Fi ou partage de connexion mobile)
* `BOT_TOKEN` (Généré en 30 secondes via `@BotFather` sur Telegram)
* `CHAT_ID` (Votre identifiant numérique obtenu via `@userinfobot` sur Telegram)

### 4. Téléversement avec l'IDE Arduino :
1. Dans le gestionnaire de bibliothèques (**Croquis > Inclure une bibliothèque > Gérer les bibliothèques...**), installez :
   * **`MAX30100lib`** (par OXullo Intervent)
   * **`UniversalTelegramBot`** (par Brian Lough)
   * **`ArduinoJson`** (par Benoit Blanchon — **Version 6.x**)
2. Ouvrez [`esp32_drowsiness_telegram.ino`](file:///d:/anti/src/esp32_drowsiness_telegram/esp32_drowsiness_telegram.ino), sélectionnez la carte **ESP32 Dev Module**, choisissez le bon port COM, et cliquez sur **Téléverser (➡️)** !

---

📖 *Pour la documentation technique complète, les formules mathématiques de l'IA et les schémas, consultez le fichier [README.md](file:///d:/anti/README.md).*
