# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information
import os
# Sửa tên project bỏ chữ Documentation thừa
project = 'Robot Hub'
copyright = '2026, ThanggTrann12'
author = 'ThanggTrann12'

# Định nghĩa chính xác tiêu đề hiển thị trên Sidebar
html_title = 'Robot Hub'
html_theme = 'furo'
html_sidebars = {
    "**": [
        "sidebar/brand.html",
        "language-switcher.html",  # Nhúng nút chọn ngôn ngữ vào đây
        "sidebar/search.html",
        "sidebar/scroll-start.html",
        "sidebar/navigation.html",
        "sidebar/ethical-ads.html",
        "sidebar/scroll-end.html",
    ]
}
# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.ifconfig',
    'breathe',
]

templates_path = ['_templates']
exclude_patterns = []

language = 'en'
locale_dirs = ['../locales/']   # Thư mục chứa bản dịch
gettext_compact = False     # Tách file dịch theo từng trang .rst

html_static_path = ['_static']
html_css_files = [
    'custom.css',
]
# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_static_path = ['_static', 'images']

breathe_projects = {
    "RobotHub": os.path.abspath("../_build/doxygen/xml")
}
breathe_default_project = "RobotHub"

DOXYGEN_INDEX = os.path.abspath("../_build/doxygen/xml/index.xml")
has_doxygen_xml = os.path.exists(DOXYGEN_INDEX)

API_AUTOGEN_PATH = os.path.join(os.path.dirname(__file__), "_api_autogen.rst")
if has_doxygen_xml:
     api_autogen = """API Index
---------

.. doxygenindex::
    :project: RobotHub

Motor Layer
-----------

.. doxygenclass:: IMotor
    :project: RobotHub
    :members:

.. doxygenclass:: MotorTA6586
    :project: RobotHub
    :members:

.. doxygenclass:: MotorBank
    :project: RobotHub
    :members:

.. doxygenclass:: Pca9685
    :project: RobotHub
    :members:

.. doxygenclass:: ServoOut
    :project: RobotHub
    :members:

.. doxygennamespace:: IoPin
    :project: RobotHub
    :members:

Drive and Configuration
-----------------------

.. doxygenclass:: Drive
    :project: RobotHub
    :members:

.. doxygenstruct:: RobotConfig
    :project: RobotHub
    :members:

.. doxygenclass:: Robot
    :project: RobotHub
    :members:

.. doxygenclass:: Hmi
    :project: RobotHub
    :members:

Component Facades
-----------------

.. doxygenfunction:: CDD_Gpio_Begin
    :project: RobotHub

.. doxygenfunction:: CDD_Gpio_ReadPin
    :project: RobotHub

.. doxygenfunction:: CDD_Gpio_WritePin
    :project: RobotHub

.. doxygenfunction:: CDD_Gpio_SetPinMode
    :project: RobotHub

.. doxygenfunction:: CDD_Motor_Begin
    :project: RobotHub

.. doxygenfunction:: CDD_Motor_Run
    :project: RobotHub

.. doxygenfunction:: CDD_Motor_Brake
    :project: RobotHub

.. doxygenfunction:: CDD_Motor_Stop
    :project: RobotHub

.. doxygenfunction:: CDD_Motor_StopAll
    :project: RobotHub

.. doxygenfunction:: CDD_Servo_Begin
    :project: RobotHub

.. doxygenfunction:: CDD_Servo_Write
    :project: RobotHub

.. doxygenfunction:: CDD_Servo_Release
    :project: RobotHub

.. doxygenfunction:: CDD_Servo_ReleaseAll
    :project: RobotHub

.. doxygenfunction:: SWC_Robot_Begin
    :project: RobotHub

.. doxygenfunction:: SWC_Robot_Update
    :project: RobotHub

.. doxygenfunction:: SWC_Robot_OnRemote
    :project: RobotHub

.. doxygenfunction:: SWC_Robot_SetSource
    :project: RobotHub

.. doxygenfunction:: SWC_Robot_Joy
    :project: RobotHub

.. doxygenfunction:: SWC_Robot_RunMotor
    :project: RobotHub

.. doxygenfunction:: SWC_Robot_BrakeMotor
    :project: RobotHub

.. doxygenfunction:: SWC_Robot_Stop
    :project: RobotHub

.. doxygenfunction:: SWC_Robot_IsReady
    :project: RobotHub

.. doxygenfunction:: SWC_Robot_IsMoving
    :project: RobotHub

.. doxygenfunction:: SWC_Robot_IsLinkAlive
    :project: RobotHub

.. doxygenfunction:: SRV_Hmi_Begin
    :project: RobotHub

.. doxygenfunction:: SRV_Hmi_Update
    :project: RobotHub

.. doxygenfunction:: SRV_EspNow_Begin
    :project: RobotHub

.. doxygenfunction:: SRV_EspNow_Update
    :project: RobotHub

Communication
-------------

.. doxygenclass:: EspNowLink
    :project: RobotHub
    :members:

Shared Data Types
-----------------

.. doxygenstruct:: ControlPacket
    :project: RobotHub
    :members:

.. doxygenstruct:: PairPacket
    :project: RobotHub
    :members:
"""
else:
    api_autogen = """API Reference (Manual Fallback)
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
- EspNowLink_
- ControlPacket_
- PairPacket_
- `Component Facades`_

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

- assign(PortRole, IMotor*, bool inverted): Connect a motor to a wheel role.
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

Component Facades
^^^^^^^^^^^^^^^^^

Every component also exposes a C-style facade; ``app/Hub_Main.cpp`` uses only these.

- CDD_Gpio_Begin(int eepromAddress): Load saved pin modes and apply them.
- CDD_Gpio_ReadPin(uint8_t), CDD_Gpio_WritePin(uint8_t, uint8_t), CDD_Gpio_SetPinMode(uint8_t, uint8_t): Pin access.
- CDD_Motor_Begin(uint16_t pwmHz), CDD_Motor_Run(slot, pwm), CDD_Motor_Brake(slot), CDD_Motor_Stop(slot), CDD_Motor_StopAll(): Motor bank.
- CDD_Servo_Begin(), CDD_Servo_Write(channel, angle), CDD_Servo_Release(channel), CDD_Servo_ReleaseAll(): Servo PCA9685.
- SWC_Robot_Begin(), SWC_Robot_Update(), SWC_Robot_OnRemote(const ControlPacket&): Robot lifecycle and remote input.
- SWC_Robot_SetSource(Source), SWC_Robot_Joy(...), SWC_Robot_RunMotor(...), SWC_Robot_BrakeMotor(...), SWC_Robot_Stop(): Manual control.
- SWC_Robot_IsReady(), SWC_Robot_IsMoving(), SWC_Robot_IsLinkAlive(): Status queries.
- SRV_Hmi_Begin(), SRV_Hmi_Update(): Start the touchscreen UART and run the command handler.
- SRV_EspNow_Begin(Handler, void* context), SRV_EspNow_Update(): Start WiFi STA + ESP-NOW and deliver queued packets.

Communication
-------------

EspNowLink
^^^^^^^^^^

- begin(Handler, void* context): Start ESP-NOW on the configured channel and register the packet handler.
- update(): Call from loop(); hands every queued ControlPacket to the handler.
- rejectedCount() const, droppedCount() const: Frames refused (length, CRC, type) and frames lost to a full queue.

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
"""

with open(API_AUTOGEN_PATH, "w", encoding="utf-8") as api_file:
     api_file.write(api_autogen)