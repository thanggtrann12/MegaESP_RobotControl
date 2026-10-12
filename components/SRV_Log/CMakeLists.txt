# SRV_Log
file(GLOB SRV_Log_SRCS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

if(SRV_Log_SRCS)
  add_library(SRV_Log STATIC ${SRV_Log_SRCS})
  set(SRV_Log_SCOPE PUBLIC)
else()
  add_library(SRV_Log INTERFACE)
  set(SRV_Log_SCOPE INTERFACE)
endif()

target_include_directories(SRV_Log ${SRV_Log_SCOPE} ${CMAKE_CURRENT_SOURCE_DIR}/api)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cfg/${HW_VARIANT})
  target_include_directories(SRV_Log ${SRV_Log_SCOPE} ${CMAKE_CURRENT_SOURCE_DIR}/cfg/${HW_VARIANT})
endif()
