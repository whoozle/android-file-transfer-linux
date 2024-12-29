if(DESIRED_QT_VERSION)
	if(DESIRED_QT_VERSION MATCHES 4)
		find_package(Qt4 QUIET)
	elseif(DESIRED_QT_VERSION MATCHES 5)
		find_package(Qt5Widgets QUIET)
		find_package(Qt5Network QUIET)
		find_package(Qt5LinguistTools QUIET)
	else()
		find_package(Qt6Widgets QUIET)
		find_package(Qt6Network QUIET)
		find_package(Qt6LinguistTools QUIET)
	endif()
else()
	find_package(Qt6Widgets QUIET)
	find_package(Qt6Network QUIET)
	find_package(Qt6LinguistTools QUIET)
	find_package(Qt5Widgets QUIET)
	find_package(Qt5Network QUIET)
	find_package(Qt5LinguistTools QUIET)
	find_package(Qt4 QUIET)
endif()

set(SOURCES main.cpp
	mainwindow.cpp
	fileuploader.cpp
	commandqueue.cpp
	mtpobjectsmodel.cpp
	mtpstoragesmodel.cpp
	progressdialog.cpp
	createdirectorydialog.cpp
	renamedialog.cpp
	devicesdialog.cpp
	qtobjectstream.cpp
	utils.cpp
)

set(HEADERS mainwindow.h
	fileuploader.h
	commandqueue.h
	mtpobjectsmodel.h
	progressdialog.h
	createdirectorydialog.h
	renamedialog.h
	devicesdialog.h
	qtobjectstream.h
)

set(FORMS mainwindow.ui
	progressdialog.ui
	createdirectorydialog.ui
	renamedialog.ui
	devicesdialog.ui
)

set(TS_FILES
	translations/android-file-transfer-linux_cs.ts
	translations/android-file-transfer-linux_it.ts
	translations/android-file-transfer-linux_nl.ts
	translations/android-file-transfer-linux_ru.ts
	translations/android-file-transfer-linux_zh-CN.ts
)

if (Qt6Widgets_FOUND)
	message(STATUS "Using Qt6")
	qt6_wrap_cpp(HEADERS_MOC ${HEADERS})
	qt6_wrap_ui(FORMS_HEADERS ${FORMS})

	qt6_add_translation(QM_FILES ${TS_FILES})
	set(QM_FILES_XML "")
	foreach(_QM ${QM_FILES})
		get_filename_component(_QM ${_QM} NAME)
		set(QM_FILES_XML "${QM_FILES_XML}<file>${_QM}</file>")
	endforeach()
	configure_file(translations/translations.qrc.in translations.qrc)

	qt6_add_resources(RESOURCES android-file-transfer.qrc ${CMAKE_CURRENT_BINARY_DIR}/translations.qrc)
	include_directories(${Qt6Widgets_INCLUDE_DIRS} ${Qt6Network_INCLUDE_DIRS})
	set(EXTRA_QT_LINK ${Qt6Widgets_LIBRARIES} ${Qt6Network_LIBRARIES})
elseif (Qt5Widgets_FOUND)
	message(STATUS "Using Qt5")
	qt5_wrap_cpp(HEADERS_MOC ${HEADERS})
	qt5_wrap_ui(FORMS_HEADERS ${FORMS})

	qt5_add_translation(QM_FILES ${TS_FILES})
	set(QM_FILES_XML "")
	foreach(_QM ${QM_FILES})
		get_filename_component(_QM ${_QM} NAME)
		set(QM_FILES_XML "${QM_FILES_XML}<file>${_QM}</file>")
	endforeach()
	configure_file(translations/translations.qrc.in translations.qrc)

	qt5_add_resources(RESOURCES android-file-transfer.qrc ${CMAKE_CURRENT_BINARY_DIR}/translations.qrc)
	include_directories(${Qt5Widgets_INCLUDE_DIRS} ${Qt5Network_INCLUDE_DIRS})
	set(EXTRA_QT_LINK ${Qt5Widgets_LIBRARIES} ${Qt5Network_LIBRARIES})
elseif(Qt4_FOUND OR QT_FOUND OR QT4_FOUND)
	message(STATUS "Using Qt4")
	qt4_wrap_cpp(HEADERS_MOC ${HEADERS})
	qt4_wrap_ui(FORMS_HEADERS ${FORMS})

	qt4_add_translation(QM_FILES ${TS_FILES})
	set(QM_FILES_XML "")
	foreach(_QM ${QM_FILES})
		get_filename_component(_QM ${_QM} NAME)
		set(QM_FILES_XML "${QM_FILES_XML}<file>${_QM}</file>")
	endforeach()
	configure_file(translations/translations.qrc.in translations.qrc)

	qt4_add_resources(RESOURCES android-file-transfer.qrc ${CMAKE_CURRENT_BINARY_DIR}/translations.qrc)
	include(${QT_USE_FILE})
	set(EXTRA_QT_LINK Qt4::QtNetwork ${QT_LIBRARIES})
else()
	message(WARNING "No Qt found")
	set(BUILD_QT_UI FALSE)
endif()

if (BUILD_MTPZ)
	set(MTPZ_DATA_SOURCE "\"https://raw.githubusercontent.com/kbhomes/libmtp-zune/master/src/.mtpz-data\"")
	add_definitions(-DMTPZ_DATA_SOURCE=${MTPZ_DATA_SOURCE})
endif()

if (BUILD_QT_UI)
  if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	string(TIMESTAMP COPYRIGHT_YEAR "%Y")
	set(MACOSX_BUNDLE_BUNDLE_VERSION "${AFT_BASE_VERSION}")
	set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${AFT_VERSION}")
	set(MACOSX_BUNDLE_COPYRIGHT "2015-${COPYRIGHT_YEAR} Vladimir Menshakov")
	set(MACOSX_BUNDLE_ICON_FILE "android-file-transfer.icns")
	set(MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.whoozle.android-file-transfer-linux")
	set(MACOSX_BUNDLE_BUNDLE_NAME "${CMAKE_PROJECT_NAME}")
	set(MACOSX_BUNDLE_EXECUTABLE_NAME "${CMAKE_PROJECT_NAME}")
	set(MACOSX_BUNDLE_RESOURCES "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.app/Contents/Resources")
	execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${MACOSX_BUNDLE_RESOURCES})
	set(MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/Info.plist)
	message(STATUS "generated bundle info is in ${MACOSX_BUNDLE_INFO_PLIST}")
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in ${MACOSX_BUNDLE_INFO_PLIST})

	set(APPLICATION_ICON ${CMAKE_SOURCE_DIR}/osx/android-file-transfer.icns)
	set_source_files_properties(${APPLICATION_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  else()
	set(APPLICATION_ICON "")
  endif()

  add_definitions(${QT_DEFINITIONS})
  include_directories(${CMAKE_CURRENT_BINARY_DIR})

  add_executable("${CMAKE_PROJECT_NAME}" MACOSX_BUNDLE ${APPLICATION_ICON} ${SOURCES} ${HEADERS_MOC} ${FORMS_HEADERS} ${RESOURCES})
  set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY MACOSX_BUNDLE_INFO_PLIST "${MACOSX_BUNDLE_INFO_PLIST}")
  target_link_libraries("${CMAKE_PROJECT_NAME}" ${EXTRA_QT_LINK} ${MTP_LIBRARIES})

  install(TARGETS ${CMAKE_PROJECT_NAME}
		  RUNTIME DESTINATION bin
		  BUNDLE DESTINATION .)

  if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(MACOSX_BUNDLE_APP_PATH "${MACOSX_BUNDLE_ROOT_DIR}/Contents/MacOS/${CMAKE_PROJECT_NAME}")

	add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
		COMMAND ${OTOOL_BIN} -L "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
	)
	if (Qt5Widgets_FOUND)
	  set(MACDEPLOYQT_TOOL ${Qt5Widgets_DIR}/../../../bin/macdeployqt)
	else()
	  get_filename_component(QT_BIN_DIR ${QT_QMAKE_EXECUTABLE} PATH)
	  set(MACDEPLOYQT_TOOL ${QT_BIN_DIR})
	endif()
	message(STATUS "Macdeployqt path: ${MACDEPLOYQT_TOOL}")

	if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_INSTALL_NAME_TOOL} -add_rpath "@executable_path/../Frameworks/" $<TARGET_FILE:${CMAKE_PROJECT_NAME}>)
		set(MACOSX_BUNDLE_LIBS_PRE_INSTALL)
		foreach(DYNLIB ${MACOSX_BUNDLE_LIBS})
			message(STATUS "additional bundle lib ${DYNLIB}")
			get_filename_component(DYNLIB_NAME "${DYNLIB}" NAME)
			set(MACOSX_BUNDLE_LIBS_PRE_INSTALL "
				${MACOSX_BUNDLE_LIBS_PRE_INSTALL}
				file(INSTALL \"${DYNLIB}\" DESTINATION \"${MACOSX_BUNDLE_ROOT_DIR}/Contents/Frameworks\" FOLLOW_SYMLINK_CHAIN)
				message(STATUS \"qt: fixing ${DYNLIB_NAME}\")
				execute_process(COMMAND \"${CMAKE_INSTALL_NAME_TOOL}\" -change \"${DYNLIB_NAME}\" \"@executable_path/../Frameworks/${DYNLIB_NAME}\" \"${MACOSX_BUNDLE_APP_PATH}\")
			")
		endforeach()
	endif()

	install (CODE "
			 message(STATUS \"Running deploy Qt tool: ${MACDEPLOYQT_TOOL}\")
			 execute_process(COMMAND \"${MACDEPLOYQT_TOOL}\" ${MACOSX_BUNDLE_ROOT_DIR})
			 ${MACOSX_BUNDLE_LIBS_PRE_INSTALL}
			 ${MACOSX_BUNDLE_LIBS_INSTALL}
			 execute_process(COMMAND \"${OTOOL_BIN}\" -L \"${MACOSX_BUNDLE_APP_PATH}\")
			 " COMPONENT Runtime)
  else()
	install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/android-file-transfer.desktop DESTINATION share/applications)
	install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/android-file-transfer.appdata.xml DESTINATION share/metainfo)
	install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/android-file-transfer.png DESTINATION share/icons/hicolor/512x512/apps)
  endif()
endif()
