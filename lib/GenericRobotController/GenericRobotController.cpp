#include "GenericRobotController.h"

GenericRobotController::GenericRobotController(IMotor& leftMotor, IMotor& rightMotor)
    : _leftMotor(leftMotor),
      _rightMotor(rightMotor),
      _lastPacketTime(0),
      _lastLogTime(0) {}

void GenericRobotController::begin() {
    _leftMotor.begin();
    _rightMotor.begin();
}

void GenericRobotController::handlePacket(const ControlPacket& packet) {
    int16_t throttle = mapAxisToMotor(packet.joy1_y);
    int16_t steering = mapAxisToMotor(packet.joy1_x);
    int16_t leftSpeed = constrain(throttle + steering, -255, 255);
    int16_t rightSpeed = constrain(throttle - steering, -255, 255);

    _leftMotor.setSpeed(leftSpeed);
    _rightMotor.setSpeed(rightSpeed);
    _lastPacketTime = millis();

    if (millis() - _lastLogTime >= 500) {
        _lastLogTime = millis();
        logPacket(packet);
        Serial.print(F("[MOTOR] throttle="));
        Serial.print(throttle);
        Serial.print(F(" steering="));
        Serial.print(steering);
        Serial.print(F(" left="));
        Serial.print(leftSpeed);
        Serial.print(F(" right="));
        Serial.println(rightSpeed);
    }
}

void GenericRobotController::update() {
    if (millis() - _lastPacketTime > 500) {
        stopMotors();
    }
}

int16_t GenericRobotController::mapAxisToMotor(int16_t value) {
    int32_t centered = static_cast<int32_t>(value) - 512;
    centered = constrain(centered, -512L, 511L);
    return static_cast<int16_t>((centered * 255L) / 512L);
}

void GenericRobotController::stopMotors() {
    _leftMotor.stop();
    _rightMotor.stop();
}

void GenericRobotController::logPacket(const ControlPacket& packet) {
    Serial.print(F("[ROBOT] J1("));
    Serial.print(packet.joy1_x);
    Serial.print(F(", "));
    Serial.print(packet.joy1_y);
    Serial.print(F(") J2X("));
    Serial.print(packet.joy2_x);
    Serial.print(F(") buttons=0x"));
    Serial.println(packet.buttons, HEX);
}