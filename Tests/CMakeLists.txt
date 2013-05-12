cmake_minimum_required(VERSION 2.6)
project(UtilitiesTests)

enable_testing()

set(util_test_sources Main.cpp Array.cpp Cryptography.cpp DataStream.cpp)

add_executable(RunTests ${util_test_sources})

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "-std=c++11 -flto")
endif()

find_package(GTest REQUIRED)

target_link_libraries(RunTests ${GTEST_LIBRARIES} Utilities)
include_directories(${GTEST_INCLUDE_DIR})

add_test(UtilitiesTests RunTests)
