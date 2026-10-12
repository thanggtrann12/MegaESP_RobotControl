#include <Arduino.h>
#include <Wire.h>
#include "check.h"
#include "RobotSystemBuilder.h"

static int g_seq = 0;

static ControlPacket Pkt(int8_t t, int8_t s, int8_t r)
{
    ControlPacket p{};
    p.msgType = CONTROL_MESSAGE; p.throttle = t; p.strafe = s; p.rotation = r;
    p.sequenceNum = ++g_seq;
    UpdateControlPacketCrc(p);
    return p;
}

// Replays the I2C log and returns the last PWM value written to each PCA9685 channel (-1 = never).
static void ReplayPca(int out[16])
{
    for (int i = 0; i < 16; ++i) out[i] = -1;
    for (auto &t : Wire.log)
    {
        if (t.size() == 5 && t[0] >= 0x06 && t[0] <= 0x45)
        {
            int ch = (t[0] - 0x06) / 4;
            if (t[2] & 0x10) out[ch] = 4095;
            else if (t[4] & 0x10) out[ch] = 0;
            else out[ch] = t[3] | ((t[4] & 0x0F) << 8);
        }
    }
}

static int CountWrites(uint8_t firstByte)
{
    int n = 0;
    for (auto &t : Wire.log) if (!t.empty() && t[0] == firstByte) ++n;
    return n;
}

static std::string HmiOutput() { return std::string(Serial2.tx.begin(), Serial2.tx.end()); }

static void HmiSend(RobotSystemBuilder &b, const char *line)
{
    for (const char *c = line; *c; ++c) Serial2.rx.push_back((uint8_t)*c);
    Serial2.rx.push_back('\n');
    b.update();
}

int main()
{
    test_set_millis(1000);
    static RobotSystemBuilder b;
    b.build(KinematicsMode::MODE_4WD_MECANUM);
    b.begin();

    printf("[5] Boot sequence\n");
    CHECK(!Wire.log.empty() && Wire.log[0][0] == 0x01, "first I2C write is PCA9685 MODE2 init (no PWM write before chip init)");
    CHECK(CountWrites(0xFE) == 1, "PCA9685 prescale/restart sequence runs exactly once");
    CHECK(CountWrites(0xFA) == 1, "ALL_LED_OFF issued at init (motors cannot keep spinning across an MCU reset)");

    printf("[6] Mecanum baseline\n");
    int ch[16];
    b.getRobotController().handlePacket(Pkt(0, 100, 0));   // pure strafe right
    ReplayPca(ch);
    CHECK(ch[1] == 4095 && ch[2] == 4095 && ch[4] == 4095 && ch[7] == 4095, "strafe: FL fwd, RL rev, FR rev, RR fwd");
    b.getRobotController().handlePacket(Pkt(100, 0, 0));   // all four running forward
    ReplayPca(ch);
    CHECK(ch[1] == 4095 && ch[3] == 4095 && ch[5] == 4095 && ch[7] == 4095, "forward: all four wheels driven");

    printf("[7] CMD_MAP reports failures\n");
    HmiSend(b, "CMD_MAP:1,2");  // slot1 -> REAR_LEFT, but slot2 already has it
    CHECK(HmiOutput().find("CMD_MAP:ERR,APPLY") != std::string::npos, "duplicate role -> CMD_MAP:ERR,APPLY (used to answer OK)");
    CHECK(b.getRobotController().getKinematicsMode() == KinematicsMode::MODE_4WD_MECANUM, "failed bind leaves mecanum config intact");

    printf("[8] Switch to 2WD via staged HMI config\n");
    CHECK(b.handleHMIConfigCommand(HmiConfigCommand::BEGIN, 0, 0), "BEGIN");
    CHECK(b.handleHMIConfigCommand(HmiConfigCommand::SET_MODE, 0, 0), "SET_MODE 2WD");
    b.handleHMIConfigCommand(HmiConfigCommand::SET_MOTOR, 0, (uint8_t)MotorRole::LEFT_MOTOR);
    b.handleHMIConfigCommand(HmiConfigCommand::SET_MOTOR, 1, (uint8_t)MotorRole::RIGHT_MOTOR);
    b.handleHMIConfigCommand(HmiConfigCommand::SET_MOTOR, 2, (uint8_t)MotorRole::UNBOUND);
    b.handleHMIConfigCommand(HmiConfigCommand::SET_MOTOR, 3, (uint8_t)MotorRole::UNBOUND);
    CHECK(b.handleHMIConfigCommand(HmiConfigCommand::VALIDATE, 0, 0), "VALIDATE staged 2WD config");
    CHECK(b.handleHMIConfigCommand(HmiConfigCommand::APPLY, 0, 0), "APPLY succeeds (was impossible: wheelCount 4 != 2)");
    CHECK(b.getRobotController().getKinematicsMode() == KinematicsMode::MODE_2WD_DIFF, "controller now in 2WD mode");
    ReplayPca(ch);
    CHECK(ch[1] == 0 && ch[3] == 0 && ch[5] == 0 && ch[7] == 0, "all motors stopped when the drive array is rebound");

    b.getRobotController().handlePacket(Pkt(100, 0, 0));
    ReplayPca(ch);
    CHECK(ch[1] == 4095 && ch[3] == 4095, "2WD forward: both driven wheels forward");
    CHECK(ch[4] == 0 && ch[5] == 0 && ch[6] == 0 && ch[7] == 0, "unbound slots stay off");
    b.getRobotController().handlePacket(Pkt(0, 0, 100));
    ReplayPca(ch);
    CHECK(ch[1] == 4095 && ch[2] == 4095 && ch[3] == 0, "2WD turn right: left fwd, right reverse");

    printf("[9] Config validation\n");
    b.handleHMIConfigCommand(HmiConfigCommand::BEGIN, 0, 0);
    b.handleHMIConfigCommand(HmiConfigCommand::SET_PWM, 0, 0);
    CHECK(!b.handleHMIConfigCommand(HmiConfigCommand::VALIDATE, 0, 0), "pwmLimit=0 rejected");
    b.handleHMIConfigCommand(HmiConfigCommand::ABORT, 0, 0);

    printf("\n%d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
