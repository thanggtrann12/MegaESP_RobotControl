# Universal Robot Hub

A layered-architecture C++ firmware for a general-purpose robot (Universal Robot Hub), designed around the **SOLID** principles and an **AUTOSAR-lite** style.

![Build Status](https://img.shields.io/badge/build-passing-brightgreen)
![Platform](https://img.shields.io/badge/platform-Arduino%20Mega%202560%20%7C%20ESP32%20%7C%20ESP8266-blue)
![C++ Standard](https://img.shields.io/badge/c%2B%2B-11%20%2F%2014-orange)
![License](https://img.shields.io/badge/license-MIT-green)

[Features](#features) • [Architecture](#architecture) • [Hardware & PCB](#hardware--pcb-design) • [Quick Start](#quick-start) • [Documentation](#documentation)

---

## Features

- **Decoupled Kinematics Engine:** Supports multiple drive mechanisms (`DifferentialKinematics`, `MecanumKinematics`, `OmniKinematics`) through the purely mathematical `IKinematics` interface.
- **Dynamic Motor Binding (`MotorRole`):** Maps wheel roles (`FRONT_LEFT`, `REAR_RIGHT`, ...) to hardware slots independently of wiring order or bitmasks.
- **Event-Driven TJC HMI Service:** Talks to TJC/Nextion displays using ASCII encoding (32-127) and supports invoking internal callbacks directly from the screen.
- **Transport Abstraction (`ITransport`):** Separates the transport layer (`UARTTransport`, `ESPNowTransport`) from the command-handling layer (`HMIService`, `RemoteControlService`).
- **EEPROM Storage & Persistence:** Keeps settings, motor role assignments and drive mode across reboots.
- **Safety Watchdog & Failsafe:** Automatically cuts PWM and stops all motors when the control signal is lost for longer than the timeout.

---

## Architecture

The system is strictly layered to keep it extensible and maintainable (Separation of Concerns):

```mermaid
graph TD
    subgraph App ["Application Layer"]
        A1["RobotController · MotorManager · Kinematics · SafetyManager"]
    end

    subgraph Service ["Service / Protocol Layer"]
        S1["HMIService · RemoteControlService · ConfigService"]
        S2["Command parsing, Protocol validation, Frame CRC & State Machine"]
    end

    subgraph Transport ["Transport Layer (ITransport)"]
        T1["UARTTransport · ESPNowTransport"]
        T2["Stream/Packet buffer management & Data flow handling"]
    end

    subgraph Hardware ["Hardware / Driver Layer"]
        H1["HardwareSerial · ESP-NOW API · TA6586 Driver · PCA9685 PWM"]
    end

    App <--> Service
    Service <--> Transport
    Transport <--> Hardware

    style App fill:#1e1e1e,stroke:#00acc1,stroke-width:2px,color:#fff
    style Service fill:#1e1e1e,stroke:#00acc1,stroke-width:2px,color:#fff
    style Transport fill:#1e1e1e,stroke:#00acc1,stroke-width:2px,color:#fff
    style Hardware fill:#1e1e1e,stroke:#00acc1,stroke-width:2px,color:#fff
```

---

## Hardware & PCB Design

- **Supported MCUs:** Arduino Mega 2560, ESP32, ESP8266.
- **Motor driver:** TA6586.
- **PWM controller:** PCA9685.
- **HMI display:** TJC / Nextion (UART).

> TODO: add schematic, PCB images and pinout table.

---

## Quick Start

### 1. Clone the repository

```bash
git clone https://github.com/YOUR_GITHUB_USERNAME/MegaESP_RobotControl.git
cd MegaESP_RobotControl
```

### 2. Build the firmware

> TODO: add build/flash instructions (Arduino IDE or PlatformIO).

### 3. Build the documentation (EN / VI)

Requirements: Python, `sphinx`, `sphinx-intl`, `potranslator`; Doxygen (optional, used for the API page).

```bat
:: Full build and package the release into docs\release
build.bat

:: Quick build, no release packaging
build.bat local
```

Output:

| Mode | Path |
|---|---|
| Local | `docs/_build/html/en/index.html`, `docs/_build/html/vi/index.html` |
| Release | `docs/release/en/index.html`, `docs/release/vi/index.html` |

---

## Documentation

- [English](https://htmlpreview.github.io/?https://github.com/thanggtrann12/MegaESP_RobotControl/blob/main/docs/release/en/index.html)
- [Tiếng Việt (Vietnamese)](https://htmlpreview.github.io/?https://github.com/thanggtrann12/MegaESP_RobotControl/blob/main/docs/release/vi/index.html)


---

## License

MIT