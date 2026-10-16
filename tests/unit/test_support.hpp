#pragma once
#include <iostream>

namespace test_support {
inline int& failures() {
    static int n = 0;
    return n;
}
inline int& checks() {
    static int n = 0;
    return n;
}
} // namespace test_support

#define CHECK(cond)                                                                      \
    do {                                                                                 \
        ++test_support::checks();                                                        \
        if (!(cond)) {                                                                   \
            ++test_support::failures();                                                  \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #cond "\n";   \
        }                                                                                \
    } while (0)

#define TEST_SUMMARY(name)                                                                         \
    (std::cout << name << ": " << test_support::checks() << " checks, " << test_support::failures() \
               << " failures\n",                                                                    \
     test_support::failures() == 0 ? 0 : 1)
