project(mtp)
cmake_minimum_required(VERSION 2.8)
set(CMAKE_USE_RELATIVE_PATHS TRUE)

include(FindPkgConfig)
find_package( Threads )

option(BUILD_QT_UI "Build reference Qt application" ON)
option(BUILD_SHARED_LIB "Build shared library" OFF)
option(USE_LIBUSB "Use libusb-1.0" ON)

add_definitions(-Wall -pthread -std=c++11)
include_directories(. ${CMAKE_SOURCE_DIR})

set(SOURCES
	mtp/ptp/Device.cpp
	mtp/ptp/ObjectFormat.cpp
	mtp/ptp/PipePacketer.cpp
	mtp/ptp/Response.cpp
	mtp/ptp/Session.cpp
)

set(MTP_LIBRARIES mtp-ng-static)

if (USE_LIBUSB)

	pkg_check_modules(LIBUSB libusb-1.0 REQUIRED)
	include_directories(${LIBUSB_INCLUDE_DIRS})
	include_directories(mtp/backend/libusb)

	list(APPEND SOURCES
		mtp/backend/libusb/usb/BulkPipe.cpp
		mtp/backend/libusb/usb/Context.cpp
		mtp/backend/libusb/usb/Device.cpp
		mtp/backend/libusb/usb/DeviceDescriptor.cpp
		mtp/backend/libusb/usb/Exception.cpp
	)
	list(APPEND MTP_LIBRARIES ${LIBUSB_LIBRARIES})
endif()


add_library(mtp-ng-static STATIC ${SOURCES})
if (BUILD_SHARED_LIB)
	add_library(mtp-ng SHARED ${SOURCES})
	target_link_libraries(mtp-ng ${CMAKE_THREAD_LIBS_INIT})

	if (USE_LIBUSB)
		target_link_libraries(mtp-ng ${LIBUSB_LIBRARIES})
	endif()
endif (BUILD_SHARED_LIB)

add_executable(mtp-test mtp/main.cpp)
target_link_libraries(mtp-test ${MTP_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

if (BUILD_QT_UI)
	add_subdirectory(qt)
endif()
