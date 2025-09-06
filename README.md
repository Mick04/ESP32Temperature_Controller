# ESP32 Temperature Controller

A comprehensive IoT temperature monitoring and heating control system built on ESP32, featuring real-time monitoring, cloud connectivity, and automated heating control.

## 🌡️ Overview

This ESP32-based system monitors temperature using multiple DS18B20 sensors and provides intelligent heating control with cloud connectivity. The system features real-time data logging to Firebase, MQTT communication, visual status indicators, and automated heating control based on configurable schedules and temperature thresholds.

## ✨ Features

### Core Functionality

- **Multi-sensor Temperature Monitoring**: 3x DS18B20 temperature sensors (red, green, blue)
- **Intelligent Heating Control**: Automated relay-controlled heating with scheduling
- **Real-time Status Indicators**: WS2811 LED strip showing system status
- **Cloud Integration**: Firebase Realtime Database for data logging and remote control
- **MQTT Communication**: HiveMQ cloud messaging for real-time updates
- **Email Alerts**: Automated email notifications for system events
- **Time Synchronization**: NTP-based accurate timekeeping
- **Persistent Storage**: SPIFFS file system for local data storage

### Connectivity

- **WiFi**: Automatic connection and reconnection handling
- **Firebase**: Real-time database synchronization
- **MQTT**: Secure TLS connection to HiveMQ cloud
- **NTP**: Network time synchronization

### Hardware Control

- **Temperature Sensors**: 3x DS18B20 digital temperature sensors
- **Heating Control**: SSR relay control for heating elements
- **Status Display**: 4x WS2811 LEDs for visual feedback
- **Current Monitoring**: SCT-013 current sensor support (optional)

## 🔧 Hardware Requirements

### Core Components

- **ESP32 Development Board** (uPesy WROOM recommended)
- **3x DS18B20 Temperature Sensors** with 4.7kΩ pull-up resistor
- **WS2811 LED Strip** (minimum 4 LEDs)
- **SSR Relay Module** (SSR-25 DA or similar)
- **Power Supply** appropriate for your heating load

### Optional Components

- **SCT-013 Current Sensor** (for power monitoring)
- **External heater elements** (100W-200W recommended)

### Pin Configuration

```cpp
#define WS2811_PIN 25    // WS2811 LED strip
#define DS18B20_PIN 27   // DS18B20 temperature sensors (OneWire bus)
#define RELAY_PIN 26     // SSR relay control
#define SCT013_PIN 33    // Current sensor (optional)
```

## 📋 Prerequisites

### Development Environment

- [PlatformIO](https://platformio.org/) IDE or VS Code with PlatformIO extension
- ESP32 development environment

### Cloud Services

- **Firebase Project** with Realtime Database enabled
- **HiveMQ Cloud Account** (or other MQTT broker)
- **Gmail Account** for email notifications (optional)

### Libraries (automatically installed via PlatformIO)

- FastLED ^3.10.1
- Firebase-ESP-Client
- OneWire ^2.3.8
- DallasTemperature ^4.0.4
- NTPClient ^3.2.1
- PubSubClient ^2.8
- ArduinoJson ^7.0.4

## 🚀 Setup Instructions

### 1. Hardware Assembly

1. Connect DS18B20 sensors to pin 27 with 4.7kΩ pull-up resistor
2. Connect WS2811 LED strip to pin 25
3. Connect SSR relay control to pin 26
4. Ensure proper power supply for all components

### 2. Firebase Setup

1. Create a new Firebase project at [Firebase Console](https://console.firebase.google.com/)
2. Enable Realtime Database
3. Note your:
   - API Key
   - Database URL
4. Configure database rules for read/write access

### 3. MQTT Setup

1. Create account at [HiveMQ Cloud](https://www.hivemq.com/mqtt-cloud-broker/)
2. Create a cluster and note:
   - Cluster URL
   - Username/Password
   - Port (8883 for TLS)

### 4. Code Configuration

Edit `src/config.h` with your credentials:

```cpp
// WiFi Configuration
#define WIFI_SSID "YourWiFiName"
#define WIFI_PASSWORD "YourWiFiPassword"

// Firebase Configuration
#define FIREBASE_API_KEY "your-firebase-api-key"
#define FIREBASE_DATABASE_URL "your-database-url"

// MQTT Configuration
#define MQTT_BROKER_HOST "your-hivemq-cluster-url"
#define MQTT_USERNAME "your-mqtt-username"
#define MQTT_PASS "your-mqtt-password"

// Email Configuration (optional)
#define SENDER_EMAIL "your-gmail@gmail.com"
#define SENDER_PASSWORD "your-app-password"
#define RECIPIENT_EMAIL "notification-recipient@email.com"
```

### 5. Build and Upload

1. Open project in PlatformIO
2. Build: `pio run`
3. Upload: `pio run --target upload`
4. Monitor: `pio device monitor`

## 📊 System Architecture

### Modular Design

```
src/
├── main.cpp              # Main application logic
├── config.h              # Configuration and pin definitions
├── WiFiManagerCustom.*   # WiFi connection management
├── TemperatureSensors.*  # DS18B20 sensor handling
├── StatusLEDs.*          # WS2811 LED status display
├── FirebaseService.*     # Firebase cloud integration
├── MQTTManager.*         # MQTT communication
├── TimeManager.*         # NTP time synchronization
├── HeaterControl.*       # Heating control logic
└── GetShedual.*          # Schedule management
```

### Data Flow

1. **Sensors** → Temperature readings every 1 second
2. **Processing** → Compare with thresholds and schedules
3. **Control** → Activate/deactivate heating relay
4. **Logging** → Send data to Firebase and MQTT
5. **Display** → Update LED status indicators

## 🔍 Status Indicators

The WS2811 LED strip provides visual feedback:

| LED | Function        | Colors                                                                |
| --- | --------------- | --------------------------------------------------------------------- |
| 0   | WiFi Status     | 🔴 Red (Disconnected) / 🟢 Green (Connected)                          |
| 1   | Firebase Status | 🔴 Red (Error) / 🟡 Yellow (Connecting) / 🟢 Green (Connected)        |
| 2   | MQTT Status     | 🔴 Red (Disconnected) / 🟡 Yellow (Connecting) / 🟢 Green (Connected) |
| 3   | Heater Status   | 🔵 Blue (Off) / 🟠 Orange (On) / 🔴 Red (Error)                       |

## 📡 MQTT Topics

### Published by ESP32

```
esp32/sensors/temperature/red      # Red sensor temperature
esp32/sensors/temperature/green    # Green sensor temperature
esp32/sensors/temperature/blue     # Blue sensor temperature
esp32/system/status                # System health status
esp32/heater/status                # Heater on/off status
```

### Subscribed by ESP32

```
esp32/heater/target_temp           # Target temperature setting
esp32/heater/schedule              # Heating schedule updates
esp32/system/commands              # System control commands
```

## 🔧 Configuration Options

### Temperature Thresholds

```cpp
#define TARGET_TEMPERATURE 21.0     // Default target temperature (°C)
#define HYSTERESIS 0.5              // Temperature control hysteresis
```

### Timing Configuration

```cpp
#define SENSOR_READ_INTERVAL 1000   // Sensor reading interval (ms)
#define FIREBASE_SYNC_INTERVAL 5000 // Firebase sync interval (ms)
#define MQTT_KEEPALIVE 60           // MQTT keepalive (seconds)
```

## 🛠️ Troubleshooting

### Common Issues

**WiFi Connection Failed**

- Check SSID and password in config.h
- Ensure ESP32 is in range of WiFi network
- Verify 2.4GHz network (ESP32 doesn't support 5GHz)

**Firebase Connection Issues**

- Verify API key and database URL
- Check Firebase database rules
- Ensure internet connectivity

**Temperature Sensors Not Detected**

- Check wiring and pull-up resistor (4.7kΩ)
- Verify DS18B20_PIN definition
- Test sensors individually

**MQTT Connection Failed**

- Verify broker URL and credentials
- Check firewall settings
- Ensure TLS/SSL support

### Debug Output

Enable verbose logging by changing in `platformio.ini`:

```ini
build_flags = -DCORE_DEBUG_LEVEL=5
```

## 📈 Performance

### Memory Usage

- **RAM**: ~15.5% (50KB of 327KB)
- **Flash**: ~98% (1.28MB of 1.31MB)
- **Free Heap**: ~290KB at startup

### Power Consumption

- **ESP32**: ~240mA (WiFi active)
- **DS18B20**: ~1mA each
- **WS2811 LEDs**: ~60mA per LED (max brightness)
- **Total System**: ~400mA (excluding heating load)

## 🚀 Future Enhancements

### Planned Features

- [ ] Web interface for configuration
- [ ] Over-the-air (OTA) updates
- [ ] Current sensor integration (SCT-013)
- [ ] Multi-zone heating control
- [ ] Advanced scheduling with multiple time periods
- [ ] Data export functionality
- [ ] Mobile app integration

### Expansion Ideas

- [ ] Integration with Home Assistant
- [ ] Voice control (Alexa/Google)
- [ ] Solar panel integration
- [ ] Weather forecast integration
- [ ] Machine learning temperature prediction

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## 📄 License

This project is open source. Please check the repository for license details.

## 👨‍💻 Author

Created by Mick Cave for intelligent Reptile vivarium heating control.

## 🆘 Support

For issues and questions:

1. Check the troubleshooting section
2. Review the hardware connections
3. Check serial monitor output for error messages
4. Create an issue on the repository

---

**⚠️ Safety Notice**: This system controls heating elements. Ensure proper electrical safety measures, appropriate fuses/breakers, and follow local electrical codes. Test thoroughly before unattended operation.

todo list:
- [ ] Add over-the-air (OTA) updates
- [ ] update voltageSensor() for Two heaters
- [ ] Remove debug output from all files
- [ ] look for unwanted code for example average calculations not wanted
- [ ] make chart's scrollable for large data sets in the web app
- [ ] get connected/unconnected status working in the web app
- [ ] try to get the dashboard to update the temperature readings in real time
