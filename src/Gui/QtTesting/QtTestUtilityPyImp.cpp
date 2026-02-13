// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "QtTestUtility.h"

#include "QtTestUtilityPy.h"
#include "QtTestUtilityPy.cpp"


using namespace QtTesting;


// returns a string which represents the object e.g. when printed in python
std::string QtTestUtilityPy::representation() const
{
    return "<QtTesting::QtTestUtility>";
}

PyObject* QtTestUtilityPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int QtTestUtilityPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}


PyObject* QtTestUtilityPy::play(PyObject* args)
{
    QStringList tests;

    PyObject* listObj = Py_None;
    char *name, *label;

    if (PyArg_ParseTuple(args, "s", &name)) {
        tests.append(QString::fromStdString(name));
    }
    else {
        PyErr_Clear();
        if (PyArg_ParseTuple(args, "O!", &PyList_Type, &listObj)) {
            Py::List list(listObj);
            for (auto itemObj : list) {
                Py::String item(itemObj);
                QString value(QString::fromStdString(item.as_string()));
                tests.append(value);
            }
        }
        else {
            Py_Return;
        }
    }

    auto mainWindow = getMainWindow();
    auto& testUtility = mainWindow->getTestUtility();
    auto pass = testUtility.playTests(tests);

    if (pass) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

PyObject* QtTestUtilityPy::playingTest()
{
    auto mainWindow = getMainWindow();
    auto& testUtility = mainWindow->getTestUtility();
    auto playing = testUtility.playingTest();

    if (playing) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

PyObject* QtTestUtilityPy::stopTests()
{
    Base::Console().log("Stopping recording\n");

    auto mainWindow = getMainWindow();
    auto& testUtility = mainWindow->getTestUtility();
    testUtility.stopTests();

    Py_RETURN_NONE;
}


PyObject* QtTestUtilityPy::record(PyObject* args)
{
    PyObject* pyObj = Py_None;
    char *name;
    QString filename;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        PyErr_Clear();
        if (PyArg_ParseTuple(args, "O", &pyObj)) {
            if (!Py_IsNone(pyObj)) {
                Py_Return;
            }
        }
        else {
                Py_Return;
        }
    }

    filename = QString::fromStdString(name);
    auto mainWindow = getMainWindow();
    QApplication::setActiveWindow(mainWindow);
    auto& testUtility = mainWindow->getTestUtility();
    if (!filename.isEmpty()) {
        testUtility.recordTests(filename);
    }
    else {
        testUtility.recordTests();
    }

    Py_RETURN_NONE;
}

PyObject* QtTestUtilityPy::stopRecording()
{
    Base::Console().log("Stopping recording\n");

    auto mainWindow = getMainWindow();
    auto& testUtility = mainWindow->getTestUtility();
    testUtility.stopRecords(1);

    Py_RETURN_NONE;
}

PyObject* QtTestUtilityPy::pauseRecording()
{
    Base::Console().log("Pausing recording\n");

    auto mainWindow = getMainWindow();
    auto& testUtility = mainWindow->getTestUtility();
    testUtility.pauseRecords(true);

    Py_RETURN_NONE;
}

PyObject* QtTestUtilityPy::resumeRecording()
{
    Base::Console().log("Resuming recording\n");

    auto mainWindow = getMainWindow();
    auto& testUtility = mainWindow->getTestUtility();
    testUtility.pauseRecords(false);

    Py_RETURN_NONE;
}
