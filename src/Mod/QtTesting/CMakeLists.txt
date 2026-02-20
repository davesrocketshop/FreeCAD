# SPDX-License-Identifier: LGPL-2.1-or-later

set(QtTesting_LIBS
    QtTesting
    FreeCADGui
)

set(QtTesting_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/QtTesting_translation.qrc)
qt_find_and_add_translation(QM_SRCS "Resources/translations/*_*.ts"
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file(${QtTesting_TR_QRC} ${QM_SRCS})
qt_add_resources(QtTesting_QRC_SRCS Resources/QtTesting.qrc ${QtTesting_TR_QRC})

generate_from_py(QtTestUtility)

SET(Python_SRCS
    QtTestUtility.pyi
    QtTestUtilityPyImp.cpp
)
SOURCE_GROUP("Python" FILES ${Python_SRCS})

# set(QtTesting_UIC_SRCS
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

SET(QtTesting_SRCS
    ${Python_SRCS}
    ${QtTesting_QRC_SRCS}
    ${QtTesting_UIC_HDRS}
    AppQtTesting.cpp
    Command.cpp
    QtTestUtility.cpp
    QtTestUtility.h
    XMLEventObserver.cpp
    XMLEventObserver.h
    XMLEventSource.cpp
    XMLEventSource.h
)

target_include_directories(
    QtTesting
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_include_directories(
    QtTesting
    SYSTEM
    PUBLIC
    ${QtConcurrent_INCLUDE_DIRS}
)
target_link_libraries(QtTesting ${QtTesting_LIBS})

if (MSVC)
    target_compile_options(QtTesting PRIVATE /wd4251 /wd4273)
endif()

if (FREECAD_WARN_ERROR)
    target_compile_warn_error(QtTesting)
endif()

SET_BIN_DIR(QtTesting QtTesting /Mod/QtTesting)
SET_PYTHON_PREFIX_SUFFIX(QtTesting)

# fc_copy_sources(QtTesting "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/QtTesting" ${QtTestingIcon_SVG})
# fc_copy_sources(QtTesting "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/QtTesting" ${QtTestingImages})
# fc_copy_sources(QtTesting "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/QtTesting" ${QtTesting_Ui_Files})

INSTALL(TARGETS QtTesting DESTINATION ${CMAKE_INSTALL_LIBDIR})
# INSTALL(FILES ${QtTestingIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/QtTesting/Resources/icons")
# INSTALL(FILES ${QtTestingImages} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/QtTesting/Resources/images")
# INSTALL(FILES ${QtTesting_Ui_Files} DESTINATION "Mod/QtTesting/Resources/ui")
