cmake_minimum_required(VERSION 2.8)
project(Utilities)

set(util_sources Cryptography.cpp DataStream.cpp Misc.cpp
	Semaphore.cpp Socket.cpp TCPConnection.cpp TCPServer.cpp
	WebSocketConnection.cpp SQLDatabase.cpp RequestServer.cpp)

file(GLOB util_headers *.h)

add_library(Utilities STATIC ${util_sources})

set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -pedantic -Wno-overloaded-virtual ${CMAKE_CXX_FLAGS}")

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PostgreSQL REQUIRED)

target_link_libraries(Utilities ${CMAKE_THREAD_LIBS_INIT}
	${OPENSSL_LIBRARIES} ${PostgreSQL_LIBRARIES})

include_directories(${PostgreSQL_INCLUDE_DIRS})

if(NOT WIN32)
	target_link_libraries(Utilities rt)
	install(FILES ${util_headers} DESTINATION include/Utilities)
	install(FILES ${CMAKE_BINARY_DIR}/libUtilities.a DESTINATION lib)
endif()
