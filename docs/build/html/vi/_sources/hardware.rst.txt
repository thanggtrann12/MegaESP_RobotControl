Hardware Layer & Pinouts
========================

Hardware Overview
-----------------
The Master Hub hardware setup consists of an **Arduino Mega 2560** connected to **6x TA6586 dual-H-bridge motor drivers** controlling DC brushed motors.

TA6586 Driver Motor Mapping (Arduino Mega 2560)
-----------------------------------------------

.. list-table::
   :widths: 20 20 30 30
   :header-rows: 1

   * - Channel
     - Hardware Timer
     - TA6586 IN1 (PWM 1)
     - TA6586 IN2 (PWM 2)
   * - Motor 0 (CH0)
     - Timer 3
     - Pin 2
     - Pin 3
   * - Motor 1 (CH1)
     - Timer 0 / 3
     - Pin 4
     - Pin 5
   * - Motor 2 (CH2)
     - Timer 4
     - Pin 6
     - Pin 7
   * - Motor 3 (CH3)
     - Timer 2 / 4
     - Pin 8
     - Pin 9
   * - Motor 4 (CH4)
     - Timer 2
     - Pin 10
     - Pin 11
   * - Motor 5 (CH5)
     - Timer 1
     - Pin 12
     - Pin 13

Handheld Remote Pinout
----------------------

* **Left Joystick (X/Y):** ``A0`` / ``A1`` ($V_x$ Lateral / $V_y$ Forward)
* **Right Joystick (X/Y):** ``A2`` / ``A3`` ($\Omega_z$ Yaw Rotation / Auxiliary)
* **Emergency Stop Switch:** Pin 21 (External Interrupt 0)
* **Mode Selector Switches:** Pins 22, 23, 24