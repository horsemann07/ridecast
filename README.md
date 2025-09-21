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
├── /app
│   └── main              # Application entry point (routes data between wireless and serial interfaces)
│
├── /cmakes               # CMake scripts 
│
├── /docs
│   ├── html              # Generated documentation in HTML format
│   ├── images            # Architecture diagrams, flowcharts, design visuals
│   └── doxygen           # Doxygen configuration files and outputs
│
├── /bsp                  # Board Support Package (MCU-specific hardware abstraction)
│   ├── include           # Generic hardware abstraction headers (common IO driver APIs)
│   ├── port              # Hardware interface implementations (UART, CAN, USB, etc.)
│   │   ├── esp           # ESP32/ESP-IDF specific drivers (UART, CAN, Wi-Fi, etc.)
│   │   ├── stm           # STM32 HAL-based implementations (UART, CAN, timers, etc.)
│   │   └── ti            # TI MCU driver implementations
│   ├── system
│       ├── config        # System-level configuration (clocks, pins, interrupts, etc.)
│       └── cli_logger    # Command-line interface logger for debugging and monitoring
│
├── /middleware           # Protocol stacks, cryptography, and middleware services
│
├── /external             # Third-party libraries and OS wrappers
│   ├── component
│   │   ├── lwip          # Lightweight TCP/IP stack (lwIP source code)
│   │   └── mbedtls       # TLS/crypto library (mbedTLS source code)
│   └── os
│       ├── cmsis         # CMSIS headers and standard ARM abstraction
│       ├── cmsis_freertos # CMSIS-RTOS wrapper for FreeRTOS
│       └── cmsis_esp     # CMSIS-RTOS adaptation layer for ESP-IDF
│
├── /tests
│   ├── integration       # End-to-end, system-level, and protocol flow tests
│   └── unit              # Unit tests for individual modules/components
│
├── /sdk                  # Vendor-specific SDKs and HAL drivers
│   ├── esp               # ESP-IDF SDK and tools
│   ├── stm               # STM32 HAL and CubeMX-generated drivers (future)
│   └── ti                # TI SDK or driver support (future)


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

```

## License
RideCast is a proprietary project. Unauthorized copying, distribution, or modification is strictly prohibited. 
For licensing inquiries, contact Navyantra.
