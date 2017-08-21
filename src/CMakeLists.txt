#execute_process(COMMAND rm ${PROJECT_SOURCE_DIR}/src/cmdline.h ${PROJECT_SOURCE_DIR}/src/cmdline.cpp)
execute_process(COMMAND gengetopt --input=${PROJECT_SOURCE_DIR}/src/cmdline.ggo --include-getopt --output-dir=${PROJECT_SOURCE_DIR}/src)
execute_process(COMMAND mv ${PROJECT_SOURCE_DIR}/src/cmdline.c ${PROJECT_SOURCE_DIR}/src/cmdline.cpp)

add_definitions("-std=c++0x")
set(flvmeta_src
  cmdline.cpp
  cmdline.h
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

# static build
if(WIN32)
  add_definitions(-DYAML_DECLARE_STATIC)
endif(WIN32)

add_executable(flvmeta ${flvmeta_src})
