API Reference (Manual Fallback)
================================

This page is generated because Doxygen XML is not available in this environment.
It provides a practical API overview extracted from the project headers.

.. contents:: Jump to API
    :local:
    :depth: 2

Quick Links
-----------

- IMotor_
- IKinematics_
- IMotorOutput_
- ComManager_
- GenericRobotController_
- MotorTA6586_
- Swc_Button_
- Swc_Joystick_
- ControlPacket_
- PairPacket_

Core Interfaces
---------------

IMotor
^^^^^^

- begin(): Initialize motor hardware.
- setSpeed(int16_t speed): Set signed speed command.
- stop(): Stop motor output.

IKinematics
^^^^^^^^^^^

- getMode() const: Return kinematics mode id.
- getWheelCount() const: Return wheel count.
- computeWheelSpeeds(...): Convert motion command to wheel speeds.

IMotorOutput
^^^^^^^^^^^^

- begin(): Initialize all motors.
- stop(): Stop all motors.
- getMotorCount() const: Return number of motors.
- getMotor(uint8_t motorId): Return motor by 1-based index.
- applyWheelSpeeds(...): Apply wheel speed array with output limit.

Communication
-------------

ComManager
^^^^^^^^^^

- Init(uint32 baudrate = 115200): Configure UART.
- SendPacket(const ControlPacket&): Send framed control packet.
- SendCommand(const char*): Send newline-terminated ASCII command.
- ReadPacket(ControlPacket&): Parse one valid framed packet.
- ReadLog(char* buffer, size_t bufferSize): Read one received log line.

Robot Control
-------------

GenericRobotController
^^^^^^^^^^^^^^^^^^^^^^

- begin(): Initialize motor output backend.
- handlePacket(const ControlPacket&): Process incoming control packet.
- update(): Run watchdog/manual/HMI control update loop.
- setKinematics(IKinematics&): Switch active kinematics strategy.
- setControlSource(ControlSource): Switch control source.
- setHmiMotion(int8_t, int8_t, int8_t): Set manual HMI motion command.
- setManualMotor(uint8_t motorId, int16_t speed): Directly command one motor.
- releaseManualMotor(uint8_t motorId): Release manual motor override.
- setPwmLimit(uint8_t limit): Update PWM saturation limit.

Motor and Input Implementations
-------------------------------

MotorTA6586
^^^^^^^^^^^

- begin(): Initialize PCA9685 and stop motor.
- setSpeed(int16_t): Drive forward/reverse via channel pair.
- stop(): Disable both direction channels.

Swc_Button
^^^^^^^^^^

- Init(): Configure button input pin.
- Update(): Debounce button state.
- GetState() const: Return debounced bitmask.

Swc_Joystick
^^^^^^^^^^^^

- Init(): Configure joystick analog pins.
- Update(): Sample and low-pass filter axes.
- GetProcessedValues(...): Return filtered + deadzone-adjusted values.

Shared Data Types
-----------------

ControlPacket
^^^^^^^^^^^^^

- msgType: CONTROL_MESSAGE or HEARTBEAT_MESSAGE.
- throttle/strafe/rotation: Signed command channels.
- buttons: Bitmask from remote buttons.
- sequenceNum: Monotonic sequence id.
- crc8: Packet CRC-8.

PairPacket
^^^^^^^^^^

- cmd: Pairing command id.
- macAddr[6]: Device MAC address.
