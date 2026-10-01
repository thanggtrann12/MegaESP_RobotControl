Dynamic Motor Mapping & Configuration
=====================================

Separation of Logical Role and Physical Channel
-----------------------------------------------

A key feature of this controller is separating logical motor roles from physical output channels.

Mecanum Profile Example
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: text

   FRONT_LEFT  -> CH0
   FRONT_RIGHT -> CH1
   REAR_LEFT   -> CH2
   REAR_RIGHT  -> CH3

Differential Profile Example
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: text

   LEFT_MOTOR  -> CH0
   RIGHT_MOTOR -> CH1

Configuration Validation Rules
------------------------------

The runtime keeps one active configuration and one staged configuration. TJC
updates the staged copy and only ``CMD_CFG:APPLY`` changes the running robot.
``CMD_CFG:SAVE`` persists the already-applied configuration to EEPROM.

The current configuration protocol is:

.. code-block:: text

   CMD_CFG:BEGIN
   CMD_CFG:MODE,<kinematics-mode>
   CMD_CFG:PWM,<0-255>
   CMD_CFG:PROFILE,<0|1>
   CMD_CFG:MOTOR,<slot 1-6>,<motor-role>
   CMD_CFG:INVERT,<slot 1-6>,<0|1>
   CMD_CFG:VALIDATE
   CMD_CFG:APPLY
   CMD_CFG:ABORT
   CMD_CFG:SAVE
   CMD_CAP:GET

The capability response has the form ``CAP:KIN,<diff>,<mecanum>,<omni3>,<omni4>,<custom>,DRV,<ta6586-pca9685>``.
The HMI must use this response to hide unsupported configuration choices.

Before applying or saving changes, ``ConfigurationManager`` validates:

1. Protocol slot exists ($1 \rightarrow 6$); firmware converts it to internal index $0 \rightarrow 5$.
2. The selected kinematics is advertised by ``CapabilityRegistry``.
3. Differential mode requires exactly one ``LEFT_MOTOR`` and one ``RIGHT_MOTOR`` role.
4. Mecanum mode requires the four front/rear left/right roles exactly once.
5. Unused physical slots may remain ``UNBOUND``.
6. The selected driver, inversion flags and PWM limit are valid.
7. Configuration version and CRC are valid before EEPROM load.

Motion profiles are currently ``0 = DIRECT`` and ``1 = LIMITED_ACCELERATION``.
The latter limits per-update wheel speed changes before PWM scaling.

The Mega2560 firmware currently advertises Differential and Mecanum
kinematics with the TA6586/PCA9685 driver. New hardware or kinematics must be
implemented and registered in firmware before the HMI can select them.