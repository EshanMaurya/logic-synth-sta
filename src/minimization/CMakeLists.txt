add_library(minimization
    cube.cpp
    boolfunc.cpp
    parser.cpp
    quine_mccluskey.cpp
    petrick.cpp
    minimize.cpp
    espresso.cpp
)
target_include_directories(minimization PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(minimization PUBLIC netlist)
if(MSVC)
    target_compile_options(minimization PRIVATE /W3)
else()
    target_compile_options(minimization PRIVATE -Wall -Wextra)
endif()

add_executable(lss_minimize lss_minimize.cpp)
target_link_libraries(lss_minimize PRIVATE minimization)
if(MSVC)
    target_compile_options(lss_minimize PRIVATE /W3)
else()
    target_compile_options(lss_minimize PRIVATE -Wall -Wextra)
endif()
