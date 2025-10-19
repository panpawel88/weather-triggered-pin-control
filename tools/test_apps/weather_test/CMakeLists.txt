cmake_minimum_required(VERSION 3.16)

# Add parent components directory to search path
set(EXTRA_COMPONENT_DIRS "../../../components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(weather_test)
