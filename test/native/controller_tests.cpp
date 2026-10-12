#include <Arduino.h>
#include "check.h"
#include "GenericRobotController.h"
#include "DifferentialKinematics.h"
#include "MecanumKinematics.h"
#include "MotorArrayOutput.h"

struct FakeMotor : IMotor
{
    int16_t last = 0; int setCalls = 0; int stopCalls = 0;
    void begin() override {}
    void setSpeed(int16_t s) override { last = s; ++setCalls; }
    void setInverted(bool) override {}
    void stop() override { last = 0; ++stopCalls; }
};

static ControlPacket MakePacket(int8_t t, int8_t s, int8_t r, uint16_t seq)
{
    ControlPacket p{};
    p.msgType = CONTROL_MESSAGE; p.throttle = t; p.strafe = s; p.rotation = r; p.sequenceNum = seq;
    UpdateControlPacketCrc(p);
    return p;
}

static void TestSequenceResyncAfterRemoteReboot()
{
    printf("[1] Remote reboot -> sequence resync\n");
    test_set_millis(1000);
    FakeMotor m[4]; IMotor *arr[4] = {&m[0], &m[1], &m[2], &m[3]};
    MotorArrayOutput out(arr, 4); MecanumKinematics k; GenericRobotController c(out, k);
    c.handlePacket(MakePacket(50, 0, 0, 1000));
    CHECK(m[0].last != 0, "packet seq=1000 accepted");
    test_advance_millis(700); c.update();           // watchdog trips (remote went away)
    CHECK(m[0].last == 0, "watchdog stops motors");
    test_advance_millis(2000);                       // remote boots again, seq restarts at 0
    c.handlePacket(MakePacket(50, 0, 0, 0));
    CHECK(m[0].last != 0, "packet seq=0 accepted after timeout (was rejected for ~11 min)");
    c.handlePacket(MakePacket(60, 0, 0, 0));         // duplicate must still be dropped
    int16_t afterDup = m[0].last;
    c.handlePacket(MakePacket(100, 0, 0, 1));
    CHECK(m[0].last != afterDup, "normal increasing sequence still works");
}

static void TestTwoWheelDifferential()
{
    printf("[2] 2WD differential kinematics\n");
    FakeMotor m[2]; IMotor *arr[2] = {&m[0], &m[1]};
    MotorArrayOutput out(arr, 2); DifferentialKinematics diff; MecanumKinematics mec;
    GenericRobotController c(out, mec);
    int16_t s[4] = {0};
    diff.computeWheelSpeeds(255, 0, 0, s);
    CHECK(s[0] == 255 && s[1] == 255, "forward: left=255 right=255");
    diff.computeWheelSpeeds(0, 0, 255, s);
    CHECK(s[0] == 255 && s[1] == -255, "turn right: left=+255 right=-255 (right wheel got LEFT speed before)");
    CHECK(diff.getWheelCount() == 2, "getWheelCount()==2 matches a 2-motor output");
    CHECK(c.setKinematics(diff), "controller accepts 2WD kinematics on 2-motor output");
}

static void TestNoIdleStopSpam()
{
    printf("[3] Idle watchdog must not hammer the motor driver\n");
    test_set_millis(1000);
    FakeMotor m[4]; IMotor *arr[4] = {&m[0], &m[1], &m[2], &m[3]};
    MotorArrayOutput out(arr, 4); MecanumKinematics k; GenericRobotController c(out, k);
    c.handlePacket(MakePacket(50, 0, 0, 1));
    test_advance_millis(700);
    for (int i = 0; i < 1000; ++i) c.update();
    CHECK(m[0].stopCalls <= 1, "stop() issued at most once while idle");
    c.handlePacket(MakePacket(50, 0, 0, 2));
    CHECK(m[0].last != 0, "motors restart after being idle");
    test_advance_millis(700);
    for (int i = 0; i < 1000; ++i) c.update();
    CHECK(m[0].last == 0 && m[0].stopCalls <= 2, "second timeout stops again, once");
}

static void TestRampIsTimeBased()
{
    printf("[4] LIMITED_ACCELERATION independent of loop rate\n");
    test_set_millis(1000);
    FakeMotor m[4]; IMotor *arr[4] = {&m[0], &m[1], &m[2], &m[3]};
    MotorArrayOutput out(arr, 4); MecanumKinematics k; GenericRobotController c(out, k);
    c.setMotionProfile(MotionProfile::LIMITED_ACCELERATION);
    c.setControlSource(ControlSource::HMI_MANUAL);
    c.setHmiMotion(100, 0, 0);
    for (int ms = 0; ms < 100; ++ms) { test_advance_millis(1); c.update(); }  // 1 kHz loop
    printf("        speed after 100 ms @1kHz loop = %d\n", m[0].last);
    CHECK(m[0].last <= 200, "ramp after 100 ms is bounded (~160), not instantly 255");
    for (int ms = 0; ms < 300; ++ms) { test_advance_millis(1); c.setHmiMotion(100, 0, 0); c.update(); }
    CHECK(m[0].last == 255, "eventually reaches full speed");
}

int main()
{
    TestSequenceResyncAfterRemoteReboot();
    TestTwoWheelDifferential();
    TestNoIdleStopSpam();
    TestRampIsTimeBased();
    printf("\n%d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
