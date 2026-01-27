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

#include <QDirIterator>
#include <QMutexLocker>

#include <App/Application.h>
#include <Base/Console.h>

#include "Model.h"
#include "ModelLoader.h"
#include "LibraryManager.h"

using namespace Materials;

TYPESYSTEM_SOURCE(Materials::LibraryManager, Base::BaseClass)

QMutex LibraryManager::_mutex;
bool LibraryManager::_useExternal = false;
LibraryManager* LibraryManager::_manager = nullptr;
std::shared_ptr<std::list<std::shared_ptr<ManagedLibrary>>> LibraryManager::_libraryList = nullptr;
std::shared_ptr<std::multimap<QString, std::shared_ptr<ManagedLibrary>>> LibraryManager::_libraryMap
    = nullptr;

LibraryManager::LibraryManager()
{
    _hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/ExternalInterface"
    );
    _useExternal = _hGrp->GetBool("UseExternal", false);
    _hGrp->Attach(this);
}

LibraryManager::~LibraryManager()
{
    _hGrp->Detach(this);
}

LibraryManager& LibraryManager::getManager()
{
    if (!_manager) {
        initManagers();
    }

    return *_manager;
}

void LibraryManager::initManagers()
{
    QMutexLocker locker(&_mutex);
    convertConfiguration();

    if (!_manager) {
        // Can't use smart pointers for this since the constructor is private
        _manager = new LibraryManager();
    }

    if (_libraryMap == nullptr) {
        _libraryMap = std::make_shared<std::multimap<QString, std::shared_ptr<ManagedLibrary>>>();
    }

    if (_libraryList == nullptr) {
        _libraryList = getConfiguredLibraries(true);  // Include disabled
    }

    _libraryMap->clear();
    for (auto library : *_libraryList) {
        _libraryMap->insert({library->getLibraryName(), library});
    }
}

void LibraryManager::OnChange(ParameterGrp::SubjectType& rCaller, ParameterGrp::MessageType Reason)
{
    const ParameterGrp& rGrp = static_cast<ParameterGrp&>(rCaller);
    if (strcmp(Reason, "UseExternal") == 0) {
        Base::Console().log("Use external changed\n");
        _useExternal = rGrp.GetBool("UseExternal", false);
    }
}

void LibraryManager::cleanup()
{
}

void LibraryManager::refresh()
{
}

//=====
//
// Library management
//
//=====

void LibraryManager::setUseExternal(bool useExternal)
{
    ParameterGrp::handle paramExternal = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/ExternalInterface"
    );

    paramExternal->SetBool("UseExternal", useExternal);
}

std::shared_ptr<std::list<std::shared_ptr<ManagedLibrary>>> LibraryManager::getLibraries(
    bool includeDisabled
)
{
    //     // External libraries take precedence over local libraries
    //     auto libMap = std::map<QString, std::shared_ptr<ModelLibrary>>();
    // #if defined(BUILD_MATERIAL_EXTERNAL)
    //     if (_useExternal) {
    //         auto remoteLibraries = _externalManager->getLibraries();
    //         for (auto& remote : *remoteLibraries) {
    //             if (includeDisabled || !remote->isDisabled()) {
    //                 libMap.try_emplace(remote->getName(), remote);
    //             }
    //         }
    //     }
    // #endif
    //     auto localLibraries = _localManager->getLibraries();
    //     for (auto& local : *localLibraries) {
    //         if (includeDisabled || !local->isDisabled()) {
    //             libMap.try_emplace(local->getName(), local);
    //         }
    //     }

    // Consolidate into a single list
    auto libraries = std::make_shared<std::list<std::shared_ptr<ManagedLibrary>>>();
    for (auto libEntry : *_libraryList) {
        libraries->push_back(libEntry);
    }

    return libraries;
}

std::shared_ptr<std::list<std::shared_ptr<ModelLibrary>>> LibraryManager::getModelLibraries(
    bool includeDisabled
)
{
    auto libraries = std::make_shared<std::list<std::shared_ptr<ModelLibrary>>>();
    for (auto libEntry : *_libraryList) {
        if (!libEntry->getModelDirectory().isEmpty()) {
            libraries->push_back(std::make_shared<ModelLibrary>(libEntry));
        }
    }

    return libraries;
}

std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> LibraryManager::getMaterialLibraries(
    bool includeDisabled
)
{
    auto libraries = std::make_shared<std::list<std::shared_ptr<MaterialLibrary>>>();
    for (auto libEntry : *_libraryList) {
        if (!libEntry->getMaterialDirectory().isEmpty()) {
            libraries->push_back(std::make_shared<MaterialLibrary>(libEntry));
        }
    }

    return libraries;
}

std::shared_ptr<std::list<std::shared_ptr<ManagedLibrary>>> LibraryManager::getLocalLibraries(
    bool includeDisabled
)
{
    // return _localManager->getLibraries();
    return getLibraries();
}

std::shared_ptr<std::list<std::shared_ptr<ModelLibraryLocal>>> LibraryManager::getLocalModelLibraries(
    bool includeDisabled
)
{
    auto libraries = std::make_shared<std::list<std::shared_ptr<ModelLibraryLocal>>>();
    for (auto libEntry : *_libraryList) {
        if (!libEntry->getModelDirectory().isEmpty()) {
            libraries->push_back(std::make_shared<ModelLibraryLocal>(libEntry));
        }
    }

    return libraries;
}

std::shared_ptr<std::list<std::shared_ptr<MaterialLibraryLocal>>> LibraryManager::getLocalMaterialLibraries(
    bool includeDisabled
)
{
    auto libraries = std::make_shared<std::list<std::shared_ptr<MaterialLibraryLocal>>>();
    for (auto libEntry : *_libraryList) {
        if (!libEntry->getMaterialDirectory().isEmpty()) {
            libraries->push_back(std::make_shared<MaterialLibraryLocal>(libEntry));
        }
    }

    return libraries;
}

std::shared_ptr<ModelLibrary> LibraryManager::getModelLibrary(const QString& repositoryName, const QString& name) const
{
    return std::make_shared<ModelLibrary>();
}

std::shared_ptr<MaterialLibrary> LibraryManager::getMaterialLibrary(const QString& repositoryName, const QString& name) const
{
    return std::make_shared<MaterialLibrary>();
}

void LibraryManager::createRemoteLibrary(
    [[maybe_unused]] const QString& repositoryName,
    [[maybe_unused]] const QString& libraryName,
    [[maybe_unused]] const QString& iconPath,
    [[maybe_unused]] bool readOnly
)
{
// #if defined(BUILD_MATERIAL_EXTERNAL)
//     auto icon = Materials::Library::getIcon(iconPath);
//     _externalManager->createLibrary(libraryName, icon, readOnly);
// #endif
}

void LibraryManager::createLocalLibrary(
    const QString& repositoryName,
    const QString& libraryName,
    const QString& materialDirectory,
    const QString& modelDirectory,
    const QString& icon,
    bool readOnly
)
{
    // _localManager->createLibrary(libraryName, directory, icon, readOnly);
}

void LibraryManager::renameLibrary(const QString& repositoryName,const QString& libraryName, const QString& newName)
{
    // _localManager->renameLibrary(libraryName, newName);
}

void LibraryManager::changeIcon(const QString& repositoryName, const QString& libraryName, const QString& icon)
{
    // _localManager->changeIcon(libraryName, icon);
}

void LibraryManager::removeLibrary(const QString& repositoryName, const QString& libraryName)
{
    // _localManager->removeLibrary(libraryName);
}

bool LibraryManager::isLocalLibrary(const QString& repositoryName, [[maybe_unused]] const QString& libraryName)
{
    return true;
}

void LibraryManager::setDisabled(const QString& repositoryName, Library& library, bool disabled)
{
}

void LibraryManager::createSystemLibraryConfig()
{
    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Resources/Local"
    );
    if (!param->HasGroup("System")) {
        Base::Console().log("No System library defined\n");
        auto path = Library::cleanPath(
            App::Application::getResourceDir() + "/Mod/Material/Resources"
        );
        auto library = param->GetGroup("System");

        QDir resourceDir;
        auto resourcePath = Library::cleanPath(path + "/Materials");
        resourceDir.mkpath(QString::fromStdString(resourcePath));
        library->SetASCII("Directory", resourcePath);
        resourcePath = Library::cleanPath(path + "/Models");
        resourceDir.mkpath(QString::fromStdString(resourcePath));
        library->SetASCII("ModelDirectory", resourcePath);

        library->SetASCII("IconPath", ":/icons/freecad.svg");
        library->SetBool("ReadOnly", false);
        library->SetBool("Disabled", false);
    }
}

void LibraryManager::createUserLibraryConfig()
{
    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Resources/Local"
    );
    if (!param->HasGroup("User")) {
        Base::Console().log("No User library defined\n");
        auto path = Library::cleanPath(App::Application::getUserAppDataDir());
        auto library = param->GetGroup("User");

        QDir resourceDir;
        auto resourcePath = Library::cleanPath(path + "/Material");
        resourceDir.mkpath(QString::fromStdString(resourcePath));
        library->SetASCII("Directory", resourcePath);
        resourcePath = Library::cleanPath(path + "/Models");
        resourceDir.mkpath(QString::fromStdString(resourcePath));
        library->SetASCII("ModelDirectory", resourcePath);

        library->SetASCII("IconPath", ":/icons/preferences-general.svg");
        library->SetBool("ReadOnly", false);
        library->SetBool("Disabled", false);
    }
}

std::shared_ptr<std::list<std::shared_ptr<ManagedLibrary>>> LibraryManager::getConfiguredLibraries(
    bool includeDisabled
)
{
    auto libraryList = std::make_shared<std::list<std::shared_ptr<ManagedLibrary>>>();

    auto localParam = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Resources/Local"
    );

    // Ensure the builtin libraries have a configuration
    if (!localParam->HasGroup("System")) {
        createSystemLibraryConfig();
    }
    if (!localParam->HasGroup("User")) {
        createUserLibraryConfig();
    }

    for (auto& group : localParam->GetGroups()) {
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
                    auto libData = std::make_shared<ManagedLibrary>(
                        libName,
                        dir.canonicalPath(),
                        libIcon,
                        libReadOnly
                    );
                    libData->setDisabled(libDisabled);
                    libraryList->push_back(libData);
                }
            }
            else {
                std::string name = libDir.toStdString();
                std::string missing = libName.toStdString();
                Base::Console()
                    .log("Missing dir '%s' for library '%s'\n", missing.c_str(), name.c_str());
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
                    auto libData = std::make_shared<ManagedLibrary>(
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

void LibraryManager::convertConfiguration()
{
    auto libraryList = std::make_shared<std::list<std::shared_ptr<MaterialLibrary>>>();

    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Resources"
    );
    if (param->HasGroup("Local")) {
        // Material configuration conversion already completed
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

    // Module directories
    newParam = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Resources/Modules"
    );
    auto groups = newParam->GetGroups();
    for (auto group : groups) {
        group->SetBool("ModuleMaterialDisabled", useMatFromModules);
    }

    // Remove the old parameters
    param->RemoveBool("UseBuiltInMaterials");
    param->RemoveBool("UseMaterialsFromWorkbenches");
    param->RemoveBool("UseMaterialsFromConfigDir");
    param->RemoveBool("UseMaterialsFromCustomDir");
    param->RemoveASCII("CustomMaterialsDir");
}
