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
#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/ModelManager.h>

#ifdef _MSC_VER
# pragma warning(disable : 4834)
#endif

// clang-format off

class TestMaterialManager : public ::testing::Test {
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
        _materialManager = &(Materials::MaterialManager::getManager());

        // Create test libraries
        QTemporaryDir dir1;
        dir1.setAutoRemove(false);
        auto path = dir1.path().toStdString();
        EXPECT_NO_THROW(library1 = _libraryManager->createLocalLibrary("TestLibrary1", path, path, "icon path", false));
        ASSERT_TRUE(library1);

        QTemporaryDir dir2;
        dir2.setAutoRemove(false);
        path = dir2.path().toStdString();
        EXPECT_NO_THROW(library2 = _libraryManager->createLocalLibrary("TestLibrary2", path, path, "icon path", false));
        ASSERT_TRUE(library2);
    }

    void TearDown() override {
        try {
            _libraryManager->removeLibrary(Materials::LibraryManager::RepositoryLocal, "TestLibrary1");
        }
        catch (...) {}
        try {
            _libraryManager->removeLibrary(Materials::LibraryManager::RepositoryLocal, "TestLibrary2");
        }
        catch (...) {}
        
        // Restore the external interface
        ParameterGrp::handle paramExternal = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Material/ExternalInterface"
        );

        paramExternal->SetBool("UseExternal", _useExternal);

    }

    Materials::LibraryManager* _libraryManager;
    Materials::ModelManager* _modelManager {};
    Materials::MaterialManager* _materialManager {};
    bool _useExternal {};
    // QTemporaryDir dir1;
    // QTemporaryDir dir2;
    std::shared_ptr<Materials::MaterialLibrary> library1;
    std::shared_ptr<Materials::MaterialLibrary> library2;
};

TEST_F(TestMaterialManager, TestInstallation)
{
    ASSERT_NE(_libraryManager, nullptr);
    ASSERT_NE(_modelManager, nullptr);
    ASSERT_NE(_materialManager, nullptr);

    // We should have loaded at least the system library
    auto libraries = _materialManager->getLibraries();
    ASSERT_GT(libraries->size(), 0);

    // We should have at least one material
    auto materials = _materialManager->getLocalMaterials();
    ASSERT_GT(materials->size(), 0);
}

TEST_F(TestMaterialManager, TestFolders)
{
    auto materials = _materialManager->getMaterialFolders(library1);
    ASSERT_EQ(materials->size(), 0);
    materials = _materialManager->getMaterialFolders(library2);
    ASSERT_EQ(materials->size(), 0);
    EXPECT_THROW(materials = _materialManager->getMaterialFolders(nullptr), Materials::LibraryNotFound);

    ASSERT_NO_THROW(_materialManager->createFolder(library1, ""));
    materials = _materialManager->getMaterialFolders(library1);
    ASSERT_EQ(materials->size(), 0);

    ASSERT_NO_THROW(_materialManager->createFolder(library1, "/"));
    materials = _materialManager->getMaterialFolders(library1);
    ASSERT_EQ(materials->size(), 0);

    EXPECT_THROW(_materialManager->createFolder(nullptr, ""), Materials::LibraryNotFound);
    materials = _materialManager->getMaterialFolders(library1);
    ASSERT_EQ(materials->size(), 0);

    ASSERT_FALSE(library1->getDirectory().empty());
    Base::FileInfo info(library1->getDirectory());
    EXPECT_TRUE(info.exists());
    EXPECT_TRUE(info.isDir());
    EXPECT_TRUE(info.isReadable());
    EXPECT_TRUE(info.isWritable());

    ASSERT_NO_THROW(_materialManager->createFolder(library1, "/x/y"));
    materials = _materialManager->getMaterialFolders(library1);
    ASSERT_EQ(materials->size(), 2);
    Base::FileInfo info1(library1->getDirectory() + "/x/y");
    EXPECT_TRUE(info1.exists());
    EXPECT_TRUE(info1.isDir());
    EXPECT_TRUE(info1.isReadable());
    EXPECT_TRUE(info1.isWritable());

    ASSERT_NO_THROW(_materialManager->createFolder(library1, "/z"));
    materials = _materialManager->getMaterialFolders(library1);
    ASSERT_EQ(materials->size(), 3);
    Base::FileInfo info2(library1->getDirectory() + "/z");
    EXPECT_TRUE(info2.exists());
    EXPECT_TRUE(info2.isDir());
    EXPECT_TRUE(info2.isReadable());
    EXPECT_TRUE(info2.isWritable());

    // No leading '/'
    ASSERT_NO_THROW(_materialManager->createFolder(library1, "z"));
    materials = _materialManager->getMaterialFolders(library1);
    ASSERT_EQ(materials->size(), 3);
    Base::FileInfo info3(library1->getDirectory() + "/z");
    EXPECT_TRUE(info3.exists());
    EXPECT_TRUE(info3.isDir());
    EXPECT_TRUE(info3.isReadable());
    EXPECT_TRUE(info3.isWritable());

    ASSERT_NO_THROW(_materialManager->createFolder(library1, "z1"));
    materials = _materialManager->getMaterialFolders(library1);
    ASSERT_EQ(materials->size(), 4);
    Base::FileInfo info4(library1->getDirectory() + "/z1");
    EXPECT_TRUE(info4.exists());
    EXPECT_TRUE(info4.isDir());
    EXPECT_TRUE(info4.isReadable());
    EXPECT_TRUE(info4.isWritable());

    EXPECT_THROW(_materialManager->renameFolder(nullptr, "a", "b"), Materials::LibraryNotFound);
    EXPECT_THROW(_materialManager->renameFolder(library1, "z1", "z"), Materials::RenameError);
    EXPECT_NO_THROW(_materialManager->renameFolder(library1, "z1", "z2"));
    EXPECT_FALSE(info4.exists());
    Base::FileInfo info5(library1->getDirectory() + "/z2");
    EXPECT_TRUE(info5.exists());
    EXPECT_TRUE(info5.isDir());
    EXPECT_TRUE(info5.isReadable());
    EXPECT_TRUE(info5.isWritable());
    ASSERT_EQ(materials->size(), 4);

    EXPECT_NO_THROW(_materialManager->renameFolder(library1, "x", "x1"));
    materials = _materialManager->getMaterialFolders(library1);
    ASSERT_EQ(materials->size(), 4);
    Base::FileInfo info6(library1->getDirectory() + "/x1/y");
    EXPECT_TRUE(info6.exists());
    EXPECT_TRUE(info6.isDir());
    EXPECT_TRUE(info6.isReadable());
    EXPECT_TRUE(info6.isWritable());

    EXPECT_THROW(_materialManager->renameFolder(library1, "y", "y1"), Materials::RenameError);
    EXPECT_NO_THROW(_materialManager->renameFolder(library1, "x1/y", "x1/y1"));
    materials = _materialManager->getMaterialFolders(library1);
    ASSERT_EQ(materials->size(), 4);
    Base::FileInfo info7(library1->getDirectory() + "/x1/y1");
    EXPECT_TRUE(info7.exists());
    EXPECT_TRUE(info7.isDir());
    EXPECT_TRUE(info7.isReadable());
    EXPECT_TRUE(info7.isWritable());

    EXPECT_NO_THROW(_materialManager->renameFolder(library1, "x1/y1", "y"));
    materials = _materialManager->getMaterialFolders(library1);
    ASSERT_EQ(materials->size(), 4);
    Base::FileInfo info8(library1->getDirectory() + "/y");
    EXPECT_TRUE(info8.exists());
    EXPECT_TRUE(info8.isDir());
    EXPECT_TRUE(info8.isReadable());
    EXPECT_TRUE(info8.isWritable());

}

TEST_F(TestMaterialManager, TestMove)
{
    auto folders = _materialManager->getMaterialFolders(library1);
    ASSERT_EQ(folders->size(), 0);
    folders = _materialManager->getMaterialFolders(library2);
    ASSERT_EQ(folders->size(), 0);

    ASSERT_NO_THROW(_materialManager->createFolder(library1, "x/y/z"));
    ASSERT_NO_THROW(_materialManager->createFolder(library1, "a"));
    ASSERT_NO_THROW(_materialManager->createFolder(library1, "b/c"));

    auto mat1 = std::make_shared<Materials::Material>();
    mat1->setName("mat1");
    auto mat2 = std::make_shared<Materials::Material>();
    mat2->setName("mat2");
    auto mat3 = std::make_shared<Materials::Material>();
    mat3->setName("mat3");
    auto mat4 = std::make_shared<Materials::Material>();
    mat4->setName("mat4");
    _materialManager->saveMaterial(library1, mat1, "a/mat1.FCMat", false, true, false);
    _materialManager->saveMaterial(library1, mat2, "x/y/z/mat2.FCMat", false, true, false);
    _materialManager->saveMaterial(library1, mat3, "b/mat3.FCMat", false, true, false);
    _materialManager->saveMaterial(library1, mat4, "b/c/mat4.FCMat", false, true, false);
    folders = _materialManager->getMaterialFolders(library1);
    ASSERT_EQ(folders->size(), 6);
    auto materials = _materialManager->libraryMaterials("TestLibrary1");
    EXPECT_EQ(materials->size(), 4);


    ASSERT_NO_THROW(_materialManager->createFolder(library2, "x1/y1/z1"));
    ASSERT_NO_THROW(_materialManager->createFolder(library2, "a1"));
    ASSERT_NO_THROW(_materialManager->createFolder(library2, "b1/c1"));

    auto mat5 = std::make_shared<Materials::Material>();
    mat1->setName("mat5");
    auto mat6 = std::make_shared<Materials::Material>();
    mat2->setName("mat6");
    auto mat7 = std::make_shared<Materials::Material>();
    mat3->setName("mat7");
    auto mat8 = std::make_shared<Materials::Material>();
    mat4->setName("mat8");
    _materialManager->saveMaterial(library2, mat5, "a1/mat5.FCMat", false, true, false);
    _materialManager->saveMaterial(library2, mat6, "x1/y1/z1/mat6.FCMat", false, true, false);
    _materialManager->saveMaterial(library2, mat7, "b1/mat7.FCMat", false, true, false);
    _materialManager->saveMaterial(library2, mat8, "b1/c1/mat8.FCMat", false, true, false);
    folders = _materialManager->getMaterialFolders(library2);
    ASSERT_EQ(folders->size(), 6);
    materials = _materialManager->libraryMaterials("TestLibrary2");
    EXPECT_EQ(materials->size(), 4);

    // Moves within a library
    EXPECT_THROW(_materialManager->moveFolder(nullptr, "a", nullptr, "b"), Materials::LibraryNotFound);
    EXPECT_THROW(_materialManager->moveFolder(library1, "a", nullptr, "b"), Materials::LibraryNotFound);
    EXPECT_THROW(_materialManager->moveFolder(nullptr, "a", library1, "b"), Materials::LibraryNotFound);
    EXPECT_NO_THROW(_materialManager->moveFolder(library1, "b/c", library1, "a"));
    folders = _materialManager->getMaterialFolders(library1);
    ASSERT_EQ(folders->size(), 6);
    Base::FileInfo info1(library1->getDirectory() + "/b/c");
    EXPECT_FALSE(info1.exists());
    Base::FileInfo info1a(library1->getDirectory() + "/a/c");
    EXPECT_TRUE(info1a.exists());
    EXPECT_TRUE(info1a.isDir());
    EXPECT_TRUE(info1a.isReadable());
    EXPECT_TRUE(info1a.isWritable());
    Base::FileInfo info1b(library1->getDirectory() + "/b/c");
    EXPECT_FALSE(info1b.exists());
    materials = _materialManager->libraryMaterials("TestLibrary1");
    EXPECT_EQ(materials->size(), 4);

    // Moves across libraries
    EXPECT_NO_THROW(_materialManager->moveFolder(library2, "b1", library1, "a"));
    folders = _materialManager->getMaterialFolders(library1);
    ASSERT_EQ(folders->size(), 8);
    folders = _materialManager->getMaterialFolders(library2);
    ASSERT_EQ(folders->size(), 4);
    Base::FileInfo info2(library1->getDirectory() + "/a/b1/c1");
    EXPECT_TRUE(info2.exists());
    EXPECT_TRUE(info2.isDir());
    EXPECT_TRUE(info2.isReadable());
    EXPECT_TRUE(info2.isWritable());
    Base::FileInfo info2a(library2->getDirectory() + "/b1");
    EXPECT_FALSE(info2a.exists());
    materials = _materialManager->libraryMaterials("TestLibrary1");
    EXPECT_EQ(materials->size(), 6);
    materials = _materialManager->libraryMaterials("TestLibrary2");
    EXPECT_EQ(materials->size(), 2);
}
