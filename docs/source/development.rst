Development, Testing & Roadmap
==============================

Staged Testing Strategy
-----------------------

1. **Motor Test:** Test individual channels manually via HMI.
2. **Kinematics Test:** Verify math using simulated inputs without physical movement.
3. **Wireless Test:** Validate ESP-NOW packet sequence numbers and CRC checks.
4. **Integration Test:** Execute full end-to-end testing from remote control to physical movement.

Current Status & Roadmap
------------------------

* **Phase 1 — Core Motor System:** Arduino Mega 2560 motor control & basic abstraction.
* **Phase 2 — HMI:** TJC display interface, manual testing, simulation mode.
* **Phase 3 — Configurable Robot:** EEPROM storage, motor role mapping, inversion settings.
* **Phase 4 — Wireless Remote:** ESP-NOW communication link with timeout watchdog.
* **Phase 5 — Multi-Kinematics:** Runtime switching between Differential, Mecanum, and Omni drive models.

PlatformIO Environment Layout
-----------------------------

Each environment selects exactly one application folder under ``src/`` with
``build_src_filter``:

* ``src/Remote_Mega2560`` — handheld Mega application.
* ``src/Remote_ESP8266_Bridge`` — handheld ESP-NOW bridge.
* ``src/Robot_ESP8266_Receiver`` — robot-side ESP-NOW receiver.
* ``src/Robot_Mega2560`` — Universal Robot Hub runtime.

Reusable components stay under ``lib/``. The robot Mega environment explicitly
owns the ``GenericRobotController`` and ``GenericLogger`` source files because
these internal libraries are compiled as part of that firmware. They are
excluded from automatic library discovery there to avoid duplicate objects.
Other environments consume only the libraries they reference.

Do not use a per-environment ``src_dir`` in this project: the installed
PlatformIO version ignores that option. Add a new firmware target by creating
one application directory under ``src/`` and adding one matching
``build_src_filter`` block in ``platformio.ini``.