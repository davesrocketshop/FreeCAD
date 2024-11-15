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

#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <QDirIterator>
#include <QMutexLocker>

#include <Base/Console.h>

#include "Model.h"
#include "ModelLoader.h"
#include "ModelManager.h"

#include "ModelManagerDB.h"
#include "ModelManagerLocal.h"

#if defined(BUILD_MATERIAL_DATABASE)
#include "Database.h"
#endif  // BUILD_MATERIAL_DATABASE


using namespace Materials;

std::unique_ptr<ModelManagerLocal> ModelManager::_localManager;
std::unique_ptr<ModelManagerDB> ModelManager::_dbManager;
QMutex ModelManager::_mutex;

TYPESYSTEM_SOURCE(Materials::ModelManager, Base::BaseClass)

ModelManager::ModelManager()
{
    initManagers();
}

void ModelManager::initManagers()
{
    QMutexLocker locker(&_mutex);

    if (!_localManager) {
        _localManager = std::make_unique<ModelManagerLocal>();
    }

    if (!_dbManager) {
        _dbManager = std::make_unique<ModelManagerDB>();
    }
}

bool ModelManager::isModel(const QString& file)
{
    return ModelManagerLocal::isModel(file);
}

void ModelManager::cleanup()
{
    return ModelManagerLocal::cleanup();
}

void ModelManager::refresh()
{
    return _localManager->refresh();
}

std::shared_ptr<std::list<std::shared_ptr<ModelLibrary>>> ModelManager::getModelLibraries()
{
    return _localManager->getModelLibraries();
}

std::shared_ptr<std::list<std::shared_ptr<ModelLibrary>>> ModelManager::getLocalModelLibraries()
{
    return _localManager->getModelLibraries();
}

std::shared_ptr<std::map<QString, std::shared_ptr<Model>>> ModelManager::getModels()
{
    return _localManager->getModels();
}

std::shared_ptr<Model> ModelManager::getModel(const QString& uuid) const
{
    auto model = _dbManager->getModel(uuid);
    if (model) {
        Base::Console().Log("Found DB model\n");
        return model;
    }
    return _localManager->getModel(uuid);
}

std::shared_ptr<Model> ModelManager::getModelByPath(const QString& path) const
{
    return _localManager->getModelByPath(path);
}

std::shared_ptr<Model> ModelManager::getModelByPath(const QString& path, const QString& lib) const
{
    return _localManager->getModelByPath(path, lib);
}

std::shared_ptr<ModelLibrary> ModelManager::getLibrary(const QString& name) const
{
    return _localManager->getLibrary(name);
}

bool ModelManager::passFilter(ModelFilter filter, Model::ModelType modelType)
{
    switch (filter) {
        case ModelFilter_None:
            return true;

        case ModelFilter_Physical:
            return (modelType == Model::ModelType_Physical);

        case ModelFilter_Appearance:
            return (modelType == Model::ModelType_Appearance);
    }

    return false;
}

void ModelManager::migrateToDatabase()
{
    return ModelManagerLocal::migrateToDatabase();
}

void ModelManager::migrateToDatabase(const std::shared_ptr<ModelLibrary>& library)
{
    return ModelManagerLocal::migrateToDatabase(library);
}

// Cache stats
double ModelManager::modelHitRate()
{
    return ModelManagerDB::modelHitRate();
}
