message(STATUS ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
include(pybind11Tools)

pybind11_add_module(aftl aftl.cpp)
target_link_libraries(aftl PRIVATE ${LIB_NAME})
