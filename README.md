# RideCast Project

## Overview
RideCast is a portable communication platform designed to bridge data between a mobile device (phone/PC) and a vehicle cluster MCU/MPU. It supports **Wi-Fi**, **Bluetooth (BLE/Classic)**, and multiple serial interfaces (**UART, CAN, USB**) with optional encryption. The platform is modular and portable, inspired by Amazon FreeRTOS architecture, allowing easy migration across different MCUs.

Key features:  
- **High-speed data transfer** for maps, music, and notifications.  
- **Configurable encryption** for secure data streams (AES-GCM / TLS).  
- **Portable HAL**: abstracts MCU peripherals and wireless interfaces.  
- **Middleware**: handles routing, communication stacks, and security.  
- **Modular design**: easily swap hardware or protocols without changing application logic.  
- **Integrated testing**: unit and integration tests to validate functionality.

---

## Folder Structure

```bash
/ridecast
│
├──/app
│   │ └── dispatcher # Routes data between wireless and serial interfaces
├──/cmakes
│   │ └── build # Build artifacts
├──/docs
│   │ ├── html # Documentation HTML files
│   │ ├── images # Architecture diagrams, flowcharts
│   │ └── doxygen # doxygen documents
├──/platform
│   │ ├── bsp # Board Support Package (MCU & hardware abstraction)
│   │ │    ├── common_io # Generic IO drivers
│   │ │    ├── serial_com # UART, CAN, USB interfaces
│   │ │    └── wireless_com # Wi-Fi and BLE drivers
│   │ ├── middleware # Communication stacks and crypto
│   │ │    ├── comm # TCP/IP, BLE stacks
│   │ │    └── crypto # TLS/AES modules
│   │ └── OS # Optional: Vendor-supplied RTOS (CMSIS, FreeRTOS)
├──/tests
│   │ ├── integration # End-to-end and protocol flow tests
│   │ └── unit # Unit tests for individual modules
├── /vendors          # Vendor-specific SDKs or drivers
│   ├── esp           # Vendor SDKs for hardware peripherals (ESP-IDF, STM32 HAL,NXP SDK, etc.)

```

## Features
### Wireless Communication

* Wi-Fi: High-bandwidth data (maps, audio), supports TLS encryption.
* Bluetooth: Low-bandwidth notifications, audio streaming (BLE/Classic).
* Dynamic routing: Dispatcher module routes data to serial interfaces.

### Serial Communication

* UART / CAN / USB: Portable HAL API abstracts hardware differences.
* High-speed data transfer for cluster MCU.
* Optional encryption for secure transmission.

### Security

* AES-GCM encryption for serial data.
TLS for Wi-Fi communication.
Configurable via /platform/middleware/crypto and /config/security_config.

### Logging & Diagnostics

* Capture metrics, logs, and errors for long-run testing.
* Supports remote logging and OTA updates.

-------------------------------------------------------------
## Getting Started

### Prerequisites
- Supported MCU (ESP32, STM32, NXP, etc.)
- CMake build system
- Vendor SDK (ESP-IDF, STM32 HAL, NXP SDK)
- FreeRTOS / Amazon FreeRTOS

### Build Instructions
```bash
# Navigate to project root
cd ridecast

# Create build directory
mkdir build && cd build

# Generate project files with CMake
cmake ..

# Build firmware
cmake --build .