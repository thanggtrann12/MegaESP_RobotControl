#!/usr/bin/env python3
"""
restructure.py - Chuyển MegaESP_RobotControl từ cây lib/ phẳng sang cây component.

Cách dùng (chạy ở thư mục gốc repo, nơi có platformio.ini):
    git add -A && git commit -m "before restructure"     # commit trước cho an toàn
    python restructure.py --dry-run                      # xem trước, không đổi gì
    python restructure.py                                # chạy thật (dùng `git mv` nếu có thể)

Tuỳ chọn:
    --root DIR     thư mục gốc repo (mặc định: thư mục hiện tại)
    --dry-run      chỉ in ra việc sẽ làm
    --no-git       dùng shutil.move thay vì `git mv`
    --variant X    tên biến thể phần cứng cho thư mục cfg/ (mặc định: s3_devkit)
    --force        chạy dù không thấy platformio.ini

Script làm các việc sau:
    1. Tạo cây thư mục components/<Comp>/{api,src,cfg/<variant>,utest}, app/, doc/, tools/, utest/, legacy/
    2. Di chuyển file theo bảng MOVES (giữ lịch sử git nếu dùng git mv)
    3. Đổi tên 2 file: EspCom.* -> EspNowLink.*, Std_Types.h -> ControlPacket.h
       và sửa các dòng #include tương ứng
    4. Tạo file mới: CMakeLists.txt, cfg/*.h (stub), tools/pio_components.py, platformio.ini mới
    5. Chuyển code của nhánh Mega/ESP8266 vào legacy/ (không xoá)
    6. Dọn các thư mục cũ đã rỗng

Script chạy lại nhiều lần vẫn an toàn: file nào đã ở đúng chỗ thì bỏ qua.
"""

import argparse
import re
import shutil
import subprocess
import sys
from pathlib import Path

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8")

# --------------------------------------------------------------------------------------
# Bảng cấu hình
# --------------------------------------------------------------------------------------

# component -> (có cfg/<variant>?, có utest/?, danh sách component phụ thuộc cho CMake)
COMPONENTS = {
    "SWC_Robot":        (True,  True,  ["SWC_Drive", "CDD_MotorTA6586", "CDD_Pca9685", "SRV_EspNow", "SRV_Log"]),
    "SWC_Drive":        (False, True,  ["CDD_MotorTA6586"]),
    "SRV_Hmi":          (True,  False, ["SWC_Robot", "CDD_Gpio", "SRV_Log"]),
    "SRV_EspNow":       (False, True,  ["SRV_Log"]),
    "SRV_Log":          (False, False, []),
    "CDD_MotorTA6586":  (False, False, ["CDD_Pca9685"]),
    "CDD_Pca9685":      (True,  False, []),
    "CDD_Gpio":         (True,  False, []),
}

# Thư mục khác cần tạo (ngoài cây components)
EXTRA_DIRS = ["app", "tools", "utest", "legacy"]

# Đổi tên header -> sửa #include trong mọi file đã chuyển
RENAMES = {
    "EspCom.h": "EspNowLink.h",
    "Std_Types.h": "ControlPacket.h",
}


def A(old, comp, new=None):
    return (old, f"components/{comp}/api/{new or Path(old).name}")


def S(old, comp, new=None):
    return (old, f"components/{comp}/src/{new or Path(old).name}")


# (đường dẫn cũ, đường dẫn mới) - tính từ thư mục gốc repo. File hoặc thư mục đều được.
MOVES = [
    # ---- app ----
    ("src/Esp32_HubUnit/Esp32_HubUnit.cpp", "app/Hub_Main.cpp"),

    # ---- SWC_Robot ----
    A("lib/Robot/Robot.h", "SWC_Robot"),
    S("lib/Robot/Robot.cpp", "SWC_Robot"),
    A("lib/RobotConfig/RobotConfig.h", "SWC_Robot"),
    S("lib/RobotConfig/RobotConfig.cpp", "SWC_Robot"),

    # ---- SWC_Drive ----
    A("lib/Drive/Drive.h", "SWC_Drive"),
    S("lib/Drive/Drive.cpp", "SWC_Drive"),

    # ---- SRV_Hmi ----
    A("lib/Hmi/Hmi.h", "SRV_Hmi"),
    S("lib/Hmi/Hmi.cpp", "SRV_Hmi"),

    # ---- SRV_EspNow ----
    A("lib/EspCom/EspCom.h", "SRV_EspNow", "EspNowLink.h"),
    S("lib/EspCom/EspCom.cpp", "SRV_EspNow", "EspNowLink.cpp"),
    A("include/Std_Types.h", "SRV_EspNow", "ControlPacket.h"),

    # ---- SRV_Log ----
    A("lib/GenericLogger/GenericLogger.h", "SRV_Log"),
    S("lib/GenericLogger/GenericLogger.cpp", "SRV_Log"),

    # ---- CDD_MotorTA6586 ----
    A("lib/Motor/Motor.h", "CDD_MotorTA6586"),
    A("lib/Motor/MotorBank.h", "CDD_MotorTA6586"),
    A("lib/Motor/MotorTA6586.h", "CDD_MotorTA6586"),
    S("lib/Motor/MotorTA6586.cpp", "CDD_MotorTA6586"),

    # ---- CDD_Pca9685 ----
    A("lib/Pca9685/Pca9685.h", "CDD_Pca9685"),
    S("lib/Pca9685/Pca9685.cpp", "CDD_Pca9685"),
    A("lib/ServoOut/ServoOut.h", "CDD_Pca9685"),

    # ---- CDD_Gpio ----
    A("lib/IoPin/IoPin.h", "CDD_Gpio"),
    A("lib/DigitalIo/DigitalIo.h", "CDD_Gpio"),
    S("lib/DigitalIo/DigitalIo.cpp", "CDD_Gpio"),
    # RobotPins.h chứa pin của nhiều component -> tạm để ở cfg của Gpio, tách dần sang *_Cfg.h
    ("include/RobotPins.h", f"components/CDD_Gpio/cfg/{{variant}}/RobotPins.h"),

    # ---- doc / tools / utest ----
    ("docs", "doc"),
    ("build.bat", "tools/build.bat"),
    ("Doxyfile", "tools/Doxyfile"),
    ("src/TcjUi", "tools/TcjUi"),
    ("test/README", "utest/README"),

    # ---- legacy: nhánh Mega2560 / ESP8266 (không xoá, chỉ dời) ----
    ("src/Remote_Mega2560", "legacy/Remote_Mega2560"),
    ("src/Remote_ESP8266_Bridge", "legacy/Remote_ESP8266_Bridge"),
    ("lib/ComManager", "legacy/lib/ComManager"),
    ("lib/GenericOTA", "legacy/lib/GenericOTA"),
    ("lib/Swc_Button", "legacy/lib/Swc_Button"),
    ("lib/Swc_Joystick", "legacy/lib/Swc_Joystick"),
    ("include/Remote_Pin_Cfg.h", "legacy/include/Remote_Pin_Cfg.h"),
    # platformio.ini cũ được lưu lại (có 3 env), file mới chỉ có env hub
    ("platformio.ini", "legacy/platformio.ini"),
]

# Thư mục cũ sẽ được xoá nếu rỗng sau khi dời
OLD_DIRS = [
    "src/Esp32_HubUnit", "src/Remote_Mega2560", "src/Remote_ESP8266_Bridge", "src/TcjUi", "src",
    "lib/Robot", "lib/RobotConfig", "lib/Drive", "lib/Hmi", "lib/EspCom", "lib/GenericLogger",
    "lib/Motor", "lib/Pca9685", "lib/ServoOut", "lib/IoPin", "lib/DigitalIo",
    "lib/ComManager", "lib/GenericOTA", "lib/Swc_Button", "lib/Swc_Joystick",
    "test",
]

# --------------------------------------------------------------------------------------
# Nội dung file mới
# --------------------------------------------------------------------------------------

PLATFORMIO_INI = """\
; Cấu trúc component: xem tools/pio_components.py
; Bản cũ (3 env: Mega2560, ESP8266 bridge, ESP32 hub) nằm ở legacy/platformio.ini

[platformio]
default_envs = hub
src_dir = app

[env:hub]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
monitor_filters = direct
; Cổng đang cứng theo máy - sửa lại cho đúng máy của bạn
upload_port = COM13
monitor_port = COM13

build_flags =
    -DARDUINO_USB_CDC_ON_BOOT=0
board_upload.flash_size = 16MB
board_build.arduino.memory_type = qio_opi

; Source của component được thêm bởi extra script nên LDF không quét thấy chúng:
; các thư viện framework mà component dùng phải khai báo ở đây.
lib_deps =
    Wire
    EEPROM
    WiFi

custom_hw_variant = {variant}
extra_scripts = pre:tools/pio_components.py
"""

PIO_COMPONENTS_PY = '''\
"""
PlatformIO extra script: gom mọi component trong components/ vào build.

  - thêm  components/<C>/api  và  components/<C>/cfg/<variant>  vào include path
  - biên dịch  components/<C>/src/*.cpp

Chọn biến thể phần cứng bằng `custom_hw_variant` trong platformio.ini.
Nếu env.BuildSources không nhận source trong phiên bản PlatformIO của bạn, thay bằng
`build_src_filter = +<*> +<../components/SWC_Robot/src/> ...` trong platformio.ini.
"""
import os

Import("env")  # noqa: F821  (do SCons cung cấp)

project_dir = env.subst("$PROJECT_DIR")
variant = env.GetProjectOption("custom_hw_variant", "{variant}")
root = os.path.join(project_dir, "components")

for name in sorted(os.listdir(root)):
    base = os.path.join(root, name)
    if not os.path.isdir(base):
        continue

    for sub in ("api", os.path.join("cfg", variant)):
        path = os.path.join(base, sub)
        if os.path.isdir(path):
            env.Append(CPPPATH=[path])

    src = os.path.join(base, "src")
    if os.path.isdir(src):
        env.BuildSources(os.path.join("$BUILD_DIR", "components", name), src)
'''

ROOT_CMAKE = """\
cmake_minimum_required(VERSION 3.16)
project(MegaESP_RobotControl CXX)

set(HW_VARIANT "{variant}" CACHE STRING "Biến thể phần cứng (tên thư mục trong cfg/)")
option(BUILD_UTEST "Build unit test trên host" OFF)

# Mỗi component có CMakeLists.txt riêng, tạo ra target trùng tên thư mục.
file(GLOB COMPONENT_DIRS LIST_DIRECTORIES true
     RELATIVE ${{CMAKE_SOURCE_DIR}}/components ${{CMAKE_SOURCE_DIR}}/components/*)
foreach(c ${{COMPONENT_DIRS}})
  if(EXISTS ${{CMAKE_SOURCE_DIR}}/components/${{c}}/CMakeLists.txt)
    add_subdirectory(components/${{c}})
  endif()
endforeach()

if(BUILD_UTEST)
  enable_testing()
  add_subdirectory(utest)
endif()
"""

COMPONENT_CMAKE = """\
# {name}
file(GLOB {name}_SRCS CONFIGURE_DEPENDS ${{CMAKE_CURRENT_SOURCE_DIR}}/src/*.cpp)

if({name}_SRCS)
  add_library({name} STATIC ${{{name}_SRCS}})
  set({name}_SCOPE PUBLIC)
else()
  add_library({name} INTERFACE)
  set({name}_SCOPE INTERFACE)
endif()

target_include_directories({name} ${{{name}_SCOPE}} ${{CMAKE_CURRENT_SOURCE_DIR}}/api)
if(EXISTS ${{CMAKE_CURRENT_SOURCE_DIR}}/cfg/${{HW_VARIANT}})
  target_include_directories({name} ${{{name}_SCOPE}} ${{CMAKE_CURRENT_SOURCE_DIR}}/cfg/${{HW_VARIANT}})
endif()
{deps}"""

UTEST_CMAKE = """\
# Runner chung cho unit test chạy trên host (env native).
# Component dùng Arduino.h nên cần mock (ArduinoFake hoặc stub tự viết) trước khi build được.
# Ví dụ:
#   add_executable(test_drive ../components/SWC_Drive/utest/test_drive.cpp)
#   target_link_libraries(test_drive PRIVATE SWC_Drive)
#   add_test(NAME test_drive COMMAND test_drive)
"""

CFG_CMAKE = """\
# Các header cấu hình theo biến thể phần cứng nằm trong cfg/<HW_VARIANT>/.
# Include path đã được CMakeLists.txt của component thêm sẵn.
"""

CFG_HEADERS = {
    f"components/SWC_Robot/cfg/{{variant}}/Robot_Cfg.h": """\
#pragma once
// TODO: chuyển hằng số cấu hình của Robot về đây, ví dụ:
//   LINK_TIMEOUT_MS, TICK_MS, TEST_TIMEOUT_MS (đang ở Robot.h)
//   CONFIG_EEPROM_ADDRESS (đang ở RobotPins.h)
""",
    f"components/SRV_Hmi/cfg/{{variant}}/Hmi_Cfg.h": """\
#pragma once
// TODO: chuyển từ RobotPins.h: HMI_UART, HMI_UART_TX, HMI_UART_RX
""",
    f"components/CDD_Pca9685/cfg/{{variant}}/Pca9685_Cfg.h": """\
#pragma once
// TODO: chuyển từ RobotPins.h: MOTOR_PCA_ADDRESS, SERVO_PCA_ADDRESS, MOTOR_PWM_HZ,
//       I2C_SDA_PIN, I2C_SCL_PIN
""",
    f"components/CDD_Gpio/cfg/{{variant}}/Gpio_Cfg.h": """\
#pragma once
// TODO: chuyển từ RobotPins.h: IO_PIN_NUM_1..8, ANALOG_PIN_NUM_*, ENCODER_PIN_NUM_*
""",
}

# Các file cần tách tay (script không tự cắt code)
STUB_SRC = {
    "components/SWC_Drive/api/Chassis.h": """\
#pragma once
// TODO: tách enum Chassis, PortRole, struct Mix và các hàm *ToString ra khỏi Drive.h vào đây.
// Khi tách, sửa luôn bảng PRESETS trong Drive.cpp cho khớp thứ tự enum.
""",
    "components/SRV_Hmi/src/HmiProtocol.cpp": "// TODO: tách từ Hmi.cpp - nhận byte, tách frame, bảng lệnh CommandEntry.\n",
    "components/SRV_Hmi/src/HmiHandlers.cpp": "// TODO: tách từ Hmi.cpp - các handler CHASSIS, MOTOR, MIX, PWM, SAVE, JOY, RUN...\n",
    "components/SRV_Hmi/src/HmiMonitor.cpp": "// TODO: tách từ Hmi.cpp - MON, push trạng thái lên màn hình, quét I2C.\n",
}

GITKEEP_DIRS_SUFFIX = ("utest",)

# --------------------------------------------------------------------------------------
# Thực thi
# --------------------------------------------------------------------------------------


class Runner:
    def __init__(self, root: Path, variant: str, dry: bool, use_git: bool):
        self.root = root
        self.variant = variant
        self.dry = dry
        self.use_git = use_git
        self.done = 0
        self.skipped = []
        self.missing = []
        self.moved_src = set()

    def p(self, rel: str) -> Path:
        return self.root / rel.replace("{variant}", self.variant)

    def say(self, msg: str) -> None:
        print(("[dry] " if self.dry else "") + msg)

    # ---- thư mục ----
    def make_dirs(self) -> None:
        dirs = []
        for name, (has_cfg, has_utest, _) in COMPONENTS.items():
            base = f"components/{name}"
            dirs += [f"{base}/api", f"{base}/src"]
            if has_cfg:
                dirs.append(f"{base}/cfg/{self.variant}")
            if has_utest:
                dirs.append(f"{base}/utest")
        dirs += EXTRA_DIRS
        for d in dirs:
            path = self.p(d)
            if not path.exists():
                self.say(f"mkdir  {d}")
                if not self.dry:
                    path.mkdir(parents=True, exist_ok=True)

    # ---- di chuyển ----
    def move(self, old: str, new: str) -> None:
        src, dst = self.p(old), self.p(new)
        if not src.exists():
            self.missing.append(old)
            return
        if dst.is_dir() and not any(dst.iterdir()):
            if not self.dry:
                dst.rmdir()  # đích là thư mục rỗng -> dời đè lên
        elif dst.exists():
            self.skipped.append(f"{old} -> {new} (đích đã tồn tại)")
            return
        self.moved_src.add(old)
        self.say(f"move   {old}  ->  {dst.relative_to(self.root).as_posix()}")
        self.done += 1
        if self.dry:
            return
        dst.parent.mkdir(parents=True, exist_ok=True)
        if self.use_git:
            r = subprocess.run(["git", "mv", str(src), str(dst)], cwd=self.root,
                               capture_output=True, text=True)
            if r.returncode == 0:
                return
        shutil.move(str(src), str(dst))

    # ---- ghi file mới ----
    def write(self, rel: str, content: str) -> None:
        path = self.p(rel)
        if path.exists() and not (self.dry and rel in self.moved_src):
            self.skipped.append(f"{rel} (đã tồn tại, giữ nguyên)")
            return
        self.say(f"create {path.relative_to(self.root).as_posix()}")
        if self.dry:
            return
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8", newline="\n")

    def create_files(self) -> None:
        v = self.variant
        self.write("platformio.ini", PLATFORMIO_INI.format(variant=v))
        self.write("tools/pio_components.py", PIO_COMPONENTS_PY.replace("{variant}", v))
        self.write("CMakeLists.txt", ROOT_CMAKE.format(variant=v))
        self.write("utest/CMakeLists.txt", UTEST_CMAKE)

        for name, (has_cfg, _, deps) in COMPONENTS.items():
            dep_txt = ""
            if deps:
                dep_txt = (f"\n# Phụ thuộc (chỉnh lại nếu sai):\n"
                           f"target_link_libraries({name} ${{{name}_SCOPE}} {' '.join(deps)})\n")
            self.write(f"components/{name}/CMakeLists.txt",
                       COMPONENT_CMAKE.format(name=name, deps=dep_txt))
        self.write("components/SWC_Robot/cfg/CMakeLists.txt", CFG_CMAKE)

        for rel, content in {**CFG_HEADERS, **STUB_SRC}.items():
            self.write(rel, content)

    # ---- sửa #include theo tên mới ----
    def fix_includes(self) -> None:
        if self.dry:
            self.say("fix #include  " + ", ".join(f"{a} -> {b}" for a, b in RENAMES.items()))
            return
        patterns = [(re.compile(r'(#\s*include\s*[<"])(?:[\w./\\-]*[/\\])?' + re.escape(a) + r'([>"])'),
                     r"\g<1>" + b + r"\g<2>") for a, b in RENAMES.items()]
        changed = 0
        for top in ("components", "app"):
            base = self.root / top
            if not base.exists():
                continue
            for f in base.rglob("*"):
                if f.suffix.lower() not in (".h", ".hpp", ".c", ".cpp", ".ino"):
                    continue
                with open(f, "r", encoding="utf-8", newline="") as fh:
                    text = fh.read()
                new = text
                for pat, rep in patterns:
                    new = pat.sub(rep, new)
                if new != text:
                    with open(f, "w", encoding="utf-8", newline="") as fh:
                        fh.write(new)
                    changed += 1
                    print(f"fix    #include trong {f.relative_to(self.root).as_posix()}")
        if not changed:
            print("fix    không có #include nào cần sửa")

    # ---- dọn ----
    def cleanup(self) -> None:
        for rel in OLD_DIRS:
            d = self.p(rel)
            if d.is_dir() and not any(d.iterdir()):
                self.say(f"rmdir  {rel}")
                if not self.dry:
                    d.rmdir()

    def gitkeep(self) -> None:
        if self.dry:
            return
        for d in self.root.rglob("*"):
            if d.is_dir() and d.name in GITKEEP_DIRS_SUFFIX and ".git" not in d.parts \
                    and not any(d.iterdir()):
                (d / ".gitkeep").write_text("", encoding="utf-8")


def main() -> int:
    ap = argparse.ArgumentParser(description="Chuyển repo sang cấu trúc component.")
    ap.add_argument("--root", default=".", help="thư mục gốc repo")
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--no-git", action="store_true", help="không dùng git mv")
    ap.add_argument("--variant", default="s3_devkit")
    ap.add_argument("--force", action="store_true")
    args = ap.parse_args()

    root = Path(args.root).resolve()
    if not (root / "platformio.ini").exists() and not (root / "components").exists() and not args.force:
        print(f"Không thấy platformio.ini trong {root}. Chạy ở thư mục gốc repo hoặc dùng --root / --force.")
        return 1

    use_git = (not args.no_git) and (root / ".git").exists()
    r = Runner(root, args.variant, args.dry_run, use_git)
    print(f"Root: {root}\nGit mv: {'có' if use_git else 'không'} | Dry-run: {args.dry_run}\n")

    r.make_dirs()
    for old, new in MOVES:
        r.move(old, new)
    r.create_files()          # sau khi platformio.ini cũ đã được dời vào legacy/
    r.fix_includes()
    r.cleanup()
    r.gitkeep()

    print(f"\nĐã {'sẽ ' if args.dry_run else ''}di chuyển: {r.done} mục")
    if r.skipped:
        print("Bỏ qua:")
        for s in r.skipped:
            print("  -", s)
    if r.missing:
        print("Không tìm thấy nguồn (đã dời từ trước hoặc không có):")
        for s in r.missing:
            print("  -", s)

    print("""
Việc cần làm tay sau khi chạy:
  1. Tách code:  Drive.h -> Chassis.h | Hmi.cpp -> HmiProtocol/HmiHandlers/HmiMonitor.cpp
                 RobotPins.h -> các *_Cfg.h trong cfg/<variant>/ của từng component
  2. Nối EspNowLink với Robot::onRemote() và gọi robot.begin() trong Hub_Main.cpp
  3. tools/build.bat và tools/Doxyfile còn đường dẫn cũ (docs\\, lib/, src/) - sửa lại
  4. Build thử:  pio run -e hub   (nếu thiếu source, xem ghi chú đầu tools/pio_components.py)
  5. Cập nhật README.md cho khớp cây mới; xoá lib/README, include/README nếu không cần
""")
    return 0


if __name__ == "__main__":
    sys.exit(main())