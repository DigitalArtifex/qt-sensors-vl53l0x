cmake_minimum_required(VERSION 3.16)

include(CMakePackageConfigHelpers)

project(QVL53L0X LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(USE_SYSTEM_PATH OFF)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS
  Core
  Sensors
)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
  Core
  Sensors
)

if(USE_SYSTEM_PATH)
  set(INSTALL_PREFIX "/usr")
  set(INSTALL_LIB_PATH "${INSTALL_PREFIX}/lib/${CMAKE_CXX_LIBRARY_ARCHITECTURE}/qt${QT_VERSION_MAJOR}")
  set(INSTALL_INCLUDE_PATH "${INSTALL_PREFIX}/include/${CMAKE_CXX_LIBRARY_ARCHITECTURE}/qt${QT_VERSION_MAJOR}")
  set(INSTALL_PLUGIN_PATH "${INSTALL_PREFIX}/include/${CMAKE_CXX_LIBRARY_ARCHITECTURE}/qt${QT_VERSION_MAJOR}/plugins")
  message("Setting Install Path to /usr")
else()
  message("Setting Install Path to CMAKE_PREFIX_PATH")
  if("${CMAKE_PREFIX_PATH}" MATCHES ";|:")
    list(GET CMAKE_PREFIX_PATH 0 INSTALL_PREFIX)
  else()
    message("--${CMAKE_PREFIX_PATH}")
    set(INSTALL_PREFIX "${CMAKE_PREFIX_PATH}")
  endif()

  #this happens when installing to a qt dev env
  if("${INSTALL_PREFIX}" MATCHES "/lib/cmake")
    string(REPLACE "/lib/cmake" "" INSTALL_PREFIX "${INSTALL_PREFIX}")
  endif()

  set(INSTALL_LIB_PATH "${INSTALL_PREFIX}/lib")
  set(INSTALL_INCLUDE_PATH "${INSTALL_PREFIX}/include")
  set(INSTALL_PLUGIN_PATH "${INSTALL_PREFIX}/plugins")

endif()

set(CMAKE_PREFIX_PATH ${INSTALL_PREFIX} CACHE PATH "Installation prefix" FORCE)

#setup lib
set(OUTPUT_NAME "qt${QT_VERSION_MAJOR}-sensors-vl53l0x")

set(COMMON_HEADERS
  qvl53l0x_global.h
  qvl53l0x.h
  qvl53l0xreading.h
  qvl53l0x_p.h
  qvl53l0xbackend.h
)

set(COMMON_SOURCES
  qvl53l0x.cpp
  qvl53l0xreading.cpp
  qvl53l0xbackend.cpp
)

add_library(${OUTPUT_NAME} SHARED
  ${COMMON_HEADERS}
  ${COMMON_SOURCES}
  #QMLX90615
)

set(INSTALL_HEADERS
  ${COMMON_HEADERS}
  #QMLX90615
)

target_link_libraries(${OUTPUT_NAME} PRIVATE
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Sensors
  i2c
)

target_compile_definitions(${OUTPUT_NAME} PRIVATE QVL53L_X_LIBRARY)

#setup lib install
target_sources(${OUTPUT_NAME} PUBLIC FILE_SET HEADERS BASE_DIRS ${CMAKE_SOURCE_DIR} FILES ${INSTALL_HEADERS})

install(
  TARGETS ${OUTPUT_NAME}
  FILE_SET
  HEADERS
  DESTINATION ${INSTALL_INCLUDE_PATH}/QVL53L0X
)
message("Headers Install Location: ${INSTALL_INCLUDE_PATH}/QVL53L0X")

install(
  TARGETS ${OUTPUT_NAME}
  DESTINATION ${INSTALL_LIB_PATH}
  NAMELINK_COMPONENT
)
message("Library Install Location: ${INSTALL_LIB_PATH}")

#setup plugin
set(PLUGIN_NAME "qt${QT_VERSION_MAJOR}-sensors-vl53l0x-plugin")

add_library(${PLUGIN_NAME} SHARED
  ${COMMON_HEADERS}
  ${COMMON_SOURCES}
  qvl53l0xplugin.h
)

target_link_libraries(${PLUGIN_NAME} PRIVATE
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Sensors
  i2c
)

#setup plugin install
install(
  TARGETS ${PLUGIN_NAME}
  DESTINATION ${INSTALL_PLUGIN_PATH}/sensors/
  NAMELINK_COMPONENT
)

message("Plugin Install Location: ${INSTALL_PLUGIN_PATH}/sensors/")
