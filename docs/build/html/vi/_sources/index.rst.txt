===================
Universal Robot Hub
===================

A modular, OOP-based PlatformIO firmware architecture for a configurable robot control system.

The system is built around an **Arduino Mega 2560 Master Hub**, integrated with a **TJC/Nextion HMI (480x320px)**, and connected to a custom handheld remote controller through **ESP8266 ESP-NOW** wireless bridges.

The project is designed as a **DIY, hobby, and educational robotics platform**. Its main purpose is to explore how a real robot controller can be structured using C++, hardware abstraction, configurable motor mapping, kinematics, communication protocols, and safety mechanisms — while keeping the overall system practical enough for a student to build and understand.

Design Inspiration
------------------
The project is inspired by the general concept of modular robot-control platforms such as the **REV Robotics Expansion Hub**. It does not attempt to reproduce the REV architecture or implementation. Instead, it explores a smaller, student-oriented approach to building a configurable robot hub from commonly available hardware.

.. toctree::
   :maxdepth: 2
   :caption: System Overview & Architecture:

   architecture
   hardware
   communication

.. toctree::
   :maxdepth: 2
   :caption: Firmware & Configuration:

   configuration
   safety
   api

.. toctree::
   :maxdepth: 2
   :caption: Development & Testing:

   development

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`