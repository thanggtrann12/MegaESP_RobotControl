# SWC_Drive
file(GLOB SWC_Drive_SRCS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

if(SWC_Drive_SRCS)
  add_library(SWC_Drive STATIC ${SWC_Drive_SRCS})
  set(SWC_Drive_SCOPE PUBLIC)
else()
  add_library(SWC_Drive INTERFACE)
  set(SWC_Drive_SCOPE INTERFACE)
endif()

target_include_directories(SWC_Drive ${SWC_Drive_SCOPE} ${CMAKE_CURRENT_SOURCE_DIR}/api)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cfg/${HW_VARIANT})
  target_include_directories(SWC_Drive ${SWC_Drive_SCOPE} ${CMAKE_CURRENT_SOURCE_DIR}/cfg/${HW_VARIANT})
endif()

# Phụ thuộc (chỉnh lại nếu sai):
target_link_libraries(SWC_Drive ${SWC_Drive_SCOPE} CDD_MotorTA6586)
