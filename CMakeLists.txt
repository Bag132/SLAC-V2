# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)
# set(__PROJECT_GROUP_LINK_COMPONENTS ON)
# set(PLANE_COMPONENT_DIRS ${PLANE_COMPONENT_DIRS}
#                         "./components/VL53L3CX_API")
# set(EXTRA_COMPONENT_DIRS "${EXTRA_COMPONENT_DIRS} ${PLANE_COMPONENT_DIRS}")
include($ENV{HOMEPATH}/esp/esp-idf/tools/cmake/project.cmake)
# include_directories(main/VL53L3CX_API/core/inc main/VL53L3CX_API/platform/inc)
# link_directories(main/VL53L3CX_API/core/inc main/VL53L3CX_API/platform/inc)
project(slacv2)
