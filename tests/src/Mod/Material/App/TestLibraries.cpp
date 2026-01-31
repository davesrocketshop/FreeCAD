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

class TestLibraries : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override {
        _modelManager = &(Materials::ModelManager::getManager());
        _materialManager = &(Materials::MaterialManager::getManager());

        // Disable the external interface
        _useExternal = _materialManager->useExternal();
        _materialManager->setUseExternal(false);

        // Disable other libraries
        auto libraries = _materialManager->getLibraries(true);
        _libraries.clear();
        for (auto& library : *libraries) {
            _libraries.emplace(library->getName(), library->isDisabled());
            _materialManager->setDisabled(*library, true);
        }

        // _materialManager->refresh();
    }

    void TearDown() override {
        // Restore other libraries
        for (auto& [name, disabled] : _libraries) {
            _materialManager->setDisabled(name, disabled, true);
        }

        // Restore the external interface AFTER the local libraries
        _materialManager->setUseExternal(_useExternal);

        // _materialManager->refresh();
    }

    Materials::ModelManager* _modelManager {};
    Materials::MaterialManager* _materialManager {};

    bool _useExternal {};
    std::map<QString, bool> _libraries;
};

TEST_F(TestLibraries, TestDisabled)
{
    auto libraries = _materialManager->getLibraries();
    ASSERT_EQ(libraries->size(), 0);

    libraries = _materialManager->getLibraries(true);
    ASSERT_GT(libraries->size(), 0);

    auto library = _materialManager->getLibrary(QStringLiteral("System"));
    EXPECT_NE(&library, nullptr);
    ASSERT_EQ(library->getName(), QStringLiteral("System"));
    ASSERT_TRUE(library->isDisabled());

    _materialManager->setDisabled(*library, false);
    ASSERT_FALSE(library->isDisabled()); // Since this is a shared pointer
    library = _materialManager->getLibrary(QStringLiteral("System"));
    ASSERT_FALSE(library->isDisabled());

    libraries = _materialManager->getLibraries();
    ASSERT_EQ(libraries->size(), 1);
}

TEST_F(TestLibraries, TestDisabledModels)
{
    auto libraries = _modelManager->getLibraries();
    ASSERT_EQ(libraries->size(), 0);

    auto library = _modelManager->getLibrary(QStringLiteral("System"));
    EXPECT_NE(&library, nullptr);
    ASSERT_EQ(library->getName(), QStringLiteral("System"));
    ASSERT_TRUE(library->isDisabled());
    auto objects = _modelManager->libraryModels(library->getName());
    ASSERT_GT(objects->size(), 0);
    for (auto modelObject : *objects) {
        EXPECT_THROW(_modelManager->getModel(modelObject.getUUID()), Materials::ModelNotFound);
        // auto model = _modelManager->getModel(modelObject.getUUID());
        // ASSERT_TRUE(model->isDisabled());
    }

    auto models = _modelManager->getModels();
    ASSERT_EQ(models->size(), 0);

    _materialManager->setDisabled(*library, false);
    ASSERT_FALSE(library->isDisabled()); // Since this is a shared pointer
    library = _modelManager->getLibrary(QStringLiteral("System"));
    ASSERT_FALSE(library->isDisabled());

    libraries = _modelManager->getLibraries();
    ASSERT_EQ(libraries->size(), 1);

    models = _modelManager->getModels();
    ASSERT_GT(models->size(), 0);
    for (auto [uuid, model] : *models) {
        ASSERT_FALSE(model->isDisabled());
        ASSERT_EQ(model->getLibrary(), library);
    }

    _materialManager->setDisabled(*library, true);
    ASSERT_TRUE(library->isDisabled()); // Since this is a shared pointer
    library = _modelManager->getLibrary(QStringLiteral("System"));
    ASSERT_TRUE(library->isDisabled());

    libraries = _modelManager->getLibraries();
    ASSERT_EQ(libraries->size(), 0);

    models = _modelManager->getModels();
    ASSERT_EQ(models->size(), 0);
}
