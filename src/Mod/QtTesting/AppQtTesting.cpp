// SPDX-License-Identifier: LGPL-2.1-or-later

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


#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>
#include <Gui/Application.h>
#include <Gui/Language/Translator.h>
#include <Gui/WidgetFactory.h>

#include "DlgSettingsDefaultQtTesting.h"
#include "DlgSettingsQtTesting.h"
#include "Workbench.h"
#include "WorkbenchManipulator.h"
#include "QtTestingTreeWidget.h"
#include "QtTestingTreeWidgetPy.h"

#if defined(BUILD_QTTESTING_EXTERNAL)
#include "DlgSettingsExternal.h"
#endif

// use a different name to CreateCommand()
void CreateQtTestingCommands();

void loadQtTestingResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(QtTesting);
    Q_INIT_RESOURCE(QtTesting_translation);
    Gui::Translator::instance()->refresh();
}

namespace QtTesting
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("QtTesting")
    {
        initialize("This module is the QtTesting module.");  // register with Python
    }

    ~Module() = default;

private:
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace QtTesting

PyMOD_INIT_FUNC(QtTesting)
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        PyMOD_Return(nullptr);
    }

    // load needed modules
    // try {
    //     Base::Interpreter().runString("import QtTestings");
    // }
    // catch (const Base::Exception& e) {
    //     PyErr_SetString(PyExc_ImportError, e.what());
    //     PyMOD_Return(nullptr);
    // }

    PyObject* qtTestingModule = QtTesting::initModule();

    Base::Console().log("Loading GUI of QtTesting module… done\n");

    // QtTesting::Workbench ::init();
    // auto manip = std::make_shared<QtTesting::WorkbenchManipulator>();
    // Gui::WorkbenchManipulator::installManipulator(manip);

    // instantiating the commands
    CreateQtTestingCommands();

    // register preferences pages on QtTesting, the order here will be the order of the tabs in pref
    // widget
//     Gui::Dialog::DlgPreferencesImp::setGroupData("QtTesting",
//                                                  "QtTesting",
//                                                  QObject::tr("QtTesting Workbench"));
//     new Gui::PrefPageProducer<QtTesting::DlgSettingsQtTesting>(
//         QT_TRANSLATE_NOOP("QObject", "QtTesting"));
//     new Gui::PrefPageProducer<QtTesting::DlgSettingsDefaultQtTesting>(
//         QT_TRANSLATE_NOOP("QObject", "QtTesting"));
// #if defined(BUILD_QTTESTING_EXTERNAL)
//     new Gui::PrefPageProducer<QtTesting::DlgSettingsExternal>(
//         QT_TRANSLATE_NOOP("QObject", "QtTesting"));
// #endif

    // add resources and reloads the translators
    loadQtTestingResource();

    Base::Interpreter().addType(&QtTesting::QtTestUtilityPy::Type,
                                qtTestingModule,
                                "QtTestUtility",);


    // Initialize types
    QtTesting::QtTestUtility::init();

    // Hook in to the main window's event loop to allow for recording and playback of GUI interactions
    getTestUtility();

    PyMOD_Return(qtTestingModule);
}
