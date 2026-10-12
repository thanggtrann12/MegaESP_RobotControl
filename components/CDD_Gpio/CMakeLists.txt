# CDD_Gpio
file(GLOB CDD_Gpio_SRCS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

if(CDD_Gpio_SRCS)
  add_library(CDD_Gpio STATIC ${CDD_Gpio_SRCS})
  set(CDD_Gpio_SCOPE PUBLIC)
else()
  add_library(CDD_Gpio INTERFACE)
  set(CDD_Gpio_SCOPE INTERFACE)
endif()

target_include_directories(CDD_Gpio ${CDD_Gpio_SCOPE} ${CMAKE_CURRENT_SOURCE_DIR}/api)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cfg/${HW_VARIANT})
  target_include_directories(CDD_Gpio ${CDD_Gpio_SCOPE} ${CMAKE_CURRENT_SOURCE_DIR}/cfg/${HW_VARIANT})
endif()
