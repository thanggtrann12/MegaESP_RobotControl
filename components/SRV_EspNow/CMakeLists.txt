# SRV_EspNow
file(GLOB SRV_EspNow_SRCS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

if(SRV_EspNow_SRCS)
  add_library(SRV_EspNow STATIC ${SRV_EspNow_SRCS})
  set(SRV_EspNow_SCOPE PUBLIC)
else()
  add_library(SRV_EspNow INTERFACE)
  set(SRV_EspNow_SCOPE INTERFACE)
endif()

target_include_directories(SRV_EspNow ${SRV_EspNow_SCOPE} ${CMAKE_CURRENT_SOURCE_DIR}/api)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cfg/${HW_VARIANT})
  target_include_directories(SRV_EspNow ${SRV_EspNow_SCOPE} ${CMAKE_CURRENT_SOURCE_DIR}/cfg/${HW_VARIANT})
endif()

# Phụ thuộc (chỉnh lại nếu sai):
target_link_libraries(SRV_EspNow ${SRV_EspNow_SCOPE} SRV_Log)
