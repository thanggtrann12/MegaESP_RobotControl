# Common: header-only shared utilities (Crc.h, Eeprom_Map.h)
add_library(Common INTERFACE)
target_include_directories(Common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/api)
