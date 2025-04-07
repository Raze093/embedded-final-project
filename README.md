# ESP32-C3 RainMaker Alert System 🚨

This project is built on the ESP32-C3 using the **ESP RainMaker framework**. It integrates **physical and virtual buttons (triggers)** to detect simulated high temperature and humidity events. When triggered, the system:

- Activates a buzzer for 5 seconds
- Turns on an LED
- Sends an **ESP RainMaker alert notification** to the user's app
- Works with Google Home (LED switch only)
- Supports OTA firmware updates

---

## 🧠 Features

- ✅ Temperature & Humidity virtual triggers (via RainMaker mobile app)
- ✅ Physical button support (GPIO 4 = Temp, GPIO 5 = Humidity)
- ✅ Buzzer notification (GPIO 3) for 5 seconds
- ✅ LED indicator (GPIO 2) turns ON during trigger, OFF afterwards
- ✅ OTA firmware update support via RainMaker Dashboard
- ✅ RainMaker alerts using `esp_rmaker_raise_alert()`

---

## 🧰 Hardware Requirements

- [XIAO ESP32-C3 Board](https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/)
- Breadboard + jumper wires
- Momentary push buttons (for GPIO 4 and 5)
- Passive Buzzer (GPIO 3)
- LED (GPIO 2) with 220Ω resistor

---

## 📦 Project Structure

project/ 
├── main/ 
│     ├── app_main.c # Main logic, triggers, alerts 
│     ├── app_driver.c # GPIO and device initialization 
│     ├── app_priv.h # Shared variables and declarations 
      ├── CMakeLists.txt 
      ├── sdkconfig 
      └── README.md # ← This file


---

## 🚀 Setup Instructions

### 1. Flash the Firmware

> ⚠️ Use **ESP-IDF v5.4+** with `idf.py`.

bash
idf.py set-target esp32c3
idf.py build
idf.py flash monitor

2. Provision the Device
Use the ESP RainMaker app to connect the device:

    Open the app → Add device

    Connect over BLE or SoftAP

    Name your device (e.g., "LED + Triggers")

    You will see three switches:

        Light

        Temperature Trigger

        Humidity Trigger

⚡ Usage
Virtual Triggers
From the RainMaker app, toggle:

    Temperature Trigger → activates LED + buzzer + alert

    Humidity Trigger → same behavior

Physical Triggers
Press physical buttons:

    GPIO 4 (Temp) or GPIO 5 (Humidity)

    Simulates the same response as virtual triggers

🔁 OTA Firmware Updates

To push a new firmware via RainMaker Dashboard:

1. Build the .bin file
2. Change version in CMakeLists.txt:
3. set(PROJECT_VER "1.0.1")
4. Go to dashboard.rainmaker.espressif.com
5. Select your node → Firmware Update → Upload new firmware
6. Wait for reboot after OTA finishes

---------If version doesn’t change, the OTA will be rejected--------

📜 Customization
Component	        GPIO Pin
LED	                GPIO 2
Buzzer	            GPIO 3
Temp Button	        GPIO 4
Humidity Button	    GPIO 5

Change these in app_driver.c if needed.

🧩 Possible Extensions
-Integrate actual DHT11/DHT22 sensor
-Telegram Bot or Email API for remote alerts
-Wi-Fi signal strength reporting
-Google Assistant trigger integration

👨‍💻 Author
Developed by Hafiz
For Universiti Teknologi PETRONAS final IoT-based Embedded Systems project

🛡 License
This project is open source and free to use for educational or non-commercial purposes.
