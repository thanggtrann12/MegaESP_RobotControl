// Host test for Drive: preset tables vs Chassis enum, mixing, scaling, bounds.
#include <stdio.h>
#include "Drive.h"

static int failures = 0;
#define CHECK(cond)                                                          \
    do                                                                       \
    {                                                                        \
        if (!(cond))                                                         \
        {                                                                    \
            printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);           \
            ++failures;                                                      \
        }                                                                    \
    } while (0)

struct MockMotor : IMotor
{
    int16_t pwm = 0;
    int runs = 0;
    void run(int16_t p) override { pwm = p; ++runs; }
    void brake() override { pwm = 0; }
    void stop() override { pwm = 0; }
};

// Six motors, one per wheel role; assigns only the roles the test asks for.
struct Rig
{
    MockMotor m[MAX_PORT_COUNT]; // index = role - 1: FL FR RL RR ML MR
    Drive drive;

    explicit Rig(Chassis chassis, uint8_t roles, uint8_t limit = 255)
    {
        for (uint8_t i = 0; i < MAX_PORT_COUNT; ++i)
        {
            if (roles & (1u << i))
            {
                drive.assign(static_cast<PortRole>(i + 1), &m[i], false);
            }
        }
        drive.setChassis(chassis, nullptr);
        drive.setPwmLimit(limit);
        drive.setAccelStep(0);
    }
    int16_t FL() const { return m[0].pwm; }
    int16_t FR() const { return m[1].pwm; }
    int16_t RL() const { return m[2].pwm; }
    int16_t RR() const { return m[3].pwm; }
    int16_t ML() const { return m[4].pwm; }
    int16_t MR() const { return m[5].pwm; }
};

constexpr uint8_t FL = 1 << 0, FR = 1 << 1, RL = 1 << 2, RR = 1 << 3, ML = 1 << 4, MR = 1 << 5;

static void twoWheel()
{
    Rig r(Chassis::TWO_WHEEL, FL | FR);
    CHECK(r.drive.ready());
    r.drive.move(100, 0, 0);
    CHECK(r.FL() == 255 && r.FR() == 255);
    r.drive.move(0, 0, 100); // turn right: left wheel forward, right wheel back
    CHECK(r.FL() == 255 && r.FR() == -255);
    CHECK(r.RL() == 0 && r.RR() == 0 && r.m[2].runs == 0); // rear motors never touched
}

static void mecanum()
{
    Rig r(Chassis::MECANUM, FL | FR | RL | RR);
    CHECK(r.drive.ready());

    r.drive.move(100, 0, 0);
    CHECK(r.FL() == 255 && r.FR() == 255 && r.RL() == 255 && r.RR() == 255);

    r.drive.move(0, 100, 0); // strafe right
    CHECK(r.FL() == 255 && r.FR() == -255 && r.RL() == -255 && r.RR() == 255);

    r.drive.move(0, 0, 100); // rotate right
    CHECK(r.FL() == 255 && r.FR() == -255 && r.RL() == 255 && r.RR() == -255);

    r.drive.move(0, 0, 0);
    CHECK(r.FL() == 0 && r.FR() == 0 && r.RL() == 0 && r.RR() == 0);
}

static void omni4MixesLikeMecanum()
{
    Rig a(Chassis::OMNI_4, FL | FR | RL | RR), b(Chassis::MECANUM, FL | FR | RL | RR);
    a.drive.move(40, -30, 20);
    b.drive.move(40, -30, 20);
    for (uint8_t i = 0; i < 4; ++i)
    {
        CHECK(a.m[i].pwm == b.m[i].pwm);
    }
}

static void tankDrivesAllFourWheels()
{
    Rig r(Chassis::TANK, FL | FR | RL | RR);
    // NOTE: Robot::apply() and Hmi only accept FL/FR for TANK today (see Chassis.h); Drive itself is four-wheel.
    r.drive.move(0, 0, 100);
    CHECK(r.FL() == 255 && r.RL() == 255 && r.FR() == -255 && r.RR() == -255);
}

static void sixWheel()
{
    Rig r(Chassis::SIX_WHEEL, FL | FR | RL | RR | ML | MR);
    r.drive.move(100, 0, 0);
    CHECK(r.FL() == 255 && r.FR() == 255 && r.RL() == 255 && r.RR() == 255 && r.ML() == 255 && r.MR() == 255);
    r.drive.move(0, 0, 100);
    CHECK(r.ML() == 255 && r.MR() == -255);
}

static void presetsAreWiredToTheRightChassis()
{
    // Each chassis has a preset that needs exactly these wheel roles assigned.
    CHECK(Rig(Chassis::TWO_WHEEL, FL | FR).drive.ready());
    CHECK(Rig(Chassis::TANK, FL | FR | RL | RR).drive.ready());
    CHECK(Rig(Chassis::OMNI_4, FL | FR | RL | RR).drive.ready());
    CHECK(Rig(Chassis::MECANUM, FL | FR | RL | RR).drive.ready());
    CHECK(Rig(Chassis::SIX_WHEEL, FL | FR | RL | RR | ML | MR).drive.ready());

    // A rear wheel missing from a four-wheel chassis must make it not ready.
    CHECK(!Rig(Chassis::MECANUM, FL | FR | RL).drive.ready());
    CHECK(!Rig(Chassis::TWO_WHEEL, FL).drive.ready());

    // No preset yet: must refuse to move rather than run with a wrong table.
    CHECK(!Rig(Chassis::OMNI_3, FL | FR | RL).drive.ready());
    CHECK(!Rig(Chassis::XDRIVE, FL | FR | RL | RR).drive.ready());
}

static void sparseRolesStillWork()
{
    // Roles FR and MR only (wheel indexes 1 and 5): the old loop bounded by "motors assigned" skipped MR.
    Mix custom[MAX_PORT_COUNT] = {};
    custom[1] = {100, 0, -100};
    custom[5] = {100, 0, -100};
    Rig r(Chassis::CUSTOM, FR | MR);
    r.drive.setChassis(Chassis::CUSTOM, custom);
    CHECK(r.drive.ready());
    r.drive.move(100, 0, 0);
    CHECK(r.FR() == 255 && r.MR() == 255);
}

static void scalesToPwmLimit()
{
    Rig r(Chassis::MECANUM, FL | FR | RL | RR, 100);
    r.drive.move(100, 100, 0); // FL and RR want 510, FR and RL want 0
    CHECK(r.FL() == 100 && r.RR() == 100 && r.FR() == 0 && r.RL() == 0);
    r.drive.move(50, 0, 50);   // ratios must be kept when scaling
    CHECK(r.FL() > r.FR());
}

static void inversionAndAccel()
{
    MockMotor a, b;
    Drive d;
    d.assign(PortRole::DRIVE_FL, &a, true); // inverted
    d.assign(PortRole::DRIVE_FR, &b, false);
    d.setChassis(Chassis::TWO_WHEEL, nullptr);
    d.setPwmLimit(255);
    d.setAccelStep(50);
    d.move(100, 0, 0);
    CHECK(a.pwm == -50 && b.pwm == 50); // limited to one step, FL reversed
    d.move(100, 0, 0);
    CHECK(a.pwm == -100 && b.pwm == 100);
}

static void notReadyStops()
{
    MockMotor a;
    Drive d;
    d.assign(PortRole::DRIVE_FL, &a, false); // FR missing
    d.setChassis(Chassis::TWO_WHEEL, nullptr);
    CHECK(!d.ready());
    a.pwm = 77;
    d.move(100, 0, 0);
    CHECK(a.pwm == 0);
}

static void auxiliaryRoleIsIgnored()
{
    // AUXILITARY (7) is below PortRole::COUNT but is not a wheel: used to index past _wheels[6].
    MockMotor a;
    Drive d;
    d.assign(PortRole::AUXILITARY, &a, false);
    d.assign(PortRole::COUNT, &a, false);
    d.setChassis(Chassis::SIX_WHEEL, nullptr);
    CHECK(!d.ready()); // reaching here without a sanitizer report is the real check
}

int main()
{
    twoWheel();
    mecanum();
    omni4MixesLikeMecanum();
    tankDrivesAllFourWheels();
    sixWheel();
    presetsAreWiredToTheRightChassis();
    sparseRolesStillWork();
    scalesToPwmLimit();
    inversionAndAccel();
    notReadyStops();
    auxiliaryRoleIsIgnored();
    printf(failures == 0 ? "test_drive: all passed\n" : "test_drive: %d FAILED\n", failures);
    return failures == 0 ? 0 : 1;
}
