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

#include <gtest/gtest.h>

#include <QLocale>
#include <QMetaType>
#include <QString>

#include <App/Application.h>
#include <Base/Interpreter.h>
#include <Base/Quantity.h>
#include <Gui/MetaTypes.h>
#include <src/App/InitApplication.h>

#include <Mod/Material/App/LibraryManager.h>

#ifdef _MSC_VER
# pragma warning(disable : 4834)
#endif

// clang-format off

class TestLibraryManager : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    if (App::Application::GetARGC() == 0) {
        tests::initApplication();
    }
  }

  void SetUp() override {
    Base::Interpreter().runString("import Part");
    _libraryManager = &(Materials::LibraryManager::getManager());
  }

  // void TearDown() override {}
  Materials::LibraryManager* _libraryManager;
};

TEST_F(TestLibraryManager, TestInstallation)
{
    ASSERT_NE(_libraryManager, nullptr);

    // We should have loaded at least the system library
    auto libraries = _libraryManager->getLibraries();
    ASSERT_GT(libraries->size(), 0);

    // We should have at least one material
    auto materials = _libraryManager->getLocalMaterialLibraries();
    ASSERT_GT(materials->size(), 0);

    auto managed = _libraryManager->getLibrary(QStringLiteral("Local"), QStringLiteral("System"));
    ASSERT_TRUE(managed);
    managed = _libraryManager->getLibrary(QStringLiteral("Local"), QStringLiteral("User"));
    ASSERT_TRUE(managed);
    auto model = _libraryManager->getModelLibrary(QStringLiteral("Local"), QStringLiteral("System"));
    ASSERT_TRUE(model);
    // model = _libraryManager->getModelLibrary(QStringLiteral("Local"), QStringLiteral("User"));
    // ASSERT_TRUE(model);
    auto material = _libraryManager->getMaterialLibrary(QStringLiteral("Local"), QStringLiteral("System"));
    ASSERT_TRUE(material);
    material = _libraryManager->getMaterialLibrary(QStringLiteral("Local"), QStringLiteral("User"));
    ASSERT_TRUE(material);
}
