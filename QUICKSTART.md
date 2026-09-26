# 🚀 Guide de Démarrage Rapide et de Déploiement

Bienvenue ! Voici toutes les instructions pour configurer, câbler et déployer rapidement le **Système Intelligent de Détection de Somnolence pour Conducteurs (ESP32 + Capteur Oculaire + MPU6050 + MAX30100 + Telegram avec IA Embarquée)**.

---

## 🚘 Vue d'Ensemble du Système (Fusion Tri-Capteurs)

Ce système surveille la vigilance du conducteur grâce à 3 canaux physiques complémentaires :
1. **👁️ Capteur Oculaire IR (GPIO 34)** : Détecte directement la fermeture prolongée des paupières (micro-sommeil instantané).
2. **🔄 MPU6050 (Volant, I2C)** : Détecte les micro-ajustements permanents et les mouvements angulaires du volant.
3. **💓 MAX30100 (Oxymètre, I2C)** : Mesure le **rythme cardiaque**, l'**oxygène (SpO2)** et confirme la **présence des mains sur le volant**.
4. **🧠 Moteur d'IA TinyML Embarqué** : Calcule en temps réel la **Variabilité Cardiaque (VRC / HRV - RMSSD)**, l'état oculaire et la variance de direction pour établir un **Indice de Fatigue de 0 à 100%**.
5. **Phase de Stabilisation (8s)** : Au démarrage, étalonne les capteurs et compense la dérive du gyroscope (la LED jaune clignote doucement).
6. **Alertes Graduées** :
   * **Instantanée** : Yeux fermés $> 1,5\text{ s} \rightarrow$ Alarme critique immédiate !
   * **Volant & Mains** : Sans action $> 4,5\text{ s} \rightarrow$ Pré-alerte jaune ; $> 8,0\text{ s} \rightarrow$ Alarme critique rouge + Alerte Telegram.

---

## 📱 Commandes Interactives sur Telegram

Le boîtier dans le véhicule peut être interrogé à tout moment depuis votre téléphone :
* `/status` $\rightarrow$ Diagnostic en direct des 3 capteurs (Yeux, Volant, Mains) et compteur d'incidents.
* `/ai` $\rightarrow$ Score de fatigue IA TinyML (0–100%), VRC (RMSSD en ms) et régularité du volant.
* `/vitals` $\rightarrow$ Fréquence cardiaque (BPM), saturation SpO2 (%) et présence des mains.
* `/test` $\rightarrow$ Test physique d'une seconde du buzzer et des LED.
* `/aide` $\rightarrow$ Liste des commandes disponibles.

---

## 🛠️ Matériel Requis (Checklist Complète)

### 1. Composants :
* [ ] 1x Carte ESP32 (ESP32-WROOM-32 ou NodeMCU ESP32) + câble Micro-USB
* [ ] 1x Module Capteur Oculaire IR (ou capteur infrarouge FC-51 / TCRT5000)
* [ ] 1x Module MPU6050 (Accéléromètre / Gyroscope 6 axes I2C)
* [ ] 1x Module MAX30100 (Oxymètre de pouls et capteur cardiaque I2C)
* [ ] 1x Buzzer actif (5V ou 3.3V)
* [ ] 1x LED Rouge + 1x LED Jaune + 2x Résistances de 220Ω
* [ ] Plaque d'essai (Breadboard) & Fils de prototypage

### 2. Câblage Électrique Complet :

| Composant | Broche Composant | Broche ESP32 | Description |
| :--- | :--- | :--- | :--- |
| **Capteur Oculaire IR** | `OUT / D0` | **GPIO 34** | Entrée numérique/analogique (Fermeture des yeux) |
| | `VCC / GND` | `3.3V / GND` | Alimentation du capteur |
| **MPU6050 (Volant)** | `SDA` | **GPIO 21** | Ligne de données I2C partagée |
| | `SCL` | **GPIO 22** | Ligne d'horloge I2C partagée |
| | `VCC / GND` | `3.3V / GND` | Alimentation du capteur |
| **MAX30100 (Pouls/Mains)** | `SDA` | **GPIO 21** | Ligne de données I2C partagée |
| | `SCL` | **GPIO 22** | Ligne d'horloge I2C partagée |
| | `VCC / GND` | `3.3V / GND` | Alimentation du capteur |
| **Buzzer Actif** | `Positif (+)` | **GPIO 25** | Sirène sonore d'alarme |
| | `Négatif (-)` | `GND` | Masse commune |
| **LED Rouge (Critique)** | `Anode (+)` via 220Ω | **GPIO 26** | Stroboscope d'alarme critique |
| **LED Jaune (Pré-Alerte)**| `Anode (+)` via 220Ω | **GPIO 27** | Témoin d'avertissement doux |

---

## 🚀 Mise en Service Pas à Pas

### 1. Configuration dans `config.h` :
Ouvrez [`src/esp32_drowsiness_telegram/config.h`](file:///d:/anti/src/esp32_drowsiness_telegram/config.h) et complétez :
* `WIFI_SSID` & `WIFI_PASSWORD` (Votre réseau Wi-Fi ou partage de connexion mobile)
* `BOT_TOKEN` (Généré en 30 secondes via `@BotFather` sur Telegram)
* `CHAT_ID` (Votre identifiant numérique obtenu via `@userinfobot` sur Telegram)

### 2. Téléversement avec l'IDE Arduino :
1. Dans le gestionnaire de bibliothèques (**Croquis > Inclure une bibliothèque > Gérer les bibliothèques...**), installez :
   * **`MAX30100lib`** (par OXullo Intervent)
   * **`UniversalTelegramBot`** (par Brian Lough)
   * **`ArduinoJson`** (par Benoit Blanchon — **Version 6.x**)
2. Ouvrez [`esp32_drowsiness_telegram.ino`](file:///d:/anti/src/esp32_drowsiness_telegram/esp32_drowsiness_telegram.ino), sélectionnez la carte **ESP32 Dev Module**, choisissez le bon port COM, et cliquez sur **Téléverser (➡️)** !

---

📖 *Pour la documentation technique complète, les formules mathématiques de l'IA et les schémas, consultez le fichier [README.md](file:///d:/anti/README.md).*
