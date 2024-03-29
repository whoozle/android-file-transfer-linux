find_package(Readline)
include(CheckCSourceCompiles)

set(CLI_SOURCES
	Command.cpp
	Session.cpp
	Tokenizer.cpp
	arg_lexer.l.cpp
	cli.cpp)

if (READLINE_FOUND)
	set(CMAKE_REQUIRED_INCLUDES ${Readline_INCLUDE_DIR})
	set(CMAKE_REQUIRED_LIBRARIES ${Readline_LIBRARY})
	set(READLINE_TEST_SRC "
#include <stdio.h>
#include <readline.h>

int main(int argc, char **argv) {
	char *line = readline(\">\");
	return 0;
}
")
	check_c_source_compiles("${READLINE_TEST_SRC}" READLINE_NO_CURSES_TEST)
	if (NOT READLINE_NO_CURSES_TEST)
		message(STATUS "readline test failed - some systems expect you to link with termcap or curses libraries")
		message(STATUS "please find an example there: https://bugzilla.redhat.com/show_bug.cgi?id=499837")
		message(STATUS "trying to find and link curses library...")
#		set(CURSES_NEED_NCURSES TRUE)
		include(FindCurses)
		list(APPEND CMAKE_REQUIRED_LIBRARIES ${CURSES_LIBRARY})
		check_c_source_compiles("${READLINE_TEST_SRC}" READLINE_CURSES_TEST)
		if (READLINE_CURSES_TEST)
			list(APPEND Readline_LIBRARY ${CURSES_LIBRARIES})
		else()
			set(READLINE_FOUND FALSE) #disabling readline completely
		endif()
	endif()

	set(CMAKE_REQUIRED_INCLUDES)
	set(CMAKE_REQUIRED_LIBRARIES)
endif()

if (READLINE_FOUND)
	list(APPEND CLI_SOURCES CommandLine.cpp)
	list(APPEND CLI_LIBRARIES ${Readline_LIBRARY})
else()
	list(APPEND CLI_SOURCES CommandLineStub.cpp)
	message(WARNING "no readline library found, using stub")
endif()

add_executable(aft-mtp-cli ${CLI_SOURCES})
target_include_directories(aft-mtp-cli SYSTEM PRIVATE ${Readline_INCLUDE_DIR})
target_link_libraries(aft-mtp-cli ${MTP_LIBRARIES} ${CLI_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
install(TARGETS aft-mtp-cli RUNTIME DESTINATION bin)

if (BUILD_QT_UI AND (${CMAKE_SYSTEM_NAME} MATCHES "Darwin"))
	add_custom_command(TARGET aft-mtp-cli POST_BUILD COMMAND ${CMAKE_INSTALL_NAME_TOOL} -add_rpath "@executable_path/../../Frameworks/" $<TARGET_FILE:aft-mtp-cli>)
	add_custom_command(TARGET aft-mtp-cli POST_BUILD COMMAND ${OTOOL_BIN} -L $<TARGET_FILE:aft-mtp-cli>)
	set(AFT_MTP_CLI_PATH "${MACOSX_BUNDLE_ROOT_DIR}/Contents/SharedSupport/bin/aft-mtp-cli")
	set(MACOSX_BUNDLE_LIBS_INSTALL "
		${MACOSX_BUNDLE_LIBS_INSTALL}
		file(INSTALL \"${CMAKE_CURRENT_BINARY_DIR}/aft-mtp-cli\" DESTINATION \"${MACOSX_BUNDLE_ROOT_DIR}/Contents/SharedSupport/bin\")
	")
	foreach(DYNLIB ${MACOSX_BUNDLE_LIBS})
		get_filename_component(DYNLIB_NAME "${DYNLIB}" NAME)
		set(MACOSX_BUNDLE_LIBS_INSTALL "
			${MACOSX_BUNDLE_LIBS_INSTALL}
			message(STATUS \"cli: fixing ${DYNLIB_NAME}\")
			execute_process(COMMAND \"${CMAKE_INSTALL_NAME_TOOL}\" -change \"${DYNLIB}\" \"@executable_path/../../Frameworks/${DYNLIB_NAME}\" \"${AFT_MTP_CLI_PATH}\")
		")
	endforeach()
	set(MACOSX_BUNDLE_LIBS_INSTALL "
		${MACOSX_BUNDLE_LIBS_INSTALL}
		execute_process(COMMAND \"${OTOOL_BIN}\" -L \"${AFT_MTP_CLI_PATH}\")
	" PARENT_SCOPE)
endif()
