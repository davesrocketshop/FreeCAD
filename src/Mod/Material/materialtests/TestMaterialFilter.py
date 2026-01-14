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
Test module for FreeCAD material cards and APIs
"""
import os

import unittest
import FreeCAD
import Materials

parseQuantity = FreeCAD.Units.parseQuantity

UUIDAcrylicLegacy = ""  # This can't be known until it is loaded
UUIDAluminumAppearance = "3c6d0407-66b3-48ea-a2e8-ee843edf0311"
UUIDAluminumMixed = "5f546608-fcbb-40db-98d7-d8e104eb33ce"
UUIDAluminumPhysical = "a8e60089-550d-4370-8e7e-1734db12a3a9"
UUIDBrassAppearance = "fff3d5c8-98c3-4ee2-8fe5-7e17403c48fcc"

UUIDBasicRendering = "f006c7e4-35b7-43d5-bbf9-c5d572309e6e"


class MaterialFilterTestCases(unittest.TestCase):
    """
    Test class for FreeCAD material cards and APIs
    """

    def setUp(self):
        self._libraries = {}

        """Setup function to initialize test data"""
        self._modelManager = Materials.ModelManager()
        self._materialManager = Materials.MaterialManager()
        self._uuids = Materials.UUIDs()

        # Disable the external interface
        self._useExternal = self._materialManager.UseExternal
        self._materialManager.UseExternal = False

        # Create a custom library for our test files
        try:
            self._materialManager.removeLibrary("__UnitTest")
        except LookupError as ex:
            # Library may not exist
            pass

        filePath = os.path.dirname(__file__) + os.sep
        materialPath = filePath + "Materials"
        modelPath = filePath + "Models"
        print(f"materialPath {materialPath}")
        self.library = self._materialManager.createLocalLibrary("__UnitTest", 
            ":/icons/preferences-general.svg",
            materialPath,
            modelPath,
            False
        )
        self._materialManager.setDisabled(self.library, False)

        # Disable other libraries
        self._libraryDisabled = {}
        for library in self._materialManager.MaterialLibraries:
            # name = library[0]
            if library.Name != "__UnitTest":
                self._libraryDisabled[library.Name] = library.Disabled
                self._materialManager.setDisabled(library, True)

        self._materialManager.refresh()

    def tearDown(self):
        try:
            self._materialManager.removeLibrary("__UnitTest")
        except LookupError as ex:
            # Library may not exist
            pass

        # Restore other libraries
        print(self._libraryDisabled)
        for name, disabled in self._libraryDisabled.items():
             self._materialManager.setDisabled(name, disabled)

        # Restore the external interface
        self._materialManager.UseExternal = self._useExternal

        self._materialManager.refresh()

    def testModelLoading(self):
        model = self._modelManager.getModel(UUIDBasicRendering)
        self.assertIsNotNone(model)

    def testFilter(self):
        """Test that our filter returns the correct materials"""

        # First check that our materials are loading
        material = self._materialManager.getMaterial(UUIDAluminumAppearance)
        self.assertIsNotNone(material)
        self.assertEqual(material.Name, "TestAluminumAppearance")
        self.assertEqual(material.UUID, UUIDAluminumAppearance)

        material = self._materialManager.getMaterial(UUIDAluminumMixed)
        self.assertIsNotNone(material)
        self.assertEqual(material.Name, "TestAluminumMixed")
        self.assertEqual(material.UUID, UUIDAluminumMixed)

        material = self._materialManager.getMaterial(UUIDAluminumPhysical)
        self.assertIsNotNone(material)
        self.assertEqual(material.Name, "TestAluminumPhysical")
        self.assertEqual(material.UUID, UUIDAluminumPhysical)

        material = self._materialManager.getMaterial(UUIDBrassAppearance)
        self.assertIsNotNone(material)
        self.assertEqual(material.Name, "TestBrassAppearance")
        self.assertEqual(material.UUID, UUIDBrassAppearance)

        # Create an empty filter
        filter = Materials.MaterialFilter()
        self.assertEqual(len(self._materialManager.MaterialLibraries), 1)

        filtered = self._materialManager.filterMaterials(filter)
        for material in filtered:
            print(f"Material {material.Name} Library {material.LibraryName}")
        self.assertEqual(len(filtered), 4)

        filtered = self._materialManager.filterMaterials(filter, includeLegacy=True)
        self.assertEqual(len(filtered), 5)

        # Create a basic rendering filter
        filter.Name = "Basic Appearance"
        filter.RequiredCompleteModels = [self._uuids.BasicRendering]

        filtered = self._materialManager.filterMaterials(filter)
        self.assertEqual(len(filtered), 3)

        filtered = self._materialManager.filterMaterials(filter, includeLegacy=True)
        self.assertEqual(len(filtered), 3)

        # Create an advanced rendering filter
        filter= Materials.MaterialFilter()
        filter.Name = "Advanced Appearance"
        filter.RequiredCompleteModels = [self._uuids.AdvancedRendering]

        filtered = self._materialManager.filterMaterials(filter)
        self.assertEqual(len(filtered), 0)

        filtered = self._materialManager.filterMaterials(filter, includeLegacy=True)
        self.assertEqual(len(filtered), 0)

        # Create a Density filter
        filter= Materials.MaterialFilter()
        filter.Name = "Density"
        filter.RequiredCompleteModels = [self._uuids.Density]

        filtered = self._materialManager.filterMaterials(filter)
        self.assertEqual(len(filtered), 2)

        filtered = self._materialManager.filterMaterials(filter, includeLegacy=True)
        self.assertEqual(len(filtered), 3)

        # Create a Hardness filter
        filter= Materials.MaterialFilter()
        filter.Name = "Hardness"
        filter.RequiredCompleteModels = [self._uuids.Hardness]

        filtered = self._materialManager.filterMaterials(filter)
        self.assertEqual(len(filtered), 0)

        filtered = self._materialManager.filterMaterials(filter, includeLegacy=True)
        self.assertEqual(len(filtered), 0)

        # Create a Density and Basic Rendering filter
        filter= Materials.MaterialFilter()
        filter.Name = "Density and Basic Rendering"
        filter.RequiredCompleteModels = [self._uuids.Density, self._uuids.BasicRendering]

        filtered = self._materialManager.filterMaterials(filter)
        self.assertEqual(len(filtered), 1)

        filtered = self._materialManager.filterMaterials(filter, includeLegacy=True)
        self.assertEqual(len(filtered), 1)

        # Create a Linear Elastic filter
        filter= Materials.MaterialFilter()
        filter.Name = "Linear Elastic"
        filter.RequiredCompleteModels = [self._uuids.LinearElastic]

        filtered = self._materialManager.filterMaterials(filter)
        self.assertEqual(len(filtered), 0)

        filtered = self._materialManager.filterMaterials(filter, includeLegacy=True)
        self.assertEqual(len(filtered), 0)

        filter= Materials.MaterialFilter()
        filter.Name = "Linear Elastic - incomplete"
        filter.RequiredModels = [self._uuids.LinearElastic]

        filtered = self._materialManager.filterMaterials(filter)
        self.assertEqual(len(filtered), 2)

        filtered = self._materialManager.filterMaterials(filter, includeLegacy=True)

    def testErrorInput(self):

        self.assertRaises(TypeError, self._materialManager.filterMaterials, [])
