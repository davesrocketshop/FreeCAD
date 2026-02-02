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

#include <QMutexLocker>

#include <App/Application.h>

#include "Exceptions.h"
#include "ExternalManager.h"
#include "MaterialLibrary.h"
#include "MaterialManagerExternal.h"


using namespace Materials;

/* TRANSLATOR Material::Materials */

QMutex MaterialManagerExternal::_mutex;
LRU::Cache<std::string, std::shared_ptr<Material>>
    MaterialManagerExternal::_cache(DEFAULT_CACHE_SIZE);

TYPESYSTEM_SOURCE(Materials::MaterialManagerExternal, Base::BaseClass)

MaterialManagerExternal::MaterialManagerExternal()
{
    initCache();
}

void MaterialManagerExternal::initCache()
{
    QMutexLocker locker(&_mutex);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/ExternalInterface");
    auto cacheSize = hGrp->GetInt("MaterialCacheSize", DEFAULT_CACHE_SIZE);
    _cache.capacity(cacheSize);

    _cache.monitor();
}

void MaterialManagerExternal::cleanup()
{}

void MaterialManagerExternal::refresh()
{
    resetCache();
}

//=====
//
// Library management
//
//=====

std::shared_ptr<std::vector<std::shared_ptr<MaterialLibrary>>> MaterialManagerExternal::getLibraries()
{
    auto libraryList = std::make_shared<std::vector<std::shared_ptr<MaterialLibrary>>>();
    try {
        auto externalLibraries = ExternalManager::getManager()->libraries();
        for (auto& entry : *externalLibraries) {
            auto library = std::make_shared<MaterialLibrary>(entry);
            libraryList->push_back(library);
        }
    }
    catch (const LibraryNotFound& e) {
    }
    catch (const ConnectionError& e) {
    }

    return libraryList;
}

std::shared_ptr<std::vector<std::shared_ptr<MaterialLibrary>>>
MaterialManagerExternal::getMaterialLibraries()
{
    auto libraryList = std::make_shared<std::vector<std::shared_ptr<MaterialLibrary>>>();
    try {
        auto externalLibraries = ExternalManager::getManager()->materialLibraries();
        for (auto& entry : *externalLibraries) {
            libraryList->push_back(entry);
        }
    }
    catch (const LibraryNotFound& e) {
    }
    catch (const ConnectionError& e) {
    }

    return libraryList;
}

std::shared_ptr<MaterialLibrary> MaterialManagerExternal::getLibrary(const QString& name) const
{
    try {
        auto lib = ExternalManager::getManager()->getLibrary(name.toStdString());
        auto library = std::make_shared<MaterialLibrary>(lib);
        return library;
    }
    catch (const LibraryNotFound& e) {
        throw LibraryNotFound(e);
    }
    catch (const ConnectionError& e) {
        throw LibraryNotFound(e.what());
    }
    catch (...) {
        throw LibraryNotFound("Unknown exception");
    }
}

std::shared_ptr<MaterialLibrary> MaterialManagerExternal::createLibrary(const QString& libraryName,
                                            const QByteArray& icon,
                                            bool readOnly)
{
    ExternalManager::getManager()->createLibrary(libraryName.toStdString(), icon, readOnly);
    return getLibrary(libraryName);
}

void MaterialManagerExternal::renameLibrary(const QString& libraryName, const QString& newName)
{
    ExternalManager::getManager()->renameLibrary(libraryName.toStdString(), newName.toStdString());
}

void MaterialManagerExternal::changeIcon(const QString& libraryName, const QByteArray& icon)
{
    ExternalManager::getManager()->changeIcon(libraryName.toStdString(), icon);
}

void MaterialManagerExternal::removeLibrary(const QString& libraryName)
{
    ExternalManager::getManager()->removeLibrary(libraryName.toStdString());
}

std::shared_ptr<std::vector<LibraryObject>>
MaterialManagerExternal::libraryMaterials(const QString& libraryName)
{
    return ExternalManager::getManager()->libraryMaterials(libraryName.toStdString());
}

std::shared_ptr<std::vector<LibraryObject>>
MaterialManagerExternal::libraryMaterials(const QString& libraryName,
                                          const MaterialFilter& filter,
                                          const MaterialFilterOptions& options)
{
    return ExternalManager::getManager()->libraryMaterials(libraryName.toStdString(), filter, options);
}

void MaterialManagerExternal::setDisabled(const QString& libraryName, bool disabled)
{
    if (!exists(libraryName)) {
        throw LibraryNotFound();
    }

    ParameterGrp::handle param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Resources/Remote"
    );
    auto group = param->GetGroup(libraryName.toStdString().c_str());
        group->SetBool("Disabled", disabled);
}

bool MaterialManagerExternal::isDisabled(const QString& libraryName)
{
    if (!exists(libraryName)) {
        throw LibraryNotFound();
    }

    ParameterGrp::handle param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Resources/Remote"
    );
    auto groups = param->GetGroups();
    for (auto group : groups) {
        if (QString::fromStdString(group->GetGroupName()) == libraryName) {
            return group->GetBool("Disabled", false);
        }
    }
    // No entry means it isn't disabled
    return false;
}

bool MaterialManagerExternal::exists(const QString& libraryName)
{
    try {
        auto lib = ExternalManager::getManager()->getLibrary(libraryName.toStdString());
        return true;
    }
    catch (const LibraryNotFound& e) {
    }
    catch (const ConnectionError& e) {
    }
    catch (...) {
    }

    return false;
}

//=====
//
// Folder management
//
//=====

void MaterialManagerExternal::createFolder(const MaterialLibrary& library,
                                           const QString& path)
{
    ExternalManager::getManager()->createFolder(library.getName(), path.toStdString());
}

void MaterialManagerExternal::renameFolder(const MaterialLibrary& library,
                                           const QString& oldPath,
                                           const QString& newPath)
{
    ExternalManager::getManager()->renameFolder(library.getName(), oldPath.toStdString(), newPath.toStdString());
}

void MaterialManagerExternal::deleteRecursive(const MaterialLibrary& library,
                                              const QString& path)
{
    ExternalManager::getManager()->deleteRecursive(library.getName(), path.toStdString());
}

//=====
//
// Material management
//
//=====

std::shared_ptr<Material> MaterialManagerExternal::materialNotFound(const QString& uuid) const
{
    // Setting the cache value to nullptr prevents repeated lookups
    _cache.emplace(uuid.toStdString(), nullptr);
    return nullptr;
}

std::shared_ptr<Material> MaterialManagerExternal::getMaterial(const QString& uuid) const
{
    if (_cache.contains(uuid.toStdString())) {
        return _cache.lookup(uuid.toStdString());
    }
    try {
        auto material = ExternalManager::getManager()->getMaterial(uuid.toStdString());
        _cache.emplace(uuid.toStdString(), material);
        return material;
    }
    catch (const MaterialNotFound& e) {
        return materialNotFound(uuid);
    }
    catch (const ConnectionError& e) {
        return materialNotFound(uuid);
    }
    catch (...) {
        return materialNotFound(uuid);
    }
}

void MaterialManagerExternal::addMaterial(const QString& libraryName,
                                          const QString& path,
                                          const Material& material)
{
    _cache.erase(material.getUUID().toStdString());
    auto stripped = stripFilename(path, material);
    ExternalManager::getManager()->addMaterial(libraryName.toStdString(), stripped, material);
}

void MaterialManagerExternal::migrateMaterial(const QString& libraryName,
                                              const QString& path,
                                              const Material& material)
{
    _cache.erase(material.getUUID().toStdString());
    auto stripped = stripFilename(path, material);
    ExternalManager::getManager()->migrateMaterial(libraryName.toStdString(), stripped, material);
}

bool MaterialManagerExternal::exists(const QString& uuid) const
{
    if (_cache.contains(uuid.toStdString())) {
        return true;
    }
    return ExternalManager::getManager()->materialExists("", uuid.toStdString());
}

bool MaterialManagerExternal::exists(const MaterialLibrary& library, const QString& uuid) const
{
    return ExternalManager::getManager()->materialExists(library.getName(), uuid.toStdString());
}

void MaterialManagerExternal::move(
    const std::shared_ptr<MaterialLibrary>& library,
    const QString& path,
    std::shared_ptr<Material> original
)
{
    _cache.erase(original->getUUID().toStdString());
    auto stripped = stripFilename(path, *original);
    ExternalManager::getManager()
        ->moveMaterial(library->getName(), stripped, original->getUUID().toStdString());
}

void MaterialManagerExternal::remove(const QString& uuid)
{
    _cache.erase(uuid.toStdString());
    ExternalManager::getManager()->removeMaterial(uuid.toStdString());
}

void MaterialManagerExternal::saveMaterial(
    const std::shared_ptr<MaterialLibrary>& library,
    const std::shared_ptr<Material>& material,
    const QString& path,
    bool overwrite
) const
{
    _cache.erase(material->getUUID().toStdString());

    auto stripped = stripFilename(path, *material);
    if (ExternalManager::getManager()->materialExists(library->getName(), material->getUUID().toStdString())) {
        if (overwrite) {
            ExternalManager::getManager()->updateMaterial(library->getName(), stripped, *material);
        }
        else {
            throw MaterialExists();
        }
    }
    else {
        ExternalManager::getManager()->addMaterial(library->getName(), stripped, *material);
    }
}

//=====
//
// Cache management
//
//=====

void MaterialManagerExternal::resetCache()
{
    _cache.clear();
}

double MaterialManagerExternal::materialHitRate()
{
    auto hitRate = _cache.stats().hit_rate();
    if (std::isnan(hitRate)) {
        return 0;
    }
    return hitRate;
}

std::string MaterialManagerExternal::stripFilename(const QString& path, const Material& material) const
{
    auto stripped = path;
    auto filename = material.getName() + QStringLiteral(".FCMat");
    if (stripped.endsWith(filename)) {
        stripped.truncate(filename.size() + 1);  // Allow for the separator
        Base::Console()
            .log("Path '%s' -> '%s'\n", path.toStdString().c_str(), stripped.toStdString().c_str());
    }
    return stripped.toStdString();
}
