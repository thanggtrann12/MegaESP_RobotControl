# SRV_Hmi
file(GLOB SRV_Hmi_SRCS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

if(SRV_Hmi_SRCS)
  add_library(SRV_Hmi STATIC ${SRV_Hmi_SRCS})
  set(SRV_Hmi_SCOPE PUBLIC)
else()
  add_library(SRV_Hmi INTERFACE)
  set(SRV_Hmi_SCOPE INTERFACE)
endif()

target_include_directories(SRV_Hmi ${SRV_Hmi_SCOPE} ${CMAKE_CURRENT_SOURCE_DIR}/api)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cfg/${HW_VARIANT})
  target_include_directories(SRV_Hmi ${SRV_Hmi_SCOPE} ${CMAKE_CURRENT_SOURCE_DIR}/cfg/${HW_VARIANT})
endif()

# Phụ thuộc (chỉnh lại nếu sai):
target_link_libraries(SRV_Hmi ${SRV_Hmi_SCOPE} SWC_Robot CDD_Gpio SRV_Log)
