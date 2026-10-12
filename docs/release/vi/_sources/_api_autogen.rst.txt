API Reference (Manual Fallback)
================================

This page is generated because Doxygen XML is not available in this environment.
It provides a practical API overview extracted from the project headers.

Quick Links
-----------

- IMotor_
- MotorTA6586_
- MotorBank_
- Pca9685_
- ServoOut_
- IoPin_
- Drive_
- RobotConfig_
- Robot_
- Hmi_
- ComManager_
- Swc_Button_
- Swc_Joystick_
- ControlPacket_
- PairPacket_

Motor Layer
-----------

IMotor
^^^^^^

- run(int16_t pwm): Run at a signed PWM from -255 to 255.
- brake(): Short the motor terminals.
- stop(): Release the motor.

MotorTA6586
^^^^^^^^^^^

- run(int16_t), brake(), stop(): Drive one TA6586 through two PCA9685 channels.

MotorBank
^^^^^^^^^

- begin(uint16_t pwmHz): Start the PCA9685 and stop all six motors.
- motor(uint8_t slot): Return the motor of slot 0 to 5.
- stopAll(): Release every motor.

Pca9685
^^^^^^^

- begin(uint16_t frequencyHz): Start the chip at a PWM frequency.
- setDuty(uint8_t channel, uint16_t ticks): Set one channel from 0 to 4095.

ServoOut
^^^^^^^^

- begin(): Start the servo PCA9685 at 50 Hz.
- write(uint8_t channel, uint8_t angle): Move a servo from 0 to 180 degrees.
- release(uint8_t channel): Stop driving a servo.

IoPin
^^^^^

- configure(uint8_t pin, Mode mode): Set a pin as IN, OUT or IN_PULLUP.
- write(uint8_t pin, bool high): Write a digital output.
- read(uint8_t pin): Read a digital input.
- readAnalog(uint8_t channel): Read an analog input.

Drive and Configuration
-----------------------

Drive
^^^^^

- assign(WheelRole, IMotor*, bool inverted): Connect a motor to a wheel role.
- clearMotors(): Disconnect every motor.
- setChassis(Chassis, const Mix* custom): Select a chassis preset or the custom table.
- setPwmLimit(uint8_t), setAccelStep(uint8_t): Set the output limits.
- ready() const: True when every wheel the chassis needs has a motor.
- move(int8_t throttle, int8_t strafe, int8_t rotation): Mix a command into wheel speeds.
- stop(): Release every motor.

RobotConfig
^^^^^^^^^^^

- defaults(): Return the factory configuration.
- isValid() const: Check version, ranges and duplicate roles.
- load(int address): Load from EEPROM; false when empty or corrupt.
- save(int address): Store with a CRC-8.

Robot
^^^^^

- begin(): Start motors, servos and load the configuration.
- update(): Run the 20 ms control tick and the link watchdog.
- onRemote(const ControlPacket&): Take a remote command.
- setSource(Source): Choose REMOTE or MANUAL control.
- joy(...), runMotor(...), brakeMotor(...), stop(): Manual control.
- setMotor(...), apply(), applyLimits(): Change the configuration.
- saveConfig(), resetConfig(): Store or restore the configuration.

Hmi
^^^

- update(): Read the touchscreen UART, run commands and send the heartbeat.

Communication
-------------

ComManager
^^^^^^^^^^

- Init(uint32 baudrate = 115200): Configure UART.
- SendPacket(const ControlPacket&): Send framed control packet.
- SendCommand(const char*): Send newline-terminated ASCII command.
- ReadPacket(ControlPacket&): Parse one valid framed packet.
- ReadLog(char* buffer, size_t bufferSize): Read one received log line.

Input Implementations
---------------------

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
