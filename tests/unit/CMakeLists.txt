# TODO: wire in Catch2 or GoogleTest via FetchContent, e.g.:
#
#include(FetchContent)
#FetchContent_Declare(catch2 GIT_REPOSITORY https://github.com/catchorg/Catch2.git GIT_TAG v3.5.0)
#FetchContent_MakeAvailable(catch2)
#
#add_executable(unit_tests test_minimization.cpp test_sta.cpp)
#target_link_libraries(unit_tests PRIVATE minimization techmap sta Catch2::Catch2WithMain)
#add_test(NAME unit_tests COMMAND unit_tests)

add_executable(test_cell_library test_cell_library.cpp)
target_link_libraries(test_cell_library PRIVATE cell_library)
add_test(NAME test_cell_library COMMAND test_cell_library)

add_executable(test_decompose test_decompose.cpp)
target_link_libraries(test_decompose PRIVATE techmap)
add_test(NAME test_decompose COMMAND test_decompose)

add_executable(test_parser test_parser.cpp)
target_link_libraries(test_parser PRIVATE minimization)
add_test(NAME test_parser COMMAND test_parser)

add_executable(test_minimization test_minimization.cpp)
target_link_libraries(test_minimization PRIVATE minimization)
add_test(NAME test_minimization COMMAND test_minimization)

add_test(NAME lss_minimize_cli_pla
         COMMAND lss_minimize --json ${CMAKE_SOURCE_DIR}/tests/data/full_adder.pla)
add_test(NAME lss_minimize_cli_minterms
         COMMAND lss_minimize --pos ${CMAKE_SOURCE_DIR}/tests/data/textbook4.min)
