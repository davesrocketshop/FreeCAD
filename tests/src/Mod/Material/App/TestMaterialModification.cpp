// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2026 David Carter <dcarter@david.carter.ca>             *
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

#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/MaterialValue.h>
#include <Mod/Material/App/Model.h>
#include <Mod/Material/App/ModelManager.h>
#include <Mod/Material/App/ModelUuids.h>

#ifdef _MSC_VER
# pragma warning(disable : 4834)
#endif

// clang-format off

class TestMaterialModification : public ::testing::Test {

protected:
    static void SetUpTestSuite() {
        if (App::Application::GetARGC() == 0) {
            tests::initApplication();
        }
    }

    void SetUp() override {
        Base::Interpreter().runString("import Part");
        _modelManager = &(Materials::ModelManager::getManager());
        _materialManager = &(Materials::MaterialManager::getManager());
    }

    // void TearDown() override {}
    Materials::ModelManager* _modelManager;
    Materials::MaterialManager* _materialManager;
};

TEST_F(TestMaterialModification, TestNew)
{
    auto library = _materialManager->getLibrary(QStringLiteral("User"));
    ASSERT_NE(library, nullptr);

    auto material = std::make_shared<Materials::Material>();
    ASSERT_NE(material, nullptr);
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_None);
    material->setEditStateNew();
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_New);

    // Set the library and path

    // Modify basic properties
    material->setName(QStringLiteral("Name"));
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_New);
    material->setAuthor(QStringLiteral("Author"));
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_New);
    material->setLicense(QStringLiteral("License"));
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_New);
    material->setParentUUID(QStringLiteral("Author"));
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_New);
    material->setDescription(QStringLiteral("Description"));
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_New);
    material->setURL(QStringLiteral("URL"));
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_New);
    material->setReference(QStringLiteral("Reference"));
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_New);

    // Test tags

    // Test adding a model

    // Test modifying a property

    // Test save
}

TEST_F(TestMaterialModification, TestAlter)
{
    ASSERT_NE(_modelManager, nullptr);

    // We should have loaded at least the system library
    auto libraries = _materialManager->getLibraries();
    ASSERT_GT(libraries->size(), 0);

    // We should have at least one material
    auto materials = _materialManager->getLocalMaterials();
    ASSERT_GT(materials->size(), 0);
}

TEST_F(TestMaterialModification, TestExtend)
{
    ASSERT_NE(_modelManager, nullptr);

    // We should have loaded at least the system library
    auto libraries = _materialManager->getLibraries();
    ASSERT_GT(libraries->size(), 0);

    // We should have at least one material
    auto materials = _materialManager->getLocalMaterials();
    ASSERT_GT(materials->size(), 0);
}

// clang-format on
