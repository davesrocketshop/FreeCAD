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
#include <random>
#endif

#include <QMutex>
#include <QMutexLocker>

#include <App/Application.h>

#include "Exceptions.h"
#include "MaterialConfigLoader.h"
#include "MaterialLoader.h"
#include "MaterialManagerDB.h"
#include "ModelManager.h"
#include "ModelUuids.h"

#if defined(BUILD_MATERIAL_DATABASE)
#include "Database.h"
#endif  // BUILD_MATERIAL_DATABASE


using namespace Materials;

/* TRANSLATOR Material::Materials */

QMutex MaterialManagerDB::_mutex;
LRU::Cache<QString, std::shared_ptr<Material>> MaterialManagerDB::_cache(100);

TYPESYSTEM_SOURCE(Materials::MaterialManagerDB, Base::BaseClass)

MaterialManagerDB::MaterialManagerDB()
{
    initCache();
}

void MaterialManagerDB::initCache()
{
    QMutexLocker locker(&_mutex);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Database");
    auto cacheSize = hGrp->GetInt("MaterialCacheSize", 100);
    _cache.capacity(cacheSize);

    _cache.monitor();
}

double MaterialManagerDB::materialHitRate()
{
    return _cache.stats().hit_rate();
}

void MaterialManagerDB::refresh()
{
    _cache.clear();
}

std::shared_ptr<Material> MaterialManagerDB::getMaterial(const QString& uuid) const
{
    if (_cache.contains(uuid)) {
        return _cache.lookup(uuid);
    }
    Database db;

    auto material = db.getMaterial(uuid);
    _cache.emplace(uuid, material);
    return material;
}
