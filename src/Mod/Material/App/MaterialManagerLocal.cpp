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

#include <QDirIterator>
#include <QMutex>
#include <QMutexLocker>

#include <App/Application.h>
#include <App/Material.h>

#include "Exceptions.h"
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
std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> MaterialManagerLocal::_materialMap
    = nullptr;
QMutex MaterialManagerLocal::_mutex;

TYPESYSTEM_SOURCE(Materials::MaterialManagerLocal, Base::BaseClass)

MaterialManagerLocal::MaterialManagerLocal()
{
    // TODO: Add a mutex or similar
    initLibraries();
}

void MaterialManagerLocal::initLibraries()
{
    QMutexLocker locker(&_mutex);
    convertConfiguration();

    if (_materialMap == nullptr) {
        // Load the models first
        ModelManager::getManager();

        _materialMap = std::make_shared<std::map<QString, std::shared_ptr<Material>>>();

        if (_libraryList == nullptr) {
            _libraryList = getConfiguredLibraries(true); // Include disabled
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

std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> MaterialManagerLocal::getLibraries()
{
    if (_libraryList == nullptr) {
        initLibraries();
    }
    return _libraryList;
}

std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> MaterialManagerLocal::getMaterialLibraries()
{
    if (_libraryList == nullptr) {
        initLibraries();
    }
    return _libraryList;
}

std::shared_ptr<MaterialLibrary> MaterialManagerLocal::getLibrary(const QString& name) const
{
    for (auto& library : *_libraryList) {
        if (library->isLocal() && library->isName(name)) {
            return library;
        }
    }

    throw LibraryNotFound();
}

void MaterialManagerLocal::createLibrary(
    const QString& libraryName,
    const QString& directory,
    const QString& iconPath,
    bool readOnly
)
{
    QDir dir;
    if (!dir.exists(directory)) {
        if (!dir.mkpath(directory)) {
            throw CreationError("Unable to create library path");
        }
    }

    auto path = Library::cleanPath(directory);
    auto library = std::make_shared<MaterialLibraryLocal>(libraryName, path, iconPath, readOnly);
    _libraryList->push_back(library);

    // Persist
    std::string libRoot("User parameter:BaseApp/Preferences/Mod/Material/Resources/Local/");
    libRoot += libraryName.toStdString();

    auto newParam = App::GetApplication().GetParameterGroupByPath(libRoot.c_str());
    newParam->SetASCII("Directory", path.toStdString().c_str());
    newParam->SetASCII("IconPath", iconPath.toStdString().c_str());
    newParam->SetBool("ReadOnly", readOnly);
    newParam->SetBool("Disabled", false);
}

void MaterialManagerLocal::renameLibrary(const QString& libraryName, const QString& newName)
{
    for (auto& library : *_libraryList) {
        if (library->isLocal() && library->isName(libraryName)) {
            auto materialLibrary
                = reinterpret_cast<const std::shared_ptr<Materials::MaterialLibraryLocal>&>(library);
            materialLibrary->setName(newName);
            return;
        }
    }

    throw LibraryNotFound();
}

void MaterialManagerLocal::changeIcon(const QString& libraryName, const QByteArray& icon)
{
    for (auto& library : *_libraryList) {
        if (library->isLocal() && library->isName(libraryName)) {
            auto materialLibrary
                = reinterpret_cast<const std::shared_ptr<Materials::MaterialLibraryLocal>&>(library);
            materialLibrary->setIcon(icon);
            return;
        }
    }

    throw LibraryNotFound();
}

void MaterialManagerLocal::removeLibrary(const QString& libraryName)
{
    for (auto& library : *_libraryList) {
        if (library->isLocal() && library->isName(libraryName)) {
            _libraryList->remove(library);

            // Persist
            ParameterGrp::handle param = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/Mod/Material/Resources/Local"
            );
            param->RemoveGrp(libraryName.toStdString().c_str());

            // At this point we should rebuild the material map
            for (auto& it : *_materialMap) {
                if (it.second->getLibrary() == library) {
                    _materialMap->erase(it.first);
                }
            }
            return;
        }
    }

    throw LibraryNotFound();
}

std::shared_ptr<std::vector<LibraryObject>> MaterialManagerLocal::libraryMaterials(
    const QString& libraryName
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
    const QString& libraryName,
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

void MaterialManagerLocal::setDisabled(const QString& libraryName, bool disabled)
{
    ParameterGrp::handle param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Resources/Local"
    );
    auto groups = param->GetGroups();
    for (auto group : groups) {
        if (QString::fromStdString(group->GetGroupName()) == libraryName) {
            group->SetBool("Disabled", disabled);
            return;
        }
    }
    param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Resources/Modules"
    );
    groups = param->GetGroups();
    for (auto group : groups) {
        if (QString::fromStdString(group->GetGroupName()) == libraryName) {
            group->SetBool("Disabled", disabled);
            return;
        }
    }
    throw LibraryNotFound();
}

bool MaterialManagerLocal::isDisabled(const QString& libraryName)
{
    ParameterGrp::handle param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Resources/Local"
    );
    auto groups = param->GetGroups();
    for (auto group : groups) {
        if (QString::fromStdString(group->GetGroupName()) == libraryName) {
            return group->GetBool("Disabled", false);
        }
    }
    param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Resources/Modules"
    );
    groups = param->GetGroups();
    for (auto group : groups) {
        if (QString::fromStdString(group->GetGroupName()) == libraryName) {
            return group->GetBool("Disabled", false);
        }
    }
    throw LibraryNotFound();
}

bool MaterialManagerLocal::exists(const QString& libraryName)
{
    for (auto& library : *_libraryList) {
        if (library->isLocal() && library->isName(libraryName)) {
            return true;
        }
    }

    return false;
}

//=====
//
// Folder management
//
//=====

std::shared_ptr<std::list<QString>> MaterialManagerLocal::getMaterialFolders(
    const std::shared_ptr<MaterialLibraryLocal>& library
) const
{
    // auto materialLibrary =
    //     reinterpret_cast<const std::shared_ptr<Materials::MaterialLibraryLocal>&>(library);
    return MaterialLoader::getMaterialFolders(*library);
}

void MaterialManagerLocal::createFolder(
    const std::shared_ptr<MaterialLibraryLocal>& library,
    const QString& path
)
{
    library->createFolder(path);
}

void MaterialManagerLocal::renameFolder(
    const std::shared_ptr<MaterialLibraryLocal>& library,
    const QString& oldPath,
    const QString& newPath
)
{
    library->renameFolder(oldPath, newPath);
}

void MaterialManagerLocal::deleteRecursive(
    const std::shared_ptr<MaterialLibraryLocal>& library,
    const QString& path
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

std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> MaterialManagerLocal::getLocalMaterials() const
{
    return _materialMap;
}

std::shared_ptr<Material> MaterialManagerLocal::getMaterial(const QString& uuid) const
{
    try {
        return _materialMap->at(uuid);
    }
    catch (std::out_of_range&) {
        throw MaterialNotFound();
    }
}

std::shared_ptr<Material> MaterialManagerLocal::getMaterialByPath(const QString& path) const
{
    QString cleanPath = Library::cleanPath(path);

    for (auto& library : *_libraryList) {
        if (library->isLocal() && !library->isDisabled()) {
            auto materialLibrary
                = reinterpret_cast<const std::shared_ptr<Materials::MaterialLibraryLocal>&>(library);
            if (cleanPath.startsWith(materialLibrary->getDirectory())) {
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
    const QString& path,
    const QString& lib
) const
{
    auto library = getLibrary(lib);  // May throw LibraryNotFound
    if (library->isLocal()) {
        auto materialLibrary
            = reinterpret_cast<const std::shared_ptr<Materials::MaterialLibraryLocal>&>(library);
        return materialLibrary->getMaterialByPath(path);  // May throw MaterialNotFound
    }

    throw LibraryNotFound();
}

bool MaterialManagerLocal::exists(const QString& uuid) const
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

bool MaterialManagerLocal::exists(const MaterialLibrary& library, const QString& uuid) const
{
    try {
        auto material = getMaterial(uuid);
        if (material && material->getLibrary()->isLocal()) {
            auto materialLibrary
                = reinterpret_cast<const std::shared_ptr<Materials::MaterialLibraryLocal>&>(
                    *(material->getLibrary())
                );
            return (*materialLibrary == library);
        }
    }
    catch (const MaterialNotFound&) {
    }

    return false;
}

void MaterialManagerLocal::remove(const QString& uuid)
{
    _materialMap->erase(uuid);
}

void MaterialManagerLocal::saveMaterial(
    const std::shared_ptr<MaterialLibraryLocal>& library,
    const std::shared_ptr<Material>& material,
    const QString& path,
    bool overwrite,
    bool saveAsCopy,
    bool saveInherited
) const
{
    if (library->isLocal()) {
        auto newMaterial = library->saveMaterial(material, path, overwrite, saveAsCopy, saveInherited);
        (*_materialMap)[newMaterial->getUUID()] = newMaterial;
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

bool MaterialManagerLocal::isMaterial(const QFileInfo& file) const
{
    if (!file.isFile()) {
        return false;
    }
    // check file extension
    if (file.suffix() == QStringLiteral("FCMat")) {
        return true;
    }
    return false;
}

std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> MaterialManagerLocal::materialsWithModel(
    const QString& uuid
) const
{
    std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> dict
        = std::make_shared<std::map<QString, std::shared_ptr<Material>>>();

    for (auto& it : *_materialMap) {
        QString key = it.first;
        auto material = it.second;

        if (material->hasModel(uuid)) {
            (*dict)[key] = material;
        }
    }

    return dict;
}

std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> MaterialManagerLocal::materialsWithModelComplete(
    const QString& uuid
) const
{
    std::shared_ptr<std::map<QString, std::shared_ptr<Material>>> dict
        = std::make_shared<std::map<QString, std::shared_ptr<Material>>>();

    for (auto& it : *_materialMap) {
        QString key = it.first;
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

    auto localParam = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Resources/Local"
    );

    // Ensure the builtin libraries have a configuration
    if (!localParam->HasGroup("System")) {
        ModelManager::createSystemLibraryConfig();
    }
    if (!localParam->HasGroup("User")) {
        ModelManager::createUserLibraryConfig();
    }

    for (auto& group : localParam->GetGroups()) {
        // auto module = moduleParam->GetGroup(group->GetGroupName());
        auto libName = QString::fromStdString(group->GetGroupName());
        auto libDir = QString::fromStdString(group->GetASCII("Directory", ""));
        auto libIcon = QString::fromStdString(group->GetASCII("IconPath", ""));
        auto libReadOnly = group->GetBool("ReadOnly", true);
        auto libDisabled = group->GetBool("Disabled", false);

        if (libDir.length() > 0) {
            QDir dir(libDir);
            if (dir.exists()) {
                if (!libDisabled || includeDisabled) {
                    // Use the canonical path to prevent issues with symbolic links
                    auto libData = std::make_shared<MaterialLibraryLocal>(
                        libName,
                        dir.canonicalPath(),
                        libIcon,
                        libReadOnly
                    );
                    libData->setDisabled(libDisabled);
                    libraryList->push_back(libData);
                }
            }
        }
    }

    auto moduleParam = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Resources/Modules"
    );
    for (auto& group : moduleParam->GetGroups()) {
        auto moduleName = QString::fromStdString(group->GetGroupName());
        auto materialDir = QString::fromStdString(Library::cleanPath(group->GetASCII("ModuleDir", "")));
        auto materialIcon = QString::fromStdString(group->GetASCII("ModuleIcon", ""));
        auto materialReadOnly = group->GetBool("ModuleReadOnly", true);
        auto materialDisabled = group->GetBool("ModuleMaterialDisabled", false);

        if (materialDir.length() > 0) {
            QDir dir(materialDir);
            if (dir.exists()) {
                if (!materialDisabled || includeDisabled) {
                    auto libData = std::make_shared<MaterialLibraryLocal>(
                        moduleName,
                        dir.canonicalPath(),
                        materialIcon,
                        materialReadOnly
                    );
                    libData->setModule(true);
                    libData->setDisabled(materialDisabled);
                    libraryList->push_back(libData);
                }
            }
        }
    }

    return libraryList;
}

void MaterialManagerLocal::convertConfiguration()
{
    auto libraryList = std::make_shared<std::list<std::shared_ptr<MaterialLibrary>>>();

    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Resources"
    );
    if (param->HasGroup("Local")) {
        Base::Console().log("Material configuration conversion already completed\n");
        return;
    }
    Base::Console().log("Material configuration conversion\n");
    bool useBuiltInMaterials = param->GetBool("UseBuiltInMaterials", true);
    bool useMatFromModules = param->GetBool("UseMaterialsFromWorkbenches", true);
    bool useMatFromConfigDir = param->GetBool("UseMaterialsFromConfigDir", true);
    bool useMatFromCustomDir = param->GetBool("UseMaterialsFromCustomDir", true);

    // Write the new configuration
    std::string materialRoot("User parameter:BaseApp/Preferences/Mod/Material/Resources/Local");
    auto newParam = App::GetApplication().GetParameterGroupByPath(materialRoot.c_str());
    newParam->Clear();

    // Built in materials
    std::string paramPath = materialRoot + "/System";
    newParam = App::GetApplication().GetParameterGroupByPath(paramPath.c_str());
    newParam->SetASCII(
        "Directory",
        Library::cleanPath(App::Application::getResourceDir() + "/Mod/Material/Resources/Materials")
            .c_str()
    );
    newParam->SetASCII(
        "ModelDirectory",
        Library::cleanPath(App::Application::getResourceDir() + "/Mod/Material/Resources/Models").c_str()
    );
    newParam->SetASCII("IconPath", ":/icons/freecad.svg");
    newParam->SetBool("ReadOnly", true);
    newParam->SetBool("Disabled", !useBuiltInMaterials);

    // User material directory
    paramPath = materialRoot + "/User";
    newParam = App::GetApplication().GetParameterGroupByPath(paramPath.c_str());
    newParam->SetASCII(
        "Directory",
        Library::cleanPath(App::Application::getUserAppDataDir() + "/Material").c_str()
    );
    newParam->SetASCII(
        "ModelDirectory",
        Library::cleanPath(App::Application::getUserAppDataDir() + "/Models").c_str()
    );
    newParam->SetASCII("IconPath", ":/icons/preferences-general.svg");
    newParam->SetBool("ReadOnly", false);
    newParam->SetBool("Disabled", !useMatFromConfigDir);

    // Custom materials directory
    if (useMatFromCustomDir) {
        paramPath = materialRoot + "/Custom";
        auto path = Library::cleanPath(param->GetASCII("CustomMaterialsDir", ""));
        param = App::GetApplication().GetParameterGroupByPath(paramPath.c_str());
        newParam->SetASCII("Directory", path.c_str());
        newParam->SetASCII("ModelDirectory", path.c_str());
        newParam->SetASCII("IconPath", ":/icons/preferences-general.svg");
        newParam->SetBool("ReadOnly", false);
        newParam->SetBool("Disabled", !useMatFromCustomDir);
    }

    // Remove the old parameters
    param->RemoveBool("UseBuiltInMaterials");
    param->RemoveBool("UseMaterialsFromWorkbenches");
    param->RemoveBool("UseMaterialsFromConfigDir");
    param->RemoveBool("UseMaterialsFromCustomDir");
    param->RemoveASCII("CustomMaterialsDir");
}
