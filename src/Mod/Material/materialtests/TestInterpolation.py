#**************************************************************************
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
#**************************************************************************

"""
Test module for FreeCAD material cards and APIs
"""

import unittest
import FreeCAD
import Materials

parseQuantity = FreeCAD.Units.parseQuantity

class InterpolationTestCases(unittest.TestCase):
    """
    Test class for FreeCAD material interpolation APIs
    """

    def setUp(self):
        """ Setup function to initialize test data """
        self.ModelManager = Materials.ModelManager()
        self.MaterialManager = Materials.MaterialManager()
        self.uuids = Materials.UUIDs()

    def getQuantity(self, value):
        quantity = parseQuantity(value)
        quantity.Format = { "NumberFormat" : "g",
                            "Precision" : 6 }
        return quantity

    def testInterp2D(self):
        """
        Test interpolation for a 2D 2 column array
        """

        mat = self.MaterialManager.getMaterial("c6c64159-19c1-40b5-859c-10561f20f979")
        self.assertIsNotNone(mat)
        self.assertEqual(mat.Name, "Test Material")
        self.assertEqual(mat.UUID, "c6c64159-19c1-40b5-859c-10561f20f979")

        self.assertTrue(mat.hasPhysicalModel(self.uuids.TestModel))

        self.assertFalse(mat.hasPhysicalProperty("Henry"))
        with self.assertRaises(ValueError):
            mat.interpolate2D("Henry", 1.0)

        self.assertTrue(mat.hasPhysicalProperty("TestArray2D"))
        maxError = self.getQuantity(".00001 kg/m^3").Value

        # This passes, which is why using delta is necessary
        #self.assertAlmostEqual(mat.interpolate2D("TestArray2D", "25.0 C"), self.getQuantity("10.00 kg/m^3").Value)

        self.assertAlmostEqual(mat.interpolate2D("TestArray2D", 25.0),
                               self.getQuantity("25.00 kg/m^3").Value,
                               delta=maxError)
        self.assertAlmostEqual(mat.interpolate2D("TestArray2D", "25.0 C"),
                               self.getQuantity("25.00 kg/m^3").Value,
                               delta=maxError)
        self.assertAlmostEqual(mat.interpolate2D("TestArray2D", self.getQuantity("25.0 C")),
                               self.getQuantity("25.00 kg/m^3").Value,
                               delta=maxError)

    def testInterp2DExtrapolate(self):

        mat = self.MaterialManager.getMaterial("c6c64159-19c1-40b5-859c-10561f20f979")
        self.assertIsNotNone(mat)

        self.assertTrue(mat.hasPhysicalProperty("TestArray2D"))
        maxError = self.getQuantity(".00001 kg/m^3").Value

        # Test extraploation limits
        with self.assertRaises(ValueError):
            self.assertAlmostEqual(mat.interpolate2D("TestArray2D", 9.99),
                                self.getQuantity("9.99 kg/m^3").Value,
                                delta=maxError)
        self.assertAlmostEqual(mat.interpolate2D("TestArray2D", 9.99, extrapolate=True),
                               self.getQuantity("9.99 kg/m^3").Value,
                               delta=maxError)
        self.assertAlmostEqual(mat.interpolate2D("TestArray2D", 0.0, extrapolate=True),
                               self.getQuantity("0.00 kg/m^3").Value,
                               delta=maxError)
        with self.assertRaises(ValueError):
            self.assertAlmostEqual(mat.interpolate2D("TestArray2D", 31.0),
                                self.getQuantity("31.00 kg/m^3").Value,
                                delta=maxError)
        self.assertAlmostEqual(mat.interpolate2D("TestArray2D", 31.0, extrapolate=True),
                               self.getQuantity("31.00 kg/m^3").Value,
                               delta=maxError)
        self.assertAlmostEqual(mat.interpolate2D("TestArray2D", 40.0, extrapolate=True),
                               self.getQuantity("40.00 kg/m^3").Value,
                               delta=maxError)

    def interpolate2DMulti(self):
        """
        Test interpolation for a 2D multi column array
        """

        mat = self.MaterialManager.getMaterial("c6c64159-19c1-40b5-859c-10561f20f979")
        self.assertIsNotNone(mat)
        self.assertEqual(mat.Name, "Test Material")
        self.assertEqual(mat.UUID, "c6c64159-19c1-40b5-859c-10561f20f979")

        self.assertTrue(mat.hasPhysicalModel(self.uuids.TestModel))

        self.assertFalse(mat.hasPhysicalProperty("Henry"))
        with self.assertRaises(ValueError):
            mat.interpolate2DMulti("Henry", 1.0)

        self.assertTrue(mat.hasPhysicalProperty("TestArray2D3Column"))

        maxErrorKg = self.getQuantity(".00001 kg/m^3").Value
        maxErrorPa = self.getQuantity(".00001 Pa").Value

        values = mat.interpolate2DMulti("TestArray2D3Column", 25.0)
        self.assertEqual(len(values), 2)
        self.assertAlmostEqual(values[0],
                               self.getQuantity("26.00 kg/m^3").Value,
                               delta=maxErrorKg)
        self.assertAlmostEqual(values[1],
                               self.getQuantity("27.00 Pa").Value,
                               delta=maxErrorPa)

        with self.assertRaises(ValueError):
            values = mat.interpolate2DMulti("TestArray2D3Column", 40.0)
        values = mat.interpolate2DMulti("TestArray2D3Column", 40.0, extrapolate=True)
        self.assertEqual(len(values), 2)
        self.assertAlmostEqual(values[0],
                               self.getQuantity("41.00 kg/m^3").Value,
                               delta=maxErrorKg)
        self.assertAlmostEqual(values[1],
                               self.getQuantity("42.00 Pa").Value,
                               delta=maxErrorPa)

    def testInterp3D(self):
        """
        Test interpolation for a 3D 2 column array
        """

        mat = self.MaterialManager.getMaterial("c6c64159-19c1-40b5-859c-10561f20f979")
        self.assertIsNotNone(mat)
        self.assertEqual(mat.Name, "Test Material")
        self.assertEqual(mat.UUID, "c6c64159-19c1-40b5-859c-10561f20f979")

        self.assertTrue(mat.hasPhysicalModel(self.uuids.TestModel))

        self.assertFalse(mat.hasPhysicalProperty("Henry"))
        with self.assertRaises(ValueError):
            mat.interpolate2D("Henry", 1.0)

        self.assertTrue(mat.hasPhysicalProperty("TestArray3D"))
        maxError = self.getQuantity(".00001 Pa").Value

        # Create some consistent data
        array = mat.getPhysicalValue("TestArray3D")
        array.Depth = 0 # Reset data
        array.Depth = 3

        for depthIndex in range(0, array.Depth):
            array.setRows(depthIndex, 3)
            array.setDepthValue(depthIndex, "{0} Pa".format(depthIndex * 10.0 + 10.0))

        table = Materials.Array2D()

        array.setValue(0, 0, 0, "{0} Pa".format(10.0))
        array.setValue(0, 1, 0, "{0} Pa".format(20.0))
        array.setValue(0, 2, 0, "{0} Pa".format(30.0))
        array.setValue(0, 0, 1, "{0} Pa".format(10.0))
        array.setValue(0, 1, 1, "{0} Pa".format(20.0))
        array.setValue(0, 2, 1, "{0} Pa".format(10.0))
        array.setValue(1, 0, 0, "{0} Pa".format(10.0))
        array.setValue(1, 1, 0, "{0} Pa".format(20.0))
        array.setValue(1, 2, 0, "{0} Pa".format(30.0))
        array.setValue(1, 0, 1, "{0} Pa".format(10.0))
        array.setValue(1, 1, 1, "{0} Pa".format(20.0))
        array.setValue(1, 2, 1, "{0} Pa".format(30.0))
        array.setValue(2, 0, 0, "{0} Pa".format(10.0))
        array.setValue(2, 1, 0, "{0} Pa".format(20.0))
        array.setValue(2, 2, 0, "{0} Pa".format(30.0))
        array.setValue(2, 0, 1, "{0} Pa".format(30.0))
        array.setValue(2, 1, 1, "{0} Pa".format(20.0))
        array.setValue(2, 2, 1, "{0} Pa".format(10.0))

        self.assertAlmostEqual(mat.interpolate3D("TestArray3D", 20.0, 25.0),
                               self.getQuantity("25.00 Pa").Value,
                               delta=maxError)
        self.assertAlmostEqual(mat.interpolate3D("TestArray3D", "20.0 C", 25.0),
                               self.getQuantity("25.00 Pa").Value,
                               delta=maxError)
        self.assertAlmostEqual(mat.interpolate3D("TestArray3D", self.getQuantity("25.0 C"), 25.0),
                               self.getQuantity("25.00 Pa").Value,
                               delta=maxError)
