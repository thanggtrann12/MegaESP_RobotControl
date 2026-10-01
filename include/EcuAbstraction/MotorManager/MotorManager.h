#ifndef MOTOR_MANAGER_H
#define MOTOR_MANAGER_H

#include <Wire.h>
#include "IMotor.h"
#include "Motor_TA6586.h"
#include "Robot_Pin_Cfg.h"
#include "Std_Types.h"

/**
 * @file MotorManager.h
 * @brief Motor hardware container with logical role binding support.
 */

struct MotorSlot
{
    IMotor *ptr = nullptr;
    MotorRole role = MotorRole::UNBOUND;
    bool isBound = false;
};

class MotorManager
{
public:
    MotorManager() : _motorCount(0) {}
    ~MotorManager() { cleanup(); }

    bool initTA6586Array(TwoWire &wire, uint8_t pcaAddress, uint16_t pwmFrequency)
    {
        cleanup();

        for (uint8_t slot = 0; slot < MAX_MOTOR_PORT; ++slot)
        {
            _slots[slot].ptr = new MotorTA6586(wire, pcaAddress, ROBOT_MOTOR_L1_BI + slot * 2, ROBOT_MOTOR_L1_FI + slot * 2, pwmFrequency);
            _slots[slot].role = MotorRole::UNBOUND;
            _slots[slot].isBound = false;
        }

        _motorCount = MAX_MOTOR_PORT;
        return true;
    }

    bool bindMotorRole(uint8_t slotIndex, MotorRole role, bool inverted = false)
    {
        if (slotIndex >= _motorCount || _slots[slotIndex].ptr == nullptr)
        {
            return false;
        }

        _slots[slotIndex].role = role;
        _slots[slotIndex].isBound = (role != MotorRole::UNBOUND);
        _slots[slotIndex].ptr->setInverted(inverted);
        return true;
    }

    uint8_t getMotorCount() const { return _motorCount; }

    bool buildDriveArray(IMotor **outMotors, uint8_t outCount, KinematicsMode mode) const
    {
        if (outMotors == nullptr)
        {
            return false;
        }

        if (mode == KinematicsMode::MODE_2WD_DIFF)
        {
            if (outCount < 2)
            {
                return false;
            }
            outMotors[0] = findByRole(MotorRole::LEFT_MOTOR);
            outMotors[1] = findByRole(MotorRole::RIGHT_MOTOR);
            return outMotors[0] != nullptr && outMotors[1] != nullptr;
        }

        if (outCount < 4)
        {
            return false;
        }

        outMotors[0] = findByRole(MotorRole::FRONT_LEFT);
        outMotors[1] = findByRole(MotorRole::REAR_LEFT);
        outMotors[2] = findByRole(MotorRole::FRONT_RIGHT);
        outMotors[3] = findByRole(MotorRole::REAR_RIGHT);

        return outMotors[0] != nullptr && outMotors[1] != nullptr &&
               outMotors[2] != nullptr && outMotors[3] != nullptr;
    }

    void cleanup()
    {
        for (uint8_t index = 0; index < MAX_MOTOR_PORT; ++index)
        {
            if (_slots[index].ptr != nullptr)
            {
                delete _slots[index].ptr;
                _slots[index].ptr = nullptr;
            }
            _slots[index].role = MotorRole::UNBOUND;
            _slots[index].isBound = false;
        }
        _motorCount = 0;
    }

private:
    IMotor *findByRole(MotorRole role) const
    {
        for (uint8_t index = 0; index < _motorCount; ++index)
        {
            if (_slots[index].isBound && _slots[index].role == role)
            {
                return _slots[index].ptr;
            }
        }
        return nullptr;
    }

    MotorSlot _slots[MAX_MOTOR_PORT];
    uint8_t _motorCount;
};

#endif
