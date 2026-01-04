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

#include "gtest/gtest.h"

#include <QMetaType>
#include <QString>
#include <QDir>

#include <App/Application.h>
#include <Base/Quantity.h>
#include <Gui/MetaTypes.h>
#include <src/App/InitApplication.h>

#include <Mod/Material/App/MaterialLibrary.h>
#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/MaterialValue.h>
#include <Mod/Material/App/Model.h>
#include <Mod/Material/App/ModelManager.h>
#include <Mod/Material/App/ModelUuids.h>

// clang-format off

class TestMaterialFilter : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override {
        _modelManager = &(Materials::ModelManager::getManager());
        _materialManager = &(Materials::MaterialManager::getManager());

        // Disable the external interface
        ParameterGrp::handle paramExternal =
            App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Material/ExternalInterface");

        _useExternal = paramExternal->GetBool("UseExternal", false);

        paramExternal->SetBool("UseExternal", false);

        // Create a custom library for our test files
        ParameterGrp::handle param = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Material/Resources/Local/__UnitTest");

        // Ensure the directory exists
        std::string testPath = App::Application::getHomePath() + "/tests/Materials/";
        QDir directory(QString::fromStdString(testPath));
        ASSERT_TRUE(directory.exists());

        param->SetASCII("Directory", testPath);
        param->SetASCII("ModelDirectory", testPath);
        param->SetASCII("IconPath", ":/icons/preferences-general.svg");
        param->SetBool("ReadOnly", false);
        param->SetBool("Disabled", false);

        // Disable other libraries
        param = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Material/Resources/Local");
        auto groups = param->GetGroups();
        for (auto group : groups) {
            // auto group = param->GetGroup(groupName);
            if (std::string(group->GetGroupName()) != "__UnitTest") {
                _libraries.emplace(group->GetGroupName(), group->GetBool("Disabled", false));
                group->SetBool("Disabled", true);
            }
        }
        param = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Material/Resources/Modules");
        groups = param->GetGroups();
        for (auto group : groups) {
            // auto group = param->GetGroup(groupName);
            if (std::string(group->GetGroupName()) != "__UnitTest") {
                _libraries.emplace(group->GetGroupName(), group->GetBool("Disabled", false));
                group->SetBool("Disabled", true);
            }
        }

        _materialManager->refresh();

        _library = _materialManager->getLibrary(QStringLiteral("__UnitTest"));
    }

    void TearDown() override {
        ParameterGrp::handle paramExternal =
            App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Material/ExternalInterface");
        paramExternal->SetBool("UseExternal", _useExternal);

        ParameterGrp::handle param = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Material/Resources/Local/__UnitTest");
        param->SetBool("Disabled", true);

        // Restore other libraries
        param = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Material/Resources/Local");
        auto groups = param->GetGroups();
        for (auto group : groups) {
            if (std::string(group->GetGroupName()) != "__UnitTest") {
                group->SetBool("Disabled", _libraries[group->GetGroupName()]);
            }
        }

        param = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Material/Resources/Modules");
        groups = param->GetGroups();
        for (auto group : groups) {
            if (std::string(group->GetGroupName()) != "__UnitTest") {
                group->SetBool("Disabled", _libraries[group->GetGroupName()]);
            }
        }

        _materialManager->refresh();
    }

    Materials::ModelManager* _modelManager {};
    Materials::MaterialManager* _materialManager {};
    std::shared_ptr<Materials::MaterialLibrary> _library;
    QString _testMaterialUUID;

    bool _useExternal {};
    std::map<std::string, bool> _libraries;

    bool _includeFavorites {};
    bool _includeRecent {};
    bool _includeFolders {};
    bool _includeLibraries {};
    bool _includeLegacy {};

    const char* UUIDAcrylicLegacy = ""; // This can't be known until it is loaded
    const char* UUIDAluminumAppearance = "3c6d0407-66b3-48ea-a2e8-ee843edf0311";
    const char* UUIDAluminumMixed =  "5f546608-fcbb-40db-98d7-d8e104eb33ce";
    const char* UUIDAluminumPhysical = "a8e60089-550d-4370-8e7e-1734db12a3a9";
    const char* UUIDBrassAppearance = "fff3d5c8-98c3-4ee2-8fe5-7e17403c48fcc";
};

TEST_F(TestMaterialFilter, TestFilters)
{
    ASSERT_NE(_modelManager, nullptr);

    // First check that our materials are loading
    auto material = _materialManager->getMaterial(QString::fromLatin1(UUIDAluminumAppearance));
    ASSERT_TRUE(material);
    ASSERT_EQ(material->getName(), QStringLiteral("TestAluminumAppearance"));
    ASSERT_EQ(material->getUUID(), QString::fromLatin1(UUIDAluminumAppearance));

    material = _materialManager->getMaterial(QString::fromLatin1(UUIDAluminumMixed));
    ASSERT_TRUE(material);
    ASSERT_EQ(material->getName(), QStringLiteral("TestAluminumMixed"));
    ASSERT_EQ(material->getUUID(), QString::fromLatin1(UUIDAluminumMixed));

    material = _materialManager->getMaterial(QString::fromLatin1(UUIDAluminumPhysical));
    ASSERT_TRUE(material);
    ASSERT_EQ(material->getName(), QStringLiteral("TestAluminumPhysical"));
    ASSERT_EQ(material->getUUID(), QString::fromLatin1(UUIDAluminumPhysical));

    material = _materialManager->getMaterial(QString::fromLatin1(UUIDBrassAppearance));
    ASSERT_TRUE(material);
    ASSERT_EQ(material->getName(), QStringLiteral("TestBrassAppearance"));
    ASSERT_EQ(material->getUUID(), QString::fromLatin1(UUIDBrassAppearance));

    material = _materialManager->getMaterialByPath(QStringLiteral("TestAcrylicLegacy.FCMat"),
        QStringLiteral("__UnitTest"));
    ASSERT_TRUE(material);
    ASSERT_EQ(material->getName(), QStringLiteral("TestAcrylicLegacy"));
    ASSERT_EQ(material->getUUID().size(), 36); // We don't know the UUID

    // Create an empty filter
    Materials::MaterialFilter filter;
    Materials::MaterialFilterOptions options;
    ASSERT_TRUE(_library);

    auto tree = _materialManager->getMaterialTree(*_library, filter, options);
    ASSERT_EQ(tree->size(), 4);

    options.setIncludeLegacy(true);
    tree = _materialManager->getMaterialTree(*_library, filter, options);
    ASSERT_EQ(tree->size(), 5);

    // Create a basic rendering filter
    filter.setName(QStringLiteral("Basic Appearance"));
    filter.addRequiredComplete(Materials::ModelUUIDs::ModelUUID_Rendering_Basic);
    options.setIncludeLegacy(false);

    tree = _materialManager->getMaterialTree(*_library, filter, options);
    ASSERT_EQ(tree->size(), 3);

    options.setIncludeLegacy(true);
    tree = _materialManager->getMaterialTree(*_library, filter, options);
    ASSERT_EQ(tree->size(), 3);

    // Create an advanced rendering filter
    filter.clear();
    filter.setName(QStringLiteral("Advanced Appearance"));
    filter.addRequiredComplete(Materials::ModelUUIDs::ModelUUID_Rendering_Advanced);
    options.setIncludeLegacy(false);

    tree = _materialManager->getMaterialTree(*_library, filter, options);
    ASSERT_EQ(tree->size(), 0);

    options.setIncludeLegacy(true);
    tree = _materialManager->getMaterialTree(*_library, filter, options);
    ASSERT_EQ(tree->size(), 0);

    // Create a Density filter
    filter.clear();
    filter.setName(QStringLiteral("Density"));
    filter.addRequiredComplete(Materials::ModelUUIDs::ModelUUID_Mechanical_Density);
    options.setIncludeLegacy(false);

    tree = _materialManager->getMaterialTree(*_library, filter, options);
    ASSERT_EQ(tree->size(), 2);

    options.setIncludeLegacy(true);
    tree = _materialManager->getMaterialTree(*_library, filter, options);
    ASSERT_EQ(tree->size(), 3);

    // Create a Hardness filter
    filter.clear();
    filter.setName(QStringLiteral("Hardness"));
    filter.addRequiredComplete(Materials::ModelUUIDs::ModelUUID_Mechanical_Hardness);
    options.setIncludeLegacy(false);

    tree = _materialManager->getMaterialTree(*_library, filter, options);
    ASSERT_EQ(tree->size(), 0);

    options.setIncludeLegacy(true);
    tree = _materialManager->getMaterialTree(*_library, filter, options);
    ASSERT_EQ(tree->size(), 0);

    // Create a Density and Basic Rendering filter
    filter.clear();
    filter.setName(QStringLiteral("Density and Basic Rendering"));
    filter.addRequiredComplete(Materials::ModelUUIDs::ModelUUID_Rendering_Basic);
    filter.addRequiredComplete(Materials::ModelUUIDs::ModelUUID_Mechanical_Density);
    options.setIncludeLegacy(false);

    tree = _materialManager->getMaterialTree(*_library, filter, options);
    ASSERT_EQ(tree->size(), 1);

    options.setIncludeLegacy(true);
    tree = _materialManager->getMaterialTree(*_library, filter, options);
    ASSERT_EQ(tree->size(), 1);

    // Create a Linear Elastic filter
    filter.clear();
    filter.setName(QStringLiteral("Linear Elastic"));
    filter.addRequiredComplete(Materials::ModelUUIDs::ModelUUID_Mechanical_LinearElastic);
    options.setIncludeLegacy(false);

    tree = _materialManager->getMaterialTree(*_library, filter, options);
    ASSERT_EQ(tree->size(), 0);

    options.setIncludeLegacy(true);
    tree = _materialManager->getMaterialTree(*_library, filter, options);
    ASSERT_EQ(tree->size(), 0);

    filter.clear();
    filter.setName(QStringLiteral("Linear Elastic"));
    filter.addRequired(Materials::ModelUUIDs::ModelUUID_Mechanical_LinearElastic);
    options.setIncludeLegacy(false);

    tree = _materialManager->getMaterialTree(*_library, filter, options);
    ASSERT_EQ(tree->size(), 2);

    options.setIncludeLegacy(true);
    tree = _materialManager->getMaterialTree(*_library, filter, options);
    ASSERT_EQ(tree->size(), 2);
}
