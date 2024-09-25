/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"

#include <Gui/MainWindow.h>
#include <Gui/PythonWrapper.h>

#include <Mod/Material/App/MaterialPy.h>

#include "MaterialsEditor.h"
#include "MaterialsEditorPy.h"

#include "MaterialsEditorPy.cpp"

using namespace MatGui;

// returns a string which represents the object e.g. when printed in python
std::string MaterialsEditorPy::representation() const
{
    std::ostringstream str;
    str << "<MaterialsEditor at " << getMaterialsEditorPtr() << ">";
    return str.str();
}

PyObject* MaterialsEditorPy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    // never create such objects with the constructor
    return new MaterialsEditorPy(new MaterialsEditor(Gui::getMainWindow()));
}

// constructor method
int MaterialsEditorPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    auto widget = new MaterialsEditor(Gui::getMainWindow());
    _pcTwinPointer = widget;
    widget->setAttribute(Qt::WA_DeleteOnClose);
    return 0;
    // PyObject* obj {};
    // if (PyArg_ParseTuple(args, "")) {
    //     return 0;
    // }

    // PyErr_Clear();
    // if (PyArg_ParseTuple(args, "O!", &(MatGui::MaterialsEditorPy::Type), &obj)) {
    //     auto widget = static_cast<MatGui::MaterialsEditorPy*>(obj)->getMaterialsEditorPtr();
    //     _pcTwinPointer = widget;
    //     return 0;
    // }

    // // PyErr_Clear();
    // // if (PyArg_ParseTuple(args, "O!", &(QWidget::Type), &obj)) {
    // //     auto widget = static_cast<MatGui::MaterialsEditor*>(obj);
    // //     _pcTwinPointer = widget;
    // //     return 0;
    // // }

    // PyErr_Clear();
    // if (PyArg_ParseTuple(args, "O", &obj)) {
    //     if (QLatin1String(obj->ob_type->tp_name) == QLatin1String("PySide2.QtWidgets.QWidget")) {
    //         Gui::PythonWrapper wrap;
    //         wrap.loadWidgetsModule();
    //         auto qObject = wrap.toQObject(Py::Object(obj));
    //         auto widget = static_cast<MatGui::MaterialsEditor*>(qObject);
    //         _pcTwinPointer = widget;
    //         return 0;
    //     }
    //     else {
    //         PyErr_Format(PyExc_TypeError,
    //                      "empty parameter list, or MaterialsEditor expected not '%s'",
    //                      obj->ob_type->tp_name);
    //         return -1;
    //     }
    // }

    // PyErr_SetString(PyExc_TypeError, "empty parameter list, or MaterialsEditor expected");
    // // PyErr_Format(PyExc_TypeError,
    // //              "empty parameter list, or MaterialsEditor expected not '%s'",
    // //              obj->ob_type->tp_name);
    // return -1;
}

Py::Object MaterialsEditorPy::getMaterial() const
{
    if (getMaterialsEditorPtr()->isMaterialSelected()) {
        auto material = getMaterialsEditorPtr()->getMaterial();
        PyObject* matPy = new Materials::MaterialPy(new Materials::Material(*material));
        return Py::Object(matPy, true);
        // return *matPy;
    }
    else {
        PyErr_SetString(PyExc_LookupError, "No material selected");
        return Py::None();
    }
}

PyObject* MaterialsEditorPy::setFilter(PyObject* args)
{
    // PyObject* obj;
    // if (!PyArg_ParseTuple(args, "O", &obj)) {
    //     return nullptr;
    // }
    // if (PyObject_TypeCheck(obj, &(Materials::MaterialFilterPy::Type))) {
    //     auto filter = static_cast<Materials::MaterialFilterPy*>(obj)->getMaterialFilterPtr();
    //     auto filterPtr = std::make_shared<Materials::MaterialFilter>(*filter);
    //     getMaterialsEditorPtr()->setFilter(filterPtr);
    // }
    // else if (PyList_Check(obj)) {
    //     // The argument is a list of filters
    //     Py_ssize_t n = PyList_Size(obj);
    //     if (n < 0) {
    //         Py_Return;
    //     }
    //     PyObject* item;
    //     auto filterList =
    //     std::make_shared<std::list<std::shared_ptr<Materials::MaterialFilter>>>(); for (int i =
    //     0; i < n; i++) {
    //         item = PyList_GetItem(obj, i);
    //         if (PyObject_TypeCheck(item, &(Materials::MaterialFilterPy::Type))) {
    //             auto filter =
    //                 static_cast<Materials::MaterialFilterPy*>(item)->getMaterialFilterPtr();
    //             auto filterPtr = std::make_shared<Materials::MaterialFilter>(*filter);
    //             filterList->push_back(filterPtr);
    //         }
    //         else {
    //             PyErr_Format(PyExc_TypeError,
    //                          "List entry must be of type 'MaterialFilter' not '%s'",
    //                          obj->ob_type->tp_name);
    //             return nullptr;
    //         }
    //     }
    //     getMaterialsEditorPtr()->setFilter(filterList);
    // }
    // else {
    //     PyErr_Format(PyExc_TypeError,
    //                  "Type must be 'MaterialFilter' or list of 'MaterialFilter' not '%s'",
    //                  obj->ob_type->tp_name);
    //     return nullptr;
    // }

    Py_Return;
}

PyObject* MaterialsEditorPy::selectFilter(PyObject* args)
{
    // char* name;
    // if (!PyArg_ParseTuple(args, "s", &name)) {
    //     return nullptr;
    // }

    Py_Return;
}

PyObject* MaterialsEditorPy::exec(PyObject* /*args*/)
{
    // char* name;
    // if (!PyArg_ParseTuple(args, "s", &name)) {
    //     return nullptr;
    // }
    int result = getMaterialsEditorPtr()->exec();
    return PyLong_FromLong(result);
}

PyObject* MaterialsEditorPy::show(PyObject* /*args*/)
{
    // char* name;
    // if (!PyArg_ParseTuple(args, "s", &name)) {
    //     return nullptr;
    // }
    // Base::Console().Log("1\n");
    // auto x = getMaterialsEditorPtr();
    // Base::Console().Log("2\n");
    getMaterialsEditorPtr()->show();
    // Base::Console().Log("3\n");
    // static QPointer<QDialog> dlg = nullptr;
    // if (!dlg) {
    //     dlg = new MatGui::MaterialsEditor(Gui::getMainWindow());
    // }
    // auto dlg = getMaterialsEditorPtr();
    // dlg->setAttribute(Qt::WA_DeleteOnClose);
    // dlg->show();

    Py_Return;
}

PyObject* MaterialsEditorPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int MaterialsEditorPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
