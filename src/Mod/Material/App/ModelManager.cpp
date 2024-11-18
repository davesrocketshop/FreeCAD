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

#include <App/Application.h>
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
bool ModelManager::_useDatabase = false;

TYPESYSTEM_SOURCE(Materials::ModelManager, Base::BaseClass)

ModelManager::ModelManager()
{
    initManagers();

    _hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Database");
    _useDatabase = _hGrp->GetBool("UseDatabase", false);
    _hGrp->Attach(this);
}

ModelManager::~ModelManager()
{
    _hGrp->Detach(this);
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

void ModelManager::OnChange(ParameterGrp::SubjectType& rCaller,
                            ParameterGrp::MessageType Reason)
{
    const ParameterGrp& rGrp = static_cast<ParameterGrp&>(rCaller);
    if (strcmp(Reason, "UseDatabase") == 0) {
        Base::Console().Log("Use database changed\n");
        _useDatabase = rGrp.GetBool("UseDatabase", false);
        _dbManager->refresh();
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
    _localManager->refresh();
    _dbManager->refresh();
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

std::shared_ptr<std::map<QString, std::shared_ptr<Model>>> ModelManager::getLocalModels()
{
    return _localManager->getModels();
}

std::shared_ptr<Model> ModelManager::getModel(const QString& uuid) const
{
    if (_useDatabase) {
        auto model = _dbManager->getModel(uuid);
        if (model) {
            return model;
        }
    }
    // We really want to return the local model if not found, such as for User folder models
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
    ModelManagerLocal::migrateToDatabase();
    _dbManager->refresh();  // Reset the cache
}

void ModelManager::migrateToDatabase(const std::shared_ptr<ModelLibrary>& library)
{
    ModelManagerLocal::migrateToDatabase(library);
    _dbManager->refresh();  // Reset the cache
}

// Cache stats
double ModelManager::modelHitRate()
{
    return ModelManagerDB::modelHitRate();
}
