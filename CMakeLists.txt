cmake_minimum_required(VERSION 2.8)
project(flvRThls)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

#platform tests
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckIncludeFile)
include(CheckTypeSize)
include(TestBigEndian)

check_include_file(sys/types.h  HAVE_SYS_TYPES_H)
check_include_file(stdint.h     HAVE_STDINT_H)
check_include_file(stddef.h     HAVE_STDDEF_H)
check_include_file(inttypes.h   HAVE_INTTYPES_H)

check_type_size("double"      SIZEOF_DOUBLE)
check_type_size("float"       SIZEOF_FLOAT)
check_type_size("long double" SIZEOF_LONG_DOUBLE)
check_type_size("long"        SIZEOF_LONG)
check_type_size("long long"   SIZEOF_LONG_LONG)

# MSVC before VS 2010 did not have stdint.h
if(MSVC AND NOT HAVE_STDINT_H)
  set(int16_t  1)
  set(int32_t  1)
  set(int64_t  1)
  set(int8_t   1)
  set(uint16_t 1)
  set(uint32_t 1)
  set(uint64_t 1)
  set(uint8_t  1)
endif(MSVC AND NOT HAVE_STDINT_H)

test_big_endian(IS_BIGENDIAN)
if(IS_BIGENDIAN)
  set(WORDS_BIGENDIAN 1)
endif(IS_BIGENDIAN)

# C99 isfinite support
check_symbol_exists("isfinite" math.h HAVE_ISFINITE)

# large file support
check_function_exists("fseeko" HAVE_FSEEKO)
if(HAVE_FSEEKO)
  execute_process(
    COMMAND getconf LFS_CFLAGS
    OUTPUT_VARIABLE LFS_CFLAGS
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if(LFS_CFLAGS)
    add_definitions(${LFS_CFLAGS})
    set(CMAKE_REQUIRED_FLAGS ${LFS_CFLAGS})
  endif(LFS_CFLAGS)

  if(WIN32)
    add_definitions(-D_FILE_OFFSET_BITS=64)
    set(CMAKE_REQUIRED_FLAGS -D_FILE_OFFSET_BITS=64)
  endif(WIN32)

  check_type_size("off_t" SIZEOF_OFF_T)
endif(HAVE_FSEEKO)

# configuration file
configure_file(config-cmake.h.in ${CMAKE_BINARY_DIR}/config.h)
include_directories(${CMAKE_BINARY_DIR})
add_definitions(-DHAVE_CONFIG_H)

# Visual C++ specific configuration
if(MSVC)
  # use static library
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")

  # C runtime deprecation in Visual C++ 2005 and later
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
endif(MSVC)

add_subdirectory(src)
