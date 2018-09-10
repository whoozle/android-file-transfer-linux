project(android-file-transfer)

set(VERSION_MAJOR "3")
set(VERSION_MINOR "6")
set(VERSION_PATCH "0")
set(VERSION_COUNT 3)
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

message(STATUS "building for ${CMAKE_SYSTEM_NAME}")

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(USB_BACKEND_DARWIN TRUE)
endif()

cmake_minimum_required(VERSION 2.8)
set(CMAKE_USE_RELATIVE_PATHS TRUE)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

include(FindPkgConfig)
include(CheckFunctionExists)
include(CheckIncludeFiles)

find_package ( Threads )

# Don't mistake osx for unix
if(UNIX AND NOT APPLE)
  set(UNIX_SYS ON)
else()
  set(UNIX_SYS OFF)
endif()

option(BUILD_FUSE "Build fuse mount helper" ${UNIX_SYS})

if (BUILD_FUSE)
	pkg_check_modules ( FUSE fuse )
endif()

if (FUSE_FOUND)
	message(STATUS "fuse found, building mount helper")
	add_definitions(${FUSE_CFLAGS} -DFUSE_USE_VERSION=26)
endif()

check_include_files (magic.h HAVE_MAGIC_H)
check_library_exists(magic magic_open "" HAVE_LIBMAGIC)

if (HAVE_MAGIC_H AND HAVE_LIBMAGIC)
	message(STATUS "libmagic found")
	add_definitions(-DHAVE_LIBMAGIC)
	list(APPEND MTP_LIBRARIES magic)
	list(APPEND MTP_SHARED_LIBRARIES magic)
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
include_directories(mtp/backend/posix)

set(SOURCES
	mtp/log.cpp
	mtp/ByteArray.cpp
	mtp/ptp/Device.cpp
	mtp/ptp/ObjectFormat.cpp
	mtp/ptp/PipePacketer.cpp
	mtp/ptp/Response.cpp
	mtp/ptp/Session.cpp

	mtp/usb/DeviceBusyException.cpp
	mtp/usb/BulkPipe.cpp
	mtp/usb/Request.cpp

	mtp/backend/posix/FileHandler.cpp
	mtp/backend/posix/Exception.cpp
)

exec_program(git ARGS describe --tags OUTPUT_VARIABLE AFT_VERSION RETURN_VALUE GIT_STATUS)
if (GIT_STATUS GREATER 0)
	file(READ ${CMAKE_SOURCE_DIR}/README.md README_MD)
	string(REGEX MATCH "version-([0-9a-fA-F.]+)-" _VERSION ${README_MD})
	set(AFT_VERSION "v${CMAKE_MATCH_1}-snapshot")
endif()
message(STATUS "version: ${AFT_VERSION}")

set(VERSION_SRC ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/version.cpp)
configure_file(mtp/version.cpp.in ${VERSION_SRC})

list(APPEND SOURCES ${VERSION_SRC})

if (USB_BACKEND_LIBUSB)

	pkg_check_modules(LIBUSB libusb-1.0 REQUIRED)
	include_directories(${LIBUSB_INCLUDE_DIRS})
	include_directories(mtp/backend/libusb)
	add_definitions(-DUSB_BACKEND_LIBUSB)

	list(APPEND SOURCES
		mtp/backend/libusb/usb/Context.cpp
		mtp/backend/libusb/usb/Device.cpp
		mtp/backend/libusb/usb/DeviceDescriptor.cpp
		mtp/backend/libusb/usb/Exception.cpp
	)
	list(APPEND MTP_LIBRARIES ${LIBUSB_LIBRARIES})
elseif (USB_BACKEND_DARWIN)
	find_library(CORE_LIBRARY CoreFoundation)
	find_library(IOKIT_LIBRARY IOKit)
	list(APPEND MTP_LIBRARIES ${IOKIT_LIBRARY} ${CORE_LIBRARY})
	include_directories(mtp/backend/darwin)
	list(APPEND SOURCES
		mtp/backend/darwin/usb/Context.cpp
		mtp/backend/darwin/usb/Device.cpp
		mtp/backend/darwin/usb/DeviceDescriptor.cpp
		mtp/backend/darwin/usb/Exception.cpp
		mtp/backend/darwin/usb/Interface.cpp
	)
else()
	include_directories(mtp/backend/linux)
	list(APPEND SOURCES
		mtp/backend/linux/usb/Endpoint.cpp
		mtp/backend/linux/usb/Context.cpp
		mtp/backend/linux/usb/Device.cpp
		mtp/backend/linux/usb/Interface.cpp
		mtp/backend/linux/usb/DeviceDescriptor.cpp
	)
endif()


if (BUILD_SHARED_LIB)
	add_library(mtp-ng SHARED ${SOURCES})
	set_target_properties(mtp-ng PROPERTIES SOVERSION ${VERSION})
	target_link_libraries(mtp-ng ${CMAKE_THREAD_LIBS_INIT} ${MTP_SHARED_LIBRARIES})

	if (USB_BACKEND_LIBUSB)
		target_link_libraries(mtp-ng ${LIBUSB_LIBRARIES})
	endif()

	list(INSERT MTP_LIBRARIES 0 mtp-ng)
	install(TARGETS mtp-ng LIBRARY DESTINATION "lib${LIB_SUFFIX}" ARCHIVE DESTINATION "lib${LIB_SUFFIX}")
else (BUILD_SHARED_LIB)
	add_library(mtp-ng-static STATIC ${SOURCES})
	target_link_libraries(mtp-ng-static ${MTP_LIBRARIES})
	list(INSERT MTP_LIBRARIES 0 mtp-ng-static)
	install(TARGETS mtp-ng-static LIBRARY DESTINATION "lib${LIB_SUFFIX}" ARCHIVE DESTINATION "lib${LIB_SUFFIX}")
endif (BUILD_SHARED_LIB)

add_definitions(-D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64)

add_subdirectory(cli)

if (FUSE_FOUND)
	add_subdirectory(fuse)
endif()

if (BUILD_QT_UI)
	add_subdirectory(qt)
endif()

add_custom_target(uninstall COMMAND xargs rm < install_manifest.txt)
