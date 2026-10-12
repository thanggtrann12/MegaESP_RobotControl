System Architecture & Design Philosophy
=======================================

System Overview
---------------
The firmware turns an ESP32-S3 into a *plug, configure, play* robot controller. One firmware drives many chassis types; the TJC touchscreen chooses the chassis, maps motors to wheels, calibrates directions and saves the result to EEPROM. No rebuild is needed to change the robot.

The ESP32-S3:

* Drives two-wheel, tank, 4-wheel omni, mecanum, 6-wheel and custom chassis
* Runs six DC motors (M1 to M6) and up to sixteen servos through PCA9685 chips
* Exposes digital and analog pins as plain inputs or outputs
* Takes commands from the TJC touchscreen over UART and from the ESP-NOW remote directly
* Stops all motors when the command link is lost
* Stores the robot configuration in EEPROM

Layers
------

.. code-block:: text

   TJC touchscreen  <-- UART2 -->  Hmi            parse commands, answer, report configuration
                                    |
                                  Robot           choose control source, watchdog, own the configuration
                                    |
                                  Drive           chassis mixing, speed scaling, output limits
                                    |
                                  MotorBank       M1..M6: run (PWM), brake, stop
                                    |
                                  Pca9685         I2C PWM chip (0x40) + TA6586 motor drivers

   ESP-NOW remote   -- ESP-NOW --> EspNowLink      validate MAC, length, type and CRC; queue packets
                                    |
                                  Robot           sequence check, source selection and watchdog
   Servos                          ServoOut        PCA9685 chip 2 (0x41)
   Digital / analog pins           IoPin           in / out only

Each layer only knows the layer directly below it. The motor layer does not know what a wheel is, and ``Drive`` does not know which chip moves a motor.

Where the code lives
--------------------

.. list-table::
   :widths: 25 75
   :header-rows: 1

   * - Folder
     - Responsibility
   * - ``components/CDD_Pca9685``
     - I2C PWM chip: set one channel duty cycle.
   * - ``components/CDD_MotorTA6586``
     - Motor interface, TA6586 driver and six-motor bank.
   * - ``components/CDD_Pca9685``
     - Servo output on a PCA9685 channel.
   * - ``components/CDD_Gpio``
     - Configure a pin as input or output, read and write it.
   * - ``components/SWC_Drive``
     - The six chassis presets, wheel mixing, PWM limit and acceleration limit.
   * - ``components/SWC_Robot``
     - Robot control, configuration validation and EEPROM load and save.
   * - ``components/SRV_Hmi``
     - TJC command protocol and handlers.
   * - ``components/SRV_EspNow``
     - ESP-NOW receive validation and packet queue.
   * - ``app/Hub_Main.cpp``
     - Composition root and ``setup()`` / ``loop()`` for the ESP32-S3 hub.

Control Pipeline
----------------

.. image:: images/control_pipeline.svg
   :width: 100%
   :alt: Complete Control Pipeline

System Architecture & Wireless Data Flow
----------------------------------------

.. image:: images/system_architecture.svg
   :width: 100%
   :alt: System Architecture & Wireless Data Flow

Design Philosophy
-----------------

1. **Simple:**
    A newcomer should be able to read ``app/Hub_Main.cpp`` and follow the whole system in a few minutes.

2. **Layered:**
   Motors only run, brake or stop. Servos only move to an angle. Pins are only inputs or outputs. Everything else is built on top.

3. **Configurable:**
   The touchscreen decides the chassis, the motor mapping and the limits. These are data in EEPROM, not code.

4. **Safe by default:**
   If the chassis is not fully mapped, or the command link is silent, the motors stay stopped.
