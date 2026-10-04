Safety & Failsafe Mechanisms
============================

Core Safety Principle
---------------------
**If the controller cannot verify that movement is safe, all motor outputs stay at zero.**

What Stops the Motors
---------------------

.. list-table::
   :widths: 35 65
   :header-rows: 1

   * - Situation
     - Behaviour
   * - No command for 500 ms
     - The drive stops and the pending command is cleared. This covers a lost remote and a silent touchscreen.
   * - Drive not ready
     - Some wheel the chassis needs has no motor. The motors never move.
   * - Chassis, mapping or custom mix changed
     - All motors stop before the change applies.
   * - Control source changed
     - All motors stop and the pending command is cleared.
   * - ``STOP`` command
     - All motors stop.
   * - Single-motor test
     - The motor stops 500 ms after the last ``RUN`` command.
   * - Remote restarted
     - After a link loss the sequence check restarts, so a remote that begins again at sequence 0 is accepted.

Limits
------

* The PWM limit caps every motor output, including the single-motor test.
* The acceleration step limits how fast a wheel can change speed.
* If a command would push a wheel past the limit, all wheels are scaled down together so the direction of travel is kept.

Control Source
--------------

Only one source drives the robot at a time. Remote packets are ignored in ``MANUAL`` and joystick commands from the touchscreen are refused in ``REMOTE``.

Not Implemented
---------------

The robot firmware has no emergency stop input. Power the motors through a switch you can reach.
