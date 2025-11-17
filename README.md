
# ESP32 Energy Monitor

Projet **Energy Monitor** pour ESP32 : mesure la consommation électrique via un capteur de tension (ZMPT101B) et un capteur de courant (SCT-013), calcule la puissance, l’énergie consommée, et envoie les données vers un serveur backend via WiFi.

---

## 🚀 Matériel utilisé

- ESP32 (devkit)
- Capteur de tension : ZMPT101B
- Capteur de courant : SCT-013 avec bias 1.65V
- LEDs pour indicateurs :
  - **LED_RUN (GPIO 2)** : clignote si le système fonctionne
  - **LED_WIFI (GPIO 4)** : ON si WiFi connecté
  - **LED_POST (GPIO 5)** : ON si POST HTTP réussi

---

## ⚡ Fonctionnalités

- Mesure en temps réel de :
  - Tension RMS (Vrms)
  - Courant RMS (Irms)
  - Puissance réelle et apparente
  - Facteur de puissance
  - Énergie consommée (kWh)
- Calcul automatique de `delta_kwh` à chaque intervalle
- Envoi des données vers le serveur backend via POST JSON
- Gestion automatique de la connexion WiFi
- LEDs indiquant l’état du système, WiFi et POST

---

## 🔧 Configuration du code

Modifier les paramètres en début de fichier `.ino` :

```cpp
const char* ssid = "ton_wifi";        // Nom du réseau WiFi
const char* password = "motdepasse";  // Mot de passe WiFi
const char* serverUrl = "http://IP_SERVER:3000/api/logs"; // URL du backend
const int device_id = 1;               // ID unique de l'appareil
````

Les constantes de calibration pour les capteurs :

```cpp
const float FACTEUR_V = 203.70;       // Facteur de conversion ZMPT101B
const float CURRENT_SENSITIVITY = 20.86; // Sensibilité SCT-013
```

---

## 📦 Installation et téléversement

1. Ouvrir Arduino IDE ou PlatformIO
2. Installer la bibliothèque [ArduinoJson](https://arduinojson.org/) via Library Manager
3. Connecter l’ESP32
4. Sélectionner la bonne carte et port
5. Téléverser le code `.ino`

---

## 🔗 Dépôt GitHub

[Smart Energy Arduino](https://github.com/michaeltandrify-lab/smart-energy-arduino)

---

## 📝 Notes

* Le code prend en compte un intervalle d’envoi (`sendInterval`) de 5 secondes par défaut.
* Les LEDs donnent un retour visuel immédiat pour le fonctionnement.
* Adapter le bias et les facteurs de conversion si vous changez de capteurs.

---

## 📄 License

MIT License
Veux‑tu que je fasse ça ?
```
