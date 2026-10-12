# CDD_MotorTA6586
file(GLOB CDD_MotorTA6586_SRCS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

if(CDD_MotorTA6586_SRCS)
  add_library(CDD_MotorTA6586 STATIC ${CDD_MotorTA6586_SRCS})
  set(CDD_MotorTA6586_SCOPE PUBLIC)
else()
  add_library(CDD_MotorTA6586 INTERFACE)
  set(CDD_MotorTA6586_SCOPE INTERFACE)
endif()

target_include_directories(CDD_MotorTA6586 ${CDD_MotorTA6586_SCOPE} ${CMAKE_CURRENT_SOURCE_DIR}/api)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cfg/${HW_VARIANT})
  target_include_directories(CDD_MotorTA6586 ${CDD_MotorTA6586_SCOPE} ${CMAKE_CURRENT_SOURCE_DIR}/cfg/${HW_VARIANT})
endif()

# Phụ thuộc (chỉnh lại nếu sai):
target_link_libraries(CDD_MotorTA6586 ${CDD_MotorTA6586_SCOPE} CDD_Pca9685)
