# ESP32 Environmental Monitoring System

A low-power environmental monitoring IoT system based on ESP32 with wireless mesh networking, cloud integration, and ultra-low-power sensor reading capabilities.

## Overview

This project implements a distributed environmental monitoring system using ESP32-WROOM-32 microcontrollers. The system features a mesh network topology with sensor nodes that communicate through ESP-NOW protocol, and a gateway that bridges the mesh network to ThingsBoard IoT platform via MQTT.

Key highlights:
- **Ultra-low power consumption**: 1.86 mA average, enabling 45+ days of battery operation
- **ULP Coprocessor**: Reads sensors during deep sleep (15 µA)
- **Custom mesh protocol**: Encrypted ESP-NOW networking with ChaCha20-Poly1305
- **Web-based provisioning**: Configure devices via WiFi Access Point
- **OTA updates**: Over-the-air firmware updates distributed through mesh network
- **Real-time monitoring**: Historical and live data visualization on ThingsBoard

## Features

### Hardware Support
- **4 Digilent Pmod Sensors**:
  - **Pmod HYGRO** (410-347): I²C Temperature & Humidity sensor
  - **Pmod AQS** (410-386): I²C Air Quality (CO₂ & VOC) sensor
  - **Pmod MIC3** (410-312): SPI Noise/Sound level sensor
  - **Pmod ALS** (410-286): SPI Ambient Light sensor

### Software Features
- **ESP-NOW Mesh Networking**: Star topology with encrypted communication
- **ULP Assembly Programming**: 447 lines of optimized I²C/SPI bit-banging code
- **Deep Sleep Management**: 2.17% duty cycle for sensor nodes
- **NVS Configuration Storage**: Persistent device settings
- **Web Provisioning Interface**: HTML/CSS/JS configuration portal
- **ThingsBoard Integration**: MQTT Device API for cloud telemetry
- **OTA Firmware Updates**: Chunk-based distribution via mesh network
- **RTC Memory Buffering**: 350-reading buffer (58 minutes autonomy)

## Architecture

```
┌──────────────────┐
│  Sensor Node 1   │───┐
│  (ESP32 + 4x     │   │
│   Pmod Sensors)  │   │
└──────────────────┘   │
                       │     ESP-NOW Mesh
┌──────────────────┐   │    (ChaCha20-Poly1305)
│  Sensor Node 2   │───┼────────────────────┐
└──────────────────┘   │                    │
                       │              ┌─────▼──────┐
┌──────────────────┐   │              │  Gateway   │
│  Sensor Node N   │───┘              │  (ESP32)   │
└──────────────────┘                  └─────┬──────┘
                                            │
                                       WiFi │ MQTT
                                            │
                                      ┌─────▼──────────┐
                                      │  ThingsBoard   │
                                      │  IoT Platform  │
                                      └────────────────┘
```

### System Components

#### Sensor Nodes
- Read environmental data every 10 seconds using ULP coprocessor
- Store readings in RTC memory (survives deep sleep)
- Wake up every 1 minute to transmit data batch via ESP-NOW
- Support OTA firmware updates from gateway
- Average power consumption: 1.86 mA

#### Gateway
- Receives telemetry from all sensor nodes
- Forwards data to ThingsBoard via MQTT
- Distributes OTA firmware updates to nodes
- Synchronizes time via SNTP
- Manages mesh network coordination

## Hardware Requirements

### Main Components
- **ESP32-WROOM-32** (1x Gateway + Nx Sensor Nodes)
- **Digilent Pmod HYGRO** (410-347) - I²C Temperature & Humidity
- **Digilent Pmod AQS** (410-386) - I²C Air Quality (CO₂ & VOC)
- **Digilent Pmod MIC3** (410-312) - SPI Noise Level
- **Digilent Pmod ALS** (410-286) - SPI Ambient Light

### Pin Connections

| Function | GPIO | RTC GPIO | Description |
|----------|------|----------|-------------|
| I²C SDA | 26 | RTC_GPIO7 | Pmod HYGRO & AQS Data |
| I²C SCL | 25 | RTC_GPIO6 | Pmod HYGRO & AQS Clock |
| SPI MISO | 27 | RTC_GPIO17 | Pmod MIC3 & ALS Data |
| SPI SCK | 32 | RTC_GPIO9 | Pmod MIC3 & ALS Clock |
| CS MIC3 | 13 | - | Chip Select Noise Sensor |
| CS ALS | 14 | - | Chip Select Light Sensor |

### Sensor Specifications

| Sensor | Interface | Address/CS | Measurement | Resolution | Range |
|--------|-----------|------------|-------------|------------|-------|
| Pmod HYGRO | I²C | 0x40 | Temperature | 14-bit | -40°C to +125°C |
| Pmod HYGRO | I²C | 0x40 | Humidity | 14-bit | 0-100% RH |
| Pmod AQS | I²C | 0x5B | CO₂ | 16-bit | 400-8192 ppm |
| Pmod AQS | I²C | 0x5B | VOC | 16-bit | 0-1187 ppb |
| Pmod MIC3 | SPI | GPIO 13 | Noise | 12-bit | 0-4095 |
| Pmod ALS | SPI | GPIO 14 | Luminosity | 8-bit | 0-255 lux |

## Software Requirements

### Development Environment
- **ESP-IDF**: v5.5.1 or higher
- **Python**: 3.8+ (for ESP-IDF tools)
- **Git**: For repository cloning

### Dependencies (Auto-managed by ESP-IDF Component Manager)
- **FreeRTOS**: Real-time operating system
- **mbedTLS**: ChaCha20-Poly1305 encryption
- **ESP-NOW**: Wireless mesh protocol
- **MQTT Client**: ThingsBoard communication
- **HTTP Server**: Web provisioning interface
- **NVS**: Non-volatile configuration storage
- **ULP**: Ultra-low-power coprocessor
- **cJSON**: JSON parsing and generation

### Custom Components
- **wmesh v0.1.0**: ESP-NOW mesh networking with encryption (473 lines)
- **thingsboard v0.1.0**: MQTT Device API client for ThingsBoard

## Installation

### 1. Clone Repository
```bash
git clone https://github.com/leonidasdev/esp32-environmental-monitoring.git
cd esp32-environmental-monitoring
```

### 2. Install ESP-IDF
Follow the [ESP-IDF Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32/get-started/index.html)

```bash
# Linux/macOS
. $HOME/esp/esp-idf/export.sh

# Windows PowerShell
.$HOME/esp/esp-idf/export.ps1
```

### 3. Configure Project
```bash
idf.py menuconfig
```

Key configuration options:
- **Component config → SPV Config**: Device role, wake interval, sensor pins
- **Component config → wmesh**: Encryption algorithm, network settings
- **Component config → thingsboard**: ThingsBoard server hostname

### 4. Build Firmware
```bash
idf.py build
```

### 5. Flash to ESP32
```bash
# Flash and monitor
idf.py -p COM3 flash monitor  # Windows
idf.py -p /dev/ttyUSB0 flash monitor  # Linux

# Flash only
idf.py -p COM3 flash
```

## Configuration

### Initial Setup (Provisioning Mode)

1. **First boot**: Device starts in Access Point mode
2. **Connect to WiFi**: `SPV-XXXXXXXX` (SSID based on MAC address)
3. **Open browser**: Navigate to `http://192.168.4.1`
4. **Configure device**:
   - Device Name
   - Role (Gateway or Node)
   - WiFi Credentials (Gateway only)
   - ThingsBoard Token (Gateway only)
   - Mesh Network Key (32-byte hex)
5. **Save**: Device reboots with new configuration

### Configuration Parameters

```c
typedef struct {
    char name[32];                  // Device name
    spv_config_role_t role;         // GATEWAY or NODE
    char wifi_ssid[32];             // WiFi SSID (gateway)
    char wifi_password[64];         // WiFi password (gateway)
    char thingsboard_token[20];     // ThingsBoard access token
    uint8_t mesh_key[32];           // Mesh encryption key
} spv_config_t;
```

### Persistent Storage
Configuration is stored in NVS partition (`spv_config`) and persists across reboots and firmware updates.

## Usage

### Sensor Node Operation

1. **Cold Boot**:
   - Initializes sensors and ULP coprocessor
   - Starts ULP program for autonomous sensor reading
   - Joins mesh network
   - Waits for gateway beacon

2. **Normal Operation Cycle** (every 60 seconds):
   - Deep sleep: 58.7 seconds (15 µA)
   - Wake up: ESP32 CPU starts
   - Read buffer: 6 accumulated readings from RTC memory
   - Transmit: Send batch to gateway via ESP-NOW (~1.3s)
   - Confirm: Wait for acknowledgment
   - Sleep: Return to deep sleep

3. **ULP Autonomous Operation** (during sleep):
   - Wake every 10 seconds
   - Read all 4 sensors via I²C/SPI
   - Store in RTC memory buffer
   - Return to ULP sleep

### Gateway Operation

1. **Startup**:
   - Connect to WiFi
   - Synchronize time via SNTP
   - Connect to ThingsBoard MQTT broker
   - Start mesh network

2. **Data Collection**:
   - Receive telemetry from sensor nodes
   - Add Unix timestamps to readings
   - Forward to ThingsBoard in JSON format

3. **OTA Distribution**:
   - Check ThingsBoard for firmware updates
   - Download new firmware binary
   - Broadcast OTA start message to nodes
   - Send firmware chunks via ESP-NOW
   - Verify successful updates

### ThingsBoard Integration

#### Telemetry Format
```json
[
  {
    "node_name": {
      "ts": 1702584000000,
      "values": {
        "temperature": 25.3,
        "humidity": 45.2,
        "co2": 450,
        "voc": 120,
        "noise": 2048,
        "luminosity": 128
      }
    }
  }
]
```

#### MQTT Topics
- **Telemetry**: `v1/devices/me/telemetry`
- **OTA Attributes**: `v1/devices/me/attributes/shared`

## Power Consumption

### Sensor Node Power Profile

| State | Current | Duration | Duty Cycle |
|-------|---------|----------|------------|
| Deep Sleep | 15 µA | 58.7 s/min | 97.83% |
| Active (Tx) | 85 mA | 1.3 s/min | 2.17% |
| **Average** | **1.86 mA** | - | - |

### Battery Life Estimates

| Battery Capacity | Autonomy |
|-----------------|----------|
| 2000 mAh | 45 days |
| 3000 mAh | 67 days |
| 5000 mAh | 112 days |

### Power Optimization Features

1. **ULP Coprocessor**: Sensors read during deep sleep
2. **RTC Memory Buffer**: 350-reading capacity (58 minutes)
3. **Batch Transmission**: Multiple readings per wake cycle
4. **Synchronized Wake**: Coordinated gateway-node timing
5. **Assembly Code**: Optimized I²C/SPI bit-banging (447 lines)

## OTA (Over-The-Air) Updates

### Update Process

1. **Gateway** checks ThingsBoard for new firmware version
2. **Download** firmware binary via HTTP
3. **Broadcast** OTA start message with version and size
4. **Chunk Distribution** (configurable chunk size)
5. **Node Verification** and flash write
6. **Reboot** to new firmware

### OTA Configuration

```c
CONFIG_SPV_OTA_SERVICE_CHUNK_SIZE=512  // Bytes per chunk
CONFIG_SPV_OTA_SERVICE_ID=0x01         // Service identifier
```

### Security
- Firmware signed with ESP32 secure boot (optional)
- Encrypted transmission via ChaCha20-Poly1305
- Version verification before flash

## Project Structure

```
esp32-environmental-monitoring/
├── CMakeLists.txt                  # Top-level CMake configuration
├── sdkconfig                       # ESP-IDF configuration
├── partitions.csv                  # Flash partition table
├── README.md                       # This file
│
├── components/                     # Custom components
│   ├── wmesh/                      # ESP-NOW mesh networking
│   │   ├── src/
│   │   │   ├── wmesh.c            # Mesh management (473 lines)
│   │   │   ├── encryption.c       # ChaCha20-Poly1305 (308 lines)
│   │   │   ├── peer.c             # Peer management
│   │   │   └── storage.c          # NVS peer storage
│   │   ├── include/wmesh/
│   │   ├── idf_component.yml      # Component metadata
│   │   └── Kconfig                # Configuration options
│   │
│   └── thingsboard/                # ThingsBoard MQTT client
│       ├── src/
│       │   ├── connection.c       # MQTT connection
│       │   ├── gateway_telemetry.c # JSON telemetry formatting
│       │   └── ota.c              # OTA metadata retrieval
│       ├── include/thingsboard/
│       └── idf_component.yml
│
├── main/                           # Main application
│   ├── project.c                  # Application entry point
│   │
│   ├── config/                    # Configuration management
│   │   ├── persistence.c          # NVS storage
│   │   └── provisioning.c         # Web provisioning
│   │
│   ├── nodes/                     # Node logic
│   │   ├── gateway.c              # Gateway application (442 lines)
│   │   ├── node.c                 # Sensor node application (342 lines)
│   │   └── common.c               # Shared utilities
│   │
│   ├── sensors/                   # Sensor management
│   │   ├── sensors.c              # Initialization and configuration
│   │   ├── ulp.c                  # ULP management
│   │   ├── ulp.S                  # ULP main loop (210 lines)
│   │   ├── i2c.S                  # Software I²C (178 lines)
│   │   └── spi.S                  # Software SPI (59 lines)
│   │
│   ├── services/                  # Mesh services
│   │   ├── gateway.c              # Gateway service protocol
│   │   ├── telemetry.c            # Telemetry service
│   │   └── ota.c                  # OTA service
│   │
│   ├── webserver/                 # Web provisioning server
│   │   ├── webserver.c            # HTTP server
│   │   ├── request.c              # Request parsing
│   │   └── fs/                    # Filesystem management
│   │
│   ├── wifi/                      # WiFi management
│   │   ├── config.c               # WiFi initialization
│   │   ├── sta_connect.c          # Station mode connection
│   │   └── scan.c                 # Network scanning
│   │
│   ├── ota/                       # OTA management
│   │   ├── ota.c                  # Flash operations
│   │   └── http.c                 # HTTP download
│   │
│   └── time/                      # Time synchronization
│       └── clock.c                # SNTP and RTC
│
├── fs/web/                        # Web interface files
│   ├── index.htm                  # Configuration form
│   ├── script.js                  # Client-side logic
│   └── styles.css                 # Responsive styling
│
└── scripts/                       # Build tools
    └── gen_kconfig_sensor_pins.py # Auto-generate GPIO configs
```

## Code Statistics

- **Total Lines**: ~5,000+
- **Assembly Code**: 447 lines (ULP I²C/SPI)
- **Custom Components**: 2 (wmesh, thingsboard)
- **Main Application Files**: 45+ C/H files
- **Web Interface**: 3 files (HTML/CSS/JS)

## Development Team

- **Andrei Daniel Cazacu**
- **Gabriel Gutiérrez Fuentes**
- **Leonardo Chen Jin**
- **Santiago Marian Molina**

**Course**: Sistemas Basados en Computadoras 2025

## License

This project is developed for academic purposes.

## Acknowledgments

- **ESP-IDF**: Espressif IoT Development Framework
- **ThingsBoard**: Open-source IoT platform
- **Digilent**: Pmod sensor modules
- **mbedTLS**: Cryptography library

## Support

For issues, questions, or contributions, please open an issue on the [GitHub repository](https://github.com/leonidasdev/esp32-environmental-monitoring).

---

**Version**: 0.1.0  
**Last Updated**: December 2025
