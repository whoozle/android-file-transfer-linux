project(mtp-qt)

if(DESIRED_QT_VERSION)
	if(DESIRED_QT_VERSION MATCHES 4)
		find_package(Qt4 QUIET)
	else()
		find_package(Qt5Widgets QUIET)
	endif()
else()
	find_package(Qt5Widgets QUIET)
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
	qtobjectstream.cpp
)

set(HEADERS mainwindow.h
	fileuploader.h
	commandqueue.h
	mtpobjectsmodel.h
	progressdialog.h
	createdirectorydialog.h
	renamedialog.h
	qtobjectstream.h
)

set(FORMS mainwindow.ui
	progressdialog.ui
	createdirectorydialog.ui
	renamedialog.ui
)

if (Qt5Widgets_FOUND)
	message(STATUS "Using Qt5")
	qt5_wrap_cpp(HEADERS_MOC ${HEADERS})
	qt5_wrap_ui(FORMS_HEADERS ${FORMS})
	qt5_add_resources(RESOURCES android-file-transfer.qrc)
	include_directories(${Qt5Widgets_INCLUDE_DIRS})
	set(EXTRA_QT_LINK ${Qt5Widgets_LIBRARIES})
elseif(Qt4_FOUND OR QT_FOUND OR QT4_FOUND)
	message(STATUS "Using Qt4")
	qt4_wrap_cpp(HEADERS_MOC ${HEADERS})
	qt4_wrap_ui(FORMS_HEADERS ${FORMS})
	qt4_add_resources(RESOURCES android-file-transfer.qrc)
	include(${QT_USE_FILE})
	set(EXTRA_QT_LINK ${QT_LIBRARIES})
else()
	message(WARNING "No Qt found")
	set(BUILD_QT_UI FALSE)
endif()

if (BUILD_QT_UI)
	add_definitions(${QT_DEFINITIONS})
	include_directories(${CMAKE_CURRENT_BINARY_DIR})

	add_executable(android-file-transfer ${SOURCES} ${HEADERS_MOC} ${FORMS_HEADERS} ${RESOURCES})
	target_link_libraries(android-file-transfer ${EXTRA_QT_LINK} ${MTP_LIBRARIES})

  if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    install(TARGETS android-file-transfer RUNTIME DESTINATION bin)
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/android-file-transfer.desktop DESTINATION share/applications)
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/android-file-transfer.appdata.xml DESTINATION share/metainfo)
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/android-file-transfer.png DESTINATION share/icons/hicolor/512x512/apps)
  endif()
endif()
