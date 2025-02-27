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

        array = mat.getPhysicalValue("TestArray2D")
        self.assertIsNotNone(array)
        self.assertEqual(array.Rows, 3)
        self.assertEqual(array.Columns, 2)

        arrayData = array.Array
        self.assertIsNotNone(arrayData)
        self.assertEqual(len(arrayData), 3)
        self.assertEqual(len(arrayData[0]), 2)
        self.assertEqual(len(arrayData[1]), 2)
        self.assertEqual(len(arrayData[2]), 2)

        self.assertEqual(arrayData[0][0].Format["NumberFormat"], "g")
        self.assertEqual(arrayData[0][1].Format["NumberFormat"], "g")
        self.assertEqual(arrayData[1][0].Format["NumberFormat"], "g")
        self.assertEqual(arrayData[1][1].Format["NumberFormat"], "g")
        self.assertEqual(arrayData[2][0].Format["NumberFormat"], "g")
        self.assertEqual(arrayData[2][1].Format["NumberFormat"], "g")

        self.assertEqual(arrayData[0][0].UserString, self.getQuantity("10.00 C").UserString)
        self.assertEqual(arrayData[0][1].UserString, self.getQuantity("10.00 kg/m^3").UserString)
        self.assertEqual(arrayData[1][0].UserString, self.getQuantity("20.00 C").UserString)
        self.assertEqual(arrayData[1][1].UserString, self.getQuantity("20.00 kg/m^3").UserString)
        self.assertEqual(arrayData[2][0].UserString, self.getQuantity("30.00 C").UserString)
        self.assertEqual(arrayData[2][1].UserString, self.getQuantity("30.00 kg/m^3").UserString)

        self.assertAlmostEqual(arrayData[0][0].Value, 10.0)
        self.assertAlmostEqual(arrayData[0][1].Value, 1e-8)
        self.assertAlmostEqual(arrayData[1][0].Value, 20.0)
        self.assertAlmostEqual(arrayData[1][1].Value, 2e-8)
        self.assertAlmostEqual(arrayData[2][0].Value, 30.0)
        self.assertAlmostEqual(arrayData[2][1].Value, 3e-8)

        self.assertAlmostEqual(arrayData[-1][0].Value, 30.0) # Last in list
        with self.assertRaises(IndexError):
            self.assertAlmostEqual(arrayData[3][0].Value, 10.0)
        self.assertAlmostEqual(arrayData[0][-1].Value, 1e-8)
        with self.assertRaises(IndexError):
            self.assertAlmostEqual(arrayData[0][2].Value, 10.0)

        self.assertEqual(array.getValue(0,0).Format["NumberFormat"], "g")
        self.assertEqual(array.getValue(0,1).Format["NumberFormat"], "g")
        self.assertEqual(array.getValue(1,0).Format["NumberFormat"], "g")
        self.assertEqual(array.getValue(1,1).Format["NumberFormat"], "g")
        self.assertEqual(array.getValue(2,0).Format["NumberFormat"], "g")
        self.assertEqual(array.getValue(2,1).Format["NumberFormat"], "g")

        self.assertEqual(array.getValue(0,0).UserString, self.getQuantity("10.00 C").UserString)
        self.assertEqual(array.getValue(0,1).UserString, self.getQuantity("10.00 kg/m^3").UserString)
        self.assertEqual(array.getValue(1,0).UserString, self.getQuantity("20.00 C").UserString)
        self.assertEqual(array.getValue(1,1).UserString, self.getQuantity("20.00 kg/m^3").UserString)
        self.assertEqual(array.getValue(2,0).UserString, self.getQuantity("30.00 C").UserString)
        self.assertEqual(array.getValue(2,1).UserString, self.getQuantity("30.00 kg/m^3").UserString)

        self.assertAlmostEqual(array.getValue(0,0).Value, 10.0)
        self.assertAlmostEqual(array.getValue(0,1).Value, 1e-8)
        self.assertAlmostEqual(array.getValue(1,0).Value, 20.0)
        self.assertAlmostEqual(array.getValue(1,1).Value, 2e-8)
        self.assertAlmostEqual(array.getValue(2,0).Value, 30.0)
        self.assertAlmostEqual(array.getValue(2,1).Value, 3e-8)

        with self.assertRaises(IndexError):
            self.assertAlmostEqual(array.getValue(-1,0).Value, 10.0)
        with self.assertRaises(IndexError):
            self.assertAlmostEqual(array.getValue(3,0).Value, 10.0)
        with self.assertRaises(IndexError):
            self.assertAlmostEqual(array.getValue(0,-1).Value, 10.0)
        with self.assertRaises(IndexError):
            self.assertAlmostEqual(array.getValue(0,2).Value, 10.0)

        for rowIndex in range(0, array.Rows):
            row = array.getRow(rowIndex)
            self.assertIsNotNone(row)
            self.assertEqual(len(row), 2)

        with self.assertRaises(IndexError):
            row = array.getRow(-1)
        with self.assertRaises(IndexError):
            row = array.getRow(3)

    def test2DArray(self):
        """
        Test API access to 2D arrays
        """

        mat = self.MaterialManager.getMaterial("c6c64159-19c1-40b5-859c-10561f20f979")
        self.assertIsNotNone(mat)
        self.assertEqual(mat.Name, "Test Material")
        self.assertEqual(mat.UUID, "c6c64159-19c1-40b5-859c-10561f20f979")

        self.assertTrue(mat.hasPhysicalModel(self.uuids.TestModel))
        self.assertFalse(mat.isPhysicalModelComplete(self.uuids.TestModel))

        self.assertTrue(mat.hasPhysicalProperty("TestArray2D"))

        array = mat.getPhysicalValue("TestArray2D")
        self.assertIsNotNone(array)
        self.assertEqual(array.Rows, 3)
        self.assertEqual(array.Columns, 2)

        arrayData = array.Array
        self.assertIsNotNone(arrayData)
        self.assertEqual(len(arrayData), 3)
        self.assertEqual(len(arrayData[0]), 2)
        self.assertEqual(len(arrayData[1]), 2)
        self.assertEqual(len(arrayData[2]), 2)

        self.assertEqual(arrayData[0][0].Format["NumberFormat"], "g")
        self.assertEqual(arrayData[0][1].Format["NumberFormat"], "g")
        self.assertEqual(arrayData[1][0].Format["NumberFormat"], "g")
        self.assertEqual(arrayData[1][1].Format["NumberFormat"], "g")
        self.assertEqual(arrayData[2][0].Format["NumberFormat"], "g")
        self.assertEqual(arrayData[2][1].Format["NumberFormat"], "g")

        self.assertEqual(arrayData[0][0].UserString, self.getQuantity("10.00 C").UserString)
        self.assertEqual(arrayData[0][1].UserString, self.getQuantity("10.00 kg/m^3").UserString)
        self.assertEqual(arrayData[1][0].UserString, self.getQuantity("20.00 C").UserString)
        self.assertEqual(arrayData[1][1].UserString, self.getQuantity("20.00 kg/m^3").UserString)
        self.assertEqual(arrayData[2][0].UserString, self.getQuantity("30.00 C").UserString)
        self.assertEqual(arrayData[2][1].UserString, self.getQuantity("30.00 kg/m^3").UserString)

        self.assertAlmostEqual(arrayData[0][0].Value, 10.0)
        self.assertAlmostEqual(arrayData[0][1].Value, 1e-8)
        self.assertAlmostEqual(arrayData[1][0].Value, 20.0)
        self.assertAlmostEqual(arrayData[1][1].Value, 2e-8)
        self.assertAlmostEqual(arrayData[2][0].Value, 30.0)
        self.assertAlmostEqual(arrayData[2][1].Value, 3e-8)

        self.assertAlmostEqual(arrayData[-1][0].Value, 30.0) # Last in list
        with self.assertRaises(IndexError):
            self.assertAlmostEqual(arrayData[3][0].Value, 10.0)
        self.assertAlmostEqual(arrayData[0][-1].Value, 1e-8)
        with self.assertRaises(IndexError):
            self.assertAlmostEqual(arrayData[0][2].Value, 10.0)

        self.assertEqual(array.getValue(0,0).Format["NumberFormat"], "g")
        self.assertEqual(array.getValue(0,1).Format["NumberFormat"], "g")
        self.assertEqual(array.getValue(1,0).Format["NumberFormat"], "g")
        self.assertEqual(array.getValue(1,1).Format["NumberFormat"], "g")
        self.assertEqual(array.getValue(2,0).Format["NumberFormat"], "g")
        self.assertEqual(array.getValue(2,1).Format["NumberFormat"], "g")

        self.assertEqual(array.getValue(0,0).UserString, self.getQuantity("10.00 C").UserString)
        self.assertEqual(array.getValue(0,1).UserString, self.getQuantity("10.00 kg/m^3").UserString)
        self.assertEqual(array.getValue(1,0).UserString, self.getQuantity("20.00 C").UserString)
        self.assertEqual(array.getValue(1,1).UserString, self.getQuantity("20.00 kg/m^3").UserString)
        self.assertEqual(array.getValue(2,0).UserString, self.getQuantity("30.00 C").UserString)
        self.assertEqual(array.getValue(2,1).UserString, self.getQuantity("30.00 kg/m^3").UserString)

        self.assertAlmostEqual(array.getValue(0,0).Value, 10.0)
        self.assertAlmostEqual(array.getValue(0,1).Value, 1e-8)
        self.assertAlmostEqual(array.getValue(1,0).Value, 20.0)
        self.assertAlmostEqual(array.getValue(1,1).Value, 2e-8)
        self.assertAlmostEqual(array.getValue(2,0).Value, 30.0)
        self.assertAlmostEqual(array.getValue(2,1).Value, 3e-8)

        with self.assertRaises(IndexError):
            self.assertAlmostEqual(array.getValue(-1,0).Value, 10.0)
        with self.assertRaises(IndexError):
            self.assertAlmostEqual(array.getValue(3,0).Value, 10.0)
        with self.assertRaises(IndexError):
            self.assertAlmostEqual(array.getValue(0,-1).Value, 10.0)
        with self.assertRaises(IndexError):
            self.assertAlmostEqual(array.getValue(0,2).Value, 10.0)

        for rowIndex in range(0, array.Rows):
            row = array.getRow(rowIndex)
            self.assertIsNotNone(row)
            self.assertEqual(len(row), 2)

        with self.assertRaises(IndexError):
            row = array.getRow(-1)
        with self.assertRaises(IndexError):
            row = array.getRow(3)

    def test3DArray(self):
        """
        Test API access to 3D arrays
        """

        mat = self.MaterialManager.getMaterial("c6c64159-19c1-40b5-859c-10561f20f979")
        self.assertIsNotNone(mat)
        self.assertEqual(mat.Name, "Test Material")
        self.assertEqual(mat.UUID, "c6c64159-19c1-40b5-859c-10561f20f979")

        self.assertTrue(mat.hasPhysicalModel(self.uuids.TestModel))
        self.assertFalse(mat.isPhysicalModelComplete(self.uuids.TestModel))

        self.assertTrue(mat.hasPhysicalProperty("TestArray3D"))

        array = mat.getPhysicalValue("TestArray3D")
        self.assertIsNotNone(array)
        self.assertEqual(array.Depth, 3)
        self.assertEqual(array.Columns, 2)
        self.assertEqual(array.getRows(), 2)
        self.assertEqual(array.getRows(0), 2)
        self.assertEqual(array.getRows(1), 0)
        self.assertEqual(array.getRows(2), 3)

        arrayData = array.Array
        self.assertIsNotNone(arrayData)
        self.assertEqual(len(arrayData), 3)
        self.assertEqual(len(arrayData[0]), 2)
        self.assertEqual(len(arrayData[1]), 0)
        self.assertEqual(len(arrayData[2]), 3)

        self.assertEqual(arrayData[0][0][0].Format["NumberFormat"], "g")
        self.assertEqual(arrayData[0][0][1].Format["NumberFormat"], "g")
        self.assertEqual(arrayData[0][1][0].Format["NumberFormat"], "g")
        self.assertEqual(arrayData[0][1][1].Format["NumberFormat"], "g")
        self.assertEqual(arrayData[2][0][0].Format["NumberFormat"], "g")
        self.assertEqual(arrayData[2][0][1].Format["NumberFormat"], "g")
        self.assertEqual(arrayData[2][1][0].Format["NumberFormat"], "g")
        self.assertEqual(arrayData[2][1][1].Format["NumberFormat"], "g")
        self.assertEqual(arrayData[2][2][0].Format["NumberFormat"], "g")
        self.assertEqual(arrayData[2][2][1].Format["NumberFormat"], "g")

        self.assertEqual(arrayData[0][0][0].UserString, self.getQuantity("11.00 Pa").UserString)
        self.assertEqual(arrayData[0][0][1].UserString, self.getQuantity("12.00 Pa").UserString)
        self.assertEqual(arrayData[0][1][0].UserString, self.getQuantity("21.00 Pa").UserString)
        self.assertEqual(arrayData[0][1][1].UserString, self.getQuantity("22.00 Pa").UserString)
        self.assertEqual(arrayData[2][0][0].UserString, self.getQuantity("10.00 Pa").UserString)
        self.assertEqual(arrayData[2][0][1].UserString, self.getQuantity("11.00 Pa").UserString)
        self.assertEqual(arrayData[2][1][0].UserString, self.getQuantity("20.00 Pa").UserString)
        self.assertEqual(arrayData[2][1][1].UserString, self.getQuantity("21.00 Pa").UserString)
        self.assertEqual(arrayData[2][2][0].UserString, self.getQuantity("30.00 Pa").UserString)
        self.assertEqual(arrayData[2][2][1].UserString, self.getQuantity("31.00 Pa").UserString)

        self.assertEqual(array.getDepthValue(0).UserString, self.getQuantity("10.00 C").UserString)
        self.assertEqual(array.getDepthValue(1).UserString, self.getQuantity("20.00 C").UserString)
        self.assertEqual(array.getDepthValue(2).UserString, self.getQuantity("30.00 C").UserString)

        self.assertEqual(arrayData[0][0][-1].UserString, self.getQuantity("12.00 Pa").UserString)
        with self.assertRaises(IndexError):
            self.assertEqual(arrayData[0][0][2].UserString, self.getQuantity("11.00 Pa").UserString)
        self.assertEqual(arrayData[0][-1][0].UserString, self.getQuantity("21.00 Pa").UserString)
        with self.assertRaises(IndexError):
            self.assertEqual(arrayData[0][2][0].UserString, self.getQuantity("11.00 Pa").UserString)
        with self.assertRaises(IndexError):
            self.assertEqual(arrayData[1][0][0].UserString, self.getQuantity("11.00 Pa").UserString)
        self.assertEqual(arrayData[-1][0][0].UserString, self.getQuantity("10.00 Pa").UserString)
        with self.assertRaises(IndexError):
            self.assertEqual(arrayData[3][0][0].UserString, self.getQuantity("11.00 Pa").UserString)

        with self.assertRaises(IndexError):
            self.assertEqual(array.getDepthValue(-1).UserString,
                             self.getQuantity("10.00 C").UserString)
        with self.assertRaises(IndexError):
            self.assertEqual(array.getDepthValue(3).UserString,
                             self.getQuantity("10.00 C").UserString)

        self.assertEqual(array.getValue(0,0,0).Format["NumberFormat"], "g")
        self.assertEqual(array.getValue(0,0,1).Format["NumberFormat"], "g")
        self.assertEqual(array.getValue(0,1,0).Format["NumberFormat"], "g")
        self.assertEqual(array.getValue(0,1,1).Format["NumberFormat"], "g")
        self.assertEqual(array.getValue(2,0,0).Format["NumberFormat"], "g")
        self.assertEqual(array.getValue(2,0,1).Format["NumberFormat"], "g")
        self.assertEqual(array.getValue(2,1,0).Format["NumberFormat"], "g")
        self.assertEqual(array.getValue(2,1,1).Format["NumberFormat"], "g")
        self.assertEqual(array.getValue(2,2,0).Format["NumberFormat"], "g")
        self.assertEqual(array.getValue(2,2,1).Format["NumberFormat"], "g")

        self.assertEqual(array.getValue(0,0,0).UserString, self.getQuantity("11.00 Pa").UserString)
        self.assertEqual(array.getValue(0,0,1).UserString, self.getQuantity("12.00 Pa").UserString)
        self.assertEqual(array.getValue(0,1,0).UserString, self.getQuantity("21.00 Pa").UserString)
        self.assertEqual(array.getValue(0,1,1).UserString, self.getQuantity("22.00 Pa").UserString)
        self.assertEqual(array.getValue(2,0,0).UserString, self.getQuantity("10.00 Pa").UserString)
        self.assertEqual(array.getValue(2,0,1).UserString, self.getQuantity("11.00 Pa").UserString)
        self.assertEqual(array.getValue(2,1,0).UserString, self.getQuantity("20.00 Pa").UserString)
        self.assertEqual(array.getValue(2,1,1).UserString, self.getQuantity("21.00 Pa").UserString)
        self.assertEqual(array.getValue(2,2,0).UserString, self.getQuantity("30.00 Pa").UserString)
        self.assertEqual(array.getValue(2,2,1).UserString, self.getQuantity("31.00 Pa").UserString)

        with self.assertRaises(IndexError):
            self.assertEqual(array.getValue(0,0,-1).UserString,
                             self.getQuantity("11.00 Pa").UserString)
        with self.assertRaises(IndexError):
            self.assertEqual(array.getValue(0,0,2).UserString,
                             self.getQuantity("11.00 Pa").UserString)
        with self.assertRaises(IndexError):
            self.assertEqual(array.getValue(0,-1,0).UserString,
                             self.getQuantity("11.00 Pa").UserString)
        with self.assertRaises(IndexError):
            self.assertEqual(array.getValue(0,2,0).UserString,
                             self.getQuantity("11.00 Pa").UserString)
        with self.assertRaises(IndexError):
            self.assertEqual(array.getValue(1,0,0).UserString,
                             self.getQuantity("11.00 Pa").UserString)
        with self.assertRaises(IndexError):
            self.assertEqual(array.getValue(-1,0,0).UserString,
                             self.getQuantity("11.00 Pa").UserString)
        with self.assertRaises(IndexError):
            self.assertEqual(array.getValue(3,0,0).UserString,
                             self.getQuantity("11.00 Pa").UserString)
