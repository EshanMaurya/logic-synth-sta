add_library(minimization
    quine_mccluskey.cpp
    petrick.cpp
    espresso.cpp
)
target_include_directories(minimization PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(minimization PUBLIC netlist)
