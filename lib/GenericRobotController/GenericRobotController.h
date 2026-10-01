#ifndef GENERIC_ROBOT_CONTROLLER_H
#define GENERIC_ROBOT_CONTROLLER_H

#include <Arduino.h>
#include "Std_Types.h"
#include "IMotor.h"
#include "IMotorOutput.h"
#include "IKinematics.h"

/**
 * @file GenericRobotController.h
 * @brief Core robot control logic, source arbitration, and watchdog handling.
 */

// Topology-agnostic: owns control state, command arbitration, and the
// watchdog. Motor count/layout and motion mixing are fully delegated to
// the injected IMotorOutput and IKinematics implementations.
class GenericRobotController
{
public:
    /**
     * @brief Constructs robot controller with output backend and kinematics strategy.
     * @param motorOutput Motor output backend.
     * @param kinematics Initial kinematics strategy.
     */
    GenericRobotController(IMotorOutput &motorOutput, IKinematics &kinematics);
    /** @brief Initializes motor output backend. */
    void begin();
    /**
     * @brief Handles one incoming control packet.
     * @param packet Incoming packet.
     */
    void handlePacket(const ControlPacket &packet);
    /** @brief Runs periodic controller state update and watchdog logic. */
    void update();
    /**
     * @brief Switches active kinematics if wheel count matches output backend.
     * @param kinematics New kinematics strategy.
     * @return true on success.
     * @return false when strategy is incompatible.
     */
    bool setKinematics(IKinematics &kinematics);
    /**
     * @brief Gets currently active kinematics mode.
     * @return Active mode.
     */
    KinematicsMode getKinematicsMode() const;
    /**
     * @brief Sets active control source.
     * @param source New control source.
     */
    void setControlSource(ControlSource source);
    /**
     * @brief Gets current control source.
     * @return Current control source.
     */
    ControlSource getControlSource() const;
    /**
     * @brief Sets HMI motion command in manual source mode.
     * @param throttle Throttle percentage [-100, 100].
     * @param strafe Strafe percentage [-100, 100].
     * @param rotation Rotation percentage [-100, 100].
     */
    void setHmiMotion(int8_t throttle, int8_t strafe, int8_t rotation);
    /**
     * @brief Applies direct manual command to one motor.
     * @param motorId 1-based motor identifier.
     * @param speed Signed speed command.
     * @return true on success.
     * @return false if source or motor id is invalid.
     */
    bool setManualMotor(uint8_t motorId, int16_t speed);
    /**
     * @brief Releases active manual motor override.
     * @param motorId Motor id to release, or 0 to release any.
     */
    void releaseManualMotor(uint8_t motorId);
    /**
     * @brief Sets maximum PWM limit for wheel outputs.
     * @param limit PWM cap.
     */
    void setPwmLimit(uint8_t limit);
    /** @brief Sets the active motion profile. */
    void setMotionProfile(MotionProfile profile);
    /**
     * @brief Gets current PWM output limit.
     * @return PWM cap value.
     */
    uint8_t getPwmLimit() const;

private:
    /**
     * @brief Maps control axis percentage to motor speed domain.
     * @param value Axis value in range [-100, 100].
     * @return Motor-domain value in range [-255, 255].
     */
    static int16_t mapAxisToMotor(int16_t value);
    /**
     * @brief Applies motion command through current kinematics and motor output.
     * @param throttle Throttle command.
     * @param strafe Strafe command.
     * @param rotation Rotation command.
     */
    void applyMotion(int8_t throttle, int8_t strafe, int8_t rotation);
    /** @brief Stops all motors via output backend. */
    void stopMotors();

    IMotorOutput &_motorOutput;
    IKinematics *_kinematics;
    uint32_t _lastPacketTime;
    uint32_t _lastLogTime;
    uint16_t _lastSequenceNumber;
    bool _hasSequenceNumber;
    ControlSource _controlSource;
    uint8_t _pwmLimit;
    MotionProfile _motionProfile;
    int16_t _lastWheelSpeeds[MAX_DRIVE_WHEELS] = {0};
    bool _hmiMotionActive;
    int8_t _hmiThrottle;
    int8_t _hmiStrafe;
    int8_t _hmiRotation;
    uint32_t _hmiMotionTime;
    bool _manualOverrideActive;
    uint8_t _manualMotorId;
    uint32_t _manualCommandTime;
};

#endif