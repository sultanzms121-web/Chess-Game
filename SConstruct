#!/usr/bin/env python
import os

# 1. Grab the environment we just built in godot-cpp
env = SConscript("godot-cpp/SConstruct")

# 2. Tell it where your custom C++ files are
sources = Glob("src/*.cpp")

# 3. Build the shared library with a platform-agnostic name
# godot-cpp sets env["platform"], env["target"], and env["arch"]
platform = env["platform"]
target = env["target"]
arch = env["arch"]

# Determine the file extension based on platform
if platform == "windows":
    suffix = ".dll"
elif platform == "linux":
    suffix = ".so"
elif platform == "macos":
    suffix = ".dylib"
elif platform == "android":
    suffix = ".so"
elif platform == "ios":
    suffix = ".dylib"
else:
    suffix = ".so"

lib_name = "bin/libchess.{}.{}.{}{}".format(platform, target, arch, suffix)
library = env.SharedLibrary(lib_name, source=sources)

Default(library)