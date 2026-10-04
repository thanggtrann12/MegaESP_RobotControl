Hardware Layer & Pinouts
========================

Hardware Overview
-----------------
The robot controller is an **Arduino Mega 2560** that talks to its outputs over I2C. Two **PCA9685** PWM chips generate all motor and servo signals; the Mega itself only provides the I2C bus, two UARTs and plain I/O pins.

* **PCA9685 at 0x40:** drives six **TA6586** motor drivers at 1 kHz.
* **PCA9685 at 0x41:** drives servos at 50 Hz.

One PCA9685 has a single PWM frequency, so motors and servos need separate chips.

Motor Slots
-----------

Each motor slot uses two adjacent PCA9685 channels connected to the two inputs of one TA6586.

.. list-table::
   :widths: 20 30 25 25
   :header-rows: 1

   * - Slot
     - Name in firmware
     - Reverse input (BI)
     - Forward input (FI)
   * - M1
     - slot 0
     - Channel 0
     - Channel 1
   * - M2
     - slot 1
     - Channel 2
     - Channel 3
   * - M3
     - slot 2
     - Channel 4
     - Channel 5
   * - M4
     - slot 3
     - Channel 6
     - Channel 7
   * - M5
     - slot 4
     - Channel 8
     - Channel 9
   * - M6
     - slot 5
     - Channel 10
     - Channel 11

The touchscreen and the protocol number motors from 1 (M1 to M6). Which wheel a motor drives is configured, not wired; see :doc:`configuration`.

Servos
------

Servos use channels 0 to 15 of the PCA9685 at address 0x41. A pulse of 0.5 ms to 2.5 ms maps to 0 to 180 degrees.

Mega 2560 Connections
---------------------

.. list-table::
   :widths: 30 30 40
   :header-rows: 1

   * - Function
     - Pins
     - Notes
   * - I2C (PCA9685 chips)
     - 20 (SDA), 21 (SCL)
     - Bus runs at 400 kHz.
   * - TJC touchscreen
     - Serial2: 16 (TX2), 17 (RX2)
     - 115200 baud.
   * - ESP8266 receiver
     - Serial3: 14 (TX3), 15 (RX3)
     - 115200 baud.
   * - General I/O
     - 2 to 13 and 22 to 69
     - Configured as input or output from the touchscreen. Pins 0 to 1 (USB) and 14 to 21 (UARTs and I2C) are refused.

Handheld Remote Pinout
----------------------

* **Left Joystick (X/Y):** ``A0`` / ``A1`` ($V_x$ Lateral / $V_y$ Forward)
* **Right Joystick (X/Y):** ``A2`` / ``A3`` ($\Omega_z$ Yaw Rotation / Auxiliary)
* **Emergency Stop Switch:** Pin 21 (External Interrupt 0)
* **Mode Selector Switches:** Pins 22, 23, 24
