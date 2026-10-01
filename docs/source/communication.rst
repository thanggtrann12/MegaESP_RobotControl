Communication Protocols & ESP-NOW
=================================

Serial Protocol (ASCII Line-Based)
----------------------------------

Communication between TJC Screen / Remote Bridge and Arduino Mega uses structured ASCII strings.

TJC <-> Robot Mega Command Table
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Commands are ASCII strings terminated by three ``0xFF`` bytes. The Mega
answers with an ASCII response using the same terminator unless stated
otherwise.

.. list-table::
   :widths: 24 32 28 36
   :header-rows: 1

   * - Direction
     - TJC command
     - Mega response
     - Meaning and conditions
   * - TJC -> Mega
     - ``CMD_CAP:GET``
     - ``CAP:KIN,<5 flags>,DRV,<flag>``
     - Returns compiled kinematics and driver capabilities.
   * - TJC -> Mega
     - ``CMD_CTRL:MANUAL``
     - ``CMD_CTRL:OK,MANUAL``
     - Selects HMI manual control source.
   * - TJC -> Mega
     - ``CMD_CTRL:REMOTE``
     - ``CMD_CTRL:OK,REMOTE``
     - Selects ESP-NOW remote control source.
   * - TJC -> Mega
     - ``CMD_CFG:BEGIN``
     - ``CMD_CFG:OK``
     - Copies active config to staged config.
   * - TJC -> Mega
     - ``CMD_CFG:MODE,<0-4>``
     - ``CMD_CFG:OK`` or ``CMD_CFG:ERR,VALUE``
     - Stages kinematics mode; it does not change runtime until ``APPLY``.
   * - TJC -> Mega
     - ``CMD_CFG:PWM,<0-255>``
     - ``CMD_CFG:OK`` or ``CMD_CFG:ERR,VALUE``
     - Stages the PWM limit.
   * - TJC -> Mega
     - ``CMD_CFG:PROFILE,<0|1>``
     - ``CMD_CFG:OK`` or ``CMD_CFG:ERR,VALUE``
     - Stages ``DIRECT`` or ``LIMITED_ACCELERATION``.
   * - TJC -> Mega
     - ``CMD_CFG:VALIDATE``
     - ``CMD_CFG:OK`` or ``CMD_CFG:ERR``
     - Validates capability, topology, roles, version and CRC.
   * - TJC -> Mega
     - ``CMD_CFG:APPLY``
     - ``CMD_CFG:OK`` or ``CMD_CFG:ERR``
     - Safely applies the staged configuration and stops motors during switch.
   * - TJC -> Mega
     - ``CMD_CFG:ABORT``
     - ``CMD_CFG:OK``
     - Discards staged changes.
   * - TJC -> Mega
     - ``CMD_CFG:SAVE``
     - ``CMD_CFG:OK`` or ``CMD_CFG:ERR``
     - Saves the active config to EEPROM when it changed and the write budget allows it.
   * - TJC -> Mega
     - ``CMD_MODE:<0-4>``
     - ``CMD_MODE:OK`` or ``CMD_MODE:ERR,UNSUPPORTED``
     - Legacy compatibility command; internally stages and applies the mode.
   * - TJC -> Mega
     - ``CMD_JOY:<throttle>,<strafe>,<rotation>``
     - Error response only on failure.
     - HMI motion in ``[-100,100]``; requires manual control source.
   * - TJC -> Mega
     - ``CMD_M:<id>,<FWD|REV|STOP>,<pwm>``
     - ``CMD_M:OK,...`` or ``CMD_M:ERR,...``
     - Manual motor test; requires manual control source.
   * - TJC -> Mega
     - ``CMD_LIMIT:PWM,<0-255>``
     - ``CMD_LIMIT:OK`` or ``CMD_LIMIT:ERR,VALUE``
     - Changes active controller PWM limit for the current runtime.
   * - TJC -> Mega
     - ``CMD_SIM:TOGGLE_IO,<pin>``
     - ``IO_STATUS:<pin>,...`` or ``CMD_SIM:ERR,...``
     - Toggles simulation state for a configured I/O pin.
   * - TJC -> Mega
     - ``CMD_SIM:RESET_IO,<pin>``
     - ``IO_STATUS:<pin>,...`` or ``CMD_SIM:ERR,...``
     - Restores a configured I/O pin to real state.
   * - Mega -> TJC
     - Periodic heartbeat
     - ``CMD_SYS:OK``
     - Sent every 500 ms while HMI service is running.
   * - Mega -> TJC
     - Invalid command
     - ``CMD_ERR:UNKNOWN_COMMAND``
     - Command was not recognized.

Slot-based configuration commands
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The protocol uses 1-based slots. Firmware converts them to internal indexes
0 through 5.

.. csv-table::
  :header: "Command", "Response", "Meaning"
  :widths: 40, 25, 35

  "``CMD_CFG:MOTOR,<slot 1-6>,<role>``", "``CMD_CFG:OK`` / ``ERR``", "Stage physical slot role mapping."
  "``CMD_CFG:INVERT,<slot 1-6>,<0|1>``", "``CMD_CFG:OK`` / ``ERR``", "Stage motor direction inversion."
  "``CMD_MAP:<slot 1-6>,<role>``", "``CMD_MAP:OK`` / ``ERR``", "Legacy mapping command."

ESP-NOW Wireless Bridge
-----------------------

The wireless link operates over 2.4 GHz ESP-NOW with an observed latency of approximately **~2.5 ms**.

Packets include:

* Protocol Version
* Sequence Number (detects duplicate, out-of-order, or dropped frames)
* Payload Data (Velocity Vectors, Buttons, Mode Flags)
* CRC Verification