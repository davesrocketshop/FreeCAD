// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 David Carter <dcarter@david.carter.ca>             *
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

#include <QMutexLocker>

#include <App/Application.h>

#include "ExternalManager.h"
#include "Model.h"
#include "ModelLoader.h"
#include "ModelManager.h"
#include "ModelManagerExternal.h"

using namespace Materials;

QMutex ModelManagerExternal::_mutex;
LRU::Cache<std::string, std::shared_ptr<Model>> ModelManagerExternal::_cache(DEFAULT_CACHE_SIZE);

TYPESYSTEM_SOURCE(Materials::ModelManagerExternal, Base::BaseClass)

ModelManagerExternal::ModelManagerExternal()
{
    initCache();
}

void ModelManagerExternal::initCache()
{
    QMutexLocker locker(&_mutex);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/ExternalInterface");
    auto cacheSize = hGrp->GetInt("ModelCacheSize", DEFAULT_CACHE_SIZE);
    _cache.capacity(cacheSize);

    _cache.monitor();
}

void ModelManagerExternal::cleanup()
{
}

void ModelManagerExternal::refresh()
{
    resetCache();
}

//=====
//
// Library management
//
//=====

std::shared_ptr<std::vector<LibraryObject>>
ModelManagerExternal::libraryModels(const std::string& libraryName)
{
    return ExternalManager::getManager()->libraryModels(libraryName);
}

//=====
//
// Model management
//
//=====

std::shared_ptr<Model> ModelManagerExternal::modelNotFound(const std::string& uuid)
{
    // Setting the cache value to nullptr prevents repeated lookups
    _cache.emplace(uuid, nullptr);
    return nullptr;
}

std::shared_ptr<Model> ModelManagerExternal::getModel(const std::string& uuid)
{
    if (_cache.contains(uuid)) {
        return _cache.lookup(uuid);
    }
    try
    {
        auto model = ExternalManager::getManager()->getModel(uuid);
        ModelManager::dereference(model);
        _cache.emplace(uuid, model);
        return model;
    }
    catch (const ModelNotFound& e) {
        return modelNotFound(uuid);
    }
    catch (const ConnectionError& e) {
        return modelNotFound(uuid);
    }
    catch (...) {
        return modelNotFound(uuid);
    }
}

std::shared_ptr<std::map<std::string, std::shared_ptr<Model>>> ModelManagerExternal::getModels()
{
    // TODO: Implement an external call
    auto models = std::make_shared<std::map<std::string, std::shared_ptr<Model>>>();
    auto libraries = ExternalManager::getManager()->modelLibraries();
    for (auto library : *libraries) {
        auto libModels = ExternalManager::getManager()->libraryModels(library->getName());
        for (auto libObject : *libModels) {
            // This dereferences and places the model in the cache
            auto model = getModel(libObject.getUUID());
            models->emplace(libObject.getUUID(), model);
        }
    }
    return models;
}

void ModelManagerExternal::addModel(const std::string& libraryName,
                                    const std::string& path,
                                    const Model& model)
{
    _cache.erase(model.getUUID().toStdString());
    ExternalManager::getManager()->addModel(libraryName, path, model);
}

void ModelManagerExternal::migrateModel(const std::string& libraryName,
                                    const std::string& path,
                                    const Model& model)
{
    _cache.erase(model.getUUID().toStdString());
    ExternalManager::getManager()->migrateModel(libraryName, path, model);
}

//=====
//
// Cache management
//
//=====

void ModelManagerExternal::resetCache()
{
    _cache.clear();
}

double ModelManagerExternal::modelHitRate()
{
    auto hitRate = _cache.stats().hit_rate();
    if (std::isnan(hitRate)) {
        return 0;
    }
    return hitRate;
}
