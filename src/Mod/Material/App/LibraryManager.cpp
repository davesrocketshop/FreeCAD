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

#include "ExternalManager.h"
#include "Model.h"
#include "ModelLoader.h"
#include "LibraryManager.h"

using namespace Materials;

TYPESYSTEM_SOURCE(Materials::LibraryManager, Base::BaseClass)

QMutex LibraryManager::_mutex;
bool LibraryManager::_useExternal = false;
LibraryManager* LibraryManager::_manager = nullptr;
std::shared_ptr<std::list<std::shared_ptr<ManagedLibrary>>> LibraryManager::_libraryList = nullptr;
std::shared_ptr<std::multimap<std::string, std::shared_ptr<ManagedLibrary>>> LibraryManager::_libraryMap
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
        _libraryMap = std::make_shared<std::multimap<std::string, std::shared_ptr<ManagedLibrary>>>();
    }

    if (_libraryList == nullptr) {
        _libraryList = std::make_shared<std::list<std::shared_ptr<ManagedLibrary>>>();
    }

    auto configured = getConfiguredLibraries(true); // Include disabled
#if defined(BUILD_MATERIAL_EXTERNAL)
    if (_useExternal) {
        auto externalList = externalManager()->libraries();
        // External libraries have priority so are added first
        for (auto library : *externalList) {
            _libraryList->push_back(library);
        }
    }
#endif
    for (auto library : *configured) {
        _libraryList->push_back(library);
    }

    _manager->updateLibraryMap();
}

void LibraryManager::updateLibraryMap()
{
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

std::shared_ptr<std::vector<std::shared_ptr<ManagedLibrary>>> LibraryManager::getLibraries(
    bool includeDisabled
)
{
    auto libraries = std::make_shared<std::vector<std::shared_ptr<ManagedLibrary>>>();
    for (auto libEntry : *_libraryMap) {
        if (includeDisabled || !libEntry.second->isDisabled()) {
            libraries->push_back(libEntry.second);
        }
    }

    return libraries;
}

std::shared_ptr<std::vector<std::shared_ptr<ModelLibrary>>> LibraryManager::getModelLibraries(
    bool includeDisabled
)
{
    auto libraries = std::make_shared<std::vector<std::shared_ptr<ModelLibrary>>>();
    for (auto libEntry : *_libraryMap) {
        if (!libEntry.second->getModelDirectory().empty()) {
            if (includeDisabled || !libEntry.second->isDisabled()) {
                libraries->push_back(std::make_shared<ModelLibrary>(libEntry.second));
            }
        }
    }

    return libraries;
}

std::shared_ptr<std::vector<std::shared_ptr<MaterialLibrary>>> LibraryManager::getMaterialLibraries(
    bool includeDisabled
)
{
    auto libraries = std::make_shared<std::vector<std::shared_ptr<MaterialLibrary>>>();
    for (auto libEntry : *_libraryMap) {
        if (!libEntry.second->getMaterialDirectory().empty()) {
            if (includeDisabled || !libEntry.second->isDisabled()) {
                libraries->push_back(std::make_shared<MaterialLibrary>(libEntry.second));
            }
        }
    }

    return libraries;
}

std::shared_ptr<std::vector<std::shared_ptr<ManagedLibrary>>> LibraryManager::getLocalLibraries(
    bool includeDisabled
)
{
    auto libraries = std::make_shared<std::vector<std::shared_ptr<ManagedLibrary>>>();
    for (auto libEntry : *_libraryList) {
        if (libEntry->isLocal()) {
            if (includeDisabled || !libEntry->isDisabled()) {
                libraries->push_back(libEntry);
            }
        }
    }

    return libraries;
}

std::shared_ptr<std::vector<std::shared_ptr<ModelLibrary>>> LibraryManager::getLocalModelLibraries(
    bool includeDisabled
)
{
    auto libraries = std::make_shared<std::vector<std::shared_ptr<ModelLibrary>>>();
    for (auto libEntry : *_libraryList) {
        if (libEntry->isLocal()) {
            if (includeDisabled || !libEntry->isDisabled()) {
                if (!libEntry->getModelDirectory().empty()) {
                    libraries->push_back(std::make_shared<ModelLibraryLocal>(libEntry));
                }
            }
        }
    }

    return libraries;
}

std::shared_ptr<std::vector<std::shared_ptr<MaterialLibrary>>> LibraryManager::getLocalMaterialLibraries(
    bool includeDisabled
)
{
    auto libraries = std::make_shared<std::vector<std::shared_ptr<MaterialLibrary>>>();
    for (auto libEntry : *_libraryList) {
        if (libEntry->isLocal()) {
            if (includeDisabled || !libEntry->isDisabled()) {
                if (!libEntry->getMaterialDirectory().empty()) {
                    libraries->push_back(std::make_shared<MaterialLibraryLocal>(libEntry));
                }
            }
        }
    }

    return libraries;
}

std::shared_ptr<ManagedLibrary> LibraryManager::getLibrary(const std::string& repositoryName, const std::string& name) const
{
    auto range = _libraryMap->equal_range(name);
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second->isRepositoryName(repositoryName)) {
            return it->second;
        }
    }

    throw LibraryNotFound();
}

std::shared_ptr<ModelLibrary> LibraryManager::getModelLibrary(
    const std::string& repositoryName,
    const std::string& name
) const
{
    auto library = getLibrary(repositoryName, name);
    if (library->isRemote() || !library->getModelDirectory().empty()) {
        return std::make_shared<ModelLibrary>(library);
    }

    throw LibraryNotFound();
}

std::shared_ptr<MaterialLibrary> LibraryManager::getMaterialLibrary(
    const std::string& repositoryName,
    const std::string& name
) const
{
    auto library = getLibrary(repositoryName, name);
    if (!library->getMaterialDirectory().empty()) {
        return std::make_shared<MaterialLibrary>(library);
    }

    throw LibraryNotFound();
}

void LibraryManager::createRemoteLibrary(
    [[maybe_unused]] const std::string& repositoryName,
    [[maybe_unused]] const std::string& libraryName,
    [[maybe_unused]] const std::string& iconPath,
    [[maybe_unused]] bool readOnly
)
{
#if defined(BUILD_MATERIAL_EXTERNAL)
    if (_useExternal) {
        auto icon = Materials::ManagedLibrary::getIcon(iconPath);
        externalManager()->createLibrary(libraryName, icon, readOnly);
    }
    else {
        throw CreationError("External interface is not enabled");
    }
#else
    throw CreationError("External interface is not enabled");
#endif
}

std::shared_ptr<MaterialLibrary> LibraryManager::createLocalLibrary(
    const std::string& libraryName,
    const std::string& materialDirectory,
    const std::string& modelDirectory,
    const std::string& iconPath,
    bool readOnly
)
{
    try {
        auto library = getLibrary("Local", libraryName);
        throw CreationError("Library already exists");
    }
    catch (const LibraryNotFound) {}

    QDir dir;
    if (!dir.exists(QString::fromStdString(materialDirectory))) {
        if (!dir.mkpath(QString::fromStdString(materialDirectory))) {
            throw CreationError("Unable to create library path");
        }
    }
    if (!modelDirectory.empty()) {
        if (!dir.exists(QString::fromStdString(modelDirectory))) {
            if (!dir.mkpath(QString::fromStdString(modelDirectory))) {
                throw CreationError("Unable to create library model path");
            }
        }
    }

    auto path = Library::cleanPath(materialDirectory);
    auto library = std::make_shared<MaterialLibraryLocal>(
        libraryName,
        path,
        iconPath,
        readOnly
    );
    auto modelPath = Library::cleanPath(modelDirectory);
    library->proxy()->setModelDirectory(modelPath);

    // Persist
    std::string libRoot = getResourceRootLocal();
    libRoot += libraryName;

    auto newParam = App::GetApplication().GetParameterGroupByPath(libRoot.c_str());
    newParam->SetASCII("Directory", path.c_str());
    if (!modelDirectory.empty()) {
        newParam->SetASCII("ModelDirectory", modelPath.c_str());
    }
    newParam->SetASCII("IconPath", iconPath.c_str());
    newParam->SetBool("ReadOnly", readOnly);
    newParam->SetBool("Disabled", false);

    _libraryList->push_back(library->proxy());
    _manager->updateLibraryMap();

    return library;
}

void LibraryManager::renameLibrary(const std::string& repositoryName,const std::string& libraryName, const std::string& newName)
{
    auto library = getLibrary(repositoryName, libraryName);
    if (library->isReadOnly()) {
        throw RenameError("Unable to rename read only library");
    }
    if (library->isLocal()) {
        renameLibraryLocal(library, newName);
    }
    else {
        renameLibraryRemote(library, newName);
    }
    
    library->setLibraryName(newName);
    updateLibraryMap();
}

void LibraryManager::renameLibraryLocal(
    const std::shared_ptr<ManagedLibrary>& library,
    const std::string& newName
)
{
    // Update the config entries
    std::string libRoot = getResourceRoot(library);
    auto param = App::GetApplication().GetParameterGroupByPath(libRoot.c_str());
    if (!param->HasGroup(library->getLibraryName().c_str())) {
        throw LibraryNotFound();
    }
    if (param->HasGroup(newName.c_str())) {
        throw RenameError("Another library with that name already exists");
    }
    param->RenameGrp(library->getLibraryName().c_str(), newName.c_str());
    if (param->HasGroup(library->getLibraryName().c_str())) {
        throw RenameError("Old library exists after rename");
    }
    if (!param->HasGroup(newName.c_str())) {
        throw RenameError("Renamed library missing after rename");
    }
}

void LibraryManager::renameLibraryRemote(
    [[maybe_unused]] const std::shared_ptr<ManagedLibrary>& library,
    [[maybe_unused]] const std::string& newName
)
{
#if defined(BUILD_MATERIAL_EXTERNAL)
    externalManager()->renameLibrary(library->getLibraryName(), newName);
#else
    throw RenameError("External interface is not enabled");
#endif
}

void LibraryManager::changeIcon(
    const std::string& repositoryName,
    const std::string& libraryName,
    const std::string& iconPath
)
{
    auto library = getLibrary(repositoryName, libraryName);
    if (library->isReadOnly()) {
        throw UpdateError("Unable to change the icon for a read only library");
    }
    if (library->isLocal()) {
        changeIconLocal(library, iconPath);
    }
    else {
        changeIconRemote(library, iconPath);
    }
}

void LibraryManager::changeIconLocal(
    const std::shared_ptr<ManagedLibrary>& library,
    const std::string& iconPath
)
{
    // Update the config entries
    std::string libRoot = getResourceRoot(library);
    auto param = App::GetApplication().GetParameterGroupByPath(libRoot.c_str());
    if (!param->HasGroup(library->getLibraryName().c_str())) {
        throw LibraryNotFound();
    }
    auto libParam = param->GetGroup(library->getLibraryName().c_str());
    libParam->SetASCII("IconPath", iconPath.c_str());
    library->setIcon(iconPath);
}

void LibraryManager::changeIconRemote(
    [[maybe_unused]] const std::shared_ptr<ManagedLibrary>& library,
    [[maybe_unused]] const std::string& iconPath
)
{
#if defined(BUILD_MATERIAL_EXTERNAL)
    auto icon = Materials::ManagedLibrary::getIcon(iconPath);
    externalManager()->changeIcon(library->getLibraryName(), icon);
#else
    throw UpdateError("External interface is not enabled");
#endif
}

void LibraryManager::removeLibrary(const std::string& repositoryName, const std::string& libraryName)
{
    for (auto& library : *_libraryList) {
        if (library->isLibraryName(libraryName) && library->isRepositoryName(repositoryName)) {
            if (library->isReadOnly()) {
                throw DeleteError("Unable to remove a read only library");
            }
            if (library->isLocal()) {
                removeLibraryLocal(library);
            }
            else {
                removeLibraryRemote(library);
            }
            _libraryList->remove(library);
            updateLibraryMap();
            return;
        }
    }

    throw LibraryNotFound();
}

void LibraryManager::removeLibraryLocal(const std::shared_ptr<ManagedLibrary>& library)
{
    if (library->isModule()) {
        throw DeleteError("Unable to remove a module defined library");
    }
    ParameterGrp::handle param = App::GetApplication().GetParameterGroupByPath(getResourceRoot(library));
    if (!param->HasGroup(library->getLibraryName().c_str())) {
        // Nothing to do
        return;
    }
    param->RemoveGrp(library->getLibraryName().c_str());
}

void LibraryManager::removeLibraryRemote([[maybe_unused]] const std::shared_ptr<ManagedLibrary>& library)
{
#if defined(BUILD_MATERIAL_EXTERNAL)
    externalManager()->removeLibrary(library->getLibraryName());
#else
    throw DeleteError("External interface is not enabled");
#endif
}

bool LibraryManager::isLocalLibrary(const std::string& repositoryName, const std::string& libraryName)
{
    try {
        auto library = getLibrary(repositoryName, libraryName);
        if (library->isLocal()) {
            return true;
        }
    }
    catch (const LibraryNotFound&) {}

    return false;
}

void LibraryManager::setDisabled(const std::string& repositoryName, const std::string& libraryName, bool disabled)
{
    auto library = getLibrary(repositoryName, libraryName);
    setDisabled(library, disabled);
}

void LibraryManager::setDisabled(Library& library, bool disabled)
{
    setDisabled(library.proxy(), disabled);
}

void LibraryManager::setDisabled(const std::shared_ptr<ManagedLibrary>& library, bool disabled)
{
    std::string libRoot = getResourceRoot(library);
    libRoot += library->getLibraryName();
    ParameterGrp::handle param = App::GetApplication().GetParameterGroupByPath(libRoot.c_str());

    param->SetBool("Disabled", disabled);
    library->setDisabled(disabled);
}

bool LibraryManager::isDisabled(const std::string& repositoryName, const std::string& libraryName) const
{
    auto library = getLibrary(repositoryName, libraryName);
    return isDisabled(library);
}

bool LibraryManager::isDisabled(const Library& library) const
{
    return isDisabled(library.proxy());
}

bool LibraryManager::isDisabled(const std::shared_ptr<ManagedLibrary>& library) const
{
    std::string libRoot = getResourceRoot(library);
    libRoot += library->getLibraryName();
    ParameterGrp::handle param = App::GetApplication().GetParameterGroupByPath(libRoot.c_str());

    return param->GetBool("Disabled", false);
}

void LibraryManager::createSystemLibraryConfig()
{
    auto param = App::GetApplication().GetParameterGroupByPath(getResourceRootLocal());
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
    auto param = App::GetApplication().GetParameterGroupByPath(getResourceRootLocal());
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

std::shared_ptr<std::vector<std::shared_ptr<ManagedLibrary>>> LibraryManager::getConfiguredLibraries(
    bool includeDisabled
)
{
    auto libraryList = std::make_shared<std::vector<std::shared_ptr<ManagedLibrary>>>();

    auto localParam = App::GetApplication().GetParameterGroupByPath(getResourceRootLocal());

    // Ensure the builtin libraries have a configuration
    if (!localParam->HasGroup("System")) {
        createSystemLibraryConfig();
    }
    if (!localParam->HasGroup("User")) {
        createUserLibraryConfig();
    }

    for (auto& group : localParam->GetGroups()) {
        auto libName = group->GetGroupName();
        auto libDir = group->GetASCII("Directory", "");
        auto libModels = group->GetASCII("ModelDirectory", "");
        auto libIcon = group->GetASCII("IconPath", "");
        auto libReadOnly = group->GetBool("ReadOnly", true);
        auto libDisabled = group->GetBool("Disabled", false);

        if (libDir.length() > 0) {
            QDir dir(QString::fromStdString(libDir));
            if (dir.exists()) {
                if (!libDisabled || includeDisabled) {
                    // Use the canonical path to prevent issues with symbolic links
                    auto libData = std::make_shared<ManagedLibrary>(
                        libName,
                        dir.canonicalPath().toStdString(),
                        libIcon,
                        libReadOnly
                    );
                    libData->setLocal(true);
                    libData->setDisabled(libDisabled);
                    libData->setModelDirectory(libModels);
                    libraryList->push_back(libData);
                }
            }
            else {
                std::string name = libDir;
                std::string missing = libName;
                Base::Console()
                    .log("Missing dir '%s' for library '%s'\n", missing.c_str(), name.c_str());
            }
        }
    }

    auto moduleParam = App::GetApplication().GetParameterGroupByPath(getResourceRootModules());
    for (auto& group : moduleParam->GetGroups()) {
        auto moduleName = group->GetGroupName();
        auto materialDir = Library::cleanPath(group->GetASCII("ModuleDir", ""));
        auto moduleDir = group->GetASCII("ModuleModelDir", "");
        auto materialIcon = group->GetASCII("ModuleIcon", "");
        auto materialReadOnly = group->GetBool("ModuleReadOnly", true);
        auto materialDisabled = group->GetBool("ModuleMaterialDisabled", false);

        if (materialDir.length() > 0) {
            QDir dir(QString::fromStdString(materialDir));
            if (dir.exists()) {
                if (!materialDisabled || includeDisabled) {
                    auto libData = std::make_shared<ManagedLibrary>(
                        moduleName,
                        dir.canonicalPath().toStdString(),
                        materialIcon,
                        materialReadOnly
                    );
                    libData->setLocal(true);
                    libData->setModule(true);
                    libData->setDisabled(materialDisabled);
                    libData->setModelDirectory(moduleDir);
                    libraryList->push_back(libData);
                }
            }
        }
    }

    return libraryList;
}

void LibraryManager::convertConfiguration()
{
    auto libraryList = std::make_shared<std::vector<std::shared_ptr<MaterialLibrary>>>();

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
    std::string materialRoot = getResourceRootLocal();
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
    newParam = App::GetApplication().GetParameterGroupByPath(getResourceRootModules());
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

const char* LibraryManager::getResourceRoot(const std::shared_ptr<ManagedLibrary>& library) const
{
    if (library->isLocal()) {
        if (library->isModule()) {
            return "User parameter:BaseApp/Preferences/Mod/Material/Resources/Modules/";
        }
        return "User parameter:BaseApp/Preferences/Mod/Material/Resources/Local/";
    }
    return "User parameter:BaseApp/Preferences/Mod/Material/Resources/Remote/";
}

const char* LibraryManager::getResourceRootLocal()
{
    return "User parameter:BaseApp/Preferences/Mod/Material/Resources/Local/";
}

const char* LibraryManager::getResourceRootModules()
{
    return "User parameter:BaseApp/Preferences/Mod/Material/Resources/Modules/";
}

const char* LibraryManager::getResourceRootRemote()
{
    return "User parameter:BaseApp/Preferences/Mod/Material/Resources/Remote/";
}
