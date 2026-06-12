# RideCast Project

## Overview
RideCast is a portable communication platform designed to bridge data between a mobile device (phone/PC) and a vehicle cluster MCU/MPU. 
It supports **Wi-Fi**, **Bluetooth (BLE/Classic)**, and multiple serial interfaces (**UART, CAN, USB**) with optional encryption. The platform is modular and portable, inspired by Amazon FreeRTOS architecture, allowing easy migration across different MCUs.

Key features:  
- **High-speed data transfer** for maps, music, and notifications.  
- **Configurable encryption** for secure data streams (AES-GCM / TLS).  
- **Portable HAL**: abstracts MCU peripherals and wireless interfaces.  
- **Middleware**: handles routing, communication stacks, and security.  
- **Modular design**: easily swap hardware or protocols without changing application logic.  
- **Integrated testing**: unit and integration tests to validate functionality.

---


## System Architecture

```lua
+------------------------------------------------------+
|          APPLICATION CONFIGURATION LAYER             |
|------------------------------------------------------|
| Tasks | Queues | Semaphores | Mutex | Macros |      |
| Peripheral Configuration                             |
+------------------------------------------------------+
                         |
                         v
+------------------------------------------------------+
|                APPLICATION LAYER                     |
|------------------------------------------------------|
| Application Logic | State Machines | Control Flow   |
+------------------------------------------------------+
                         |
                         v
+------------------------------------------------------+
|             BSP CONFIGURATION LAYER                  |
|------------------------------------------------------|
| Tasks | Queues | Semaphores | Mutex | Macros        |
+------------------------------------------------------+
                         |
                         v
+------------------------------------------------------+
|         BSP (Board Support Package Layer)            |
|------------------------------------------------------|
| Internal Headers | Vendor-Independent API |          |
| Vendor-Dependent Port Layer                          |
+------------------------------------------------------+
                         |
                         v
+------------------------------------------------------+
|         HAL / VENDOR SDK (LOW-LEVEL LAYER)           |
|------------------------------------------------------|
| Register Access | Vendor Drivers | Startup Code      |
+------------------------------------------------------+
                         |
                         v
+------------------------------------------------------+
|                     HARDWARE                         |
|------------------------------------------------------|
| MCU | Peripherals | External Devices                 |
+------------------------------------------------------+
```


## Folder Structure

```bash
/ridecast
│
├── app/
│   ├── app_config/              # App-level config: tasks, queues, semaphores, mutex, macros, peripheral cfg
│   ├── include/                 # App public headers
│   ├── main/                    # Application entry point
│   └── src/                     # Application source files
│
├── bsp/
│   ├── bsp_config/              # BSP config: board/port/task/macro setup
│   ├── include/
│   │   ├── internal/            # BSP private/internal headers
│   │   └── public/              # Vendor-independent BSP APIs
│   ├── port/
│   │   ├── esp/                 # ESP32/ESP-IDF port implementations
│   │   ├── stm/                 # STM32 HAL port implementations (future)
│   │   └── ti/                  # TI MCU port implementations (future)
│   └── src/
│       ├── internal/            # BSP internal source files
│       ├── bsp_err_sts.c        # Error status string helpers
│       └── bsp_log.c            # Logger / debug output
│
├── nal/                         # Network Abstraction Layer
│   ├── include/                 # NAL interface headers
│   └── src/                     # NAL implementations
│
├── external/                    # Third-party libs and OS wrappers
│   ├── component/
│   │   ├── lwip/                # Lightweight TCP/IP stack (lwIP source code)
│   │   └── mbedtls/             # TLS/crypto library (mbedTLS source code)
│   ├── cli/
│   │   ├── embedded-cli/        # CLI library for command-line interface
│   │   └── freertos-cli/        # TODO- CLI library for command-line interface
│   └── os/
│       ├── cmsis/               # CMSIS headers and standard ARM abstraction
│       ├── cmsis_freertos/      # CMSIS-RTOS wrapper for FreeRTOS
│       └── cmsis_esp/           # CMSIS-RTOS adaptation layer for ESP-IDF
│
├── sdk/                         # Vendor SDK/HAL packages
│   ├── esp/
│   ├── stm/                     # future
│   └── ti/                      # future
│
├── tests/
│   ├── unit/                    # Unit tests
│   └── integration/             # End-to-end/system tests
│
├── docs/
│   ├── images/                  # Architecture diagrams and visuals
│   ├── doxygen/                 # Doxygen config/output
│   └── html/                    # Generated docs
│
├── cmake/                       # CMake modules/toolchain files
├── CMakeLists.txt               # Root build entry
└── README.md

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

## How RideCast Works (Flow Overview)

### Wireless Data Ingress

* Data arrives via Wi-Fi or Bluetooth
* TLS terminates at the NAL layer

### NAL Processing

* Protocol decoding
* Authentication / validation
* Routing decision
* Dispatcher
* Determines target serial interface (UART / CAN / USB)

### BSP Layer

* Abstracts hardware differences
* Ensures consistent behavior across MCUs

### Serial Transmission

* Optional encryption
* High-speed, non-blocking transfers

### Logging & Diagnostics

* All events captured via BSP logger
* Accessible through CLI or remote logging


Getting Started


## Getting Started

### Prerequisites

* Supported MCU (ESP32, STM32, TI, NXP)
* CMake (≥ 3.16)
* Python ≥ 3.8
* pip (Python package manager)
* Vendor SDK:
    * ESP-IDF
    * STM32 HAL
    * TI SDK
* Toolchain (GCC, ARM, etc.)
* FreeRTOS / Amazon FreeRTOS

### Python Environment Setup

RideCast uses a Python helper script (build.py) for build orchestration and utility commands.

#### Windows Setup

* Install Python
*   Download from: https://www.python.org/downloads/

* During installation, enable:

```
✔ Add Python to PATH
✔ Install pip

```

* Verify installation

```
python --version
pip --version

```

* If Python is installed but not in PATH
* Add these to Environment Variables → Path:

```
C:\Python39\
C:\Python39\Scripts\

@note (Adjust version/path as per installation)
```

* Install pip (if not already installed)
*   Download from: https://bootstrap.pypa.io/get-pip.py
*   Run: `python get-pip.py`


#### Linux Setup (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install -y python3 python3-pip

```

* Verify:

```bash
python3 --version
pip3 --version
```


* (Optional) Create aliases:

```bash
alias python=python3
alias pip=pip3
```

#### Python Dependencies

If a requirements.txt is present:

```bash
pip3 install -r requirements.txt
```

(or)

```bash
pip3 install -r requirements.txt
```

### Build Using Python Helper (Recommended)

RideCast provides a unified build script:

```bash
python3 build.py
```

View Supported Commands
Windows
```bash
python .\build.py -h
```

Linux
```bash
python3 ./build.py -h
```

This will list all supported build targets, platforms, and options.

## Documentation

* API documentation is generated using Doxygen
* Output available under:

```bash
/docs/html

```
* Architecture diagrams are stored in:

```bash
/docs/images

```


## Contributing

* For contributing, please follow the guidelines in the CONTRIBUTING.md file.

## License

RideCast is licensed under the Apache License 2.0.
See the LICENSE file for more information.
