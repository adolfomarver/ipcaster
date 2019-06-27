cmake_minimum_required(VERSION 3.0)
 
# Locate GTest
#find_package(GTest REQUIRED)
#include_directories(${GTEST_INCLUDE_DIRS})
 
# Link runTests with what we want to test and the GTest and pthread library
add_executable(tests tests.cpp)
target_include_directories(tests PRIVATE ../)
#target_link_libraries(tests ${GTEST_LIBRARIES} pthread boost_system)
target_link_libraries(tests pthread boost_system)
#target_link_libraries(tests boost_system)
#target_include_directories(tests PRIVATE "C:/Program Files (x86)/boost_1_70_0")
#link_directories("C:/Program Files (x86)/boost_1_70_0/stage/lib")

add_test(NAME all_tests COMMAND tests)