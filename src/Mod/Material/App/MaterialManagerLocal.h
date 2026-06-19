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

#pragma once

#include <memory>

#include <filesystem>

#include <Mod/Material/MaterialGlobal.h>

#include "FolderTree.h"
#include "Materials.h"

namespace fs = std::filesystem;

class QMutex;

namespace App
{
class Material;
}

namespace Materials
{

class Library;
class LibraryObject;
class MaterialLibrary;
class MaterialLibraryLocal;
class MaterialFilter;
class MaterialFilterOptions;

class MaterialsExport MaterialManagerLocal: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    MaterialManagerLocal();
    ~MaterialManagerLocal() override = default;

    static void cleanup();
    static void refresh();

    // Library management
    std::shared_ptr<std::vector<LibraryObject>> libraryMaterials(const std::string& libraryName);
    std::shared_ptr<std::vector<LibraryObject>> libraryMaterials(
        const std::string& libraryName,
        const MaterialFilter& filter,
        const MaterialFilterOptions& options
    );

    // Folder management
    std::shared_ptr<std::vector<std::string>> getMaterialFolders(const MaterialLibrary& library) const;
    std::shared_ptr<std::vector<std::string>> getMaterialSubFolders(
        const MaterialLibrary& library,
        const std::string& path
    ) const;
    void createFolder(const std::shared_ptr<MaterialLibraryLocal>& library, const std::string& path);
    void renameFolder(
        const std::shared_ptr<MaterialLibraryLocal>& library,
        const std::string& oldPath,
        const std::string& newPath
    );
    void moveFolderLocal(
        const std::shared_ptr<MaterialLibrary>& sourceLibrary,
        const std::string& sourcePath,
        const std::shared_ptr<MaterialLibrary>& destinationLibrary,
        const std::string& destinationPath
    );
    void deleteRecursive(const std::shared_ptr<MaterialLibraryLocal>& library, const std::string& path);
    std::shared_ptr<std::vector<Material>> folderMaterials(
        MaterialLibrary& library,
        const std::string& sourcePath
    ) const;

    // Material management
    std::shared_ptr<std::map<std::string, std::shared_ptr<Material>>> getLocalMaterials() const;
    std::shared_ptr<Material> getMaterial(const std::string& uuid) const;
    std::shared_ptr<Material> getMaterialByPath(const std::string& path) const;
    std::shared_ptr<Material> getMaterialByPath(const std::string& path, const std::string& library) const;
    bool exists(const std::string& uuid) const;
    bool exists(const MaterialLibrary& library, const std::string& uuid) const;
    void move(
        const std::shared_ptr<MaterialLibrary>& library,
        const std::string& path,
        std::shared_ptr<Material> original
    );
    void remove(const std::string& uuid);

    void saveMaterial(
        const std::shared_ptr<MaterialLibraryLocal>& library,
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

protected:
    static std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> getConfiguredLibraries(
        bool includeDisabled = false
    );
    bool passFilter(
        const Material& material,
        const Materials::MaterialFilter& filter,
        const Materials::MaterialFilterOptions& options
    ) const;

private:
    /*
     * List of local libraries
     */
    static std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>> _libraryList;

    /*
     * Map of materials using the UUID as the key
     */
    static std::shared_ptr<std::map<std::string, std::shared_ptr<Material>>> _materialMap;

    static QMutex _mutex;

    static void initLibraries();
    // void setDisabledOnLibraryList(const std::string& libraryName, bool disabled);

    /*
     * Update the libraries and paths of affected materials after a move
     */
    void updateMovedMaterials(
        const std::shared_ptr<MaterialLibrary>& sourceLibrary,
        const std::string& sourcePath,
        const std::shared_ptr<MaterialLibrary>& destinationLibrary,
        const std::string& destinationPath
    );
};

}  // namespace Materials