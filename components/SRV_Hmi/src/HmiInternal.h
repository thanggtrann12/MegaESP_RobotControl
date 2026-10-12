#ifndef HMI_INTERNAL_H
#define HMI_INTERNAL_H

// Private to the SRV_Hmi component (kept in src/, not api/): constants, parsing helpers and the
// frame builder shared by HmiProtocol.cpp, HmiHandlers.cpp and HmiMonitor.cpp.

#include "Hmi.h"
#include <Arduino.h>
#include <stdlib.h>
#include <string.h>
#include "Gpio_Cfg.h"

namespace hmi_detail
{
    // ---------------------------------------------------------------- hằng số
    constexpr const char *UI_PAGE = "Motor_sel"; // chứa c0..c5, b3..b8
    constexpr const char *MON_PAGE = "Monitor";
    constexpr const char *DIO_PAGE = "Digital_test";

    constexpr uint16_t COLOR_GREEN = 2016;
    constexpr uint16_t COLOR_RED = 63488;
    constexpr uint16_t COLOR_GREY = 33808;

    constexpr const char *DIO_MODE_TXT[3] = {"INPUT", "OUTPUT", "PULL_UP"};
    constexpr const char *ROLE_SHORT[] = {"--", "FL", "FR", "RL", "RR", "ML", "MR"};
    constexpr uint8_t ROLE_SHORT_COUNT = sizeof(ROLE_SHORT) / sizeof(ROLE_SHORT[0]);

    // Thứ tự nhãn t0..t5 trên các trang chassis (trang 4 bánh chỉ dùng t0..t3, six-wheel dùng đủ t0..t5)
    constexpr uint8_t LABEL_MAX = 6;
    constexpr PortRole LABEL_ORDER[LABEL_MAX] = {PortRole::DRIVE_FL, PortRole::DRIVE_FR,
                                                 PortRole::DRIVE_RL, PortRole::DRIVE_RR,
                                                 PortRole::DRIVE_ML, PortRole::DRIVE_MR};

    constexpr uint8_t MAX_FIELDS = 6;
    constexpr uint8_t TERMINATOR_BYTE = 0xFF;
    constexpr uint8_t TERMINATOR_COUNT = 3;
    constexpr uint32_t HEARTBEAT_MS = 500;
    constexpr uint8_t MAX_RX_PER_UPDATE = 64; // giới hạn byte đọc mỗi lần update(): màn hình flood không làm đói vòng điều khiển

    // Watchdog: dừng xe nếu lệnh lái dạng stream không còn đến.
    constexpr uint32_t JOY_TIMEOUT_MS = 400; // JOY phải được gửi lặp lại khi đang giữ joystick
    constexpr uint32_t RUN_TIMEOUT_MS = 0;   // 0 = tắt. Bật (vd 1000) nếu UI gửi lặp RUN khi giữ nút test motor
    constexpr long NUMBER_LIMIT = 100000L;   // mọi tham số HMI đều nhỏ hơn: chặn tràn khi parse

    // 8 chân IO đa dụng (IO_PIN_NUM_1..8) do DigitalIo quản lý: DigitalIo::pin(i) / indexOfPin(pin).
    // IoPin::readAnalog() nhận SỐ KÊNH (0..15, giống lệnh AIN). ANALOG_PIN_NUM_x là A0.. (số chân
    // 54.. trên Mega); analogRead() chấp nhận cả số kênh lẫn số chân, nên dùng thẳng được.
    constexpr uint8_t ADC_CHANNELS[6] = {ANALOG_PIN_NUM_1, ANALOG_PIN_NUM_2, ANALOG_PIN_NUM_3,
                                         ANALOG_PIN_NUM_4, ANALOG_PIN_NUM_5, ANALOG_PIN_NUM_6};

    constexpr uint8_t I2C_FIRST = 0x08;
    constexpr uint8_t I2C_LAST = 0x77;
    constexpr uint16_t I2C_TIMEOUT_US = 3000;
    constexpr uint8_t I2C_CHUNK = 8; // số địa chỉ quét mỗi lần update()

    // Chỉ số trường trên trang Monitor (khoá của _monHash). Thêm trường mới: thêm một hằng số ở đây,
    // không phụ thuộc thứ tự gọi trong syncMonitor().
    constexpr uint8_t MON_SRC = 0;
    constexpr uint8_t MON_LINK = 1;
    constexpr uint8_t MON_TOUT = 2;
    constexpr uint8_t MON_THR = 3;
    constexpr uint8_t MON_STR = 4;
    constexpr uint8_t MON_ROT = 5;
    constexpr uint8_t MON_CHS = 6;
    constexpr uint8_t MON_PWM = 7;
    constexpr uint8_t MON_ACC = 8;
    constexpr uint8_t MON_ROLE_BASE = 9;                          // + slot
    constexpr uint8_t MON_REV_BASE = MON_ROLE_BASE + MOTOR_SLOTS; // + slot
    constexpr uint8_t MON_DIO = MON_REV_BASE + MOTOR_SLOTS;
    constexpr uint8_t MON_AIN = MON_DIO + 1;
    constexpr uint8_t MON_SLOT_COUNT = MON_AIN + 1;

    // ---------------------------------------------------------------- parse
    /**
     * @brief So sánh hai chuỗi, trả về true nếu chúng giống nhau.
     */
    inline bool is(const char *a, const char *b) { return strcmp(a, b) == 0; }

    /**
     * @brief Chuyển chuỗi số nguyên (có thể có dấu) thành long, kiểm tra trong khoảng [low, high].
     *
     * Tự viết thay cho strtol: nhẹ hơn, không nhận khoảng trắng đầu chuỗi, và chặn tràn số.
     * @param text Chuỗi cần chuyển
     * @param low Giá trị nhỏ nhất
     * @param high Giá trị lớn nhất
     * @param value Chỉ được ghi khi trả về true
     * @return true nếu hợp lệ và nằm trong khoảng
     */
    inline bool number(const char *text, long low, long high, long &value)
    {
        if (text == nullptr || *text == '\0')
            return false;

        bool negative = false;

        if (*text == '-' || *text == '+')
        {
            negative = (*text == '-');
            ++text;
        }

        if (*text == '\0')
            return false;

        long parsed = 0;

        for (; *text != '\0'; ++text)
        {
            if (*text < '0' || *text > '9')
                return false;

            const long digit = *text - '0';

            if (parsed > (NUMBER_LIMIT - digit) / 10)
                return false;

            parsed = parsed * 10 + digit;
        }

        if (negative)
            parsed = -parsed;

        if (parsed < low || parsed > high)
            return false;

        value = parsed;
        return true;
    }

    /**
     * @brief Đổi số chân Arduino (từ lệnh IO) thành chỉ số GPIO 0..7 của DigitalIo.
     *
     * Danh sách trắng: chỉ 8 chân IO đa dụng mới điều khiển được từ màn hình. Motor chạy qua
     * PCA9685 (I2C), UART là Serial2/3, nên mọi chân khác mặc định KHÔNG đụng được.
     * @return true nếu pin là một trong 8 chân; index chỉ được ghi khi true
     */
    inline bool parseIoIndex(const char *text, uint8_t &index)
    {
        long pin = 0;
        if (!number(text, 0, 255, pin))
        {
            return false;
        }
        const int8_t found = DigitalIo::indexOfPin(static_cast<uint8_t>(pin));
        if (found < 0)
        {
            return false;
        }
        index = static_cast<uint8_t>(found);
        return true;
    }

    /**
     * @brief Tách chuỗi thành các trường, phân tách bằng dấu phẩy (sửa trực tiếp vào line).
     * @return Số lượng trường đã tách (tối đa MAX_FIELDS)
     */
    inline uint8_t split(char *line, char *fields[])
    {
        uint8_t count = 0;
        char *cursor = line;
        while (count < MAX_FIELDS)
        {
            fields[count++] = cursor;
            cursor = strchr(cursor, ',');
            if (cursor == nullptr)
            {
                break;
            }
            *cursor++ = '\0';
        }
        return count;
    }

    // ---------------------------------------------------------------- tiện ích định dạng
    /**
     * @brief Ghép tên component dạng "<prefix><index>", ví dụ ("tr", 3) -> "tr3".
     * @param buf Bộ đệm >= 4 byte
     * @param prefix Tối đa 2 ký tự
     * @param index 0..9
     */
    inline const char *indexed(char *buf, const char *prefix, uint8_t index)
    {
        uint8_t n = 0;
        while (*prefix != '\0' && n < 2)
        {
            buf[n++] = *prefix++;
        }
        buf[n++] = static_cast<char>('0' + index);
        buf[n] = '\0';
        return buf;
    }

    // Nhanh và nhẹ hơn snprintf("%ld")
    inline const char *num(char *buf, long value) { return ltoa(value, buf, 10); }

    inline const char *roleShort(PortRole role)
    {
        const uint8_t index = static_cast<uint8_t>(role);
        return index < ROLE_SHORT_COUNT ? ROLE_SHORT[index] : "--";
    }

    /**
     * @brief Số bánh có nhãn trên trang chassis (t0..t3 theo LABEL_ORDER).
     */
    inline uint8_t labelCount(Chassis c)
    {
        switch (c)
        {
        case Chassis::TWO_WHEEL:
        case Chassis::TANK:
            return 2;
        case Chassis::HOLONOMIC:
        case Chassis::OMNI_3:
            return 3;
        default: // OMNI_4, MECANUM, XDRIVE, SIX_WHEEL, ...
            // Chassis có bánh giữa (ML/MR) -> 6 nhãn, ngược lại 4
            return (roleUsedByChassis(c, PortRole::DRIVE_ML) || roleUsedByChassis(c, PortRole::DRIVE_MR)) ? 6 : 4;
        }
    }

    /**
     * @brief Tìm slot đang giữ một role.
     * @return slot 1..MOTOR_SLOTS, hoặc 0 nếu chưa gán
     */
    inline uint8_t slotOf(const PortRole *roles, PortRole role)
    {
        for (uint8_t s = 0; s < MOTOR_SLOTS; ++s)
        {
            if (roles[s] == role)
            {
                return s + 1;
            }
        }
        return 0;
    }

    // Hash 32-bit FNV-1a để nhận biết trường monitor nào đã đổi (va chạm ~1/4 tỉ, thay cho 16-bit ~1/65536)
    constexpr uint32_t FNV_OFFSET = 2166136261UL;
    constexpr uint32_t FNV_PRIME = 16777619UL;

    inline uint32_t hashByte(uint32_t h, uint8_t b) { return (h ^ b) * FNV_PRIME; }

    inline uint32_t hashText(const char *s)
    {
        uint32_t h = FNV_OFFSET;
        while (*s != '\0')
        {
            h = hashByte(h, static_cast<uint8_t>(*s++));
        }
        return h;
    }

    // Trộn màu vào hash; byte 0xA5 làm dấu ngăn cách (text chỉ là ASCII nên không trùng)
    inline uint32_t hashColor(uint32_t h, uint16_t color)
    {
        h = hashByte(h, 0xA5);
        h = hashByte(h, static_cast<uint8_t>(color & 0xFF));
        return hashByte(h, static_cast<uint8_t>(color >> 8));
    }

    // ---------------------------------------------------------------- gửi xuống màn hình
    // Ghép cả lệnh (kèm 3 byte kết thúc) vào một bộ đệm rồi ghi một lần: một write() thay cho
    // hàng chục print() nhỏ, giảm overhead UART/mutex và tránh các lệnh bị xen kẽ.
    class Frame
    {
    public:
        Frame &add(const char *s)
        {
            while (*s != '\0' && _length < CAPACITY)
            {
                _buffer[_length++] = *s++;
            }
            return *this;
        }

        Frame &add(char c)
        {
            if (_length < CAPACITY)
            {
                _buffer[_length++] = c;
            }
            return *this;
        }

        Frame &add(long value)
        {
            char digits[12];
            return add(static_cast<const char *>(ltoa(value, digits, 10)));
        }

        bool send(Print &out)
        {
            for (uint8_t i = 0; i < TERMINATOR_COUNT; ++i)
            {
                _buffer[_length++] = static_cast<char>(TERMINATOR_BYTE);
            }

            const size_t written = out.write(reinterpret_cast<const uint8_t *>(_buffer), _length);

            return written == _length;
        }

    private:
        static constexpr size_t CAPACITY = 128;
        char _buffer[CAPACITY + TERMINATOR_COUNT];
        size_t _length = 0;
    };

    // "<page>.<comp><attr>" (page == nullptr: trang đang hiển thị)
    inline Frame &beginAttr(Frame &frame, const char *page, const char *comp, const char *attr)
    {
        if (page != nullptr)
        {
            frame.add(page).add('.');
        }
        return frame.add(comp).add(attr);
    }

    inline void endMsg(Print &out)
    {
        Frame frame;
        frame.send(out);
    }

    inline void sendTxt(Print &out, const char *page, const char *comp, const char *text)
    {
        Frame frame;
        beginAttr(frame, page, comp, ".txt=\"").add(text).add('"').send(out);
    }

    inline void sendVal(Print &out, const char *page, const char *comp, long value)
    {
        Frame frame;
        beginAttr(frame, page, comp, ".val=").add(value).send(out);
    }

    inline void sendCol(Print &out, const char *page, const char *comp, uint16_t color)
    {
        Frame frame;
        beginAttr(frame, page, comp, ".pco=").add(static_cast<long>(color)).send(out);
    }

    // Biến int global trong program.s: "DRIVE_FL=3"
    inline void sendGlobal(Print &out, const char *name, long value)
    {
        Frame frame;
        frame.add(name).add('=').add(value).send(out);
    }

    // Gửi text rồi màu của cùng một component trong một lần ghi
    inline bool sendTxtCol(Print &out, const char *page, const char *comp, const char *text, int32_t color)
    {
        Frame frame;
        beginAttr(frame, page, comp, ".txt=\"").add(text).add('"');
        if (color >= 0)
        {
            // Hai lệnh liên tiếp: chèn terminator giữa chúng
            for (uint8_t i = 0; i < TERMINATOR_COUNT; ++i)
            {
                frame.add(static_cast<char>(TERMINATOR_BYTE));
            }
            beginAttr(frame, page, comp, ".pco=").add(static_cast<long>(color));
        }
        return frame.send(out);
    }

    /**
     * @brief Gửi một trường nếu hash (text + màu) khác lần trước.
     * @param cache Ô cache của trường (0 = chưa gửi lần nào)
     */
    inline void sendIfChanged(Print &out, uint32_t &cache, const char *page, const char *comp,
                       const char *text, int32_t color)
    {
        uint32_t h = hashText(text);
        if (color >= 0)
        {
            h = hashColor(h, static_cast<uint16_t>(color));
        }
        if (h == 0)
        {
            h = 1; // 0 nghĩa là "chưa gửi lần nào"
        }
        if (h == cache)
        {
            return;
        }

        if (sendTxtCol(out, page, comp, text, color))
        {
            cache = h;
        }
    }
}

using namespace hmi_detail;

#endif // HMI_INTERNAL_H
