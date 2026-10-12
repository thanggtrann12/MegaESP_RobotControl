Communication Protocols & ESP-NOW
=================================

The Mega has two UART links. Both carry commands that end up in the same ``Robot`` object.

* **Serial2:** TJC touchscreen, ASCII text commands.
* **Serial3:** ESP8266 receiver, framed binary packets from the ESP-NOW remote.

TJC Protocol (Serial2)
----------------------

A command is ASCII text with comma separated fields. It ends with three ``0xFF`` bytes (what the TJC sends) or with a newline. Every reply ends with three ``0xFF`` bytes.

* Motor slots are numbered from 1 (M1 to M6).
* Wheel roles are numbered 0 to 6; see :doc:`configuration`.
* A command is answered with ``OK,<NAME>`` or ``ERR,<NAME>``, except where noted.

Configuration
~~~~~~~~~~~~~

.. list-table::
   :widths: 34 66
   :header-rows: 1

   * - Command
     - Description
   * - ``GET``
     - Reports the configuration (see below). No ``OK`` reply.
   * - ``CHASSIS,<0-5>``
     - Selects two-wheel, tank, omni-4, mecanum, six-wheel or custom. Stops the motors.
   * - ``MOTOR,<slot 1-6>,<role 0-6>,<0|1>``
     - Assigns a wheel role to a motor slot and sets its reverse flag. Role 0 frees the slot. Stops the motors.
   * - ``MIX,<role 1-6>,<t>,<s>,<r>``
     - Sets the custom chassis mix of one wheel, each value from -100 to 100. Stops the motors.
   * - ``PWM,<0-255>``
     - Sets the output limit. Applies immediately without stopping.
   * - ``ACCEL,<0-255>``
     - Sets the largest speed change per 20 ms. 0 turns it off.
   * - ``SAVE``
     - Writes the configuration to EEPROM.
   * - ``DEFAULT``
     - Restores the factory configuration in RAM. Use ``SAVE`` to keep it.

Control
~~~~~~~

.. list-table::
   :widths: 34 66
   :header-rows: 1

   * - Command
     - Description
   * - ``CTRL,<REMOTE|MANUAL>``
     - Chooses who drives the robot. Stops the motors.
   * - ``JOY,<t>,<s>,<r>``
     - Manual drive, each value from -100 to 100. Needs ``MANUAL``. Send it repeatedly; no reply when it succeeds, ``ERR,JOY`` otherwise.
   * - ``RUN,<slot 1-6>,<pwm>``
     - Runs one motor from -255 to 255, limited by the PWM limit and using its reverse flag. Needs ``MANUAL``. The motor stops 500 ms after the last ``RUN``.
   * - ``BRAKE,<slot 1-6>``
     - Brakes one motor. Needs ``MANUAL``.
   * - ``STOP``
     - Stops every motor.

Servos and I/O
~~~~~~~~~~~~~~

.. list-table::
   :widths: 34 66
   :header-rows: 1

   * - Command
     - Description
   * - ``SERVO,<channel 0-15>,<angle 0-180>``
     - Moves a servo on the second PCA9685.
   * - ``IO,<pin>,<IN|OUT|PULLUP>``
     - Configures a pin. Allowed pins: 2 to 13 and 22 to 69.
   * - ``IO,<pin>,W,<0|1>``
     - Writes a digital output.
   * - ``IO,<pin>,R``
     - Reads a digital pin. Reply: ``IO,<pin>,<0|1>``.
   * - ``AIN,<channel 0-15>``
     - Reads an analog input. Reply: ``AIN,<channel>,<value>``.

Messages from the Mega
~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :widths: 34 66
   :header-rows: 1

   * - Message
     - Description
   * - ``SYS,OK``
     - Heartbeat, sent every 500 ms.
   * - ``CFG,<chassis>,<pwm>,<accel>,<ready>,<source>``
     - First line of the ``GET`` reply. ``ready`` is 1 when every wheel the chassis needs has a motor. ``source`` is ``REMOTE`` or ``MANUAL``.
   * - ``MAP,<slot>,<role>,<reverse>``
     - One line per motor slot in the ``GET`` reply.
   * - ``MIX,<role>,<t>,<s>,<r>``
     - One line per wheel role in the ``GET`` reply.

Example: configure a two-wheel robot with the motors on M1 and M2.

.. code-block:: text

   CHASSIS,0
   MOTOR,1,1,0
   MOTOR,2,2,1
   SAVE

ESP-NOW Remote (Serial3)
------------------------

The remote sends its sticks over ESP-NOW at 2.4 GHz with an observed latency of approximately **~2.5 ms**. The ESP8266 receiver forwards each packet to the Mega in a frame:

.. code-block:: text

   0xAA 0xFF | ControlPacket (8 bytes) | XOR checksum | 0x55

``ControlPacket`` carries:

* Message type: control (0x01) or heartbeat (0x02)
* Throttle, strafe and rotation, each from -100 to 100
* Button bitmask
* Sequence number (rejects duplicate and out-of-order packets)
* CRC-8

The Mega accepts remote packets only while the source is ``REMOTE``. A heartbeat keeps the link alive and the last command running. At start-up the Mega sends ``ESP_RESET`` to the receiver.
