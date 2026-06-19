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
#include <QTemporaryDir>

#include <App/Application.h>
#include <Base/Interpreter.h>
#include <Base/Quantity.h>
#include <Gui/MetaTypes.h>
#include <src/App/InitApplication.h>

#include <Mod/Material/App/LibraryManager.h>
#include <Mod/Material/App/ModelManager.h>

#ifdef _MSC_VER
# pragma warning(disable : 4834)
#endif

// clang-format off

class TestModelManager : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        if (App::Application::GetARGC() == 0) {
            tests::initApplication();
        }
    }

    void SetUp() override {
        // Disable the external interface
        // Using the MaterialManager functions will cause a boot strapping issue so
        // this needs to access the configuration directly
        ParameterGrp::handle paramExternal = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Material/ExternalInterface"
        );

        _useExternal = paramExternal->GetBool("UseExternal", false);
        paramExternal->SetBool("UseExternal", false);

        _libraryManager = &(Materials::LibraryManager::getManager());
        _modelManager = &(Materials::ModelManager::getManager());
    }

    void TearDown() override {
        // Restore the external interface
        ParameterGrp::handle paramExternal = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Material/ExternalInterface"
        );

        paramExternal->SetBool("UseExternal", _useExternal);

    }

    Materials::LibraryManager* _libraryManager;
    Materials::ModelManager* _modelManager {};
    bool _useExternal {};
};

TEST_F(TestModelManager, TestInstallation)
{
    ASSERT_NE(_libraryManager, nullptr);

    // We should have loaded at least the system library
    auto libraries = _modelManager->getLibraries();
    ASSERT_GT(libraries->size(), 0);

    auto model = _modelManager->getLibrary("System");
    ASSERT_TRUE(model);
    // User folder may not have content
    model = _modelManager->getLibrary("User");
    ASSERT_TRUE(model);
}

TEST_F(TestModelManager, TestSystemModels)
{
    std::shared_ptr<Materials::ModelLibrary> library;
    EXPECT_NO_THROW(library = _modelManager->getLibrary("System"));
    EXPECT_NE(&library, nullptr);
    ASSERT_EQ(library->getName(), "System");
    ASSERT_FALSE(library->isDisabled());
    ASSERT_TRUE(library->isLocal());
    ASSERT_FALSE(library->isModule());
    ASSERT_EQ(library->getRepositoryName(), Materials::LibraryManager::RepositoryLocal);
    ASSERT_NE(library->getRepositoryName(), Materials::LibraryManager::RepositoryRemote);
    ASSERT_TRUE(library->isRepositoryName(Materials::LibraryManager::RepositoryLocal));
    ASSERT_FALSE(library->isRepositoryName(Materials::LibraryManager::RepositoryRemote));
    // auto objects = _modelManager->libraryModels(QString::fromStdString(library->getName()));

}
