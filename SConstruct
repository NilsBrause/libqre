# -*- python -*-

import os;

env = Environment()

env["CXX"] = os.environ.get("CXX", "g++")
env["CXXFLAGS"] = "-std=c++11 -Wall -Werror -O2 -ggdb"

regexp = env.SharedLibrary("regexp",
                           ["src/regexp.cpp",
                            "src/test.cpp",
                            "src/tokeniser.cpp",
                            "src/parser.cpp",
                            "src/fsm.cpp"],
                           CPPPATH = "include")

example = env.Program("example",
                      "example.cpp",
                      CPPPATH = "include",
                      LIBPATH = ".",
                      LIBS = "regexp")

prefix = os.environ.get("PREFIX", "/usr/local")

env.Install(os.path.join(prefix, "lib"), regexp)
env.Install(os.path.join(prefix, "include"), "include/regexp.hpp")

env.Alias("install", os.path.join(prefix, "lib"))
env.Alias("install", os.path.join(prefix, "include"))
