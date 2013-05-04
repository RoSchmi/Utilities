cmake_minimum_required(VERSION 2.6)
project(CUtilities C)

set(cutil_sources Cryptography.c Socket.c Thread.c Time.c Common.c)
file(GLOB_RECURSE cutil_headers include/*.h)

include_directories(include)

add_library(CUtilities SHARED ${cutil_sources})

if(MSVC)
  set(CMAKE_C_FLAGS_RELEASE "/arch:AVX /Gr /O2 /MP /Za /Fx /FAs /Fa")
  set(CMAKE_C_FLAGS_DEBUG "/arch:AVX /Od /Za /MP /FAs /Fa /Zi")
endif()

if(NOT WIN32)
  install(FILES ${cutil_headers} DESTINATION include/C-Utilities)
endif()
