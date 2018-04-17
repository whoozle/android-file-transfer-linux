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
  set(APP_NAME "android-file-transfer")

  if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX_BUNDLE_INFO_STRING "${APP_NAME} ${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "${APP_NAME} ${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${APP_NAME} ${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_COPYRIGHT "2015-2018 Vladimir Menshakov")
    set(MACOSX_BUNDLE_ICON_FILE "android-file-transfer.icns")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.android-file-transfer")
    set(MACOSX_BUNDLE_BUNDLE_NAME "${APP_NAME}")
    set(MACOSX_BUNDLE_RESOURCES "${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.app/Contents/Resources")
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${MACOSX_BUNDLE_RESOURCES})

    set(APPLICATION_ICON ${CMAKE_CURRENT_SOURCE_DIR}/../osx/android-file-transfer.icns)
    set_source_files_properties(${APPLICATION_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  else()
    set(APPLICATION_ICON "")
  endif()

  add_definitions(${QT_DEFINITIONS})
  include_directories(${CMAKE_CURRENT_BINARY_DIR})

  add_executable("${APP_NAME}" MACOSX_BUNDLE ${APPLICATION_ICON} ${SOURCES} ${HEADERS_MOC} ${FORMS_HEADERS} ${RESOURCES})
  target_link_libraries("${APP_NAME}" ${EXTRA_QT_LINK} ${MTP_LIBRARIES})

  install(TARGETS ${APP_NAME}
          RUNTIME DESTINATION bin
          BUNDLE DESTINATION .)

  if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    if (Qt5Widgets_FOUND)
      set(MACDEPLOYQT_TOOL ${Qt5Widgets_DIR}/../../../bin/macdeployqt)
    else()
      get_filename_component(QT_BIN_DIR ${QT_QMAKE_EXECUTABLE} PATH)
      set(MACDEPLOYQT_TOOL ${QT_BIN_DIR})
    endif()
    message(STATUS "Macdeployqt path: ${MACDEPLOYQT_TOOL}")
    install (CODE "
             message(STATUS \"Running deploy Qt tool: ${MACDEPLOYQT_TOOL}\")
             execute_process(COMMAND \"${MACDEPLOYQT_TOOL}\" ${CMAKE_INSTALL_PREFIX}/android-file-transfer.app ERROR_QUIET)
             execute_process(COMMAND \"${CMAKE_INSTALL_NAME_TOOL}\" -add_rpath @executable_path/../Frameworks \"${CMAKE_INSTALL_PREFIX}/${APP_NAME}.app/Contents/MacOS/${APP_NAME}\")
             " COMPONENT Runtime)
  else()
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/android-file-transfer.desktop DESTINATION share/applications)
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/android-file-transfer.appdata.xml DESTINATION share/metainfo)
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/android-file-transfer.png DESTINATION share/icons/hicolor/512x512/apps)
  endif()
endif()
