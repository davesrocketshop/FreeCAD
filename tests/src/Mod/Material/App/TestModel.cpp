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

#include <QString>

#include <App/Application.h>
#include <src/App/InitApplication.h>

#include <Mod/Material/App/LibraryManager.h>
#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/Model.h>
#include <Mod/Material/App/ModelManager.h>

// clang-format off

class TestModel : public ::testing::Test {
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

        _modelManager = &(Materials::ModelManager::getManager());
        _libraryManager = &(Materials::LibraryManager::getManager());
    }

    void TearDown() override {
        _libraryManager->setUseExternal(_useExternal);
    }

    Materials::LibraryManager* _libraryManager;
    Materials::ModelManager* _modelManager;
    bool _useExternal {};
};

TEST_F(TestModel, TestApplication)
{
    ASSERT_NO_THROW(App::GetApplication());
}

TEST_F(TestModel, TestResources)
{
    try {
        auto param = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Material/Resources");
        EXPECT_NE(param, nullptr);
    }
    catch (const std::exception &e)
    {
        FAIL() << "Exception: " << e.what() << "\n";
    }
}

TEST_F(TestModel, TestInstallation)
{
    ASSERT_NE(_modelManager, nullptr);

    // We should have loaded at least the system library
    auto libraries = _modelManager->getLibraries();
    ASSERT_GT(libraries->size(), 0);

    // We should have at least one model
    auto models = _modelManager->getModels();
    ASSERT_GT(models->size(), 0);
}

TEST_F(TestModel, TestModelLoad)
{
    ASSERT_NE(_modelManager, nullptr);

    auto density = _modelManager->getModel("454661e5-265b-4320-8e6f-fcf6223ac3af");
    EXPECT_EQ(density->getName(), "Density");
    EXPECT_EQ(density->getUUID(), "454661e5-265b-4320-8e6f-fcf6223ac3af");

    auto& prop = (*density)["Density"];
    EXPECT_EQ(prop.getName(), "Density");
}

TEST_F(TestModel, TestModelByPath)
{
    ASSERT_NE(_modelManager, nullptr);

    std::shared_ptr<Materials::Model> linearElastic;
    ASSERT_NO_THROW(linearElastic = _modelManager->getModelByPath(
        "Mechanical/LinearElastic.yml",
        "System"));
    EXPECT_NE(&linearElastic, nullptr);
    EXPECT_EQ(linearElastic->getName(), "Linear Elastic");
    EXPECT_EQ(linearElastic->getUUID(), "7b561d1d-fb9b-44f6-9da9-56a4f74d7536");

    // The same but with a leading '/'
    std::shared_ptr<Materials::Model> linearElastic2;
    ASSERT_NO_THROW(linearElastic2 = _modelManager->getModelByPath(
        "/Mechanical/LinearElastic.yml",
        "System"));
    EXPECT_NE(&linearElastic2, nullptr);
    EXPECT_EQ(linearElastic2->getName(), "Linear Elastic");
    EXPECT_EQ(linearElastic2->getUUID(), "7b561d1d-fb9b-44f6-9da9-56a4f74d7536");

    // Same with the library name as a prefix
    std::shared_ptr<Materials::Model> linearElastic3;
    ASSERT_NO_THROW(linearElastic3 = _modelManager->getModelByPath(
        "[System]/Mechanical/LinearElastic.yml",
        "System"));
    EXPECT_NE(&linearElastic3, nullptr);
    EXPECT_EQ(linearElastic3->getName(), "Linear Elastic");
    EXPECT_EQ(linearElastic3->getUUID(), "7b561d1d-fb9b-44f6-9da9-56a4f74d7536");

    // Test with the file system path
    ASSERT_NO_THROW(linearElastic->getLibrary());
    ASSERT_NO_THROW(linearElastic->getLibrary()->getName());
    ASSERT_NO_THROW(linearElastic->getLibrary()->getDirectoryPath());
    EXPECT_EQ(linearElastic->getLibrary()->getName(), "System");
    auto path = linearElastic->getLibrary()->getDirectoryPath() + "/Mechanical/LinearElastic.yml";

    ASSERT_NO_THROW(_modelManager->getModelByPath(path));
    auto linearElastic4 = _modelManager->getModelByPath(path);
    EXPECT_NE(&linearElastic4, nullptr);
    EXPECT_EQ(linearElastic4->getName(), "Linear Elastic");
    EXPECT_EQ(linearElastic4->getUUID(), "7b561d1d-fb9b-44f6-9da9-56a4f74d7536");
}

TEST_F(TestModel, TestTestModel)
{
    ASSERT_NE(_modelManager, nullptr);

    // Load 'Test Model.yml'
    auto testModel = _modelManager->getModel("34d0583d-f999-49ba-99e6-aa40bd5c3a6b");
    EXPECT_EQ(testModel->getName(), "Test Model");
    EXPECT_EQ(testModel->getUUID(), "34d0583d-f999-49ba-99e6-aa40bd5c3a6b");

    auto& prop = (*testModel)["TestString"];
    EXPECT_EQ(prop.getName(), "TestString");
    EXPECT_EQ(prop.getDisplayName(), "TestString");
    EXPECT_EQ(prop.getPropertyType(), "String");
    EXPECT_EQ(prop.getUnits(), "");
    EXPECT_EQ(prop.getURL(), "");
    EXPECT_EQ(prop.getDescription(), "A String");
    EXPECT_TRUE(prop.getInheritance().empty());
    EXPECT_FALSE(prop.isInherited());
    EXPECT_EQ(prop.columns(), 0);

    prop = (*testModel)["TestURL"];
    EXPECT_EQ(prop.getName(), "TestURL");
    EXPECT_EQ(prop.getDisplayName(), "TestURL");
    EXPECT_EQ(prop.getPropertyType(), "URL");
    EXPECT_EQ(prop.getUnits(), "");
    EXPECT_EQ(prop.getURL(), "");
    EXPECT_EQ(prop.getDescription(), "A URL");
    EXPECT_TRUE(prop.getInheritance().empty());
    EXPECT_FALSE(prop.isInherited());
    EXPECT_EQ(prop.columns(), 0);

    prop = (*testModel)["TestList"];
    EXPECT_EQ(prop.getName(), "TestList");
    EXPECT_EQ(prop.getDisplayName(), "TestList");
    EXPECT_EQ(prop.getPropertyType(), "List");
    EXPECT_EQ(prop.getUnits(), "");
    EXPECT_EQ(prop.getURL(), "");
    EXPECT_EQ(prop.getDescription(), "A List");
    EXPECT_TRUE(prop.getInheritance().empty());
    EXPECT_FALSE(prop.isInherited());
    EXPECT_EQ(prop.columns(), 0);

    prop = (*testModel)["TestFileList"];
    EXPECT_EQ(prop.getName(), "TestFileList");
    EXPECT_EQ(prop.getDisplayName(), "TestFileList");
    EXPECT_EQ(prop.getPropertyType(), "FileList");
    EXPECT_EQ(prop.getUnits(), "");
    EXPECT_EQ(prop.getURL(), "");
    EXPECT_EQ(prop.getDescription(), "A List of file paths");
    EXPECT_TRUE(prop.getInheritance().empty());
    EXPECT_FALSE(prop.isInherited());
    EXPECT_EQ(prop.columns(), 0);

    prop = (*testModel)["TestImageList"];
    EXPECT_EQ(prop.getName(), "TestImageList");
    EXPECT_EQ(prop.getDisplayName(), "TestImageList");
    EXPECT_EQ(prop.getPropertyType(), "ImageList");
    EXPECT_EQ(prop.getUnits(), "");
    EXPECT_EQ(prop.getURL(), "");
    EXPECT_EQ(prop.getDescription(), "A List of embedded images");
    EXPECT_TRUE(prop.getInheritance().empty());
    EXPECT_FALSE(prop.isInherited());
    EXPECT_EQ(prop.columns(), 0);

    prop = (*testModel)["TestInteger"];
    EXPECT_EQ(prop.getName(), "TestInteger");
    EXPECT_EQ(prop.getDisplayName(), "TestInteger");
    EXPECT_EQ(prop.getPropertyType(), "Integer");
    EXPECT_EQ(prop.getUnits(), "");
    EXPECT_EQ(prop.getURL(), "");
    EXPECT_EQ(prop.getDescription(), "A Integer");
    EXPECT_TRUE(prop.getInheritance().empty());
    EXPECT_FALSE(prop.isInherited());
    EXPECT_EQ(prop.columns(), 0);

    prop = (*testModel)["TestFloat"];
    EXPECT_EQ(prop.getName(), "TestFloat");
    EXPECT_EQ(prop.getDisplayName(), "TestFloat");
    EXPECT_EQ(prop.getPropertyType(), "Float");
    EXPECT_EQ(prop.getUnits(), "");
    EXPECT_EQ(prop.getURL(), "");
    EXPECT_EQ(prop.getDescription(), "A Float");
    EXPECT_TRUE(prop.getInheritance().empty());
    EXPECT_FALSE(prop.isInherited());
    EXPECT_EQ(prop.columns(), 0);

    prop = (*testModel)["TestBoolean"];
    EXPECT_EQ(prop.getName(), "TestBoolean");
    EXPECT_EQ(prop.getDisplayName(), "TestBoolean");
    EXPECT_EQ(prop.getPropertyType(), "Boolean");
    EXPECT_EQ(prop.getUnits(), "");
    EXPECT_EQ(prop.getURL(), "");
    EXPECT_EQ(prop.getDescription(), "A Boolean");
    EXPECT_TRUE(prop.getInheritance().empty());
    EXPECT_FALSE(prop.isInherited());
    EXPECT_EQ(prop.columns(), 0);

    prop = (*testModel)["TestColor"];
    EXPECT_EQ(prop.getName(), "TestColor");
    EXPECT_EQ(prop.getDisplayName(), "TestColor");
    EXPECT_EQ(prop.getPropertyType(), "Color");
    EXPECT_EQ(prop.getUnits(), "");
    EXPECT_EQ(prop.getURL(), "");
    EXPECT_EQ(prop.getDescription(), "A Color");
    EXPECT_TRUE(prop.getInheritance().empty());
    EXPECT_FALSE(prop.isInherited());
    EXPECT_EQ(prop.columns(), 0);

    prop = (*testModel)["TestFile"];
    EXPECT_EQ(prop.getName(), "TestFile");
    EXPECT_EQ(prop.getDisplayName(), "TestFile");
    EXPECT_EQ(prop.getPropertyType(), "File");
    EXPECT_EQ(prop.getUnits(), "");
    EXPECT_EQ(prop.getURL(), "");
    EXPECT_EQ(prop.getDescription(), "A File");
    EXPECT_TRUE(prop.getInheritance().empty());
    EXPECT_FALSE(prop.isInherited());
    EXPECT_EQ(prop.columns(), 0);

    prop = (*testModel)["TestSVG"];
    EXPECT_EQ(prop.getName(), "TestSVG");
    EXPECT_EQ(prop.getDisplayName(), "TestSVG");
    EXPECT_EQ(prop.getPropertyType(), "SVG");
    EXPECT_EQ(prop.getUnits(), "");
    EXPECT_EQ(prop.getURL(), "");
    EXPECT_EQ(prop.getDescription(), "An SVG");
    EXPECT_TRUE(prop.getInheritance().empty());
    EXPECT_FALSE(prop.isInherited());
    EXPECT_EQ(prop.columns(), 0);

    prop = (*testModel)["TestImage"];
    EXPECT_EQ(prop.getName(), "TestImage");
    EXPECT_EQ(prop.getDisplayName(), "TestImage");
    EXPECT_EQ(prop.getPropertyType(), "Image");
    EXPECT_EQ(prop.getUnits(), "");
    EXPECT_EQ(prop.getURL(), "");
    EXPECT_EQ(prop.getDescription(), "An Image");
    EXPECT_TRUE(prop.getInheritance().empty());
    EXPECT_FALSE(prop.isInherited());
    EXPECT_EQ(prop.columns(), 0);

    prop = (*testModel)["TestQuantity"];
    EXPECT_EQ(prop.getName(), "TestQuantity");
    EXPECT_EQ(prop.getDisplayName(), "TestQuantity");
    EXPECT_EQ(prop.getPropertyType(), "Quantity");
    EXPECT_EQ(prop.getUnits(), "kg/m^3");
    EXPECT_EQ(prop.getURL(), "");
    EXPECT_EQ(prop.getDescription(), "A Quantity");
    EXPECT_TRUE(prop.getInheritance().empty());
    EXPECT_FALSE(prop.isInherited());
    EXPECT_EQ(prop.columns(), 0);

    prop = (*testModel)["TestMultiLineString"];
    EXPECT_EQ(prop.getName(), "TestMultiLineString");
    EXPECT_EQ(prop.getDisplayName(), "TestMultiLineString");
    EXPECT_EQ(prop.getPropertyType(), "MultiLineString");
    EXPECT_EQ(prop.getUnits(), "");
    EXPECT_EQ(prop.getURL(), "");
    EXPECT_EQ(prop.getDescription(), "A string that spans multiple lines");
    EXPECT_TRUE(prop.getInheritance().empty());
    EXPECT_FALSE(prop.isInherited());
    EXPECT_EQ(prop.columns(), 0);

    prop = (*testModel)["TestArray2D"];
    EXPECT_EQ(prop.getName(), "TestArray2D");
    EXPECT_EQ(prop.getDisplayName(), "TestArray2D");
    EXPECT_EQ(prop.getPropertyType(), "2DArray");
    EXPECT_EQ(prop.getUnits(), "");
    EXPECT_EQ(prop.getURL(), "");
    EXPECT_EQ(prop.getDescription(), "2 Dimensional array showing density with temperature");
    EXPECT_TRUE(prop.getInheritance().empty());
    EXPECT_FALSE(prop.isInherited());
    EXPECT_EQ(prop.columns(), 2);

    auto columns = prop.getColumns();
    auto entry = columns.at(0);
    EXPECT_EQ(entry.getName(), "Temperature");
    EXPECT_EQ(entry.getDisplayName(), "Temperature");
    EXPECT_EQ(entry.getPropertyType(), "Quantity");
    EXPECT_EQ(entry.getUnits(), "C");
    EXPECT_EQ(entry.getURL(), "");
    EXPECT_EQ(entry.getDescription(), "Temperature");
    EXPECT_TRUE(entry.getInheritance().empty());
    EXPECT_FALSE(entry.isInherited());
    EXPECT_EQ(entry.columns(), 0);

    entry = columns.at(1);
    EXPECT_EQ(entry.getName(), "Density");
    EXPECT_EQ(entry.getDisplayName(), "Density");
    EXPECT_EQ(entry.getPropertyType(), "Quantity");
    EXPECT_EQ(entry.getUnits(), "kg/m^3");
    EXPECT_EQ(entry.getURL(), "https://en.wikipedia.org/wiki/Density");
    EXPECT_EQ(entry.getDescription(), "Density in [FreeCAD Density unit]");
    EXPECT_TRUE(entry.getInheritance().empty());
    EXPECT_FALSE(entry.isInherited());
    EXPECT_EQ(entry.columns(), 0);

    prop = (*testModel)["TestArray2D3Column"];
    EXPECT_EQ(prop.getName(), "TestArray2D3Column");
    EXPECT_EQ(prop.getDisplayName(), "TestArray2D3Column");
    EXPECT_EQ(prop.getPropertyType(), "2DArray");
    EXPECT_EQ(prop.getUnits(), "");
    EXPECT_EQ(prop.getURL(), "");
    EXPECT_EQ(prop.getDescription(), "2 Dimensional array showing density and initial yield stress with temperature");
    EXPECT_TRUE(prop.getInheritance().empty());
    EXPECT_FALSE(prop.isInherited());
    EXPECT_EQ(prop.columns(), 3);

    columns = prop.getColumns();
    entry = columns.at(0);
    EXPECT_EQ(entry.getName(), "Temperature");
    EXPECT_EQ(entry.getDisplayName(), "Temperature");
    EXPECT_EQ(entry.getPropertyType(), "Quantity");
    EXPECT_EQ(entry.getUnits(), "C");
    EXPECT_EQ(entry.getURL(), "");
    EXPECT_EQ(entry.getDescription(), "Temperature");
    EXPECT_TRUE(entry.getInheritance().empty());
    EXPECT_FALSE(entry.isInherited());
    EXPECT_EQ(entry.columns(), 0);

    entry = columns.at(1);
    EXPECT_EQ(entry.getName(), "Density");
    EXPECT_EQ(entry.getDisplayName(), "Density");
    EXPECT_EQ(entry.getPropertyType(), "Quantity");
    EXPECT_EQ(entry.getUnits(), "kg/m^3");
    EXPECT_EQ(entry.getURL(), "https://en.wikipedia.org/wiki/Density");
    EXPECT_EQ(entry.getDescription(), "Density in [FreeCAD Density unit]");
    EXPECT_TRUE(entry.getInheritance().empty());
    EXPECT_FALSE(entry.isInherited());
    EXPECT_EQ(entry.columns(), 0);

    entry = columns.at(2);
    EXPECT_EQ(entry.getName(), "InitialYieldStress");
    EXPECT_EQ(entry.getDisplayName(), "InitialYieldStress");
    EXPECT_EQ(entry.getPropertyType(), "Quantity");
    EXPECT_EQ(entry.getUnits(), "kPa");
    EXPECT_EQ(entry.getURL(), "");
    EXPECT_EQ(entry.getDescription(), "Saturation stress for Voce isotropic hardening [FreeCAD Pressure unit]");
    EXPECT_TRUE(entry.getInheritance().empty());
    EXPECT_FALSE(entry.isInherited());
    EXPECT_EQ(entry.columns(), 0);

    prop = (*testModel)["TestArray3D"];
    EXPECT_EQ(prop.getName(), "TestArray3D");
    EXPECT_EQ(prop.getDisplayName(), "TestArray3D");
    EXPECT_EQ(prop.getPropertyType(), "3DArray");
    EXPECT_EQ(prop.getUnits(), "");
    EXPECT_EQ(prop.getURL(), "");
    EXPECT_EQ(prop.getDescription(), "3 Dimensional array showing stress and strain as a function of temperature");
    EXPECT_TRUE(prop.getInheritance().empty());
    EXPECT_FALSE(prop.isInherited());
    EXPECT_EQ(prop.columns(), 3);

    columns = prop.getColumns();
    entry = columns.at(0);
    EXPECT_EQ(entry.getName(), "Temperature");
    EXPECT_EQ(entry.getDisplayName(), "Temperature");
    EXPECT_EQ(entry.getPropertyType(), "Quantity");
    EXPECT_EQ(entry.getUnits(), "C");
    EXPECT_EQ(entry.getURL(), "");
    EXPECT_EQ(entry.getDescription(), "Temperature");
    EXPECT_TRUE(entry.getInheritance().empty());
    EXPECT_FALSE(entry.isInherited());
    EXPECT_EQ(entry.columns(), 0);

    entry = columns.at(1);
    EXPECT_EQ(entry.getName(), "Stress");
    EXPECT_EQ(entry.getDisplayName(), "Stress");
    EXPECT_EQ(entry.getPropertyType(), "Quantity");
    EXPECT_EQ(entry.getUnits(), "MPa");
    EXPECT_EQ(entry.getURL(), "");
    EXPECT_EQ(entry.getDescription(), "Stress");
    EXPECT_TRUE(entry.getInheritance().empty());
    EXPECT_FALSE(entry.isInherited());
    EXPECT_EQ(entry.columns(), 0);

    entry = columns.at(2);
    EXPECT_EQ(entry.getName(), "Strain");
    EXPECT_EQ(entry.getDisplayName(), "Strain");
    EXPECT_EQ(entry.getPropertyType(), "Quantity");
    EXPECT_EQ(entry.getUnits(), "MPa");
    EXPECT_EQ(entry.getURL(), "");
    EXPECT_EQ(entry.getDescription(), "Strain");
    EXPECT_TRUE(entry.getInheritance().empty());
    EXPECT_FALSE(entry.isInherited());
    EXPECT_EQ(entry.columns(), 0);
}

// clang-format on
