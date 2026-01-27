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
    material->setLibrary(library);
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_New);
    material->setDirectory("a/b/c";)
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_New);

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
    material->addTag(QStringLiteral("Henry"));
    material->addTag(QStringLiteral("Ralph"));
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_New);
    material->removeTag(QStringLiteral("Henry"));
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_New);

    // Test adding a model
    material->addPhysical(QStringLiteral("454661e5-265b-4320-8e6f-fcf6223ac3af")); // Density
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_New);
    material->addAppearance(QStringLiteral("f006c7e4-35b7-43d5-bbf9-c5d572309e6e")); // BasicRendering
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_New);

    // Test modifying a property
    material->setPhysicalValue("Density", "1.0 kg/m^3");
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_New);
    material->setAppearanceValue("DiffuseColor", "(0.7804, 0.5686, 0.1137, 1.0)");
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_New);

    // Test removing a model
    material->removePhysical(QStringLiteral("454661e5-265b-4320-8e6f-fcf6223ac3af")); // Density
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_New);
    material->removeAppearance(QStringLiteral("f006c7e4-35b7-43d5-bbf9-c5d572309e6e")); // BasicRendering
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_New);

    // Test save
}

TEST_F(TestMaterialModification, TestAlter)
{
    auto library = _materialManager->getLibrary(QStringLiteral("User"));
    ASSERT_NE(library, nullptr);

    auto systemMaterial = _materialManager->getMaterial(QStringLiteral("c6c64159-19c1-40b5-859c-10561f20f979"));
    auto material = std::make_shared<Materials::Material>(systemMaterial);
    ASSERT_NE(material, nullptr);
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_None);
    material->resetEditState();

    // Set the library and path
    material->setLibrary(library);
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Extend);
    material->resetEditState();
    material->setDirectory("a/b/c";)
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Extend);
    material->resetEditState();

    // Modify basic properties
    material->setName(QStringLiteral("Name"));
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Extend);
    material->resetEditState();
    material->setAuthor(QStringLiteral("Author"));
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Extend);
    material->resetEditState();
    material->setLicense(QStringLiteral("License"));
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Extend);
    material->resetEditState();
    material->setParentUUID(QStringLiteral("Author"));
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Extend);
    material->resetEditState();
    material->setDescription(QStringLiteral("Description"));
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Extend);
    material->resetEditState();
    material->setURL(QStringLiteral("URL"));
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Extend);
    material->resetEditState();
    material->setReference(QStringLiteral("Reference"));
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Extend);
    material->resetEditState();

    // Test tags
    material->addTag(QStringLiteral("Henry"));
    material->addTag(QStringLiteral("Ralph"));
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Extend);
    material->resetEditState();
    material->removeTag(QStringLiteral("Henry"));
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Extend);
    material->resetEditState();

    // Test adding a model
    material->addPhysical(QStringLiteral("454661e5-265b-4320-8e6f-fcf6223ac3af")); // Density
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Extend);
    material->resetEditState();
    material->addAppearance(QStringLiteral("f006c7e4-35b7-43d5-bbf9-c5d572309e6e")); // BasicRendering
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Extend);
    material->resetEditState();

    // Test modifying a property
    material->setPhysicalValue("Density", "1.0 kg/m^3"); // No previous value
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Extend);
    material->resetEditState();
    material->setPhysicalValue("TestQuantity", "1.0 kg/m^3");
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Alter);
    material->resetEditState();
    material->setAppearanceValue("DiffuseColor", "(0.7804, 0.5686, 0.1137, 1.0)"); // No previous value
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Extend);
    material->resetEditState();

    // Test removing a model
    material->removePhysical(QStringLiteral("454661e5-265b-4320-8e6f-fcf6223ac3af")); // Density
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Alter);
    material->resetEditState();
    material->removeAppearance(QStringLiteral("f006c7e4-35b7-43d5-bbf9-c5d572309e6e")); // BasicRendering
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Alter);
    material->resetEditState();

    // Test adding and modifying a model
    material->addPhysical(QStringLiteral("454661e5-265b-4320-8e6f-fcf6223ac3af")); // Density
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Extend);
    material->addAppearance(QStringLiteral("f006c7e4-35b7-43d5-bbf9-c5d572309e6e")); // BasicRendering
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Extend);
    material->setPhysicalValue("Density", "1.0 kg/m^3"); // No previous value
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Extend);
    material->setPhysicalValue("TestQuantity", "1.0 kg/m^3");
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Extend);
    material->setAppearanceValue("DiffuseColor", "(0.7804, 0.5686, 0.1137, 1.0)"); // No previous value
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Extend);
    material->removePhysical(QStringLiteral("454661e5-265b-4320-8e6f-fcf6223ac3af")); // Density
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Extend);
    material->removeAppearance(QStringLiteral("f006c7e4-35b7-43d5-bbf9-c5d572309e6e")); // BasicRendering
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Extend);
    material->resetEditState();

    // Test adding and modifying a model with existing values
    material->addPhysical(QStringLiteral("454661e5-265b-4320-8e6f-fcf6223ac3af")); // Density
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Extend);
    material->addAppearance(QStringLiteral("f006c7e4-35b7-43d5-bbf9-c5d572309e6e")); // BasicRendering
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Extend);
    material->setPhysicalValue("Density", "1.0 kg/m^3"); // No previous value
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Extend);
    material->setPhysicalValue("TestQuantity", "1.0 kg/m^3");
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Extend);
    material->setAppearanceValue("DiffuseColor", "(0.7804, 0.5686, 0.1137, 1.0)"); // No previous value
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Extend);
    material->setPhysicalValue("TestQuantity", "1.0 kg/m^3");
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Alter);
    material->removePhysical(QStringLiteral("454661e5-265b-4320-8e6f-fcf6223ac3af")); // Density
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Alter);
    material->removeAppearance(QStringLiteral("f006c7e4-35b7-43d5-bbf9-c5d572309e6e")); // BasicRendering
    ASSERT_EQ(material->getEditState(), Materials::Material::ModelEdit_Alter);
    material->resetEditState();

    // Test save
}

// clang-format on
