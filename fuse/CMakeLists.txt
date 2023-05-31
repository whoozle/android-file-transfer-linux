add_executable(aft-mtp-mount fuse.cpp)

target_link_libraries(aft-mtp-mount ${MTP_LIBRARIES} ${FUSE_LDFLAGS} ${CMAKE_THREAD_LIBS_INIT})
install(TARGETS aft-mtp-mount RUNTIME DESTINATION bin)

if (BUILD_QT_UI AND (${CMAKE_SYSTEM_NAME} MATCHES "Darwin"))
	add_custom_command(TARGET aft-mtp-mount POST_BUILD COMMAND ${CMAKE_INSTALL_NAME_TOOL} -add_rpath "@executable_path/../../Frameworks/" $<TARGET_FILE:aft-mtp-mount>)
	add_custom_command(TARGET aft-mtp-mount POST_BUILD COMMAND ${OTOOL_BIN} -L $<TARGET_FILE:aft-mtp-mount>)
	set(AFT_MTP_MOUNT_PATH "${MACOSX_BUNDLE_ROOT_DIR}/Contents/SharedSupport/bin/aft-mtp-mount")
	set(MACOSX_BUNDLE_LIBS_INSTALL "
		${MACOSX_BUNDLE_LIBS_INSTALL}
		file(INSTALL \"${CMAKE_CURRENT_BINARY_DIR}/aft-mtp-mount\" DESTINATION \"${MACOSX_BUNDLE_ROOT_DIR}/Contents/SharedSupport/bin\")
	")
	foreach(DYNLIB ${MACOSX_BUNDLE_LIBS})
		get_filename_component(DYNLIB_NAME "${DYNLIB}" NAME)
		set(MACOSX_BUNDLE_LIBS_INSTALL "
			${MACOSX_BUNDLE_LIBS_INSTALL}
			message(STATUS \"fuse: fixing ${DYNLIB_NAME}\")
			execute_process(COMMAND \"${CMAKE_INSTALL_NAME_TOOL}\" -change \"${DYNLIB}\" \"@executable_path/../../Frameworks/${DYNLIB_NAME}\" \"${AFT_MTP_MOUNT_PATH}\")
			")
	endforeach()

	set(MACOSX_BUNDLE_LIBS_INSTALL "
		${MACOSX_BUNDLE_LIBS_INSTALL}
		execute_process(COMMAND \"${OTOOL_BIN}\" -L \"${AFT_MTP_MOUNT_PATH}\")
	" PARENT_SCOPE)

endif()
