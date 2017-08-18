add_definitions("-std=c++0x")
set(flvmeta_src
  amf.cpp
  amf.h
  avc.cpp
  avc.h
  dump.cpp
  dump.h
  dump_hls.cpp
  dump_hls.h
  flv.cpp
  flv.h
  flvmeta.cpp
  flvmeta.h
  types.cpp
  types.h
  ngx_rtmp_hls_module.cpp
  ngx_rtmp_hls_module.h
  ngx_rtmp_mpegts.cpp
  ngx_rtmp_mpegts.h
  ngx_linux_config.h
  ngx_win32_config.h
  ${CMAKE_BINARY_DIR}/config.h
)

# add support for getopt and gettext in windows
if(WIN32)
  set(flvmeta_src
    ${flvmeta_src}
    compat/getopt1.c
    compat/getopt.c
    compat/getopt.h
    compat/gettext.h
  )
  include_directories(compat)
endif(WIN32)

# static build
if(WIN32)
  add_definitions(-DYAML_DECLARE_STATIC)
endif(WIN32)

add_executable(flvmeta ${flvmeta_src})
