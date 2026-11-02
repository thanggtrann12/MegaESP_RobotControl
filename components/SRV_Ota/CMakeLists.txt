file(GLOB SRV_Ota_SRCS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

if(SRV_Ota_SRCS)
  add_library(SRV_Ota STATIC ${SRV_Ota_SRCS})
  set(SRV_Ota_SCOPE PUBLIC)
else()
  add_library(SRV_Ota INTERFACE)
  set(SRV_Ota_SCOPE INTERFACE)
endif()

target_include_directories(SRV_Ota ${SRV_Ota_SCOPE} ${CMAKE_CURRENT_SOURCE_DIR}/api)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cfg/${HW_VARIANT})
  target_include_directories(SRV_Ota ${SRV_Ota_SCOPE} ${CMAKE_CURRENT_SOURCE_DIR}/cfg/${HW_VARIANT})
endif()

target_link_libraries(SRV_Ota ${SRV_Ota_SCOPE} SRV_Log)
