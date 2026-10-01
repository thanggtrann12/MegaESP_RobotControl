Communication Protocols & ESP-NOW
=================================

Serial Protocol (ASCII Line-Based)
----------------------------------

Communication between TJC Screen / Remote Bridge and Arduino Mega uses structured ASCII strings.

TJC/Remote -> Robot Mega
~~~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :widths: 35 65
   :header-rows: 1

   * - Command Format
     - Action Description
   * - ``CMD_JOY:<Vx>,<Vy>,<Wz>``
     - Passes velocity vector inputs normalized within $[-1.0, 1.0]$.
   * - ``CMD_MODE:<0-4>``
     - Switches kinematic mode profile at runtime.
   * - ``CMD_MAP:<ID>,<ROLE>``
     - Maps physical motor channel ``<ID>`` to a logical role (e.g., ``CMD_MAP:0,FL``).
   * - ``CMD_ESTOP:1``
     - Triggers an immediate emergency stop.
   * - ``CMD_M:<ID>,<DIR>,<PWM>``
     - Manual press-to-run motor test command.

ESP-NOW Wireless Bridge
-----------------------

The wireless link operates over 2.4 GHz ESP-NOW with an observed latency of approximately **~2.5 ms**.

Packets include:

* Protocol Version
* Sequence Number (detects duplicate, out-of-order, or dropped frames)
* Payload Data (Velocity Vectors, Buttons, Mode Flags)
* CRC Verification