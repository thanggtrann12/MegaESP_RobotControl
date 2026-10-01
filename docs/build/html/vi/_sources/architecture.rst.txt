System Architecture & Design Philosophy
=======================================

System Overview
---------------
The system turns an Arduino Mega 2560 into a configurable robot-control hub capable of:

* Multiple drivetrain / kinematics modes
* Runtime kinematics mode switching
* Per-mode logical-to-physical motor mapping
* Manual motor testing
* Real-time I/O monitoring
* HMI-based simulation and testing
* ESP-NOW wireless remote control
* Communication watchdogs
* Emergency stop and motor failsafe handling
* Persistent robot configuration
* Modular OOP-based firmware architecture

The architecture is designed so that the robot-control logic does not need to be tightly coupled to a fixed number of motors or a single drivetrain.

System Architecture & Wireless Data Flow
----------------------------------------

.. image:: images/system_architecture.svg
   :width: 100%
   :alt: System Architecture & Wireless Data Flow

Complete Control Pipeline
-------------------------

.. image:: images/control_pipeline.svg
   :width: 100%
   :alt: Complete Control Pipeline

Design Philosophy
-----------------

1. **Simple:**
   The architecture should be modular without becoming unnecessarily complicated. Simple enough to understand, modular enough to extend, practical enough to run on a real robot.

2. **Modular:**
   Each component has a clear responsibility. The HMI does not directly drive motors; the kinematics engine does not know pin numbers; the motor controller does not care about drivetrain type.

3. **Configurable:**
   The software separates **Logical Robot Role** from **Physical Motor Channel**.

