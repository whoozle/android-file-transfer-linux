add_executable(aft-mtp-mount fuse_ll.cpp)

target_link_libraries(aft-mtp-mount ${MTP_LIBRARIES} ${FUSE_LDFLAGS} ${CMAKE_THREAD_LIBS_INIT})
install(TARGETS aft-mtp-mount RUNTIME DESTINATION bin)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	add_custom_command(TARGET aft-mtp-mount POST_BUILD COMMAND ${CMAKE_INSTALL_NAME_TOOL} -add_rpath "@executable_path/../Frameworks/" $<TARGET_FILE:aft-mtp-mount>)
endif()
