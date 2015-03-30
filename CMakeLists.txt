project(mtp)
cmake_minimum_required(VERSION 2.8)
set(CMAKE_USE_RELATIVE_PATHS TRUE)

include(FindPkgConfig)
find_package( Threads )

pkg_check_modules(LIBUSB libusb-1.0 REQUIRED)

option(USE_QT5 "Enable Qt5 interface" OFF)
option(BUILD_QT_UI "Build reference Qt application" ON)

add_definitions(-Wall -pthread -std=c++11)
include_directories(. ${CMAKE_SOURCE_DIR} ${LIBUSB_INCLUDE_DIRS})

set(SOURCES
	mtp/usb/BulkPipe.cpp
	mtp/usb/Context.cpp
	mtp/usb/Device.cpp
	mtp/usb/DeviceDescriptor.cpp
	mtp/usb/Exception.cpp

	mtp/ptp/Device.cpp
	mtp/ptp/ObjectFormat.cpp
	mtp/ptp/Response.cpp
	mtp/ptp/Session.cpp
)

add_library(mtp-ng STATIC ${SOURCES})

add_executable(mtp-test mtp/main.cpp)
target_link_libraries(mtp-test mtp-ng ${LIBUSB_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

set(MTP_LIBRARIES mtp-ng ${LIBUSB_LIBRARIES})
if (BUILD_QT_UI)
	add_subdirectory(qt)
endif()
