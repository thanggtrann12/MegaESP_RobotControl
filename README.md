# MegaESP Robot Control

A modular PlatformIO firmware project for controlling a two-wheel robot with an Arduino Mega 2560 remote and robot controller, connected through ESP8266 ESP-NOW wireless nodes.

## Overview

The system is split into four firmware roles:

- **Remote Mega 2560** reads two joystick axes for driving, one joystick axis for auxiliary control, and a debounced button input.
- **Remote ESP8266 bridge** receives framed control packets over UART, echoes validated packets, and forwards them to the robot through ESP-NOW.
- **Robot ESP8266 receiver** receives ESP-NOW control packets and forwards validated packets to the robot Mega 2560 over UART.
- **Robot Mega 2560** applies differential-drive mixing and controls two TA6586 motor channels. A 500 ms communication timeout stops both motors.

ESP8266 nodes include non-blocking Wi-Fi reconnection and ArduinoOTA support for wireless firmware updates.

## Features

- Arduino Mega 2560 and ESP8266 targets managed with PlatformIO
- 50 Hz remote control packet transmission
- UART framing with `0xAA 0xFF` header, XOR checksum, and `0x55` terminator
- ESP-NOW wireless packet forwarding
- Joystick low-pass filtering and center deadzone
- Debounced button input
- Differential steering with motor speed limiting from `-255` to `255`
- Motor failsafe on lost control packets
- Non-blocking Wi-Fi and OTA handling on ESP8266

## Hardware Roles

| Target | Board | Purpose |
|---|---|---|
| `remote_mega2560` | Arduino Mega 2560 | Joystick/button input and UART packet source |
| `remote_ESP8266_bridge` | NodeMCU v2 / ESP8266 | UART-to-ESP-NOW bridge |
| `robot_ESP8266_receiver` | NodeMCU v2 / ESP8266 | ESP-NOW-to-UART receiver |
| Robot controller | Arduino Mega 2560 | TA6586 motor control and failsafe |

## Build and Upload

Install [PlatformIO](https://platformio.org/) and select an environment from `platformio.ini`:

```bash
pio run -e remote_mega2560
pio run -e remote_ESP8266_bridge
pio run -e robot_ESP8266_receiver
```

Upload a selected target with:

```bash
pio run -e <environment> -t upload
```

The ESP8266 environments are configured for OTA upload. Update the Wi-Fi credentials and OTA/network settings in the corresponding source files before deployment.

## Project Structure

```text
include/  Shared types, pin configuration, drivers, and communication interfaces
lib/      Reusable libraries, including GenericOTA
src/      Firmware implementations for the remote, bridge, receiver, and robot
platformio.ini  PlatformIO environments and build filters
```

## Communication Flow

```text
Joysticks and buttons
        |
        v
Remote Mega 2560 --UART--> Remote ESP8266 --ESP-NOW--> Robot ESP8266 --UART--> Robot Mega 2560 --PWM--> TA6586 motors
```

## Current Scope

The current robot control path implements differential drive using joystick 1. Servo and auxiliary actuator interfaces are reserved in the pin configuration and packet structure for future expansion.
