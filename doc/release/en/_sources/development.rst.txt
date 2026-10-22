Development, Testing & Roadmap
==============================

Reading the Code
----------------

Start at ``app/Hub_Main.cpp``. ``setup()`` and ``loop()`` initialize and update the hub services through the C-style facade of each component:

1. ``SRV_EspNow_Update()`` delivers queued remote packets to ``SWC_Robot_OnRemote()``; ``Robot`` validates the sequence and source.
2. ``SRV_Hmi_Update()`` reads touchscreen commands and drives ``Robot`` through ``SWC_Robot_Joy()``, ``SWC_Robot_RunMotor()`` and friends.
3. ``SWC_Robot_Update()`` applies drive commands on a 20 ms tick and stops after the 500 ms link timeout.

``subModuleInit()`` wires the hub in one place: ``Wire.begin()`` for the I2C bus, then ``CDD_Gpio_Begin()``, ``SWC_Robot_Begin()``, ``SRV_Hmi_Begin()`` and ``SRV_EspNow_Begin()``. Each facade owns its own hardware setup (the HMI UART and the WiFi/ESP-NOW start live inside their components), so ``Hub_Main.cpp`` contains no component classes or globals.

Reusable modules are under ``components/``. Public headers are in each component's ``api/`` directory, implementation files are in ``src/``, and board-specific settings are in ``cfg/s3_devkit/``.

Building the Firmware
---------------------

Install PlatformIO Core or the PlatformIO IDE extension, then run these commands from the repository root:

.. code-block:: console

   pio run -e hub
   pio run -e hub -t upload
   pio device monitor -e hub

Specify ``-e hub`` explicitly because the legacy PlatformIO configuration is also imported by the root project. The Mega 2560 and ESP8266 firmware is kept under ``legacy/`` and is not part of the hub build.

The hub build compiles ``app/`` and ``components/*/src/``. ``tools/pio_components.py`` adds each component's ``api/`` and selected ``cfg/s3_devkit/`` directories to the include path.

Staged Testing Strategy
-----------------------

1. **Motor Test:** run each motor with ``RUN`` from the touchscreen and set its reverse flag.
2. **Chassis Test:** use ``JOY`` in manual mode with the wheels lifted and check every wheel direction.
3. **Wireless Test:** verify the configured remote MAC and channel, CRC validation, sequence handling and link timeout.
4. **Integration Test:** full test from the remote to physical movement.

Extending the Firmware
----------------------

**New motor driver**
   Implement the motor API in the appropriate driver component, then connect it through ``CDD_MotorTA6586`` or the motor bank.

**New chassis**
   Update the chassis type and mixing logic in ``components/SWC_Drive``. Keep existing presets and custom-mix behavior covered by drive tests.

**New TJC command**
   Add the handler in ``components/SRV_Hmi`` and document the command in :doc:`communication`.

**New configuration field**
   Add it to ``RobotConfig`` in ``components/SWC_Robot`` and update its validation and persistence versioning.

Current Status & Roadmap
------------------------

* **Done:** layered firmware (PCA9685, motor, drive, config, HMI), six chassis types, motor mapping and reverse flags, EEPROM configuration, ESP-NOW remote, link watchdog, servos and I/O from the touchscreen.
* **Next:** update the TJC project to the command set in :doc:`communication`, add telemetry, and add a hardware emergency stop.

Building the Documentation
--------------------------

.. code-block:: text

   sphinx-build -b html doc/source doc/_build/html

.. note::

   To build both translated pages on Windows, run ``tools/build.bat local`` from the repository root. To refresh the Vietnamese translation catalog manually, run ``sphinx-build -b gettext doc/source doc/_build/gettext`` followed by ``sphinx-intl update -p doc/_build/gettext -l vi -d doc/locales``.
