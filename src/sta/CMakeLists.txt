add_library(sta
    sta.cpp
)
target_include_directories(sta PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(sta PUBLIC netlist)
