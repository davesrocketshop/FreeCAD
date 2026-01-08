# SPDX-License-Identifier: LGPL-2.1-or-later

# **************************************************************************
#   Copyright (c) 2023 David Carter <dcarter@davidcarter.ca>              *
#                                                                         *
#   This file is part of the FreeCAD CAx development system.              *
#                                                                         *
#   This program is free software; you can redistribute it and/or modify  *
#   it under the terms of the GNU Lesser General Public License (LGPL)    *
#   as published by the Free Software Foundation; either version 2 of     *
#   the License, or (at your option) any later version.                   *
#   for detail see the LICENCE text file.                                 *
#                                                                         *
#   FreeCAD is distributed in the hope that it will be useful,            *
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#   GNU Library General Public License for more details.                  *
#                                                                         *
#   You should have received a copy of the GNU Library General Public     *
#   License along with FreeCAD; if not, write to the Free Software        *
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#   USA                                                                   *
# **************************************************************************

"""
Test module for FreeCAD library management
"""
import os

import unittest
import FreeCAD
import Materials

class LibraryTestCases(unittest.TestCase):
    """
    Test class for FreeCAD library management
    """

    def setUp(self):
        self._libraries = {}

        """Setup function to initialize test data"""
        self._modelManager = Materials.ModelManager()
        self._materialManager = Materials.MaterialManager()
        self._uuids = Materials.UUIDs()

        # Disable the external interface
        paramExternal = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material/ExternalInterface")
        self._useExternal = paramExternal.GetBool("UseExternal", False)

        paramExternal.SetBool("UseExternal", False)

        # Disable other libraries
        libraries = self._materialManager.getLibraries(True) # Include disabled
        for library in libraries:
            self._libraries[library.getName()] = self._materialManager.isDisabled(library)
            self._materialManager.setDisabled(library, True)

        self._materialManager.refresh()

    def tearDown(self):
        paramExternal = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material/ExternalInterface")
        paramExternal.SetBool("UseExternal", self._useExternal)

        param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material/Resources/Local/__UnitTest")
        param.SetBool("Disabled", True)

        # Restore other libraries
        libraries = self._materialManager.getLibraries(True) # Include disabled
        for library in libraries:
            self._materialManager.setDisabled(library, self._libraries[library.getName()])

        self._materialManager.refresh()

    def testIsDisabled(self):
        self.assertEqual(len(self._materialManager.getLibraries()), 0)
