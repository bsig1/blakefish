# Download GoogleTest if not already available
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/release-1.12.1.zip
)
FetchContent_MakeAvailable(googletest)

# Create test executable
add_executable(tests
        test_movegen.cpp
)

# Link the C code and GoogleTest
target_link_libraries(tests
        blakefish
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(tests)
