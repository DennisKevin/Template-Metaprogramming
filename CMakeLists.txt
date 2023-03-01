
cmake_minimum_required (VERSION 3.12)
project ("TypeList")
include_directories(include)

add_executable (TypeList  "TypeList.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET TypeList PROPERTY CXX_STANDARD 20)
endif()
