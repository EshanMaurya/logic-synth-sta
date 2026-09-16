# TODO: wire in Catch2 or GoogleTest via FetchContent, e.g.:
#
# include(FetchContent)
# FetchContent_Declare(catch2 GIT_REPOSITORY https://github.com/catchorg/Catch2.git GIT_TAG v3.5.0)
# FetchContent_MakeAvailable(catch2)
#
# add_executable(unit_tests test_minimization.cpp test_sta.cpp)
# target_link_libraries(unit_tests PRIVATE minimization techmap sta Catch2::Catch2WithMain)
# add_test(NAME unit_tests COMMAND unit_tests)
