Chassis & Motor Configuration
=============================

Concepts
--------

* **Motor slot:** a physical motor port, M1 to M6.
* **Wheel role:** where a motor sits on the chassis. A slot is assigned one role, or none.
* **Chassis:** a mixing table that says how much of throttle, strafe and rotation each wheel receives.

Changing the chassis or the mapping never needs new firmware.

Wheel Roles
-----------

.. list-table::
   :widths: 15 85
   :header-rows: 1

   * - Role
     - Wheel
   * - 0
     - Unused
   * - 1
     - Front left
   * - 2
     - Front right
   * - 3
     - Middle left
   * - 4
     - Middle right
   * - 5
     - Rear left
   * - 6
     - Rear right

Assigning a role to a slot takes it away from any other slot that had it, so each wheel has exactly one motor.

Chassis Types
-------------

.. list-table::
   :widths: 10 25 25 40
   :header-rows: 1

   * - Id
     - Chassis
     - Required roles
     - Mixing
   * - 0
     - Two-wheel
     - 1, 2
     - Differential: left = throttle + rotation, right = throttle - rotation.
   * - 1
     - Tank
     - 1, 2, 5, 6
     - Four-wheel skid steer, same mixing as two-wheel on each side.
   * - 2
     - 4-wheel omni
     - 1, 2, 5, 6
     - X layout; mixes exactly like mecanum.
   * - 3
     - Mecanum
     - 1, 2, 5, 6
     - Throttle, strafe and rotation combined per wheel.
   * - 4
     - 6-wheel
     - 1 to 6
     - Six-wheel skid steer, same mixing on each side.
   * - 5
     - Custom
     - Every role with a non-zero mix
     - Mixing table entered from the touchscreen.

Positive rotation turns right. A wheel at full command receives 255 PWM.

Custom Chassis
--------------

For each wheel role the table stores three percentages from -100 to 100: throttle, strafe and rotation. With the commands T, S and R also in the range -100 to 100, the wheel speed in percent of full PWM is:

.. code-block:: text

   speed = (throttle * T + strafe * S + rotation * R) / 100

A role whose three values are all zero is not used. If a command would push any wheel above the PWM limit, all wheels are scaled down together so the direction of travel is kept.

Calibration
-----------

1. Select the chassis.
2. Map each motor slot to its wheel role.
3. Switch to manual control and press the single-motor test for each slot. If a wheel turns the wrong way, set its reverse flag and test again. The test already applies the reverse flag.
4. Set the PWM limit and, if wanted, the acceleration step.
5. Save.

Settings
--------

.. list-table::
   :widths: 25 75
   :header-rows: 1

   * - Setting
     - Meaning
   * - PWM limit
     - Highest PWM sent to a motor, 0 to 255.
   * - Acceleration step
     - Largest change of one wheel speed per 20 ms control tick. 0 turns the limit off.
   * - Reverse flag
     - Reverses one motor so a wheel turns forward when told to.

Persistence
-----------

The whole configuration is one 30-byte structure with a version byte and a CRC-8, stored in EEPROM from address ``0x10``. At start-up the firmware loads it and checks:

1. The version matches this firmware.
2. The CRC is correct.
3. The chassis id and all roles are in range and no role is used twice.
4. The custom mix values are within -100 to 100.

If any check fails the defaults are used: mecanum, motors M1 to M4 as front left, rear left, front right and rear right, PWM limit 255, no acceleration limit.

Changes take effect immediately and are only written to EEPROM by the save command. Saving only rewrites bytes that changed.

Not Ready State
---------------

The drive is *ready* when every wheel the chassis needs has a motor. When it is not ready the motors stay stopped. The ``GET`` command reports the ready flag so the touchscreen can show what is missing.
