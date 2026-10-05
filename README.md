# Universal Robot Hub

Một hệ thống C++ Firmware kiến trúc phân tầng (Layered Architecture), thiết kế theo nguyên lý **SOLID** và chuẩn **AUTOSAR-lite** dành cho Robot đa năng (Universal Robot Hub).

![Build Status](https://img.shields.io/badge/build-passing-brightgreen)
![Platform](https://img.shields.io/badge/platform-Arduino%20Mega%202560%20%7C%20ESP32%20%7C%20ESP8266-blue)
![C++ Standard](https://img.shields.io/badge/c%2B%2B-11%20%2F%2014-orange)
![License](https://img.shields.io/badge/license-MIT-green)

[Features](#features) • [Architecture](#architecture) • [Hardware & PCB](#hardware--pcb-design) • [Quick Start](#quick-start) • [Documentation](#documentation)

---

## Features

- **Decoupled Kinematics Engine:** Hỗ trợ linh hoạt các cơ cấu động học (`DifferentialKinematics`, `MecanumKinematics`, `OmniKinematics`) thông qua giao diện `IKinematics` thuần toán học.
- **Dynamic Motor Binding (`MotorRole`):** Ánh xạ vai trò bánh xe (`FRONT_LEFT`, `REAR_RIGHT`,...) linh hoạt tới các Slot phần cứng mà không phụ thuộc vào thứ tự cắm dây hay bitmask.
- **Event-Driven TJC HMI Service:** Giao tiếp với màn hình TJC/Nextion qua chuẩn mã hóa ASCII (32-127), hỗ trợ gọi Callback nội bộ trực tiếp trên màn hình.
- **Transport Abstraction (`ITransport`):** Tách biệt tầng truyền dẫn (`UARTTransport`, `ESPNowTransport`) khỏi tầng nghiệp vụ xử lý lệnh (`HMIService`, `RemoteControlService`).
- **EEPROM Storage & Persistence:** Lưu trữ cấu hình cài đặt, gán role motor và chế độ lái mượt mà qua các lần khởi động.
- **Safety Watchdog & Failsafe:** Tự động cắt PWM dừng toàn bộ động cơ khi mất tín hiệu điều khiển quá ngưỡng timeout.

---

## Architecture

Hệ thống được thiết kế phân tầng nghiêm ngặt nhằm đảm bảo tính mở rộng và dễ bảo trì (Separation of Concerns):

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
