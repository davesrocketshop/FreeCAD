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

std::shared_ptr<MaterialLibrary> MaterialManagerExternal::getLibrary(const std::string& name) const
{
    try {
        auto lib = ExternalManager::getManager()->getLibrary(name);
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

std::shared_ptr<MaterialLibrary> MaterialManagerExternal::createLibrary(const std::string& libraryName,
                                            const QByteArray& icon,
                                            bool readOnly)
{
    ExternalManager::getManager()->createLibrary(libraryName, icon, readOnly);
    return getLibrary(libraryName);
}

void MaterialManagerExternal::renameLibrary(const std::string& libraryName, const std::string& newName)
{
    ExternalManager::getManager()->renameLibrary(libraryName, newName);
}

void MaterialManagerExternal::changeIcon(const std::string& libraryName, const QByteArray& icon)
{
    ExternalManager::getManager()->changeIcon(libraryName, icon);
}

void MaterialManagerExternal::removeLibrary(const std::string& libraryName)
{
    ExternalManager::getManager()->removeLibrary(libraryName);
}

std::shared_ptr<std::vector<LibraryObject>>
MaterialManagerExternal::libraryMaterials(const std::string& libraryName)
{
    return ExternalManager::getManager()->libraryMaterials(libraryName);
}

std::shared_ptr<std::vector<LibraryObject>>
MaterialManagerExternal::libraryMaterials(const std::string& libraryName,
                                          const MaterialFilter& filter,
                                          const MaterialFilterOptions& options)
{
    return ExternalManager::getManager()->libraryMaterials(libraryName, filter, options);
}

void MaterialManagerExternal::setDisabled(const std::string& libraryName, bool disabled)
{
    if (!exists(libraryName)) {
        throw LibraryNotFound();
    }

    ParameterGrp::handle param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Resources/Remote"
    );
    auto group = param->GetGroup(libraryName.c_str());
        group->SetBool("Disabled", disabled);
}

bool MaterialManagerExternal::isDisabled(const std::string& libraryName)
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

bool MaterialManagerExternal::exists(const std::string& libraryName)
{
    try {
        auto lib = ExternalManager::getManager()->getLibrary(libraryName);
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

std::shared_ptr<std::vector<std::string>> MaterialManagerExternal::getMaterialFolders(
    const MaterialLibrary& library
) const
{
    return ExternalManager::getManager()->libraryFolders(library.getName());
}

std::shared_ptr<std::vector<std::string>> MaterialManagerExternal::getMaterialSubFolders(
    const MaterialLibrary& library,
    const std::string& path
) const
{
    return ExternalManager::getManager()->librarySubFolders(library.getName(), path);
}

void MaterialManagerExternal::createFolder(const MaterialLibrary& library,
                                           const std::string& path)
{
    ExternalManager::getManager()->createFolder(library.getName(), path);
}

void MaterialManagerExternal::renameFolder(const MaterialLibrary& library,
                                           const std::string& oldPath,
                                           const std::string& newPath)
{
    ExternalManager::getManager()->renameFolder(
        library.getName(),
        oldPath,
        newPath
    );
}

void MaterialManagerExternal::moveFolder(
    const std::shared_ptr<MaterialLibrary>& sourceLibrary,
    const std::string& sourcePath,
    const std::shared_ptr<MaterialLibrary>& destinationLibrary,
    const std::string& destinationPath
)
{
    ExternalManager::getManager()->moveFolder(
        sourceLibrary->getName(),
        sourcePath,
        destinationLibrary->getName(),
        destinationPath
    );
}

void MaterialManagerExternal::deleteRecursive(const MaterialLibrary& library,
                                              const std::string& path)
{
    ExternalManager::getManager()->deleteRecursive(library.getName(), path);
}

std::shared_ptr<std::vector<Material>> MaterialManagerExternal::folderMaterials(
    const MaterialLibrary& library,
    const std::string& sourcePath
) const
{
    if (library.isLocal()) {
        throw LibraryNotFound("Library is local");
    }

    auto materials = ExternalManager::getManager()->folderMaterials(library.getName(), sourcePath);
    auto materialList = std::make_shared<std::vector<Material>>();
    for (const auto& entry : *materials) {
        try {
            auto material = getMaterial(entry.getUUID());
            if (material) {
                materialList->push_back(*material);
            }
        }
        catch (const MaterialNotFound& e) {
            Base::Console().log("Material '%s' not found\n", entry.getUUID().c_str());
        }
    }
    return materialList;
}

//=====
//
// Material management
//
//=====

std::shared_ptr<Material> MaterialManagerExternal::materialNotFound(const std::string& uuid) const
{
    // Setting the cache value to nullptr prevents repeated lookups
    _cache.emplace(uuid, nullptr);
    return nullptr;
}

std::shared_ptr<Material> MaterialManagerExternal::getMaterial(const std::string& uuid) const
{
    if (_cache.contains(uuid)) {
        return _cache.lookup(uuid);
    }
    try {
        auto material = ExternalManager::getManager()->getMaterial(uuid);
        _cache.emplace(uuid, material);
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

void MaterialManagerExternal::addMaterial(const std::string& libraryName,
                                          const std::string& path,
                                          const Material& material)
{
    _cache.erase(material.getUUID());
    auto stripped = stripFilename(path, material);
    ExternalManager::getManager()->addMaterial(libraryName, stripped, material);
}

void MaterialManagerExternal::migrateMaterial(const std::string& libraryName,
                                              const std::string& path,
                                              const Material& material)
{
    _cache.erase(material.getUUID());
    auto stripped = stripFilename(path, material);
    ExternalManager::getManager()->migrateMaterial(libraryName, stripped, material);
}

bool MaterialManagerExternal::exists(const std::string& uuid) const
{
    if (_cache.contains(uuid)) {
        return true;
    }
    return ExternalManager::getManager()->materialExists("", uuid);
}

bool MaterialManagerExternal::exists(const MaterialLibrary& library, const std::string& uuid) const
{
    return ExternalManager::getManager()->materialExists(library.getName(), uuid);
}

void MaterialManagerExternal::move(
    const std::shared_ptr<MaterialLibrary>& library,
    const std::string& path,
    std::shared_ptr<Material> original
)
{
    _cache.erase(original->getUUID());
    auto stripped = stripFilename(path, *original);
    ExternalManager::getManager()
        ->moveMaterial(library->getName(), stripped, original->getUUID());
}

void MaterialManagerExternal::remove(const std::string& uuid)
{
    _cache.erase(uuid);
    ExternalManager::getManager()->removeMaterial(uuid);
}

void MaterialManagerExternal::saveMaterial(
    const MaterialLibrary& library,
    const Material& material,
    const std::string& path,
    bool overwrite
) const
{
    _cache.erase(material.getUUID());

    auto stripped = stripFilename(path, material);
    if (ExternalManager::getManager()
            ->materialExists(library.getName(), material.getUUID())) {
        if (overwrite) {
            ExternalManager::getManager()->updateMaterial(library.getName(), stripped, material);
        }
        else {
            throw MaterialExists();
        }
    }
    else {
        ExternalManager::getManager()->addMaterial(library.getName(), stripped, material);
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

std::string MaterialManagerExternal::stripFilename(const std::string& path, const Material& material) const
{
    auto stripped = path;
    auto filename = material.getName() + ".FCMat";
    if (stripped.ends_with(filename)) {
        stripped.erase(filename.size() + 1);  // Allow for the separator
        Base::Console()
            .log("Path '%s' -> '%s'\n", path.c_str(), stripped.c_str());
    }
    return stripped;
}
