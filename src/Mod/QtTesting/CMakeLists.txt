# SPDX-License-Identifier: LGPL-2.1-or-later

set(QtTesting_Scripts
    Init.py
)

if(BUILD_GUI)
    add_subdirectory(Gui)
    list (APPEND QtTesting_Scripts InitGui.py)
endif(BUILD_GUI)

add_custom_target(QtTestingScripts ALL
        SOURCES ${QtTesting_Scripts}
        )

fc_target_copy_resource(QtTestingScripts
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/Mod/QtTesting
        ${QtTesting_Scripts})

install(
        FILES
        ${QtTesting_Scripts}
        DESTINATION
        Mod/QtTesting
)
