Development, Testing & Roadmap
==============================

Reading the Code
----------------

Start at ``src/Robot_Mega2560/Robot_Mega2560.cpp``. ``setup()`` and ``loop()`` connect three objects:

1. ``Hmi`` reads the touchscreen and calls ``Robot``.
2. ``ComManager`` reads the remote and calls ``Robot::onRemote()``.
3. ``Robot::update()`` runs every loop, applies the current command to ``Drive`` every 20 ms and enforces the 500 ms watchdog.

Everything below ``Robot`` is in ``lib/`` and can be read one folder at a time.

Staged Testing Strategy
-----------------------

1. **Motor Test:** run each motor with ``RUN`` from the touchscreen and set its reverse flag.
2. **Chassis Test:** use ``JOY`` in manual mode with the wheels lifted and check every wheel direction.
3. **Wireless Test:** validate ESP-NOW packet sequence numbers and CRC checks.
4. **Integration Test:** full test from the remote to physical movement.

Extending the Firmware
----------------------

**New motor driver**
   Implement ``IMotor`` (``run``, ``brake``, ``stop``) in ``lib/Motor``, then create the motors in ``MotorBank`` or in a new bank.

**New chassis**
   Add a value to ``Chassis`` and a row of six ``Mix`` entries to the preset table in ``lib/Drive/Drive.cpp``. Existing chassis keep working, and the custom chassis needs no code.

**New TJC command**
   Add one ``else if`` branch to ``Hmi::handle()`` and one line to the table in :doc:`communication`.

**New configuration field**
   Add it to ``RobotConfig``, check it in ``isValid()``, and increase ``ROBOT_CONFIG_VERSION`` so old EEPROM data is replaced by defaults.

Current Status & Roadmap
------------------------

* **Done:** layered firmware (PCA9685, motor, drive, config, HMI), six chassis types, motor mapping and reverse flags, EEPROM configuration, ESP-NOW remote, link watchdog, servos and I/O from the touchscreen.
* **Next:** update the TJC project to the command set in :doc:`communication`, telemetry from the Mega to the screen, and a hardware emergency stop.

PlatformIO Environment Layout
-----------------------------

Each environment selects exactly one application folder under ``src/`` with ``build_src_filter``:

* ``src/Remote_Mega2560``: handheld Mega application.
* ``src/Remote_ESP8266_Bridge``: handheld ESP-NOW bridge.
* ``src/Robot_ESP8266_Receiver``: robot-side ESP-NOW receiver.
* ``src/Robot_Mega2560``: robot controller.

Reusable components live in ``lib/``. PlatformIO finds them from the ``#include`` lines, so an environment only compiles the libraries it uses. Do not add per-environment ``src_dir``: the installed PlatformIO ignores it. To add a firmware target, create one folder under ``src/`` and one matching ``build_src_filter`` block in ``platformio.ini``.

Building the Documentation
--------------------------

.. code-block:: text

   sphinx-build -b html docs/source docs/_build/html

To refresh the Vietnamese translation after editing the English pages, run ``sphinx-build -b gettext docs/source docs/_build/gettext`` and then ``sphinx-intl update -p docs/_build/gettext -l vi -d docs/locales``.
