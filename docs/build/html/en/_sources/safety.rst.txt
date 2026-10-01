Safety & Failsafe Mechanisms
============================

Safety State Machine
--------------------

.. code-block:: text

   POWER ON -> INIT -> SAFE STOP -> READY -> RUNNING
                                              |   |
                          Communication Timeout   Emergency Stop
                                              |   |
                                              v   v
                                            SAFE STOP

Core Safety Principle
----------------------
**If the controller cannot verify that movement is safe, all motor outputs remain at zero.**

Emergency Stop Path
-------------------

1. Emergency Stop triggered (Hardware Button, HMI request, or ESP-NOW timeout).
2. ``SafetyManager`` intercepts state.
3. Call ``MotorController.stopAll()``.
4. All PWM outputs driven to 0 immediately.