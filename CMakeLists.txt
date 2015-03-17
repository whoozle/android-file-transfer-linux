project(mtp)
cmake_minimum_required(VERSION 2.8)
set(CMAKE_USE_RELATIVE_PATHS TRUE)

include(FindPkgConfig)
find_package( Threads )

pkg_check_modules(LIBUSB libusb REQUIRED)

add_definitions(-Wall -pthread -std=c++1y)
include_directories(. ${CMAKE_SOURCE_DIR} ${LIBUSB_INCLUDE_DIRS})

set(SOURCES
	mtp/usb/BulkPipe.cpp
	mtp/usb/Context.cpp
	mtp/usb/Device.cpp
	mtp/usb/DeviceDescriptor.cpp
	mtp/usb/Exception.cpp

	mtp/ptp/Device.cpp
	mtp/ptp/Session.cpp

	mtp/main.cpp
)
add_library(mtp-ng STATIC ${SOURCES})

add_executable(mtp-test mtp/main.cpp)
target_link_libraries(mtp-test mtp-ng -lusb-1.0 ${CMAKE_THREAD_LIBS_INIT})
