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

#include <App/Application.h>
#include <Base/FileInfo.h>

#include "MaterialFilter.h"
#include "MaterialLibrary.h"
#include "MaterialLoader.h"
#include "MaterialManager.h"
#include "Materials.h"
#include "ModelManager.h"


using namespace Materials;

/* TRANSLATOR Material::Materials */

TYPESYSTEM_SOURCE(Materials::MaterialLibrary, Base::BaseClass)

MaterialLibrary::MaterialLibrary(const std::shared_ptr<ManagedLibrary>& library)
    : Library(library)
{}

MaterialLibrary::MaterialLibrary(const std::string& libraryName, const std::string& icon, bool readOnly)
    : Library(libraryName, icon, readOnly)
{}

MaterialLibrary::MaterialLibrary(const std::string& libraryName,
                                 const std::string& dir,
                                 const std::string& icon,
                                 bool readOnly)
    : Library(libraryName, dir, icon, readOnly)
{}

MaterialLibrary::MaterialLibrary(const Library& library)
    : Library(library)
{}

bool MaterialLibrary::isRoot(const std::string& path) const
{
    std::string localPath = getLocalPath(cleanPath(path));
    std::string clean = getLocalPath("");
    return (clean == localPath);
}

std::string MaterialLibrary::getLocalPath(const std::string& path) const
{
    return Library::getLocalPath(getMaterialDirectoryPath(), path);
}

std::shared_ptr<std::map<std::string, std::shared_ptr<MaterialTreeNode>>>
MaterialLibrary::getMaterialTree(const Materials::MaterialFilter& filter,
                                 const Materials::MaterialFilterOptions& options) const
{
    std::shared_ptr<std::map<std::string, std::shared_ptr<MaterialTreeNode>>> materialTree =
        std::make_shared<std::map<std::string, std::shared_ptr<MaterialTreeNode>>>();

    auto materials = MaterialManager::getManager().libraryMaterials(*this, filter, options);
    for (auto& it : *materials) {
        auto uuid = it.getUUID();
        auto path = it.getPath();
        auto filename = it.getName();

        QStringList list = QString::fromStdString(path).split(QStringLiteral("/"));

        // Start at the root
        std::shared_ptr<std::map<std::string, std::shared_ptr<MaterialTreeNode>>> node =
            materialTree;
        for (auto& itp : list) {
            if (!itp.isEmpty()) {
                // Add the folder only if it's not already there
                if (!node->contains(itp.toStdString())) {
                    auto mapPtr = std::make_shared<
                        std::map<std::string, std::shared_ptr<MaterialTreeNode>>>();
                    std::shared_ptr<MaterialTreeNode> child =
                        std::make_shared<MaterialTreeNode>();
                    child->setFolder(mapPtr);
                    child->setReadOnly(isReadOnly());
                    (*node)[itp.toStdString()] = child;
                    node = mapPtr;
                }
                else {
                    node = (*node)[itp.toStdString()]->getFolder();
                }
            }
        }
        std::shared_ptr<MaterialTreeNode> child = std::make_shared<MaterialTreeNode>();
        child->setUUID(uuid);
        child->setReadOnly(isReadOnly());
        if (isLocal()) {
            auto material = MaterialManager::getManager().getMaterial(uuid);
            child->setOldFormat(material->isOldFormat());
        }
        (*node)[filename] = child;
    }

    // Empty folders aren't included in _materialPathMap, so we add them by looking at the file
    // system
    if (options.includeEmptyFolders()) {
        if (isLocal()) {
            auto& materialLibrary =
                *(reinterpret_cast<const Materials::MaterialLibraryLocal*>(this));
            auto folderList = MaterialLoader::getMaterialFolders(materialLibrary);
            for (auto& folder : *folderList) {
                QStringList list = QString::fromStdString(folder).split(QStringLiteral("/"));

                // Start at the root
                auto node = materialTree;
                for (auto& itp : list) {
                    // Add the folder only if it's not already there
                    if (!node->contains(itp.toStdString())) {
                        std::shared_ptr<std::map<std::string, std::shared_ptr<MaterialTreeNode>>>
                            mapPtr = std::make_shared<
                                std::map<std::string, std::shared_ptr<MaterialTreeNode>>>();
                        std::shared_ptr<MaterialTreeNode> child =
                            std::make_shared<MaterialTreeNode>();
                        child->setFolder(mapPtr);
                        (*node)[itp.toStdString()] = child;
                        node = mapPtr;
                    }
                    else {
                        node = (*node)[itp.toStdString()]->getFolder();
                    }
                }
            }
        }
    }

    return materialTree;
}

/* TRANSLATOR Material::Materials */

TYPESYSTEM_SOURCE(Materials::MaterialLibraryLocal, Materials::MaterialLibrary)

MaterialLibraryLocal::MaterialLibraryLocal()
{
    setLocal(true);
}

MaterialLibraryLocal::MaterialLibraryLocal(const std::shared_ptr<ManagedLibrary>& library)
    : MaterialLibrary(library)
{
    setLocal(true);
}

MaterialLibraryLocal::MaterialLibraryLocal(const Library& other)
    : MaterialLibrary(other)
{
    setLocal(true);
}

MaterialLibraryLocal::MaterialLibraryLocal(
    const std::string& libraryName,
    const std::string& dir,
    const std::string& icon,
    bool readOnly
)
    : MaterialLibrary(libraryName, dir, icon, readOnly)
{
    setLocal(true);
}

void MaterialLibraryLocal::createFolder(const std::string& path)
{
    std::string filePath = getLocalPath(path);

    Base::FileInfo fileDir(filePath);
    if (!fileDir.isDir()) {
        if (!fileDir.createDirectories()) {
            Base::Console().error("Unable to create directory path '%s'\n",
                                  filePath.c_str());
        }
    }
}

void MaterialLibraryLocal::renameFolder(const std::string& oldPath, const std::string& newPath)
{
    std::string filePath = getLocalPath(oldPath);
    std::string newFilePath = getLocalPath(newPath);

    Base::FileInfo fileDir(filePath);
    if (fileDir.isDir()) {
        if (!fileDir.renameFile(newFilePath.c_str())) {
            Base::Console().error("Unable to rename directory path '%s'\n",
                                  filePath.c_str());
        }
    }

    Base::Console().log(
        "updatePaths('%s', '%s)\n",
        oldPath.c_str(),
        newPath.c_str()
    );
    updatePaths(oldPath, newPath);
}

void MaterialLibraryLocal::deleteRecursive(const std::string& path)
{
    if (isRoot(path)) {
        return;
    }

    std::string filePath = getLocalPath(path);
    auto& manager = MaterialManager::getManager();

    Base::FileInfo info(filePath);
    if (info.isDir()) {
        deleteDir(manager, filePath);
    }
    else {
        deleteFile(manager, filePath);
    }
}

// This accepts the filesystem path as returned from getLocalPath
void MaterialLibraryLocal::deleteDir([[maybe_unused]] MaterialManager& manager, const std::string& path)
{
    Base::FileInfo file(path);
    if (file.isDir()) {
        file.deleteDirectoryRecursive();
    }
}

// This accepts the filesystem path as returned from getLocalPath
void MaterialLibraryLocal::deleteFile(MaterialManager& manager, const std::string& path)
{
    Base::FileInfo file(path);
    if (file.deleteFile()) {
        // Remove from the map
        std::string rPath = getRelativePath(path);
        try {
            auto material = getMaterialByPath(rPath);
            manager.remove(material->getUUID().toStdString());
        }
        catch (const MaterialNotFound&) {
            Base::Console().log("Unable to remove file from materials list\n");
        }
        proxy()->_materialPathMap->erase(rPath);
    }
    else {
        std::string error = "DeleteError: Unable to delete " + path;
        throw DeleteError(error);
    }
}

void MaterialLibraryLocal::updatePaths(const std::string& oldPath, const std::string& newPath)
{
    // Update the path map
    std::string op = getRelativePath(oldPath);
    std::string np = getRelativePath(newPath);
    std::unique_ptr<std::map<std::string, std::shared_ptr<Material>>> pathMap =
        std::make_unique<std::map<std::string, std::shared_ptr<Material>>>();
    for (auto& itp : *proxy()->_materialPathMap) {
        std::string path = itp.first;
        if (path.starts_with(op)) {
            path = np + QString::fromStdString(path).remove(0, op.size()).toStdString();
        }

        // Don't include the filename
        Base::FileInfo filepath(path);
        itp.second->setDirectory(QString::fromStdString(filepath.dirPath()));

        (*pathMap)[path] = itp.second;
    }

    proxy()->_materialPathMap = std::move(pathMap);
}

std::shared_ptr<Material>
MaterialLibraryLocal::saveMaterial(const std::shared_ptr<Material>& material,
                                   const std::string& path,
                                   bool overwrite,
                                   bool saveAsCopy,
                                   bool saveInherited)
{
    std::string filePath = getLocalPath(path);
    QFile file(QString::fromStdString(filePath));

    Base::FileInfo info(filePath);
    Base::FileInfo fileDir(info.dirPath());
    if (!fileDir.exists()) {
        if (!fileDir.createDirectories()) {
            Base::Console().error(
                "Unable to create directory path '%s'\n",
                info.dirPath().c_str()
            );
            throw SaveError();
        }
    }

    if (info.exists()) {
        if (!overwrite) {
            Base::Console().error("File already exists '%s'\n", info.filePath().c_str());
            throw MaterialExists();
        }
    }

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        stream.setCodec("UTF-8");
#endif
        stream.setGenerateByteOrderMark(true);

        // Write the contents
        material->setName(QString::fromStdString(info.fileNamePure()));
        material->setLibrary(getptr());
        material->setDirectory(QString::fromStdString(getRelativePath(info.dirPath())));
        material->save(stream, overwrite, saveAsCopy, saveInherited);
    }

    return addMaterial(material, path);
}

bool MaterialLibraryLocal::fileExists(const std::string& path) const
{
    std::string filePath = getLocalPath(path);
    Base::FileInfo info(filePath);

    return info.exists();
}

std::shared_ptr<Material>
MaterialLibraryLocal::addMaterial(const std::shared_ptr<Material>& material, const std::string& path)
{
    std::string filePath = getRelativePath(path);
    Base::FileInfo info(filePath);
    std::shared_ptr<Material> newMaterial = std::make_shared<Material>(*material);
    newMaterial->setLibrary(getptr());
    newMaterial->setDirectory(QString::fromStdString(getLibraryPath(filePath, info.fileName())));
    // newMaterial->setFilename(info.fileName());

    (*proxy()->_materialPathMap)[filePath] = newMaterial;

    return newMaterial;
}

std::shared_ptr<Material> MaterialLibraryLocal::getMaterialByPath(const std::string& path) const
{
    std::string filePath = getRelativePath(path);

    auto search = proxy()->_materialPathMap->find(filePath);
    if (search != proxy()->_materialPathMap->end()) {
        return search->second;
    }

    throw MaterialNotFound();
}

std::string MaterialLibraryLocal::getUUIDFromPath(const std::string& path) const
{
    std::string filePath = getRelativePath(path);

    auto search = proxy()->_materialPathMap->find(filePath);
    if (search != proxy()->_materialPathMap->end()) {
        return search->second->getUUID().toStdString();
    }

    throw MaterialNotFound();
}
