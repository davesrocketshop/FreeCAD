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

#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <QDirIterator>
#include <QMutexLocker>

#include <App/Application.h>
#include <Base/Console.h>

#include "Model.h"
#include "ModelLoader.h"
#include "ModelManagerDB.h"

#if defined(BUILD_MATERIAL_DATABASE)
#include "Database.h"
#endif  // BUILD_MATERIAL_DATABASE


using namespace Materials;

std::shared_ptr<std::map<QString, std::shared_ptr<Model>>> ModelManagerDB::_modelMap = nullptr;
QMutex ModelManagerDB::_mutex;
LRU::Cache<QString, std::shared_ptr<Model>> ModelManagerDB::_cache(100);


TYPESYSTEM_SOURCE(Materials::ModelManagerDB, Base::BaseClass)

    ModelManagerDB::ModelManagerDB()
{
    initCache();
}

void ModelManagerDB::initCache()
{
    QMutexLocker locker(&_mutex);

    if (_modelMap == nullptr) {
        _modelMap = std::make_shared<std::map<QString, std::shared_ptr<Model>>>();
    }

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Database");
    auto cacheSize = hGrp->GetInt("ModelCacheSize", 100);
    _cache.capacity(cacheSize);

    _cache.monitor();
}

double ModelManagerDB::modelHitRate()
{
    return _cache.stats().hit_rate();
}

void ModelManagerDB::cleanup()
{
}

void ModelManagerDB::refresh()
{
}

std::shared_ptr<Model> ModelManagerDB::getModel(const QString& uuid) const
{
    // if (_modelMap->find(uuid) != _modelMap->end()) {
    //     Base::Console().Log("Model cache hit\n");
    //     return nullptr;
    // }
    if (_cache.contains(uuid)) {
        return _cache.lookup(uuid);
    }
    Database db;

    auto model = db.getModel(uuid);
    // if (model) {
        // (*_modelMap)[uuid] = model;
    // }
    _cache.emplace(uuid, model);
    return model;
}
