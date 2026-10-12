# SWC_Robot
file(GLOB SWC_Robot_SRCS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

if(SWC_Robot_SRCS)
  add_library(SWC_Robot STATIC ${SWC_Robot_SRCS})
  set(SWC_Robot_SCOPE PUBLIC)
else()
  add_library(SWC_Robot INTERFACE)
  set(SWC_Robot_SCOPE INTERFACE)
endif()

target_include_directories(SWC_Robot ${SWC_Robot_SCOPE} ${CMAKE_CURRENT_SOURCE_DIR}/api)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cfg/${HW_VARIANT})
  target_include_directories(SWC_Robot ${SWC_Robot_SCOPE} ${CMAKE_CURRENT_SOURCE_DIR}/cfg/${HW_VARIANT})
endif()

# Phụ thuộc (chỉnh lại nếu sai):
target_link_libraries(SWC_Robot ${SWC_Robot_SCOPE} SWC_Drive CDD_MotorTA6586 CDD_Pca9685 SRV_EspNow SRV_Log)
