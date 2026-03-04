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

#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/MaterialValue.h>
#include <Mod/Material/App/Model.h>
#include <Mod/Material/App/ModelManager.h>
#include <Mod/Material/App/ModelUuids.h>

#ifdef _MSC_VER
# pragma warning(disable : 4834)
#endif

// clang-format off

class TestMaterial : public ::testing::Test {
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
        _materialManager = &(Materials::MaterialManager::getManager());

        // Create a custom library for our test files
        // Ensure the directory exists
        std::string testPath = App::Application::getHomePath() + "/tests/Materials/";
        Base::FileInfo directory(testPath);
        ASSERT_TRUE(directory.exists());
        std::string modelPath = App::Application::getHomePath() + "/tests/Models/";
        // std::string modelPath = App::Application::getResourceDir() + "/Mod/Material/Resources/Models";
        Base::FileInfo modelDirectory(modelPath);
        ASSERT_TRUE(modelDirectory.exists());

        // Remove the library if it exists
        try {
            _materialManager->removeLibrary("__UnitTest");
        }
        catch (const Materials::LibraryNotFound&) {
            // ignore
        }

        _materialManager->createLocalLibrary("__UnitTest",
                            testPath,
                            modelPath,
                            ":/icons/preferences-general.svg",
                            false);
        _materialManager->refresh();

        ASSERT_NO_THROW(_materialManager->getLibrary("__UnitTest"));
    }

    void TearDown() override {
        // Restore the external interface
        ParameterGrp::handle paramExternal = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Material/ExternalInterface"
        );

        paramExternal->SetBool("UseExternal", _useExternal);

        try {
            _materialManager->removeLibrary("__UnitTest");
        }
        catch (const Materials::LibraryNotFound&) {
            // ignore
        }
        ASSERT_THROW(_materialManager->getLibrary("__UnitTest"), Materials::LibraryNotFound);
    }

    Materials::ModelManager* _modelManager;
    Materials::MaterialManager* _materialManager;
    bool _useExternal {};
};

TEST_F(TestMaterial, TestInstallation)
{
    ASSERT_NE(_modelManager, nullptr);

    // We should have loaded at least the system library
    auto libraries = _materialManager->getLibraries();
    ASSERT_GT(libraries->size(), 0);

    // We should have at least one material
    auto materials = _materialManager->getLocalMaterials();
    ASSERT_GT(materials->size(), 0);
}

TEST_F(TestMaterial, TestMaterialsWithModel)
{
    ASSERT_NE(_materialManager, nullptr);

    auto materials = _materialManager->materialsWithModel(
        "f6f9e48c-b116-4e82-ad7f-3659a9219c50"); // IsotropicLinearElastic
    EXPECT_GT(materials->size(), 0);

    auto materialsComplete = _materialManager->materialsWithModelComplete(
        "f6f9e48c-b116-4e82-ad7f-3659a9219c50");  // IsotropicLinearElastic
    EXPECT_LE(materialsComplete->size(), materials->size());

    auto materialsLinearElastic = _materialManager->materialsWithModel(
        "7b561d1d-fb9b-44f6-9da9-56a4f74d7536"); // LinearElastic

    // All LinearElastic models should be in IsotropicLinearElastic since it is inherited
    EXPECT_LE(materialsLinearElastic->size(), materials->size());
    for (auto &itp : *materialsLinearElastic) {
        auto mat = itp.first;
        EXPECT_NO_THROW(materials->at(mat));
    }
}

TEST_F(TestMaterial, TestMaterialByPath)
{
    ASSERT_NE(_materialManager, nullptr);

    std::shared_ptr<Materials::Material> steel;
    ASSERT_NO_THROW(steel = _materialManager->getMaterialByPath(
        "Standard/Metal/Steel/CalculiX-Steel.FCMat",
        "System"));
    ASSERT_NE(&steel, nullptr);
    EXPECT_EQ(steel->getName(), QStringLiteral("CalculiX-Steel"));
    EXPECT_EQ(steel->getUUID(), QStringLiteral("92589471-a6cb-4bbc-b748-d425a17dea7d"));

    // The same but with a leading '/'
    std::shared_ptr<Materials::Material> steel2;
    ASSERT_NO_THROW(steel2 = _materialManager->getMaterialByPath(
        "/Standard/Metal/Steel/CalculiX-Steel.FCMat",
        "System"));
    ASSERT_NE(&steel2, nullptr);
    EXPECT_EQ(steel2->getName(), QStringLiteral("CalculiX-Steel"));
    EXPECT_EQ(steel2->getUUID(), QStringLiteral("92589471-a6cb-4bbc-b748-d425a17dea7d"));

    // Same with the library name as a prefix
    std::shared_ptr<Materials::Material> steel3;
    ASSERT_NO_THROW(steel3 = _materialManager->getMaterialByPath(
        "[System]/Standard/Metal/Steel/CalculiX-Steel.FCMat",
        "System"));
    ASSERT_NE(&steel3, nullptr);
    EXPECT_EQ(steel3->getName(), QStringLiteral("CalculiX-Steel"));
    EXPECT_EQ(steel3->getUUID(), QStringLiteral("92589471-a6cb-4bbc-b748-d425a17dea7d"));
}

TEST_F(TestMaterial, TestAddPhysicalModel)
{
    // Start with an empty material
    Materials::Material material;
    auto models = material.getPhysicalModels();
    EXPECT_NE(&models, nullptr);
    EXPECT_EQ(models->size(), 0);

    // Add a model
    material.addPhysical(Materials::ModelUUIDs::ModelUUID_Electromagnetic_Default);
    models = material.getPhysicalModels();
    EXPECT_EQ(models->size(), 1);

    // Add a second model
    material.addPhysical(Materials::ModelUUIDs::ModelUUID_Mechanical_LinearElastic);
    models = material.getPhysicalModels();
    EXPECT_EQ(models->size(), 2);

    // Add an inherited model
    material.addPhysical(Materials::ModelUUIDs::ModelUUID_Mechanical_IsotropicLinearElastic);
    models = material.getPhysicalModels();
    EXPECT_EQ(models->size(), 2);

    // Add a super model
    material.clearModels();
    EXPECT_EQ(models->size(), 0);

    material.addPhysical(Materials::ModelUUIDs::ModelUUID_Mechanical_IsotropicLinearElastic);
    models = material.getPhysicalModels();
    EXPECT_EQ(models->size(), 1);
    material.addPhysical(Materials::ModelUUIDs::ModelUUID_Mechanical_LinearElastic);
    models = material.getPhysicalModels();
    EXPECT_EQ(models->size(), 1);

    // Remove the inherited model
    material.removePhysical(Materials::ModelUUIDs::ModelUUID_Mechanical_IsotropicLinearElastic);
    models = material.getPhysicalModels();
    EXPECT_EQ(models->size(), 1);

    // Remove the super model
    material.removePhysical(Materials::ModelUUIDs::ModelUUID_Mechanical_LinearElastic);
    models = material.getPhysicalModels();
    EXPECT_EQ(models->size(), 0);
}

TEST_F(TestMaterial, TestAddAppearanceModel)
{
    // Start with an empty material
    Materials::Material material;
    auto models = material.getAppearanceModels();
    EXPECT_NE(models, nullptr);
    EXPECT_EQ(models->size(), 0);

    // Add a model
    material.addAppearance(Materials::ModelUUIDs::ModelUUID_Rendering_Vector);
    models = material.getAppearanceModels();
    EXPECT_EQ(models->size(), 1);

    // Add a second model
    material.addAppearance(Materials::ModelUUIDs::ModelUUID_Rendering_Advanced);
    models = material.getAppearanceModels();
    EXPECT_EQ(models->size(), 2);

    // Add an inherited model
    material.addAppearance(Materials::ModelUUIDs::ModelUUID_Rendering_Basic);
    models = material.getAppearanceModels();
    EXPECT_EQ(models->size(), 2);

    // Add a super model
    material.clearModels();
    EXPECT_EQ(models->size(), 0);

    material.addAppearance(Materials::ModelUUIDs::ModelUUID_Rendering_Basic);
    models = material.getAppearanceModels();
    EXPECT_EQ(models->size(), 1);
    material.addAppearance(Materials::ModelUUIDs::ModelUUID_Rendering_Advanced);
    models = material.getAppearanceModels();
    EXPECT_EQ(models->size(), 1);

    // Remove the inherited model
    material.removeAppearance(Materials::ModelUUIDs::ModelUUID_Rendering_Basic);
    models = material.getAppearanceModels();
    EXPECT_EQ(models->size(), 1);

    // Remove the super model
    material.removeAppearance(Materials::ModelUUIDs::ModelUUID_Rendering_Advanced);
    models = material.getAppearanceModels();
    EXPECT_EQ(models->size(), 0);
}

QString parseQuantity(const std::string& value)
{
    auto quantity = Base::Quantity::parse(value);
    quantity.setFormat(Materials::MaterialValue::getQuantityFormat());
    return QString::fromStdString(quantity.getUserString());
}

TEST_F(TestMaterial, TestCalculiXSteel)
{
    ASSERT_NE(_materialManager, nullptr);

    auto steel = _materialManager->getMaterial("92589471-a6cb-4bbc-b748-d425a17dea7d");
    EXPECT_EQ(steel->getName(), "CalculiX-Steel");
    EXPECT_EQ(steel->getUUID(), "92589471-a6cb-4bbc-b748-d425a17dea7d");

    EXPECT_TRUE(steel->hasPhysicalModel(Materials::ModelUUIDs::ModelUUID_Mechanical_Density)); // Density
    EXPECT_TRUE(steel->hasPhysicalModel(Materials::ModelUUIDs::ModelUUID_Mechanical_IsotropicLinearElastic)); // IsotropicLinearElastic
    EXPECT_TRUE(steel->hasPhysicalModel(Materials::ModelUUIDs::ModelUUID_Thermal_Default)); // Thermal
    EXPECT_FALSE(steel->hasPhysicalModel(Materials::ModelUUIDs::ModelUUID_Mechanical_LinearElastic)); // Legacy linear elastic - Not in the model
    EXPECT_TRUE(steel->hasAppearanceModel(Materials::ModelUUIDs::ModelUUID_Rendering_Basic)); // BasicRendering - inherited from Steel.FCMat

    EXPECT_TRUE(steel->isPhysicalModelComplete(Materials::ModelUUIDs::ModelUUID_Mechanical_Density)); // Density
    EXPECT_FALSE(steel->isPhysicalModelComplete(Materials::ModelUUIDs::ModelUUID_Mechanical_IsotropicLinearElastic)); // IsotropicLinearElastic - incomplete
    EXPECT_FALSE(steel->isPhysicalModelComplete(Materials::ModelUUIDs::ModelUUID_Thermal_Default)); // Thermal
    EXPECT_FALSE(steel->isPhysicalModelComplete(Materials::ModelUUIDs::ModelUUID_Mechanical_LinearElastic)); // Legacy linear elastic - Not in the model
    EXPECT_TRUE(steel->isAppearanceModelComplete(Materials::ModelUUIDs::ModelUUID_Rendering_Basic)); // BasicRendering - inherited from Steel.FCMat

    EXPECT_TRUE(steel->hasPhysicalProperty("Density"));
    EXPECT_TRUE(steel->hasPhysicalProperty("BulkModulus"));
    EXPECT_TRUE(steel->hasPhysicalProperty("PoissonRatio"));
    EXPECT_TRUE(steel->hasPhysicalProperty("YoungsModulus"));
    EXPECT_TRUE(steel->hasPhysicalProperty("ShearModulus"));
    EXPECT_TRUE(steel->hasPhysicalProperty("SpecificHeat"));
    EXPECT_TRUE(steel->hasPhysicalProperty("ThermalConductivity"));
    EXPECT_TRUE(steel->hasPhysicalProperty("ThermalExpansionCoefficient"));
    EXPECT_TRUE(steel->hasAppearanceProperty("AmbientColor"));
    EXPECT_TRUE(steel->hasAppearanceProperty("DiffuseColor"));
    EXPECT_TRUE(steel->hasAppearanceProperty("EmissiveColor"));
    EXPECT_TRUE(steel->hasAppearanceProperty("Shininess"));
    EXPECT_TRUE(steel->hasAppearanceProperty("SpecularColor"));
    EXPECT_TRUE(steel->hasAppearanceProperty("Transparency"));

    auto& properties = steel->getPhysicalProperties();
    EXPECT_NO_THROW(properties.at("Density"));
    EXPECT_NO_THROW(properties.at("BulkModulus")); // This is different from the Python behaviour
    EXPECT_NO_THROW(properties.at("PoissonRatio"));
    EXPECT_NO_THROW(properties.at("YoungsModulus"));
    EXPECT_NO_THROW(properties.at("ShearModulus"));
    EXPECT_NO_THROW(properties.at("SpecificHeat"));
    EXPECT_NO_THROW(properties.at("ThermalConductivity"));
    EXPECT_NO_THROW(properties.at("ThermalExpansionCoefficient"));
    EXPECT_THROW(properties.at("AmbientColor"), std::out_of_range);
    EXPECT_THROW(properties.at("DiffuseColor"), std::out_of_range);
    EXPECT_THROW(properties.at("EmissiveColor"), std::out_of_range);
    EXPECT_THROW(properties.at("Shininess"), std::out_of_range);
    EXPECT_THROW(properties.at("SpecularColor"), std::out_of_range);
    EXPECT_THROW(properties.at("Transparency"), std::out_of_range);

    auto& properties1 = steel->getAppearanceProperties();
    EXPECT_THROW(properties1.at("Density"), std::out_of_range);
    EXPECT_THROW(properties1.at("BulkModulus"), std::out_of_range);
    EXPECT_THROW(properties1.at("PoissonRatio"), std::out_of_range);
    EXPECT_THROW(properties1.at("YoungsModulus"), std::out_of_range);
    EXPECT_THROW(properties1.at("ShearModulus"), std::out_of_range);
    EXPECT_THROW(properties1.at("SpecificHeat"), std::out_of_range);
    EXPECT_THROW(properties1.at("ThermalConductivity"), std::out_of_range);
    EXPECT_THROW(properties1.at("ThermalExpansionCoefficient"), std::out_of_range);
    EXPECT_NO_THROW(properties1.at("AmbientColor"));
    EXPECT_NO_THROW(properties1.at("DiffuseColor"));
    EXPECT_NO_THROW(properties1.at("EmissiveColor"));
    EXPECT_NO_THROW(properties1.at("Shininess"));
    EXPECT_NO_THROW(properties1.at("SpecularColor"));
    EXPECT_NO_THROW(properties1.at("Transparency"));

    EXPECT_FALSE(properties["Density"]->isNull());
    EXPECT_TRUE(properties["BulkModulus"]->isNull());
    EXPECT_FALSE(properties["PoissonRatio"]->isNull());
    EXPECT_FALSE(properties["YoungsModulus"]->isNull());
    EXPECT_TRUE(properties["ShearModulus"]->isNull());
    EXPECT_FALSE(properties["SpecificHeat"]->isNull());
    EXPECT_FALSE(properties["ThermalConductivity"]->isNull());
    EXPECT_FALSE(properties["ThermalExpansionCoefficient"]->isNull());
    EXPECT_FALSE(properties1["AmbientColor"]->isNull());
    EXPECT_FALSE(properties1["DiffuseColor"]->isNull());
    EXPECT_FALSE(properties1["EmissiveColor"]->isNull());
    EXPECT_FALSE(properties1["Shininess"]->isNull());
    EXPECT_FALSE(properties1["SpecularColor"]->isNull());
    EXPECT_FALSE(properties1["Transparency"]->isNull());

    QLocale locale;
    EXPECT_EQ(properties["Density"]->getString(), parseQuantity("7900.00 kg/m^3"));
    EXPECT_EQ(properties["PoissonRatio"]->getString(), locale.toString(0.3));
    EXPECT_EQ(properties["YoungsModulus"]->getString(), parseQuantity("210.00 GPa"));
    EXPECT_EQ(properties["SpecificHeat"]->getString(), parseQuantity("590.00 J/kg/K"));
    EXPECT_EQ(properties["ThermalConductivity"]->getString(), parseQuantity("43.00 W/m/K"));
    EXPECT_EQ(properties["ThermalExpansionCoefficient"]->getString(), parseQuantity("12.00 µm/m/K"));
    EXPECT_EQ(properties1["AmbientColor"]->getString(), "(0.0020, 0.0020, 0.0020, 1.0)");
    EXPECT_EQ(properties1["DiffuseColor"]->getString(), "(0.0000, 0.0000, 0.0000, 1.0)");
    EXPECT_EQ(properties1["EmissiveColor"]->getString(), "(0.0000, 0.0000, 0.0000, 1.0)");
    EXPECT_EQ(properties1["Shininess"]->getString(), locale.toString(0.06));
    EXPECT_EQ(properties1["SpecularColor"]->getString(), "(0.9800, 0.9800, 0.9800, 1.0)");
    EXPECT_EQ(properties1["Transparency"]->getString(), "0");

    EXPECT_TRUE(properties["BulkModulus"]->getString().empty());
    EXPECT_TRUE(properties["ShearModulus"]->getString().empty());

    // These are the preferred method of access
    //
    EXPECT_DOUBLE_EQ(steel->getPhysicalQuantity("Density").getValue(), 7.9e-06);
    EXPECT_NEAR(steel->getPhysicalValue("PoissonRatio").toDouble(), 0.3, 1e-6);
    EXPECT_DOUBLE_EQ(steel->getPhysicalQuantity("YoungsModulus").getValue(), 210000000.0);
    EXPECT_DOUBLE_EQ(steel->getPhysicalQuantity("SpecificHeat").getValue(), 590000000.0);
    EXPECT_DOUBLE_EQ(steel->getPhysicalQuantity("ThermalConductivity").getValue(), 43000.0);
    EXPECT_DOUBLE_EQ(steel->getPhysicalQuantity("ThermalExpansionCoefficient").getValue(), 1.2e-05);
    EXPECT_EQ(steel->getAppearanceValue("AmbientColor"), "(0.0020, 0.0020, 0.0020, 1.0)");
    EXPECT_EQ(steel->getAppearanceValue("DiffuseColor"), "(0.0000, 0.0000, 0.0000, 1.0)");
    EXPECT_EQ(steel->getAppearanceValue("EmissiveColor"), "(0.0000, 0.0000, 0.0000, 1.0)");
    EXPECT_NEAR(steel->getAppearanceValue("Shininess").toDouble(), 0.06, 1e-6);
    EXPECT_EQ(steel->getAppearanceValue("SpecularColor"), "(0.9800, 0.9800, 0.9800, 1.0)");
    EXPECT_DOUBLE_EQ(steel->getAppearanceValue("Transparency").toDouble(), 0.0);

    EXPECT_EQ(steel->getPhysicalQuantity("Density").getUserString(), parseQuantity("7900.00 kg/m^3").toStdString());
    EXPECT_EQ(steel->getPhysicalQuantity("YoungsModulus").getUserString(), parseQuantity("210.00 GPa").toStdString());
    EXPECT_EQ(steel->getPhysicalQuantity("SpecificHeat").getUserString(), parseQuantity("590.00 J/kg/K").toStdString());
    EXPECT_EQ(steel->getPhysicalQuantity("ThermalConductivity").getUserString(), parseQuantity("43.00 W/m/K").toStdString());
    EXPECT_EQ(steel->getPhysicalQuantity("ThermalExpansionCoefficient").getUserString(), parseQuantity("12.00 µm/m/K").toStdString());
}

TEST_F(TestMaterial, TestColumns)
{
    // Start with an empty material
    Materials::Material testMaterial;
    auto models = testMaterial.getPhysicalModels();
    EXPECT_NE(&models, nullptr);
    EXPECT_EQ(models->size(), 0);

    // Add a model
    testMaterial.addPhysical(Materials::ModelUUIDs::ModelUUID_Test_Model);
    models = testMaterial.getPhysicalModels();
    EXPECT_EQ(models->size(), 1);

    EXPECT_TRUE(testMaterial.hasPhysicalProperty("TestArray2D"));
    auto array2d = testMaterial.getPhysicalProperty("TestArray2D")->getMaterialValue();
    EXPECT_TRUE(array2d);
    EXPECT_EQ(dynamic_cast<Materials::Array2D &>(*array2d).columns(), 2);

    EXPECT_TRUE(testMaterial.hasPhysicalProperty("TestArray2D3Column"));
    auto array2d3Column = testMaterial.getPhysicalProperty("TestArray2D3Column")->getMaterialValue();
    EXPECT_TRUE(array2d3Column);
    EXPECT_EQ(dynamic_cast<Materials::Array2D &>(*array2d3Column).columns(), 3);

    EXPECT_TRUE(testMaterial.hasPhysicalProperty("TestArray3D"));
    auto array3d = testMaterial.getPhysicalProperty("TestArray3D")->getMaterialValue();
    EXPECT_TRUE(array3d);
    EXPECT_EQ(dynamic_cast<Materials::Array3D &>(*array3d).columns(), 2);
}

TEST_F(TestMaterial, TestTestMaterial)
{
    auto testMaterial = _materialManager->getMaterial("c6c64159-19c1-40b5-859c-10561f20f979");
    EXPECT_EQ(testMaterial->getName(), "Test Material");
    EXPECT_EQ(testMaterial->getUUID(), "c6c64159-19c1-40b5-859c-10561f20f979");
    EXPECT_EQ(testMaterial->getLibrary()->getName(), "System");
    EXPECT_FALSE(testMaterial->isDisabled());
    EXPECT_EQ(testMaterial->getDirectory(), "Test");
    // EXPECT_EQ(testMaterial->getFilePath(), ""); - this is installation dependent
    EXPECT_EQ(testMaterial->getLibraryPath(), "[System]/Test/Test Material");
    EXPECT_EQ(testMaterial->getAuthorAndLicense(), "David Carter LGPL-2.1-or-later");
    EXPECT_EQ(testMaterial->getAuthor(), "David Carter");
    EXPECT_EQ(testMaterial->getLicense(), "LGPL-2.1-or-later");
    EXPECT_EQ(testMaterial->getParentUUID(), "5dbb7be6-8b63-479b-ab4c-87be02ead973"); // Default appearance
    EXPECT_EQ(testMaterial->getDescription(), "This material is for testing purposes only. It does not describe any real material.");
    EXPECT_EQ(testMaterial->getURL(), "");
    EXPECT_EQ(testMaterial->getReference(), "");

    EXPECT_TRUE(testMaterial->hasPhysicalProperty("TestArray2D"));
    auto array2d = testMaterial->getPhysicalProperty("TestArray2D")->getMaterialValue();
    ASSERT_TRUE(array2d);
    EXPECT_EQ(array2d->getType(), Materials::MaterialValue::Array2D);
    auto actual2D = dynamic_cast<Materials::Array2D &>(*array2d);
    EXPECT_EQ(actual2D.rows(), 3);
    EXPECT_EQ(actual2D.columns(), 2);
    EXPECT_TRUE(actual2D.getValue(0, 0).canConvert<Base::Quantity>());
    EXPECT_THROW(actual2D.getValue(-1, 0), Materials::InvalidIndex);
    EXPECT_THROW(actual2D.getValue(0, -1), Materials::InvalidIndex);
    EXPECT_EQ(actual2D.getValue(0, 0).value<Base::Quantity>(), Base::Quantity::parse("10.00 C"));
    EXPECT_EQ(actual2D.getValue(0, 1).value<Base::Quantity>(), Base::Quantity::parse("10.00 kg/m^3"));
    EXPECT_THROW(actual2D.getValue(0, 3), Materials::InvalidIndex);
    EXPECT_EQ(actual2D.getValue(1, 0).value<Base::Quantity>(), Base::Quantity::parse("20.00 C"));
    EXPECT_EQ(actual2D.getValue(1, 1).value<Base::Quantity>(), Base::Quantity::parse("20.00 kg/m^3"));
    EXPECT_THROW(actual2D.getValue(1, 3), Materials::InvalidIndex);
    EXPECT_EQ(actual2D.getValue(2, 0).value<Base::Quantity>(), Base::Quantity::parse("30.00 C"));
    EXPECT_EQ(actual2D.getValue(2, 1).value<Base::Quantity>(), Base::Quantity::parse("30.00 kg/m^3"));
    EXPECT_THROW(actual2D.getValue(2, 3), Materials::InvalidIndex);
    EXPECT_THROW(actual2D.getValue(3, 0), Materials::InvalidIndex);

    EXPECT_TRUE(testMaterial->hasPhysicalProperty("TestArray2D3Column"));
    auto array2d3c = testMaterial->getPhysicalProperty("TestArray2D3Column")->getMaterialValue();
    ASSERT_TRUE(array2d3c);
    EXPECT_EQ(array2d3c->getType(), Materials::MaterialValue::Array2D);
    auto actual2d3c = dynamic_cast<Materials::Array2D &>(*array2d3c);
    EXPECT_EQ(actual2d3c.rows(), 3);
    EXPECT_EQ(actual2d3c.columns(), 3);
    EXPECT_TRUE(actual2d3c.getValue(0, 0).canConvert<Base::Quantity>());
    EXPECT_THROW(actual2d3c.getValue(-1, 0), Materials::InvalidIndex);
    EXPECT_THROW(actual2d3c.getValue(0, -1), Materials::InvalidIndex);
    EXPECT_EQ(actual2d3c.getValue(0, 0).value<Base::Quantity>(), Base::Quantity::parse("10.00 C"));
    EXPECT_EQ(actual2d3c.getValue(0, 1).value<Base::Quantity>(), Base::Quantity::parse("11.00 kg/m^3"));
    EXPECT_EQ(actual2d3c.getValue(0, 2).value<Base::Quantity>(), Base::Quantity::parse("12.00 Pa"));
    EXPECT_THROW(actual2d3c.getValue(0, 3), Materials::InvalidIndex);
    EXPECT_EQ(actual2d3c.getValue(1, 0).value<Base::Quantity>(), Base::Quantity::parse("20.00 C"));
    EXPECT_EQ(actual2d3c.getValue(1, 1).value<Base::Quantity>(), Base::Quantity::parse("21.00 kg/m^3"));
    EXPECT_EQ(actual2d3c.getValue(1, 2).value<Base::Quantity>(), Base::Quantity::parse("22.00 Pa"));
    EXPECT_THROW(actual2d3c.getValue(1, 3), Materials::InvalidIndex);
    EXPECT_EQ(actual2d3c.getValue(2, 0).value<Base::Quantity>(), Base::Quantity::parse("30.00 C"));
    EXPECT_EQ(actual2d3c.getValue(2, 1).value<Base::Quantity>(), Base::Quantity::parse("31.00 kg/m^3"));
    EXPECT_EQ(actual2d3c.getValue(2, 2).value<Base::Quantity>(), Base::Quantity::parse("32.00 Pa"));
    EXPECT_THROW(actual2d3c.getValue(2, 3), Materials::InvalidIndex);
    EXPECT_THROW(actual2d3c.getValue(3, 0), Materials::InvalidIndex);

    EXPECT_TRUE(testMaterial->hasPhysicalProperty("TestArray3D"));
    auto array3d = testMaterial->getPhysicalProperty("TestArray3D")->getMaterialValue();
    ASSERT_TRUE(array3d);
    EXPECT_EQ(array3d->getType(), Materials::MaterialValue::Array3D);
    auto actual3d = dynamic_cast<Materials::Array3D &>(*array3d);
    EXPECT_EQ(actual3d.depth(), 3);
    EXPECT_EQ(actual3d.rows(0), 2);
    EXPECT_EQ(actual3d.rows(1), 0);
    EXPECT_EQ(actual3d.rows(2), 3);
    EXPECT_EQ(actual3d.columns(), 2);
    EXPECT_EQ(actual3d.getDepthValue(0), Base::Quantity::parse("10.00 C"));
    EXPECT_THROW(actual3d.getValue(-1, 0, 0), Materials::InvalidIndex);
    EXPECT_THROW(actual3d.getValue(0, -1, 0), Materials::InvalidIndex);
    EXPECT_THROW(actual3d.getValue(0, 0, -1), Materials::InvalidIndex);
    EXPECT_EQ(actual3d.getValue(0, 0, 0), Base::Quantity::parse("11.00 Pa"));
    EXPECT_EQ(actual3d.getValue(0, 0, 1), Base::Quantity::parse("12.00 Pa"));
    EXPECT_EQ(actual3d.getValue(0, 1, 0), Base::Quantity::parse("21.00 Pa"));
    EXPECT_EQ(actual3d.getValue(0, 1, 1), Base::Quantity::parse("22.00 Pa"));
    EXPECT_THROW(actual3d.getValue(0, 0, 2), Materials::InvalidIndex);
    EXPECT_THROW(actual3d.getValue(0, 2, 0), Materials::InvalidIndex);
    EXPECT_EQ(actual3d.getDepthValue(1), Base::Quantity::parse("20.00 C"));
    EXPECT_THROW(actual3d.getValue(1, 0, 0), Materials::InvalidIndex);
    EXPECT_EQ(actual3d.getDepthValue(2), Base::Quantity::parse("30.00 C"));
    EXPECT_EQ(actual3d.getValue(2, 0, 0), Base::Quantity::parse("10.00 Pa"));
    EXPECT_EQ(actual3d.getValue(2, 0, 1), Base::Quantity::parse("11.00 Pa"));
    EXPECT_EQ(actual3d.getValue(2, 1, 0), Base::Quantity::parse("20.00 Pa"));
    EXPECT_EQ(actual3d.getValue(2, 1, 1), Base::Quantity::parse("21.00 Pa"));
    EXPECT_EQ(actual3d.getValue(2, 2, 0), Base::Quantity::parse("30.00 Pa"));
    EXPECT_EQ(actual3d.getValue(2, 2, 1), Base::Quantity::parse("31.00 Pa"));
    EXPECT_THROW(actual3d.getValue(2, 0, 2), Materials::InvalidIndex);
    EXPECT_THROW(actual3d.getValue(2, 3, 0), Materials::InvalidIndex);
    EXPECT_THROW(actual3d.getValue(3, 0, 0), Materials::InvalidIndex);

    EXPECT_TRUE(testMaterial->hasPhysicalProperty("TestBoolean"));
    auto boolean = testMaterial->getPhysicalProperty("TestBoolean")->getMaterialValue();
    EXPECT_EQ(boolean->getType(), Materials::MaterialValue::Boolean);
    EXPECT_TRUE(boolean->getValue().canConvert<bool>());
    EXPECT_EQ(boolean->getValue().toBool(), true);

    EXPECT_TRUE(testMaterial->hasPhysicalProperty("TestColor"));
    auto color = testMaterial->getPhysicalProperty("TestColor")->getMaterialValue();
    EXPECT_EQ(color->getType(), Materials::MaterialValue::Color);
    EXPECT_EQ(color->getValue().toString().toStdString(), "(1,0,0,1)");

    EXPECT_TRUE(testMaterial->hasPhysicalProperty("TestFloat"));
    auto floatNumber = testMaterial->getPhysicalProperty("TestFloat")->getMaterialValue();
    EXPECT_EQ(floatNumber->getType(), Materials::MaterialValue::Float);
    EXPECT_TRUE(floatNumber->getValue().canConvert<double>());
    EXPECT_EQ(floatNumber->getValue().toDouble(), QString::fromStdString("32.17").toFloat());

    EXPECT_TRUE(testMaterial->hasPhysicalProperty("TestInteger"));
    auto integerNumber = testMaterial->getPhysicalProperty("TestInteger")->getMaterialValue();
    EXPECT_EQ(integerNumber->getType(), Materials::MaterialValue::Integer);
    EXPECT_TRUE(integerNumber->getValue().canConvert<int>());
    EXPECT_EQ(integerNumber->getValue().toInt(), 42);

    EXPECT_TRUE(testMaterial->hasPhysicalProperty("TestList"));
    auto list = testMaterial->getPhysicalProperty("TestList")->getMaterialValue();
    ASSERT_TRUE(list);
    EXPECT_EQ(list->getType(), Materials::MaterialValue::List);
    EXPECT_TRUE(list->getValue().canConvert<std::vector<QVariant>>());
    auto actualList = list->getList();
    ASSERT_FALSE(actualList.empty());
    EXPECT_EQ(actualList.size(), 6);
    EXPECT_EQ(actualList.at(0).toString().toStdString(), "Now is the time for all good men to come to the aid of the party");
    EXPECT_EQ(actualList.at(1).toString().toStdString(), "The quick brown fox jumps over the lazy dogs back");
    EXPECT_EQ(actualList.at(2).toString().toStdString(), "Lore Ipsum");
    EXPECT_EQ(actualList.at(3).toString().toStdString(), "Single quote '");
    EXPECT_EQ(actualList.at(4).toString().toStdString(), "Double quote \"");
    EXPECT_EQ(actualList.at(5).toString().toStdString(), "Backslash \\");

    EXPECT_TRUE(testMaterial->hasPhysicalProperty("TestMultiLineString"));
    auto multiline = testMaterial->getPhysicalProperty("TestMultiLineString")->getMaterialValue();
    EXPECT_EQ(multiline->getType(), Materials::MaterialValue::MultiLineString);
    EXPECT_EQ(multiline->getValue().toString().toStdString(), 
        "Now is the time for 'all' \\  <good> \"men\" come to the aid of the party\n"
        "  Indentation is significant\n"
        "Similar to Python\n"
    );

    EXPECT_TRUE(testMaterial->hasPhysicalProperty("TestQuantity"));
    auto quantity = testMaterial->getPhysicalProperty("TestQuantity")->getMaterialValue();
    EXPECT_TRUE(quantity->getValue().canConvert<Base::Quantity>());
    EXPECT_EQ(quantity->getType(), Materials::MaterialValue::Quantity);
    EXPECT_EQ(quantity->getValue().value<Base::Quantity>(), Base::Quantity::parse("19.76 kg/m^3"));

    EXPECT_TRUE(testMaterial->hasPhysicalProperty("TestString"));
    auto stringValue = testMaterial->getPhysicalProperty("TestString")->getMaterialValue();
    EXPECT_EQ(stringValue->getType(), Materials::MaterialValue::String);
    EXPECT_EQ(stringValue->getValue().toString().toStdString(), "Now is the time for 'all' \\  <good> \"men\" come to the aid of the party");

    EXPECT_TRUE(testMaterial->hasPhysicalProperty("TestURL"));
    auto url = testMaterial->getPhysicalProperty("TestURL")->getMaterialValue();
    EXPECT_EQ(url->getType(), Materials::MaterialValue::URL);
    EXPECT_EQ(url->getValue().toString().toStdString(), "https://www.freecad.org/");
}

TEST_F(TestMaterial, TestSparseArrays2D)
{
    // First validate the model
    std::shared_ptr<Materials::Model> testModel;
    ASSERT_NO_THROW(testModel = _modelManager->getModel("807a4b37-da41-4b7a-a730-8555cae4146b"));
    EXPECT_EQ(testModel->getName(), "Test Sparse Model");
    EXPECT_EQ(testModel->getUUID(), "807a4b37-da41-4b7a-a730-8555cae4146b");
    EXPECT_TRUE(testModel->hasProperty("TestArray2D"));
    EXPECT_TRUE(testModel->hasProperty("TestArray3D"));

    auto testMaterial = _materialManager->getMaterial("4704ec99-2914-4a72-9a71-a781d2655ee9");
    EXPECT_EQ(testMaterial->getName(), "TestSparseArray");
    EXPECT_EQ(testMaterial->getUUID(), "4704ec99-2914-4a72-9a71-a781d2655ee9");
    EXPECT_EQ(testMaterial->getLibrary()->getName(), "__UnitTest");
    EXPECT_FALSE(testMaterial->isDisabled());
    EXPECT_EQ(testMaterial->getDirectory(), "");
    EXPECT_EQ(testMaterial->getLibraryPath(), "[__UnitTest]/TestSparseArray");
    EXPECT_EQ(testMaterial->getAuthorAndLicense(), "David Carter LGPL-2.0-or-later");
    EXPECT_EQ(testMaterial->getAuthor(), "David Carter");
    EXPECT_EQ(testMaterial->getLicense(), "LGPL-2.0-or-later");
    EXPECT_TRUE(testMaterial->getParentUUID().empty());
    EXPECT_EQ(testMaterial->getDescription(), "Tests sparse arrays");
    EXPECT_TRUE(testMaterial->getURL().empty());
    EXPECT_TRUE(testMaterial->getReference().empty());

    // Validate the sparse arrays
    EXPECT_TRUE(testMaterial->hasPhysicalProperty("TestArray2D"));
    auto array2d = testMaterial->getPhysicalProperty("TestArray2D")->getMaterialValue();
    ASSERT_TRUE(array2d);
    EXPECT_EQ(array2d->getType(), Materials::MaterialValue::Array2D);
    auto actual2D = dynamic_cast<Materials::Array2D &>(*array2d);
    EXPECT_EQ(actual2D.rows(), 4);
    EXPECT_EQ(actual2D.columns(), 3);

    EXPECT_TRUE(actual2D.getValue(0, 0).canConvert<Base::Quantity>());
    EXPECT_THROW(actual2D.getValue(-1, 0), Materials::InvalidIndex);
    EXPECT_THROW(actual2D.getValue(0, -1), Materials::InvalidIndex);
    EXPECT_THROW(actual2D.getValue(-1, -1), Materials::InvalidIndex);
    EXPECT_EQ(actual2D.getValue(0, 0).value<Base::Quantity>(), Base::Quantity::parse("1 K"));
    EXPECT_TRUE(actual2D.getValue(0, 1).canConvert<Base::Quantity>());
    EXPECT_TRUE(actual2D.getValue(0, 1).value<Base::Quantity>().isValid());
    EXPECT_EQ(actual2D.getValue(0, 1).value<Base::Quantity>(), Base::Quantity::parse("1 kg/mm^3"));
    EXPECT_TRUE(actual2D.getValue(0, 2).canConvert<Base::Quantity>());
    EXPECT_TRUE(actual2D.getValue(0, 2).value<Base::Quantity>().isValid());
    EXPECT_EQ(actual2D.getValue(0, 2).value<Base::Quantity>(), Base::Quantity::parse("1.0"));
    EXPECT_THROW(actual2D.getValue(0, 3), Materials::InvalidIndex);
    
    EXPECT_TRUE(actual2D.getValue(1, 0).canConvert<Base::Quantity>());
    EXPECT_EQ(actual2D.getValue(1, 0).value<Base::Quantity>(), Base::Quantity::parse("2 K"));
    EXPECT_TRUE(actual2D.getValue(1, 1).canConvert<Base::Quantity>());
    EXPECT_TRUE(actual2D.getValue(1, 1).value<Base::Quantity>().isValid());
    EXPECT_EQ(actual2D.getValue(1, 1).value<Base::Quantity>(), Base::Quantity::parse("2 kg/mm^3"));
    EXPECT_TRUE(actual2D.getValue(1, 2).canConvert<Base::Quantity>());
    EXPECT_FALSE(actual2D.getValue(1, 2).value<Base::Quantity>().isValid());
    EXPECT_THROW(actual2D.getValue(1, 3), Materials::InvalidIndex);
    
    EXPECT_TRUE(actual2D.getValue(2, 0).canConvert<Base::Quantity>());
    EXPECT_EQ(actual2D.getValue(2, 0).value<Base::Quantity>(), Base::Quantity::parse("3 K"));
    EXPECT_TRUE(actual2D.getValue(2, 1).canConvert<Base::Quantity>());
    EXPECT_FALSE(actual2D.getValue(2, 1).value<Base::Quantity>().isValid());
    EXPECT_TRUE(actual2D.getValue(2, 2).canConvert<Base::Quantity>());
    EXPECT_TRUE(actual2D.getValue(2, 2).value<Base::Quantity>().isValid());
    EXPECT_EQ(actual2D.getValue(2, 2).value<Base::Quantity>(), Base::Quantity::parse("3.0"));
    EXPECT_THROW(actual2D.getValue(2, 3), Materials::InvalidIndex);
    
    EXPECT_TRUE(actual2D.getValue(3, 0).canConvert<Base::Quantity>());
    EXPECT_EQ(actual2D.getValue(3, 0).value<Base::Quantity>(), Base::Quantity::parse("4 K"));
    EXPECT_TRUE(actual2D.getValue(3, 1).canConvert<Base::Quantity>());
    EXPECT_FALSE(actual2D.getValue(3, 1).value<Base::Quantity>().isValid());
    EXPECT_TRUE(actual2D.getValue(3, 2).canConvert<Base::Quantity>());
    EXPECT_FALSE(actual2D.getValue(3, 2).value<Base::Quantity>().isValid());
    EXPECT_THROW(actual2D.getValue(3, 3), Materials::InvalidIndex);

    // Test the copy constructor
    Materials::Array2D copy2D(actual2D);
    EXPECT_EQ(copy2D.rows(), 4);
    EXPECT_EQ(copy2D.columns(), 3);

    EXPECT_TRUE(copy2D.getValue(0, 0).canConvert<Base::Quantity>());
    EXPECT_THROW(copy2D.getValue(-1, 0), Materials::InvalidIndex);
    EXPECT_THROW(copy2D.getValue(0, -1), Materials::InvalidIndex);
    EXPECT_THROW(copy2D.getValue(-1, -1), Materials::InvalidIndex);
    EXPECT_EQ(copy2D.getValue(0, 0).value<Base::Quantity>(), Base::Quantity::parse("1 K"));
    EXPECT_TRUE(copy2D.getValue(0, 1).canConvert<Base::Quantity>());
    EXPECT_TRUE(copy2D.getValue(0, 1).value<Base::Quantity>().isValid());
    EXPECT_EQ(copy2D.getValue(0, 1).value<Base::Quantity>(), Base::Quantity::parse("1 kg/mm^3"));
    EXPECT_TRUE(copy2D.getValue(0, 2).canConvert<Base::Quantity>());
    EXPECT_TRUE(copy2D.getValue(0, 2).value<Base::Quantity>().isValid());
    EXPECT_EQ(copy2D.getValue(0, 2).value<Base::Quantity>(), Base::Quantity::parse("1.0"));
    EXPECT_THROW(copy2D.getValue(0, 3), Materials::InvalidIndex);
    
    EXPECT_TRUE(copy2D.getValue(1, 0).canConvert<Base::Quantity>());
    EXPECT_EQ(copy2D.getValue(1, 0).value<Base::Quantity>(), Base::Quantity::parse("2 K"));
    EXPECT_TRUE(copy2D.getValue(1, 1).canConvert<Base::Quantity>());
    EXPECT_TRUE(copy2D.getValue(1, 1).value<Base::Quantity>().isValid());
    EXPECT_EQ(copy2D.getValue(1, 1).value<Base::Quantity>(), Base::Quantity::parse("2 kg/mm^3"));
    EXPECT_TRUE(copy2D.getValue(1, 2).canConvert<Base::Quantity>());
    EXPECT_FALSE(copy2D.getValue(1, 2).value<Base::Quantity>().isValid());
    EXPECT_THROW(copy2D.getValue(1, 3), Materials::InvalidIndex);
    
    EXPECT_TRUE(copy2D.getValue(2, 0).canConvert<Base::Quantity>());
    EXPECT_EQ(copy2D.getValue(2, 0).value<Base::Quantity>(), Base::Quantity::parse("3 K"));
    EXPECT_TRUE(copy2D.getValue(2, 1).canConvert<Base::Quantity>());
    EXPECT_FALSE(copy2D.getValue(2, 1).value<Base::Quantity>().isValid());
    EXPECT_TRUE(copy2D.getValue(2, 2).canConvert<Base::Quantity>());
    EXPECT_TRUE(copy2D.getValue(2, 2).value<Base::Quantity>().isValid());
    EXPECT_EQ(copy2D.getValue(2, 2).value<Base::Quantity>(), Base::Quantity::parse("3.0"));
    EXPECT_THROW(copy2D.getValue(2, 3), Materials::InvalidIndex);
    
    EXPECT_TRUE(copy2D.getValue(3, 0).canConvert<Base::Quantity>());
    EXPECT_EQ(copy2D.getValue(3, 0).value<Base::Quantity>(), Base::Quantity::parse("4 K"));
    EXPECT_TRUE(copy2D.getValue(3, 1).canConvert<Base::Quantity>());
    EXPECT_FALSE(copy2D.getValue(3, 1).value<Base::Quantity>().isValid());
    EXPECT_TRUE(copy2D.getValue(3, 2).canConvert<Base::Quantity>());
    EXPECT_FALSE(copy2D.getValue(3, 2).value<Base::Quantity>().isValid());
    EXPECT_THROW(copy2D.getValue(3, 3), Materials::InvalidIndex);

}

TEST_F(TestMaterial, TestSparseArrays3D)
{
    // First validate the model
    std::shared_ptr<Materials::Model> testModel;
    ASSERT_NO_THROW(testModel = _modelManager->getModel("807a4b37-da41-4b7a-a730-8555cae4146b"));
    EXPECT_EQ(testModel->getName(), "Test Sparse Model");
    EXPECT_EQ(testModel->getUUID(), "807a4b37-da41-4b7a-a730-8555cae4146b");
    EXPECT_TRUE(testModel->hasProperty("TestArray2D"));
    EXPECT_TRUE(testModel->hasProperty("TestArray3D"));

    auto testMaterial = _materialManager->getMaterial("4704ec99-2914-4a72-9a71-a781d2655ee9");
    EXPECT_EQ(testMaterial->getName(), "TestSparseArray");
    EXPECT_EQ(testMaterial->getUUID(), "4704ec99-2914-4a72-9a71-a781d2655ee9");
    EXPECT_EQ(testMaterial->getLibrary()->getName(), "__UnitTest");
    EXPECT_FALSE(testMaterial->isDisabled());
    EXPECT_EQ(testMaterial->getDirectory(), "");
    EXPECT_EQ(testMaterial->getLibraryPath(), "[__UnitTest]/TestSparseArray");
    EXPECT_EQ(testMaterial->getAuthorAndLicense(), "David Carter LGPL-2.0-or-later");
    EXPECT_EQ(testMaterial->getAuthor(), "David Carter");
    EXPECT_EQ(testMaterial->getLicense(), "LGPL-2.0-or-later");
    EXPECT_TRUE(testMaterial->getParentUUID().empty());
    EXPECT_EQ(testMaterial->getDescription(), "Tests sparse arrays");
    EXPECT_TRUE(testMaterial->getURL().empty());
    EXPECT_TRUE(testMaterial->getReference().empty());

    // Validate the sparse arrays
    ASSERT_TRUE(testMaterial->hasPhysicalProperty("TestArray3D"));
    auto array3d = testMaterial->getPhysicalProperty("TestArray3D")->getMaterialValue();
    ASSERT_TRUE(array3d);
    EXPECT_EQ(array3d->getType(), Materials::MaterialValue::Array3D);
    auto actual3D = dynamic_cast<Materials::Array3D &>(*array3d);
    EXPECT_EQ(actual3D.depth(), 3);
    EXPECT_EQ(actual3D.rows(0), 4);
    EXPECT_EQ(actual3D.rows(1), 0);
    EXPECT_EQ(actual3D.rows(2), 3);
    EXPECT_EQ(actual3D.columns(), 2);

    EXPECT_THROW(actual3D.getValue(-1, 0, 0), Materials::InvalidIndex);
    EXPECT_THROW(actual3D.getValue(0, -1, 0), Materials::InvalidIndex);
    EXPECT_THROW(actual3D.getValue(0, 0, -1), Materials::InvalidIndex);
    EXPECT_THROW(actual3D.getValue(-1, -1), Materials::InvalidIndex);
    EXPECT_EQ(actual3D.getDepthValue(0), Base::Quantity::parse("10.00 C"));
    EXPECT_TRUE(actual3D.getValue(0, 0, 0).isValid());
    EXPECT_EQ(actual3D.getValue(0, 0, 0), Base::Quantity::parse("11.00 Pa"));
    EXPECT_TRUE(actual3D.getValue(0, 0, 1).isValid());
    EXPECT_EQ(actual3D.getValue(0, 0, 1), Base::Quantity::parse("12.00 Pa"));
    EXPECT_THROW(actual3D.getValue(0, 0, 2), Materials::InvalidIndex);

    EXPECT_TRUE(actual3D.getValue(0, 1, 0).isValid());
    EXPECT_EQ(actual3D.getValue(0, 1, 0), Base::Quantity::parse("21.00 Pa"));
    EXPECT_FALSE(actual3D.getValue(0, 1, 1).isValid());
    EXPECT_THROW(actual3D.getValue(0, 1, 2), Materials::InvalidIndex);

    EXPECT_FALSE(actual3D.getValue(0, 2, 0).isValid());
    EXPECT_TRUE(actual3D.getValue(0, 2, 1).isValid());
    EXPECT_EQ(actual3D.getValue(0, 2, 1), Base::Quantity::parse("32.00 Pa"));
    EXPECT_THROW(actual3D.getValue(0, 2, 2), Materials::InvalidIndex);

    EXPECT_FALSE(actual3D.getValue(0, 3, 0).isValid());
    EXPECT_FALSE(actual3D.getValue(0, 3, 1).isValid());
    EXPECT_THROW(actual3D.getValue(0, 3, 2), Materials::InvalidIndex);

    EXPECT_THROW(actual3D.getValue(0, 4, 0), Materials::InvalidIndex);

    // Test the copy constructor
    Materials::Array3D copy3D(actual3D);
    EXPECT_EQ(copy3D.depth(), 3);
    EXPECT_EQ(copy3D.rows(0), 4);
    EXPECT_EQ(copy3D.rows(1), 0);
    EXPECT_EQ(copy3D.rows(2), 3);
    EXPECT_EQ(copy3D.columns(), 2);

    EXPECT_THROW(copy3D.getValue(-1, 0, 0), Materials::InvalidIndex);
    EXPECT_THROW(copy3D.getValue(0, -1, 0), Materials::InvalidIndex);
    EXPECT_THROW(copy3D.getValue(0, 0, -1), Materials::InvalidIndex);
    EXPECT_THROW(copy3D.getValue(-1, -1), Materials::InvalidIndex);
    EXPECT_EQ(copy3D.getDepthValue(0), Base::Quantity::parse("10.00 C"));
    EXPECT_TRUE(copy3D.getValue(0, 0, 0).isValid());
    EXPECT_EQ(copy3D.getValue(0, 0, 0), Base::Quantity::parse("11.00 Pa"));
    EXPECT_TRUE(copy3D.getValue(0, 0, 1).isValid());
    EXPECT_EQ(copy3D.getValue(0, 0, 1), Base::Quantity::parse("12.00 Pa"));
    EXPECT_THROW(copy3D.getValue(0, 0, 2), Materials::InvalidIndex);

    EXPECT_TRUE(copy3D.getValue(0, 1, 0).isValid());
    EXPECT_EQ(copy3D.getValue(0, 1, 0), Base::Quantity::parse("21.00 Pa"));
    EXPECT_FALSE(copy3D.getValue(0, 1, 1).isValid());
    EXPECT_THROW(copy3D.getValue(0, 1, 2), Materials::InvalidIndex);

    EXPECT_FALSE(copy3D.getValue(0, 2, 0).isValid());
    EXPECT_TRUE(copy3D.getValue(0, 2, 1).isValid());
    EXPECT_EQ(copy3D.getValue(0, 2, 1), Base::Quantity::parse("32.00 Pa"));
    EXPECT_THROW(copy3D.getValue(0, 2, 2), Materials::InvalidIndex);

    EXPECT_FALSE(copy3D.getValue(0, 3, 0).isValid());
    EXPECT_FALSE(copy3D.getValue(0, 3, 1).isValid());
    EXPECT_THROW(copy3D.getValue(0, 3, 2), Materials::InvalidIndex);

    EXPECT_THROW(copy3D.getValue(0, 4, 0), Materials::InvalidIndex);
}

// clang-format on
