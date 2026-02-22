# SPDX-License-Identifier: LGPL-2.1-or-later

set(QtTestingGui_LIBS
    qttesting
    FreeCADGui
)

set(QtTestingGui_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/QtTestingGui_translation.qrc)
qt_find_and_add_translation(QM_SRCS "Resources/translations/*_*.ts"
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file(${QtTestingGui_TR_QRC} ${QM_SRCS})
qt_add_resources(QtTestingGui_QRC_SRCS Resources/QtTestingGui.qrc ${QtTestingGui_TR_QRC})

generate_from_py(QtTestUtility)

SET(Python_SRCS
    QtTestUtility.pyi
    QtTestUtilityPyImp.cpp
)
SOURCE_GROUP("Python" FILES ${Python_SRCS})

# set(QtTestingGui_UIC_SRCS
#     Array2D.ui
#     Array3D.ui
#     DlgDisplayProperties.ui
#     DlgInspectAppearance.ui
#     DlgInspectQtTesting.ui
#     DlgQtTesting.ui
#     DlgSettingsDefaultQtTesting.ui
#     DlgSettingsQtTesting.ui
#     ImageEdit.ui
#     ListEdit.ui
#     QtTestingSave.ui
#     QtTestingsEditor.ui
#     ModelSelect.ui
#     TextEdit.ui
# )

SET(QtTestingGui_SRCS
    ${Python_SRCS}
    ${QtTestingGui_QRC_SRCS}
    ${QtTestingGui_UIC_HDRS}
    AppQtTesting.cpp
    Command.cpp
    QtTestUtility.cpp
    QtTestUtility.h
    XMLEventObserver.cpp
    XMLEventObserver.h
    XMLEventSource.cpp
    XMLEventSource.h
)

add_library(QtTestingGui SHARED
    ${QtTestingGui_SRCS}
    ${QtTestingGuiIcon_SVG}
    ${QtTestingGuiImages}
    ${QtTestingGui_Ui_Files}
)

target_include_directories(
    QtTestingGui
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_include_directories(
    QtTestingGui
    SYSTEM
    PUBLIC
    ${QtConcurrent_INCLUDE_DIRS}
)
target_link_libraries(QtTestingGui ${QtTestingGui_LIBS})

if (MSVC)
    target_compile_options(QtTestingGui PRIVATE /wd4251 /wd4273)
endif()

if (FREECAD_WARN_ERROR)
    target_compile_warn_error(QtTestingGui)
endif()

SET_BIN_DIR(QtTestingGui QtTestingGui /Mod/QtTestingGui)
SET_PYTHON_PREFIX_SUFFIX(QtTestingGui)

# fc_copy_sources(QtTestingGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/QtTestingGui" ${QtTestingGuiIcon_SVG})
# fc_copy_sources(QtTestingGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/QtTestingGui" ${QtTestingGuiImages})
# fc_copy_sources(QtTestingGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/QtTestingGui" ${QtTestingGui_Ui_Files})

INSTALL(TARGETS QtTestingGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
# INSTALL(FILES ${QtTestingGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/QtTestingGui/Resources/icons")
# INSTALL(FILES ${QtTestingGuiImages} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/QtTestingGui/Resources/images")
# INSTALL(FILES ${QtTestingGui_Ui_Files} DESTINATION "Mod/QtTestingGui/Resources/ui")
