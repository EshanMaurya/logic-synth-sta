cmake_minimum_required(VERSION 3.16)
project(logic_synth_sta CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Each pipeline stage builds as its own static library so it can be
# developed and tested independently of the others.
add_subdirectory(src/netlist)
add_subdirectory(src/minimization)
add_subdirectory(src/techmap)
add_subdirectory(src/sta)

enable_testing()
add_subdirectory(tests/unit)
