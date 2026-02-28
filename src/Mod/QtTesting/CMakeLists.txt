# SPDX-License-Identifier: LGPL-2.1-or-later

if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(Import_Scripts
    Init.py
)

if(BUILD_GUI)
    list (APPEND Import_Scripts InitGui.py)
endif(BUILD_GUI)
