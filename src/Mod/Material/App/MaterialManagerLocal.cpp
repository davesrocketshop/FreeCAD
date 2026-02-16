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

#include <random>

#include <QMutex>
#include <QMutexLocker>

#include <App/Application.h>
#include <App/Material.h>

#include "Exceptions.h"
#include "LibraryManager.h"
#include "MaterialConfigLoader.h"
#include "MaterialFilter.h"
#include "MaterialLibrary.h"
#include "MaterialLoader.h"
#include "MaterialManagerLocal.h"
#include "ModelManager.h"
#include "ModelUuids.h"


using namespace Materials;

/* TRANSLATOR Material::Materials */

std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> MaterialManagerLocal::_libraryList = nullptr;
std::shared_ptr<std::map<std::string, std::shared_ptr<Material>>> MaterialManagerLocal::_materialMap
    = nullptr;
QMutex MaterialManagerLocal::_mutex;

TYPESYSTEM_SOURCE(Materials::MaterialManagerLocal, Base::BaseClass)

MaterialManagerLocal::MaterialManagerLocal()
{
    initLibraries();
}

void MaterialManagerLocal::initLibraries()
{
    QMutexLocker locker(&_mutex);

    if (_materialMap == nullptr) {
        // Load the models first
        ModelManager::getManager();

        _materialMap = std::make_shared<std::map<std::string, std::shared_ptr<Material>>>();

        if (_libraryList == nullptr) {
            _libraryList = getConfiguredLibraries(true);  // Include disabled
        }

        // Load the libraries
        MaterialLoader loader(_materialMap, _libraryList);
    }
}

void MaterialManagerLocal::cleanup()
{
    QMutexLocker locker(&_mutex);

    if (_libraryList) {
        _libraryList->clear();
        _libraryList = nullptr;
    }

    if (_materialMap) {
        for (auto& it : *_materialMap) {
            // This is needed to resolve cyclic dependencies
            it.second->setLibrary(nullptr);
        }
        _materialMap->clear();
        _materialMap = nullptr;
    }
}

void MaterialManagerLocal::refresh()
{
    // This is very expensive and can be improved using observers?
    ModelManager::getManager().refresh();
    cleanup();
    initLibraries();
}

//=====
//
// Library management
//
//=====

std::shared_ptr<std::vector<LibraryObject>> MaterialManagerLocal::libraryMaterials(
    const std::string& libraryName
)
{
    auto materials = std::make_shared<std::vector<LibraryObject>>();

    for (auto& it : *_materialMap) {
        // This is needed to resolve cyclic dependencies
        auto library = it.second->getLibrary();
        if (library->isName(libraryName)) {
            materials->push_back(
                LibraryObject(it.first, it.second->getDirectory(), it.second->getName())
            );
        }
    }

    return materials;
}

bool MaterialManagerLocal::passFilter(
    const Material& material,
    const Materials::MaterialFilter& filter,
    const Materials::MaterialFilterOptions& options
) const
{
    // filter out old format files
    if (material.isOldFormat() && !options.includeLegacy()) {
        return false;
    }

    // filter based on models
    return filter.modelIncluded(material);
}

std::shared_ptr<std::vector<LibraryObject>> MaterialManagerLocal::libraryMaterials(
    const std::string& libraryName,
    const MaterialFilter& filter,
    const MaterialFilterOptions& options
)
{
    auto materials = std::make_shared<std::vector<LibraryObject>>();

    for (auto& it : *_materialMap) {
        // This is needed to resolve cyclic dependencies
        auto library = it.second->getLibrary();
        if (library->isName(libraryName)) {
            if (passFilter(*it.second, filter, options)) {
                materials->push_back(
                    LibraryObject(it.first, it.second->getDirectory(), it.second->getName())
                );
            }
        }
    }

    return materials;
}

// bool MaterialManagerLocal::exists(const std::string& libraryName)
// {
//     for (auto& library : *_libraryList) {
//         if (library->isLocal() && library->isName(libraryName)) {
//             return true;
//         }
//     }

//     return false;
// }

//=====
//
// Folder management
//
//=====

std::shared_ptr<std::list<std::string>> MaterialManagerLocal::getMaterialFolders(
    const std::shared_ptr<MaterialLibraryLocal>& library
) const
{
    return MaterialLoader::getMaterialFolders(*library);
}

void MaterialManagerLocal::createFolder(
    const std::shared_ptr<MaterialLibraryLocal>& library,
    const std::string& path
)
{
    library->createFolder(path);
}

void MaterialManagerLocal::renameFolder(
    const std::shared_ptr<MaterialLibraryLocal>& library,
    const std::string& oldPath,
    const std::string& newPath
)
{
    library->renameFolder(oldPath, newPath);
}

void MaterialManagerLocal::moveFolderLocal(
    const std::shared_ptr<MaterialLibrary>& sourceLibrary,
    const std::string& sourcePath,
    const std::shared_ptr<MaterialLibrary>& destinationLibrary,
    const std::string& destinationPath
)
{
    if (!sourceLibrary->isLocal() || !destinationLibrary->isLocal()) {
        throw MoveError("Non-local folder move");
    }
    auto from = sourceLibrary->getLocalPath(sourcePath);
    Base::FileInfo fromInfo(from);
    auto to = destinationLibrary->getLocalPath(destinationPath + "/" + fromInfo.fileName());
    try {
        std::cout << "Copy from " << from << " to " << to << "\n";
        // Copy the directory recursively (works across file systems)
        fs::copy(from, to, fs::copy_options::recursive | fs::copy_options::overwrite_existing);

        // Remove the original directory
        fs::remove_all(from);
    }
    catch (const fs::filesystem_error& e) {
        std::cout << e.what() << "\n";
        throw MoveError(e.what());
    }
}

void MaterialManagerLocal::deleteRecursive(
    const std::shared_ptr<MaterialLibraryLocal>& library,
    const std::string& path
)
{
    library->deleteRecursive(path);
    dereference();
}

//=====
//
// Material management
//
//=====

std::shared_ptr<std::map<std::string, std::shared_ptr<Material>>> MaterialManagerLocal::getLocalMaterials() const
{
    return _materialMap;
}

std::shared_ptr<Material> MaterialManagerLocal::getMaterial(const std::string& uuid) const
{
    try {
        return _materialMap->at(uuid);
    }
    catch (std::out_of_range&) {
        throw MaterialNotFound();
    }
}

std::shared_ptr<Material> MaterialManagerLocal::getMaterialByPath(const std::string& path) const
{
    std::string cleanPath = Library::cleanPath(path);

    for (auto& library : *_libraryList) {
        if (library->isLocal() && !library->isDisabled()) {
            auto materialLibrary = std::make_shared<MaterialLibraryLocal>(*library);
            if (cleanPath.starts_with(materialLibrary->getDirectory())) {
                try {
                    return materialLibrary->getMaterialByPath(cleanPath);
                }
                catch (const MaterialNotFound&) {
                }

                // See if it's a new file saved by the old editor
                {
                    QMutexLocker locker(&_mutex);

                    if (MaterialConfigLoader::isConfigStyle(path)) {
                        auto material
                            = MaterialConfigLoader::getMaterialFromPath(materialLibrary, path);
                        if (material) {
                            (*_materialMap)[material->getUUID()]
                                = materialLibrary->addMaterial(material, path);
                        }

                        return material;

                    }
                }
            }
        }
    }

    // Older workbenches may try files outside the context of a library
    {
        QMutexLocker locker(&_mutex);

        if (MaterialConfigLoader::isConfigStyle(path)) {
            auto material = MaterialConfigLoader::getMaterialFromPath(nullptr, path);

            return material;
        }
    }

    throw MaterialNotFound();
}

std::shared_ptr<Material> MaterialManagerLocal::getMaterialByPath(
    const std::string& path,
    const std::string& lib
) const
{
    auto library = LibraryManager::getManager().getLibrary(lib);  // May throw LibraryNotFound
    if (library->isLocal()) {
        auto materialLibrary
            = std::make_shared<Materials::MaterialLibraryLocal>(library);
        if (materialLibrary) {
            return materialLibrary->getMaterialByPath(path);  // May throw MaterialNotFound
        }
    }

    throw LibraryNotFound();
}

bool MaterialManagerLocal::exists(const std::string& uuid) const
{
    try {
        auto material = getMaterial(uuid);
        if (material) {
            return true;
        }
    }
    catch (const MaterialNotFound&) {
    }

    return false;
}

bool MaterialManagerLocal::exists(const MaterialLibrary& library, const std::string& uuid) const
{
    try {
        auto material = getMaterial(uuid);
        if (material && material->getLibrary()->isLocal()) {
            auto materialLibrary = std::make_shared<MaterialLibraryLocal>(*material->getLibrary());
            return (*materialLibrary == library);
        }
    }
    catch (const MaterialNotFound&) {
    }

    return false;
}

void MaterialManagerLocal::move(
    const std::shared_ptr<MaterialLibrary>& library,
    const std::string& path,
    std::shared_ptr<Material> original
)
{
    if (*library != *original->getLibrary()) {
        original->setLibrary(library);
    }
    original->setDirectory(path);
}

void MaterialManagerLocal::remove(const std::string& uuid)
{
    try {
        auto material = getMaterial(uuid);
        auto path = material->getLibrary()->getDirectory() + "/"
            + material->getDirectory() + "/" + material->getName()
            + ".FCMat";

        Base::FileInfo file(path);
        if (!file.deleteFile()) {
            Base::Console().log("Unable to remove '%s'\n", path.c_str());
        }
        _materialMap->erase(uuid);
    }
    catch (const MaterialNotFound &) {
        // Nothing to remove
    }
}

void MaterialManagerLocal::saveMaterial(
    const std::shared_ptr<MaterialLibraryLocal>& library,
    const std::shared_ptr<Material>& material,
    const std::string& path,
    bool overwrite,
    bool saveAsCopy,
    bool saveInherited
) const
{
    if (library->isLocal()) {
        auto newMaterial = library->saveMaterial(material, path, overwrite, saveAsCopy, saveInherited);
        newMaterial->resetEditState();
        (*_materialMap)[newMaterial->getUUID()] = newMaterial;
    }
    else {
        throw LibraryNotFound();
    }
}

bool MaterialManagerLocal::isMaterial(const fs::path& p) const
{
    if (!fs::is_regular_file(p)) {
        return false;
    }
    // check file extension
    if (p.extension() == ".FCMat") {
        return true;
    }
    return false;
}

bool MaterialManagerLocal::isMaterial(const Base::FileInfo& file) const
{
    if (!file.isFile()) {
        return false;
    }
    // check file extension
    if (file.hasExtension("FCMat")) {
        return true;
    }
    return false;
}

std::shared_ptr<std::map<std::string, std::shared_ptr<Material>>> MaterialManagerLocal::materialsWithModel(
    const std::string& uuid
) const
{
    std::shared_ptr<std::map<std::string, std::shared_ptr<Material>>> dict
        = std::make_shared<std::map<std::string, std::shared_ptr<Material>>>();

    for (auto& it : *_materialMap) {
        std::string key = it.first;
        auto material = it.second;

        if (material->hasModel(uuid)) {
            (*dict)[key] = material;
        }
    }

    return dict;
}

std::shared_ptr<std::map<std::string, std::shared_ptr<Material>>> MaterialManagerLocal::materialsWithModelComplete(
    const std::string& uuid
) const
{
    std::shared_ptr<std::map<std::string, std::shared_ptr<Material>>> dict
        = std::make_shared<std::map<std::string, std::shared_ptr<Material>>>();

    for (auto& it : *_materialMap) {
        std::string key = it.first;
        auto material = it.second;

        if (material->isModelComplete(uuid)) {
            (*dict)[key] = material;
        }
    }

    return dict;
}

void MaterialManagerLocal::dereference() const
{
    // First clear the inheritences
    for (auto& it : *_materialMap) {
        auto material = it.second;
        material->clearDereferenced();
        material->clearInherited();
    }

    // Run the dereference again
    for (auto& it : *_materialMap) {
        dereference(it.second);
    }
}

void MaterialManagerLocal::dereference(std::shared_ptr<Material> material) const
{
    MaterialLoader::dereference(_materialMap, material);
}

std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> MaterialManagerLocal::getConfiguredLibraries(
    bool includeDisabled
)
{
    auto libraryList = std::make_shared<std::list<std::shared_ptr<MaterialLibrary>>>();
    auto local = LibraryManager::getManager().getConfiguredLibraries(includeDisabled);
    for (auto library : *local) {
        libraryList->push_back(std::make_shared<MaterialLibrary>(library));
    }

    return libraryList;
}
