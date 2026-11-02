"""After each build, copy firmware.bin to <build>/release/hub_v<HUB_OTA_FIRMWARE_VERSION>.bin."""
import os
import re
import shutil

Import("env")  # noqa: F821  (do SCons cung cấp)

HEADER_GLOB = os.path.join("components", "SRV_Ota", "cfg", "{variant}", "SRV_Ota_Cfg.h")


def firmware_version():
    # A -DHUB_OTA_FIRMWARE_VERSION=\"x\" build flag wins, same as in the header's #ifndef.
    for flag in env.get("BUILD_FLAGS", []):  # noqa: F821
        match = re.search(r'HUB_OTA_FIRMWARE_VERSION=\\?"?([^"\\\s]+)', str(flag))
        if match:
            return match.group(1)
    variant = env.GetProjectOption("custom_hw_variant", "s3_devkit")  # noqa: F821
    header = os.path.join(env.subst("$PROJECT_DIR"), HEADER_GLOB.format(variant=variant))  # noqa: F821
    with open(header, encoding="utf-8") as handle:
        match = re.search(r'#define\s+HUB_OTA_FIRMWARE_VERSION\s+"([^"]+)"', handle.read())
    return match.group(1) if match else "unknown"


def copy_versioned(target, source, env):  # noqa: ARG001
    version = re.sub(r"[^0-9A-Za-z._-]", "_", firmware_version())
    out_dir = os.path.join(env.subst("$BUILD_DIR"), "release")
    os.makedirs(out_dir, exist_ok=True)
    out = os.path.join(out_dir, f"hub_v{version}.bin")
    shutil.copyfile(str(target[0]), out)
    print(f"Versioned firmware: {out}")


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_versioned)  # noqa: F821
