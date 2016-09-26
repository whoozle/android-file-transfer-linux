find_package(Readline)

set(CLI_SOURCES
	Command.cpp
	Session.cpp
	Tokenizer.cpp
	arg_lexer.l.cpp
	cli.cpp)

if (READLINE_FOUND)
	list(APPEND CLI_SOURCES CommandLine.cpp)
	list(APPEND CLI_LIBRARIES ${Readline_LIBRARY})
else()
	list(APPEND CLI_SOURCES CommandLineStub.cpp)
	message(WARNING "no readline library found")
endif()

add_executable(aft-mtp-cli ${CLI_SOURCES})
target_link_libraries(aft-mtp-cli ${MTP_LIBRARIES} ${CLI_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
install(TARGETS aft-mtp-cli RUNTIME DESTINATION bin)
