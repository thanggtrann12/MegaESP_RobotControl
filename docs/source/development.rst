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