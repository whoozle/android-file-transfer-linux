project(mtp)
cmake_minimum_required(VERSION 2.8)
set(CMAKE_USE_RELATIVE_PATHS TRUE)

include(FindPkgConfig)
include(CheckFunctionExists)

find_package ( Threads )
pkg_check_modules ( FUSE fuse )

if (FUSE_FOUND)
	message(STATUS "fuse found, building mount helper")
	add_definitions(${FUSE_CFLAGS} -DFUSE_USE_VERSION=26)
endif()

option(BUILD_QT_UI "Build reference Qt application" ON)
option(BUILD_SHARED_LIB "Build shared library" OFF)

option(USB_BACKEND_LIBUSB "Use libusb-1.0" OFF)
if (USB_BACKEND_LIBUSB)
	message(WARNING "WARNING! You're using libusb, this is known to be broken -- large memory consumption, violating kernel memory limits and bugs. Continue at your own risk")
endif()

add_definitions(-Wall -pthread -std=c++11)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

if (CMAKE_BUILD_TYPE STREQUAL Debug)
	add_definitions(-ggdb)
endif()

include_directories(. ${CMAKE_SOURCE_DIR})

set(SOURCES
	mtp/ptp/Device.cpp
	mtp/ptp/ObjectFormat.cpp
	mtp/ptp/PipePacketer.cpp
	mtp/ptp/Response.cpp
	mtp/ptp/Session.cpp

	mtp/usb/BulkPipe.cpp
)

set(MTP_LIBRARIES mtp-ng-static)

if (USB_BACKEND_LIBUSB)

	pkg_check_modules(LIBUSB libusb-1.0 REQUIRED)
	include_directories(${LIBUSB_INCLUDE_DIRS})
	include_directories(mtp/backend/libusb)

	list(APPEND SOURCES
		mtp/backend/libusb/usb/Context.cpp
		mtp/backend/libusb/usb/Device.cpp
		mtp/backend/libusb/usb/DeviceDescriptor.cpp
		mtp/backend/libusb/usb/Exception.cpp
	)
	list(APPEND MTP_LIBRARIES ${LIBUSB_LIBRARIES})
else()
	include_directories(mtp/backend/linux)
	list(APPEND SOURCES
		mtp/backend/linux/usb/Exception.cpp
		mtp/backend/linux/usb/Context.cpp
		mtp/backend/linux/usb/Device.cpp
		mtp/backend/linux/usb/Interface.cpp
		mtp/backend/linux/usb/DeviceDescriptor.cpp
	)
endif()


add_library(mtp-ng-static STATIC ${SOURCES})
if (BUILD_SHARED_LIB)
	add_library(mtp-ng SHARED ${SOURCES})
	target_link_libraries(mtp-ng ${CMAKE_THREAD_LIBS_INIT})

	if (USB_BACKEND_LIBUSB)
		target_link_libraries(mtp-ng ${LIBUSB_LIBRARIES})
	endif()
endif (BUILD_SHARED_LIB)

check_function_exists(stat64 USE_STAT64)
if (USE_STAT64)
	add_definitions(-DMTP_USES_STAT64=1)
endif()

add_executable(aft-mtp-cli mtp/cli.cpp)
target_link_libraries(aft-mtp-cli ${MTP_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

if (FUSE_FOUND)
	add_executable(aft-mtp-mount mtp/fuse.cpp)
	target_link_libraries(aft-mtp-mount ${MTP_LIBRARIES} ${FUSE_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
endif()

if (BUILD_QT_UI)
	add_subdirectory(qt)
endif()
