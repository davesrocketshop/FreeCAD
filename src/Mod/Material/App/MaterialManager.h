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

#ifndef MATERIAL_MATERIALMANAGER_H
#define MATERIAL_MATERIALMANAGER_H

#include <memory>

#include <filesystem>

#include <Base/FileInfo.h>
#include <Base/Parameter.h>
#include <Mod/Material/MaterialGlobal.h>

#include "FolderTree.h"
#include "LibraryManager.h"
#include "Materials.h"

#include "MaterialFilter.h"
#include "MaterialLibrary.h"

namespace fs = std::filesystem;

class QMutex;

namespace App
{
class Material;
}

namespace Materials
{
class MaterialManagerExternal;
class MaterialManagerLocal;
class MaterialFilter;
class MaterialFilterOptions;

class MaterialsExport MaterialManager: public Base::BaseClass,
                                       ParameterGrp::ObserverType,
                                       LibraryManager::ObserverType
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    ~MaterialManager() override;

    static MaterialManager& getManager();

    static void cleanup();
    static void refresh();

    // Defaults
    static std::shared_ptr<App::Material> defaultAppearance();
    static std::shared_ptr<Material> defaultMaterial();
    static std::string defaultMaterialUUID();

    // Library management
    bool useExternal() const
    {
        return _useExternal;
    }
    void setUseExternal(bool useExternal);
    std::shared_ptr<std::vector<std::shared_ptr<MaterialLibrary>>> getLibraries(
        bool includeDisabled = false,
        bool includeMasked = false
    );
    std::shared_ptr<std::vector<std::shared_ptr<MaterialLibrary>>> getLocalLibraries(
        bool includeDisabled = false
    );
#if defined(BUILD_MATERIAL_EXTERNAL)
    std::shared_ptr<std::vector<std::shared_ptr<MaterialLibrary>>> getRemoteLibraries(
        bool includeDisabled = false
    );
#endif
    std::shared_ptr<MaterialLibrary> getLibrary(const std::string& name) const;
    std::shared_ptr<MaterialLibrary> getDefaultLibrary() const;
    std::shared_ptr<MaterialLibrary> createLibrary(
        const std::string& libraryName,
        const std::string& iconPath,
        bool readOnly = true
    );
    std::shared_ptr<MaterialLibrary> createLocalLibrary(
        const std::string& libraryName,
        const std::string& materialDirectory,
        const std::string& modelDirectory,
        const std::string& iconPath,
        bool readOnly = true
    );
    std::shared_ptr<MaterialLibrary> createLocalLibrary(
        const std::string& libraryName,
        const std::string& materialDirectory,
        const std::string& iconPath,
        bool readOnly = true
    )
    {
        return createLocalLibrary(libraryName, materialDirectory, std::string(), iconPath, readOnly);
    }
    void renameLibrary(const std::string& libraryName, const std::string& newName);
    void changeIcon(const std::string& libraryName, const std::string& iconPath);
    void removeLibrary(const std::string& libraryName);
    std::shared_ptr<std::vector<LibraryObject>> libraryMaterials(const std::string& libraryName);
    std::shared_ptr<std::vector<LibraryObject>> libraryMaterials(
        const std::string& libraryName,
        const MaterialFilter& filter,
        const MaterialFilterOptions& options
    );
    std::shared_ptr<std::vector<LibraryObject>> libraryMaterials(
        const MaterialLibrary& library,
        const MaterialFilter& filter,
        const MaterialFilterOptions& options
    );
    bool isLocalLibrary(const std::string& libraryName) const;
    void setDisabled(const std::string& libraryName, bool disabled);
    void setDisabled(Library& library, bool disabled);
    bool isDisabled(const std::string& libraryName) const;
    bool isDisabled(const Library& library) const;

    // Folder management
    std::shared_ptr<std::list<std::string>> getMaterialFolders(
        const std::shared_ptr<MaterialLibrary>& library
    ) const;
    void createFolder(const std::shared_ptr<MaterialLibrary>& library, const std::string& path);
    void renameFolder(
        const std::shared_ptr<MaterialLibrary>& library,
        const std::string& oldPath,
        const std::string& newPath
    );
    void moveFolder(
        const std::shared_ptr<MaterialLibrary>& sourceLibrary,
        const std::string& sourcePath,
        const std::shared_ptr<MaterialLibrary>& destinationLibrary,
        const std::string& destinationPath
    );
    void copyFolder(
        const std::shared_ptr<MaterialLibrary>& sourceLibrary,
        const std::string& sourcePath,
        const std::shared_ptr<MaterialLibrary>& destinationLibrary,
        const std::string& destinationPath
    );
    void deleteRecursive(const std::shared_ptr<MaterialLibrary>& library, const std::string& path);

    // Tree management
    std::shared_ptr<std::map<std::string, std::shared_ptr<MaterialTreeNode>>> getMaterialTree(
        const MaterialLibrary& library,
        const Materials::MaterialFilter& filter
    ) const;
    std::shared_ptr<std::map<std::string, std::shared_ptr<MaterialTreeNode>>> getMaterialTree(
        const MaterialLibrary& library,
        const Materials::MaterialFilter& filter,
        const MaterialFilterOptions& options
    ) const;
    std::shared_ptr<std::map<std::string, std::shared_ptr<MaterialTreeNode>>> getMaterialTree(
        const MaterialLibrary& library
    ) const;

    // Material management
    std::shared_ptr<std::map<std::string, std::shared_ptr<Material>>> getLocalMaterials() const;
    std::shared_ptr<Material> getMaterial(const std::string& uuid) const;
    static std::shared_ptr<Material> getMaterial(const App::Material& material);
    std::shared_ptr<Material> getMaterialByPath(const std::string& path) const;
    std::shared_ptr<Material> getMaterialByPath(const std::string& path, const std::string& library) const;
    std::shared_ptr<Material> getParent(const std::shared_ptr<Material>& material) const;
    std::shared_ptr<Material> copyNew(const Material& original, const std::string& name) const;
    std::shared_ptr<Material> copyInherited(const Material& original, const std::string& name) const;
    bool exists(const std::string& uuid) const;
    bool exists(const MaterialLibrary& library, const std::string& uuid) const;
    void move(
        const std::shared_ptr<MaterialLibrary>& library,
        const std::string& path,
        const std::shared_ptr<Material>& original
    );
    void move(
        const std::shared_ptr<MaterialLibrary>& library,
        const std::string& path,
        const std::string& uuid
    );
    void copy(
        const std::shared_ptr<MaterialLibrary>& library,
        const std::string& path,
        const Material& original
    );
    void copy(
        const std::shared_ptr<MaterialLibrary>& library,
        const std::string& path,
        const std::string& uuid
    );
    void remove(const std::string& uuid) const;

    void saveMaterial(
        const std::shared_ptr<MaterialLibrary>& library,
        const std::shared_ptr<Material>& material,
        const std::string& path,
        bool overwrite,
        bool saveAsCopy,
        bool saveInherited
    ) const;

    bool isMaterial(const fs::path& p) const;
    bool isMaterial(const Base::FileInfo& file) const;

    std::shared_ptr<std::map<std::string, std::shared_ptr<Material>>> materialsWithModel(
        const std::string& uuid
    ) const;
    std::shared_ptr<std::map<std::string, std::shared_ptr<Material>>> materialsWithModelComplete(
        const std::string& uuid
    ) const;
    void dereference(std::shared_ptr<Material> material) const;
    void dereference() const;

    /// Observer message from the ParameterGrp
    void OnChange(ParameterGrp::SubjectType& rCaller, ParameterGrp::MessageType Reason) override;
    /// Observer message from the LibraryManager
    void OnChange(LibraryManager::SubjectType& manager, LibraryManager::MessageType reason) override;

#if defined(BUILD_MATERIAL_EXTERNAL)
    void migrateToExternal(const std::shared_ptr<Materials::MaterialLibrary>& library);
    void validateMigration(const std::shared_ptr<Materials::MaterialLibrary>& library);

    // Cache functions
    static double materialHitRate();
#endif

private:
    MaterialManager();

    FC_DISABLE_COPY_MOVE(MaterialManager);

    static void initManagers();
    static LibraryManager& libraryManager();

    static MaterialManager* _manager;

#if defined(BUILD_MATERIAL_EXTERNAL)
    static std::unique_ptr<MaterialManagerExternal> _externalManager;
#endif
    static std::unique_ptr<MaterialManagerLocal> _localManager;
    static QMutex _mutex;
    static bool _useExternal;

    ParameterGrp::handle _hGrp;
};

}  // namespace Materials

#endif  // MATERIAL_MATERIALMANAGER_H