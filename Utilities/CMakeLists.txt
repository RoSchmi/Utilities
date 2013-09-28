# vim: tw=1000
cmake_minimum_required(VERSION 2.8.8)
project(Utilities)

set(util_sources Cryptography.cpp DataStream.cpp Misc.cpp
	Socket.cpp TCPConnection.cpp TCPServer.cpp
	WebSocketConnection.cpp SQLDatabase.cpp RequestServer.cpp)

file(GLOB util_headers *.h)

add_library(UtilitiesObjects OBJECT ${util_sources})
add_library(Utilities SHARED $<TARGET_OBJECTS:UtilitiesObjects>)
add_library(UtilitiesStatic STATIC $<TARGET_OBJECTS:UtilitiesObjects>)

# requires clang
set(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++ -lc++abi -fPIC -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-cast-align -Wno-exit-time-destructors -Wno-global-constructors")

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PostgreSQL REQUIRED)

target_link_libraries(Utilities ${CMAKE_THREAD_LIBS_INIT}
	${OPENSSL_LIBRARIES} ${PostgreSQL_LIBRARIES})

include_directories(${PostgreSQL_INCLUDE_DIRS})

if(NOT WIN32)
	target_link_libraries(Utilities rt)
	set_target_properties(UtilitiesStatic PROPERTIES OUTPUT_NAME Utilities)
	install(FILES ${util_headers} DESTINATION include/Utilities)
	install(FILES ${CMAKE_BINARY_DIR}/libUtilities.a DESTINATION lib)
	install(FILES ${CMAKE_BINARY_DIR}/libUtilities.so DESTINATION lib)
endif()
