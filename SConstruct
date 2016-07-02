# -*- python -*-

import os;

env = Environment()

env["CXX"] = os.environ.get("CXX", "g++")
env["CXXFLAGS"] = "-std=c++11 -Wall -Werror -O2"

qre = env.SharedLibrary("qre",
                        ["src/qre.cpp",
                         "src/test.cpp",
                         "src/tokeniser.cpp",
                         "src/parser.cpp",
                         "src/fsm.cpp",
                         "src/match.cpp"],
                        CPPPATH = "include")

example = env.Program("example",
                      "example.cpp",
                      CPPPATH = "include",
                      LIBPATH = ".",
                      LIBS = "qre")

prefix = os.environ.get("PREFIX", "/usr/local")

env.Install(os.path.join(prefix, "lib"), qre)
env.Install(os.path.join(prefix, "include"), "include/qre.hpp")

env.Alias("install", os.path.join(prefix, "lib"))
env.Alias("install", os.path.join(prefix, "include"))
