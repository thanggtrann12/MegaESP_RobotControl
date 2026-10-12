"""Add component API and selected hardware-config directories to the include path."""
import os

Import("env")  # noqa: F821  (do SCons cung cấp)

project_dir = env.subst("$PROJECT_DIR")
variant = env.GetProjectOption("custom_hw_variant", "s3_devkit")
root = os.path.join(project_dir, "components")

for name in sorted(os.listdir(root)):
    base = os.path.join(root, name)
    if not os.path.isdir(base):
        continue

    for sub in ("api", os.path.join("cfg", variant)):
        path = os.path.join(base, sub)
        if os.path.isdir(path):
            env.Append(CPPPATH=[path])

