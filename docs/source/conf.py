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

Core Interfaces
---------------

.. doxygenclass:: IMotor
    :project: RobotHub
    :members:

.. doxygenclass:: ComManager
    :project: RobotHub
    :members:

Motor Drivers
-------------

.. doxygenclass:: MotorTA6586
    :project: RobotHub
    :members:

Input Components
----------------

.. doxygenclass:: Swc_Button
    :project: RobotHub
    :members:

.. doxygenclass:: Swc_Joystick
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
"""

with open(API_AUTOGEN_PATH, "w", encoding="utf-8") as api_file:
     api_file.write(api_autogen)