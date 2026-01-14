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

#include <QMetaType>
#include <QString>

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

class TestMaterialCards : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        if (App::Application::GetARGC() == 0) {
            tests::initApplication();
        }
    }

    void SetUp() override {
        _modelManager = &(Materials::ModelManager::getManager());
        _materialManager = &(Materials::MaterialManager::getManager());

        // Disable the external interface
        _useExternal = _materialManager->useExternal();
        _materialManager->setUseExternal(false);

        _systemDisabled = _materialManager->isDisabled(QStringLiteral("System"), true);
        _materialManager->setDisabled(QStringLiteral("System"), false, true);
        if (_materialManager->isDisabled(QStringLiteral("System"), true)) {
            FAIL() << "System is disabled";
        }

        // Create a temporary library
        QString libPath = QDir::tempPath() + QStringLiteral("/TestMaterialCards");
        QDir libDir(libPath);
        libDir.removeRecursively(); // Clear any old run data
        libDir.mkdir(libPath);

        _library = _materialManager->createLocalLibrary(QStringLiteral("TestMaterialCards"),
                            libPath,
                            QStringLiteral(":/icons/preferences-general.svg"),
                            false);

        // Test Material.FCMat
        _testMaterialUUID = QStringLiteral("c6c64159-19c1-40b5-859c-10561f20f979");
        _materialManager->refresh();
    }

    void TearDown() override {
        _materialManager->removeLibrary(QStringLiteral("TestMaterialCards"), false); // Remove the library
        _materialManager->setDisabled(QStringLiteral("System"), _systemDisabled, true);
        _materialManager->setUseExternal(_useExternal);
        _materialManager->refresh();
    }

    Materials::ModelManager* _modelManager;
    Materials::MaterialManager* _materialManager;
    std::shared_ptr<Materials::MaterialLibrary> _library;
    QString _testMaterialUUID;
    bool _useExternal {};
    bool _systemDisabled {};
};

TEST_F(TestMaterialCards, TestCopy)
{
    ASSERT_NE(_modelManager, nullptr);
    ASSERT_TRUE(_library);
    // FAIL() << "Test library " << _library->getDirectoryPath().toStdString() << "\n";
    ASSERT_GT(_materialManager->getLocalMaterials()->size(), 0);

    std::shared_ptr<Materials::Material> newMaterial;

    try {
        auto testMaterial = _materialManager->getMaterial(_testMaterialUUID);
        newMaterial = std::make_shared<Materials::Material>(*testMaterial);

        EXPECT_EQ(testMaterial->getUUID(), _testMaterialUUID);
        EXPECT_EQ(newMaterial->getUUID(), _testMaterialUUID);
    }
    catch (const Materials::MaterialReadError&) {
        FAIL() << "Material read error 1\n";
    }
    catch (const Materials::MaterialNotFound&) {
        FAIL() << "Material not found error 1\n";
    }
    catch (...) {
        FAIL() << "An unknown exception has occured 1\n";
    }

        // Save the material
    try {
        _materialManager->saveMaterial(_library,
                        newMaterial,
                        QStringLiteral("/Test Material2.FCMat"),
                        false, // overwrite
                        true,  // saveAsCopy
                        false); // saveInherited
        EXPECT_EQ(newMaterial->getUUID(), _testMaterialUUID);
        EXPECT_EQ(newMaterial->getName(), QStringLiteral("Test Material2"));
    }
    catch (...) {
        FAIL() << "An unknown exception has occured 2\n";
    }

        // Save it when it already exists throwing an error
    try {
        EXPECT_THROW(_materialManager->saveMaterial(_library,
                        newMaterial,
                        QStringLiteral("/Test Material2.FCMat"),
                        false, // overwrite
                        true,  // saveAsCopy
                        false) // saveInherited
                        , Materials::MaterialExists);
        EXPECT_EQ(newMaterial->getUUID(), _testMaterialUUID);
        EXPECT_EQ(newMaterial->getName(), QStringLiteral("Test Material2"));
    }
    catch (...) {
        FAIL() << "An unknown exception has occured 3\n";
    }

        // Overwrite the existing file
    try {
        _materialManager->saveMaterial(_library,
                        newMaterial,
                        QStringLiteral("/Test Material2.FCMat"),
                        true,  // overwrite
                        true,  // saveAsCopy
                        false);// saveInherited
        EXPECT_EQ(newMaterial->getUUID(), _testMaterialUUID);
        EXPECT_EQ(newMaterial->getName(), QStringLiteral("Test Material2"));
    }
    catch (...) {
        FAIL() << "An unknown exception has occured 4\n";
    }

        // Save to a new file, inheritance mode
    try {
        _materialManager->saveMaterial(_library,
                        newMaterial,
                        QStringLiteral("/Test Material3.FCMat"),
                        false,  // overwrite
                        true,  // saveAsCopy
                        true);// saveInherited
        EXPECT_EQ(newMaterial->getUUID(), _testMaterialUUID);
        EXPECT_EQ(newMaterial->getName(), QStringLiteral("Test Material3"));
    }
    catch (...) {
        FAIL() << "An unknown exception has occured 5\n";
    }

        // Save to a new file, inheritance mode. no copy
    QString uuid1;
    try {
        _materialManager->saveMaterial(_library,
                        newMaterial,
                        QStringLiteral("/Test Material4.FCMat"),
                        false,  // overwrite
                        false,  // saveAsCopy
                        true);// saveInherited
        EXPECT_NE(newMaterial->getUUID(), _testMaterialUUID);
        EXPECT_EQ(newMaterial->getName(), QStringLiteral("Test Material4"));
        uuid1 = newMaterial->getUUID();
    }
    catch (...) {
        FAIL() << "An unknown exception has occured 6\n";
    }

        // Save to a new file, inheritance mode, testing overwrite, new copy
    try {
        _materialManager->saveMaterial(_library,
                        newMaterial,
                        QStringLiteral("/Test Material5.FCMat"),
                        false,  // overwrite
                        true,  // saveAsCopy
                        true);// saveInherited
        EXPECT_EQ(newMaterial->getUUID(), uuid1);
        EXPECT_EQ(newMaterial->getName(), QStringLiteral("Test Material5"));
    }
    catch (...) {
        FAIL() << "An unknown exception has occured 7\n";
    }

    try {
        _materialManager->saveMaterial(_library,
                        newMaterial,
                        QStringLiteral("/Test Material5.FCMat"),
                        true,  // overwrite
                        true,  // saveAsCopy
                        true);// saveInherited
        EXPECT_EQ(newMaterial->getUUID(), uuid1);
        EXPECT_EQ(newMaterial->getName(), QStringLiteral("Test Material5"));
    }
    catch (...) {
        FAIL() << "An unknown exception has occured 8\n";
    }

        // Save to a new file, inheritance mode, testing overwrite as no copy, new copy
    try {
        _materialManager->saveMaterial(_library,
                        newMaterial,
                        QStringLiteral("/Test Material6.FCMat"),
                        false,  // overwrite
                        true,  // saveAsCopy
                        true);// saveInherited
        EXPECT_EQ(newMaterial->getUUID(), uuid1);
        EXPECT_EQ(newMaterial->getName(), QStringLiteral("Test Material6"));
    }
    catch (...) {
        FAIL() << "An unknown exception has occured 9\n";
    }

    try {
        _materialManager->saveMaterial(_library,
                        newMaterial,
                        QStringLiteral("/Test Material6.FCMat"),
                        true,  // overwrite
                        false,  // saveAsCopy
                        true);// saveInherited
        EXPECT_EQ(newMaterial->getUUID(), uuid1);
        EXPECT_EQ(newMaterial->getName(), QStringLiteral("Test Material6"));
    }
    catch (...) {
        FAIL() << "An unknown exception has occured 10\n";
    }
}

TEST_F(TestMaterialCards, TestColumns)
{
    ASSERT_NE(_modelManager, nullptr);
    ASSERT_TRUE(_library);

    auto testMaterial = _materialManager->getMaterial(_testMaterialUUID);

    EXPECT_TRUE(testMaterial->hasPhysicalProperty(QStringLiteral("TestArray2D")));
    auto array2d = testMaterial->getPhysicalProperty(QStringLiteral("TestArray2D"))->getMaterialValue();
    EXPECT_TRUE(array2d);
    EXPECT_EQ(dynamic_cast<Materials::Array2D &>(*array2d).columns(), 2);

    EXPECT_TRUE(testMaterial->hasPhysicalProperty(QStringLiteral("TestArray2D3Column")));
    auto array2d3Column = testMaterial->getPhysicalProperty(QStringLiteral("TestArray2D3Column"))->getMaterialValue();
    EXPECT_TRUE(array2d3Column);
    EXPECT_EQ(dynamic_cast<Materials::Array2D &>(*array2d3Column).columns(), 3);

    EXPECT_TRUE(testMaterial->hasPhysicalProperty(QStringLiteral("TestArray3D")));
    auto array3d = testMaterial->getPhysicalProperty(QStringLiteral("TestArray3D"))->getMaterialValue();
    EXPECT_TRUE(array3d);
    EXPECT_EQ(dynamic_cast<Materials::Array3D &>(*array3d).columns(), 2);
}

// clang-format on
