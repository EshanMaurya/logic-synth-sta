add_library(cell_library
    gates.cpp
)
target_include_directories(cell_library PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})