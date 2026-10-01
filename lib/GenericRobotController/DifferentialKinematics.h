/**
 * @file DifferentialKinematics.h
 * @brief Differential Drive Kinematics implementation conforming to IKinematics interface.
 */

#ifndef DIFFERENTIAL_KINEMATICS_H
#define DIFFERENTIAL_KINEMATICS_H

#include <IKinematics.h>

class DifferentialKinematics : public IKinematics
{
public:
    /**
     * @brief Constructor without bitmask. Wheel order is expected as FL, RL, FR, RR.
     */
    DifferentialKinematics() = default;
    ~DifferentialKinematics() override = default;

    /**
     * @brief Trả về định danh chế độ động học (MODE_2WD_DIFF).
     */
    KinematicsMode getMode() const override;

    /** @brief Returns 4 wheel channels (FL, RL, FR, RR). */
    uint8_t getWheelCount() const override;

    /**
    * @brief Tính toán tốc độ cho 4 bánh theo cặp trái/phải.
     * @param throttle Lệnh tiến/lùi (Y axis)
     * @param strafe Lệnh sang ngang (X axis - Bỏ qua đối với Differential)
     * @param rotation Lệnh xoay góc (Yaw axis)
    * @param outSpeeds Mảng kết quả [0:FL, 1:RL, 2:FR, 3:RR]
     */
    void computeWheelSpeeds(int16_t throttle,
                            int16_t strafe,
                            int16_t rotation,
                            int16_t *outSpeeds) const override;
};

#endif // DIFFERENTIAL_KINEMATICS_H