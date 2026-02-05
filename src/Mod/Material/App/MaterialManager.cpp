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
#include "MaterialLoader.h"
#include "MaterialManager.h"
#if defined(BUILD_MATERIAL_EXTERNAL)
# include "MaterialManagerExternal.h"
#endif
#include "MaterialManagerLocal.h"
#include "ModelManager.h"
#include "ModelUuids.h"

#include <Base/Tools.h>


using namespace Materials;

/* TRANSLATOR Material::Materials */

TYPESYSTEM_SOURCE(Materials::MaterialManager, Base::BaseClass)

QMutex MaterialManager::_mutex;
bool MaterialManager::_useExternal = false;
MaterialManager* MaterialManager::_manager = nullptr;
std::unique_ptr<MaterialManagerLocal> MaterialManager::_localManager;
#if defined(BUILD_MATERIAL_EXTERNAL)
std::unique_ptr<MaterialManagerExternal> MaterialManager::_externalManager;
#endif

MaterialManager::MaterialManager()
{
#if defined(BUILD_MATERIAL_EXTERNAL)
    _hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/ExternalInterface"
    );
    _useExternal = _hGrp->GetBool("UseExternal", false);
    _hGrp->Attach(this);
#else
    _useExternal = false;
#endif
    LibraryManager::getManager().Attach(this);
}

MaterialManager::~MaterialManager()
{
#if defined(BUILD_MATERIAL_EXTERNAL)
    _hGrp->Detach(this);
#endif
    LibraryManager::getManager().Detach(this);
}

MaterialManager& MaterialManager::getManager()
{
    if (!_manager) {
        initManagers();
    }
    return *_manager;
}

void MaterialManager::initManagers()
{
    QMutexLocker locker(&_mutex);

    if (!_manager) {
        // Can't use smart pointers for this since the constructor is private
        _manager = new MaterialManager();
    }
    if (!_localManager) {
        _localManager = std::make_unique<MaterialManagerLocal>();
    }

#if defined(BUILD_MATERIAL_EXTERNAL)
    if (!_externalManager) {
        _externalManager = std::make_unique<MaterialManagerExternal>();
    }
#endif
}

LibraryManager& MaterialManager::libraryManager() {
    return LibraryManager::getManager();
}

void MaterialManager::OnChange(ParameterGrp::SubjectType& rCaller, ParameterGrp::MessageType Reason)
{
    const ParameterGrp& rGrp = static_cast<ParameterGrp&>(rCaller);
    if (strcmp(Reason, "UseExternal") == 0) {
        Base::Console().log("Use external changed\n");
        _useExternal = rGrp.GetBool("UseExternal", false);
    }
}

void MaterialManager::OnChange([[maybe_unused]] LibraryManager::SubjectType& manager, LibraryManager::MessageType reason)
{
    if (reason.eventType == LibraryEventType_Create) {
        Base::Console().log("New library '%s'\n", reason.library->getLibraryName().c_str());
        // _modelManager->refresh();
        refresh();
    }
}

void MaterialManager::cleanup()
{
    if (_localManager) {
        _localManager->cleanup();
    }
#if defined(BUILD_MATERIAL_EXTERNAL)
    if (_externalManager) {
        _externalManager->cleanup();
    }
#endif
}

void MaterialManager::refresh()
{
    _localManager->refresh();
}

//=====
//
// Defaults
//
//=====

std::shared_ptr<App::Material> MaterialManager::defaultAppearance()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View"
    );

    auto getColor = [hGrp](const char* parameter, Base::Color& color) {
        uint32_t packed = color.getPackedRGB();
        packed = hGrp->GetUnsigned(parameter, packed);
        color.setPackedRGB(packed);
        color.a = 1.0;  // The default color sets fully transparent, not opaque
    };
    auto intRandom = [](int min, int max) -> int {
        static std::mt19937 generator;
        std::uniform_int_distribution<int> distribution(min, max);
        return distribution(generator);
    };

    App::Material mat(App::Material::DEFAULT);
    bool randomColor = hGrp->GetBool("RandomColor", false);

    if (randomColor) {
        float red = static_cast<float>(intRandom(0, 255)) / 255.0F;
        float green = static_cast<float>(intRandom(0, 255)) / 255.0F;
        float blue = static_cast<float>(intRandom(0, 255)) / 255.0F;
        mat.diffuseColor = Base::Color(red, green, blue, 1.0);
    }
    else {
        getColor("DefaultShapeColor", mat.diffuseColor);
    }

    getColor("DefaultAmbientColor", mat.ambientColor);
    getColor("DefaultEmissiveColor", mat.emissiveColor);
    getColor("DefaultSpecularColor", mat.specularColor);

    long initialTransparency = hGrp->GetInt("DefaultShapeTransparency", 0);
    long initialShininess = hGrp->GetInt("DefaultShapeShininess", 90);
    mat.shininess = Base::fromPercent(initialShininess);
    mat.transparency = Base::fromPercent(initialTransparency);

    return std::make_shared<App::Material>(mat);
}

std::shared_ptr<Material> MaterialManager::defaultMaterial()
{
    MaterialManager manager;

    auto mat = defaultAppearance();
    auto material = getManager().getMaterial(defaultMaterialUUID());
    if (!material) {
        material = getManager().getMaterial("7f9fd73b-50c9-41d8-b7b2-575a030c1eeb");
    }
    if (material->hasAppearanceModel(ModelUUIDs::ModelUUID_Rendering_Basic)) {
        material->getAppearanceProperty(QStringLiteral("DiffuseColor"))->setColor(mat->diffuseColor);
        material->getAppearanceProperty(QStringLiteral("AmbientColor"))->setColor(mat->ambientColor);
        material->getAppearanceProperty(QStringLiteral("EmissiveColor"))->setColor(mat->emissiveColor);
        material->getAppearanceProperty(QStringLiteral("SpecularColor"))->setColor(mat->specularColor);
        material->getAppearanceProperty(QStringLiteral("Transparency"))->setFloat(mat->transparency);
        material->getAppearanceProperty(QStringLiteral("Shininess"))->setFloat(mat->shininess);
    }

    return material;
}

std::string MaterialManager::defaultMaterialUUID()
{
    // Make this a preference
    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material"
    );
    auto uuid = param->GetASCII("DefaultMaterial", "7f9fd73b-50c9-41d8-b7b2-575a030c1eeb");
    return uuid;
}

//=====
//
// Library management
//
//=====

void MaterialManager::setUseExternal(bool useExternal)
{
    ParameterGrp::handle paramExternal = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/ExternalInterface"
    );

    paramExternal->SetBool("UseExternal", useExternal);
}

std::shared_ptr<std::vector<std::shared_ptr<MaterialLibrary>>> MaterialManager::getLibraries(
    bool includeDisabled,
    bool includeMasked
)
{
    return libraryManager().getMaterialLibraries(includeDisabled);
}

std::shared_ptr<std::vector<std::shared_ptr<MaterialLibrary>>> MaterialManager::getLocalLibraries(
    bool includeDisabled
)
{
    return libraryManager().getLocalMaterialLibraries(includeDisabled);
}

#if defined(BUILD_MATERIAL_EXTERNAL)
std::shared_ptr<std::vector<std::shared_ptr<MaterialLibrary>>> MaterialManager::getRemoteLibraries(
    bool includeDisabled
)
{
    return libraryManager().getRemoteMaterialLibraries(includeDisabled);
}
#endif

std::shared_ptr<MaterialLibrary> MaterialManager::getLibrary(const std::string& name) const
{
    return libraryManager().getMaterialLibrary(name);
}

std::shared_ptr<MaterialLibrary> MaterialManager::createLibrary(
    const std::string& libraryName,
    const std::string& iconPath,
    bool readOnly
)
{
    libraryManager().createRemoteLibrary(LibraryManager::RepositoryRemote, libraryName, iconPath, readOnly);
    return libraryManager().getMaterialLibrary(LibraryManager::RepositoryRemote, libraryName);
}

std::shared_ptr<MaterialLibrary> MaterialManager::createLocalLibrary(const std::string& libraryName,
    const std::string& materialDirectory,
    const std::string& modelDirectory,
    const std::string& iconPath,
    bool readOnly
)
{
    libraryManager().createLocalLibrary(libraryName, materialDirectory, modelDirectory, iconPath, readOnly);
    return libraryManager().getMaterialLibrary(LibraryManager::RepositoryLocal, libraryName);
}

void MaterialManager::renameLibrary(const std::string& libraryName, const std::string& newName)
{
    libraryManager().renameLibrary(libraryName, newName);
}

void MaterialManager::changeIcon(const std::string& libraryName, const std::string& iconPath)
{
    libraryManager().changeIcon(libraryName, iconPath);
}

void MaterialManager::removeLibrary(const std::string& libraryName)
{
    libraryManager().removeLibrary(libraryName);
}

std::shared_ptr<std::vector<LibraryObject>> MaterialManager::libraryMaterials(
    const std::string& libraryName
)
{
    try {
        auto library = libraryManager().getLibrary(libraryName);
#if defined(BUILD_MATERIAL_EXTERNAL)
        if (_useExternal && library->isRemote()) {
            auto materials = _externalManager->libraryMaterials(libraryName);
            if (materials) {
                return materials;
            }
        }
#endif
        if (library->isLocal()) {
            return _localManager->libraryMaterials(libraryName);
        }
    }
    catch (const LibraryNotFound& e) {
    }
    return std::make_shared<std::vector<LibraryObject>>();
}

std::shared_ptr<std::vector<LibraryObject>> MaterialManager::libraryMaterials(
    const std::string& libraryName,
    const MaterialFilter& filter,
    const MaterialFilterOptions& options
)
{
    try {
        auto library = libraryManager().getMaterialLibrary(libraryName);
        return libraryMaterials(*library, filter, options);
    }
    catch (const LibraryNotFound& e) {
    }
    return std::make_shared<std::vector<LibraryObject>>();
}

std::shared_ptr<std::vector<LibraryObject>> MaterialManager::libraryMaterials(
    const MaterialLibrary& library,
    const MaterialFilter& filter,
    const MaterialFilterOptions& options
)
{
#if defined(BUILD_MATERIAL_EXTERNAL)
    if (_useExternal && !library.isLocal()) {
        auto materials = _externalManager->libraryMaterials(library.getName(), filter, options);
        if (materials) {
            return materials;
        }
    }
#endif
    if (library.isLocal()) {
        return _localManager->libraryMaterials(library.getName(), filter, options);
    }
    return std::make_shared<std::vector<LibraryObject>>();
}

bool MaterialManager::isLocalLibrary(const std::string& libraryName) const
{
    try {
        auto library = libraryManager().getLibrary(libraryName);
        return library->isLocal();
    }
    catch (const LibraryNotFound& e) {
    }
    return false;
}

void MaterialManager::setDisabled(const std::string& libraryName, bool disabled)
{
    libraryManager().setDisabled(libraryName, disabled);
}

void MaterialManager::setDisabled(Library& library, bool disabled)
{
    libraryManager().setDisabled(library, disabled);
}

bool MaterialManager::isDisabled(const std::string& libraryName) const
{
    auto library = getLibrary(libraryName);
    return isDisabled(*library);
}

bool MaterialManager::isDisabled(const Library& library) const
{
    return libraryManager().isDisabled(library);
}

//=====
//
// Folder management
//
//=====

std::shared_ptr<std::list<std::string>> MaterialManager::getMaterialFolders(
    const std::shared_ptr<MaterialLibrary>& library
) const
{
    if (library->isLocal()) {
        auto materialLibrary = std::make_shared<MaterialLibraryLocal>(*library);
        return _localManager->getMaterialFolders(materialLibrary);
    }

    return std::make_shared<std::list<std::string>>();
}

void MaterialManager::createFolder(const std::shared_ptr<MaterialLibrary>& library, const std::string& path)
{
    if (library->isLocal()) {
        auto materialLibrary = std::make_shared<MaterialLibraryLocal>(*library);
        _localManager->createFolder(materialLibrary, path);
    }
#if defined(BUILD_MATERIAL_EXTERNAL)
    else if (_useExternal) {
        _externalManager->createFolder(*library, path);
    }
    else {
        throw Materials::CreationError("External materials are not enabled");
    }
#endif
}

void MaterialManager::renameFolder(
    const std::shared_ptr<MaterialLibrary>& library,
    const std::string& oldPath,
    const std::string& newPath
)
{
    if (library->isLocal()) {
        auto materialLibrary = std::make_shared<MaterialLibraryLocal>(*library);
        _localManager->renameFolder(materialLibrary, oldPath, newPath);
    }
#if defined(BUILD_MATERIAL_EXTERNAL)
    else if (_useExternal) {
        _externalManager->renameFolder(*library, oldPath, newPath);
    }
    else {
        throw Materials::RenameError("External materials are not enabled");
    }
#endif
}

void MaterialManager::deleteRecursive(const std::shared_ptr<MaterialLibrary>& library, const std::string& path)
{
    if (library->isLocal()) {
        auto materialLibrary = std::make_shared<MaterialLibraryLocal>(*library);
        _localManager->deleteRecursive(materialLibrary, path);
    }
#if defined(BUILD_MATERIAL_EXTERNAL)
    else if (_useExternal) {
        _externalManager->deleteRecursive(*library, path);
    }
    else {
        throw Materials::DeleteError("External materials are not enabled");
    }
#endif
}

//=====
//
// Tree management
//
//=====

std::shared_ptr<std::map<std::string, std::shared_ptr<MaterialTreeNode>>> MaterialManager::getMaterialTree(
    const MaterialLibrary& library,
    const Materials::MaterialFilter& filter
) const
{
    MaterialFilterOptions options;
    return library.getMaterialTree(filter, options);
}

std::shared_ptr<std::map<std::string, std::shared_ptr<MaterialTreeNode>>> MaterialManager::getMaterialTree(
    const MaterialLibrary& library,
    const Materials::MaterialFilter& filter,
    const MaterialFilterOptions& options
) const
{
    return library.getMaterialTree(filter, options);
}

std::shared_ptr<std::map<std::string, std::shared_ptr<MaterialTreeNode>>> MaterialManager::getMaterialTree(
    const MaterialLibrary& library
) const
{
    Materials::MaterialFilter filter;
    MaterialFilterOptions options;
    return library.getMaterialTree(filter, options);
}

//=====
//
// Material management
//
//=====

std::shared_ptr<std::map<std::string, std::shared_ptr<Material>>> MaterialManager::getLocalMaterials() const
{
    return _localManager->getLocalMaterials();
}

std::shared_ptr<Material> MaterialManager::getMaterial(const std::string& uuid) const
{
#if defined(BUILD_MATERIAL_EXTERNAL)
    if (_useExternal) {
        auto material = _externalManager->getMaterial(uuid);
        if (material) {
            return material;
        }
    }
#endif
    // We really want to return the local material if not found, such as for User folder models
    return _localManager->getMaterial(uuid);
}

std::shared_ptr<Material> MaterialManager::getMaterial(const App::Material& material)
{
    MaterialManager manager;

    return manager.getMaterial(material.uuid);
}

std::shared_ptr<Material> MaterialManager::getMaterialByPath(const std::string& path) const
{
    return _localManager->getMaterialByPath(path);
}

std::shared_ptr<Material> MaterialManager::getMaterialByPath(const std::string& path, const std::string& lib) const
{
    return _localManager->getMaterialByPath(path, lib);
}

std::shared_ptr<Material> MaterialManager::getParent(const std::shared_ptr<Material>& material) const
{
    if (material->getParentUUID().isEmpty()) {
        throw MaterialNotFound();
    }

    return getMaterial(material->getParentUUID().toStdString());
}

std::shared_ptr<Material> MaterialManager::copyNew(const Material& original, const std::string& name) const
{
    auto newMaterial = std::make_shared<Material>(original);
    newMaterial->newUuid();
    newMaterial->setName(QString::fromStdString(name));

    return newMaterial;
}

std::shared_ptr<Material> MaterialManager::copyInherited(
    const Material& original,
    const std::string& name
) const
{
    auto newMaterial = copyNew(original, name);
    newMaterial->setParentUUID(original.getUUID());

    return newMaterial;
}

bool MaterialManager::exists(const std::string& uuid) const
{
    return _localManager->exists(uuid);
}

bool MaterialManager::exists(const MaterialLibrary& library, const std::string& uuid) const
{
    if (library.isLocal()) {
        return _localManager->exists(library, uuid);
    }
    return false;
}

void MaterialManager::move(
    const std::shared_ptr<MaterialLibrary>& library,
    const std::string& path,
    const std::shared_ptr<Material>& original
)
{
    if (library->isLocal() && original->getLibrary()->isLocal()) {
        // Local to local
        _localManager->move(library, path, original);
    }
#if defined(BUILD_MATERIAL_EXTERNAL)
    else if (library->isLocal()) {
        // Remote to local
        auto newMaterial = std::make_shared<Material>(*original);
        saveMaterial(library, newMaterial, path, false, false, true);
        _externalManager->remove(original->getUUID().toStdString());
    }
    else if (original->getLibrary()->isLocal()) {
        // Local to remote
        auto newMaterial = std::make_shared<Material>(*original);
        saveMaterial(library, newMaterial, path, false, false, true);
        _localManager->remove(original->getUUID().toStdString());
    }
    else {
        // Remote to remote
        _externalManager->move(library, path, original);
    }
#endif
}

void MaterialManager::move(
    const std::shared_ptr<MaterialLibrary>& library,
    const std::string& path,
    const std::string& uuid
)
{
    move(library, path, getMaterial(uuid));
}

void MaterialManager::copy(
    const std::shared_ptr<MaterialLibrary>& library,
    const std::string& path,
    const Material& original
)
{
    auto newMaterial = std::make_shared<Material>(original);
    saveMaterial(library, newMaterial, path, false, false, true);
}

void MaterialManager::copy(
    const std::shared_ptr<MaterialLibrary>& library,
    const std::string& path,
    const std::string& uuid
)
{
    copy(library, path, *getMaterial(uuid));
}

void MaterialManager::remove(const std::string& uuid) const
{
    _localManager->remove(uuid);
}

void MaterialManager::saveMaterial(
    const std::shared_ptr<MaterialLibrary>& library,
    const std::shared_ptr<Material>& material,
    const std::string& path,
    bool overwrite,
    bool saveAsCopy,
    bool saveInherited
) const
{
    if (library->isLocal()) {
        auto materialLibrary = std::make_shared<MaterialLibraryLocal>(*library);
        _localManager
            ->saveMaterial(materialLibrary, material, path, overwrite, saveAsCopy, saveInherited);
    }
#if defined(BUILD_MATERIAL_EXTERNAL)
    else {
        _externalManager->saveMaterial(library, material, path, overwrite);
    }
#endif
    material->resetEditState();
}

bool MaterialManager::isMaterial(const fs::path& p) const
{
    return _localManager->isMaterial(p);
}

bool MaterialManager::isMaterial(const Base::FileInfo& file) const
{
    return _localManager->isMaterial(file);
}

std::shared_ptr<std::map<std::string, std::shared_ptr<Material>>> MaterialManager::materialsWithModel(
    const std::string& uuid
) const
{
    return _localManager->materialsWithModel(uuid);
}

std::shared_ptr<std::map<std::string, std::shared_ptr<Material>>> MaterialManager::materialsWithModelComplete(
    const std::string& uuid
) const
{
    return _localManager->materialsWithModelComplete(uuid);
}

void MaterialManager::dereference() const
{
    _localManager->dereference();
}

void MaterialManager::dereference(std::shared_ptr<Material> material) const
{
    _localManager->dereference(material);
}

#if defined(BUILD_MATERIAL_EXTERNAL)
void MaterialManager::migrateToExternal(const std::shared_ptr<Materials::MaterialLibrary>& library)
{
    if (!_useExternal) {
        Base::Console().error("External interface not enabled\n");
        throw ConnectionError("External interface not enabled");
    }

    try {
        _externalManager->createLibrary(
            library->getName(),
            library->getIcon(),
            library->isReadOnly()
        );
    }
    catch (const CreationError&) {
    }
    catch (const ConnectionError&) {
    }

    auto materials = _localManager->libraryMaterials(library->getName());
    for (auto& it : *materials) {
        auto uuid = it.getUUID();
        auto path = it.getPath();
        auto name = it.getName();
        Base::Console().log(
            "\t('%s', '%s', '%s')\n",
            uuid.c_str(),
            path.c_str(),
            name.c_str()
        );

        auto material = _localManager->getMaterial(uuid);
        if (!material->isOldFormat()) {
            _externalManager->migrateMaterial(library->getName(), path, *material);
        }
    }
}

void MaterialManager::validateMigration(const std::shared_ptr<Materials::MaterialLibrary>& library)
{
    auto materials = _localManager->libraryMaterials(library->getName());
    _externalManager->resetCache();
    for (auto& it : *materials) {
        auto uuid = it.getUUID();
        auto path = it.getPath();
        auto name = it.getName();
        Base::Console().log(
            "\t('%s', '%s', '%s')\n",
            uuid.c_str(),
            path.c_str(),
            name.c_str()
        );

        auto material = _localManager->getMaterial(uuid);
        if (!material->isOldFormat()) {
            auto externalMaterial = _externalManager->getMaterial(uuid);
            material->validate(*externalMaterial);
        }
    }
}

// Cache stats
double MaterialManager::materialHitRate()
{
    initManagers();
    return _externalManager->materialHitRate();
}
#endif
