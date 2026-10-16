add_library(techmap
    techmap.cpp
    decompose.cpp
)
target_include_directories(techmap PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(techmap PUBLIC netlist)
target_link_libraries(techmap PUBLIC netlist cell_library)
