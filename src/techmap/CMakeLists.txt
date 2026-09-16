add_library(techmap
    techmap.cpp
)
target_include_directories(techmap PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(techmap PUBLIC netlist)
