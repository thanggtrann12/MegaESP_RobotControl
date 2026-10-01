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

Before saving changes to EEPROM, ``ConfigManager`` validates:

1. Physical channel exists ($0 \rightarrow 5$).
2. Assigned role is valid for the current kinematics mode.
3. No duplicate channel conflicts exist.
4. Maximum power limits fall within valid bounds ($0 \rightarrow 255$).