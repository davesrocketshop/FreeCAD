// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 David Carter <dcarter@david.carter.ca>             *
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

#ifndef MATERIAL_MANAGEDLIBRARY_H
#define MATERIAL_MANAGEDLIBRARY_H

#include <QByteArray>
#include <QString>

# include <Base/BaseClass.h>

# include <Mod/Material/MaterialGlobal.h>

namespace Materials
{

class Model;
class ModelLoader;
class ModelManagerLocal;
class Material;
class MaterialManagerLocal;

class MaterialsExport ManagedLibrary: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    ManagedLibrary() = default;
    ManagedLibrary(const ManagedLibrary& other) = default;
    ManagedLibrary(const std::string& libraryName, const std::string& icon, bool readOnly = true);
    ManagedLibrary(const std::string& libraryName, const QByteArray& icon, bool readOnly);
    ManagedLibrary(
        const std::string& libraryName,
        const std::string& dir,
        const std::string& iconPath,
        bool readOnly = true
    );
    ~ManagedLibrary() override = default;

    bool isLocal() const;
    bool isRemote() const;
    void setLocal(bool local);

    bool isModule() const;
    void setModule(bool module);

    std::string getRepositoryName() const
    {
        return _repositoryName;
    }
    void setRepositoryName(const std::string& newName)
    {
        _repositoryName = newName;
    }
    bool isRepositoryName(const std::string& name) const
    {
        return (_repositoryName == name);
    }

    std::string getLibraryName() const
    {
        return _libraryName;
    }
    void setLibraryName(const std::string& newName)
    {
        _libraryName = newName;
    }
    bool isLibraryName(const std::string& name) const
    {
        return (_libraryName == name);
    }

    QByteArray getIcon() const
    {
        return _icon;
    }
    static QByteArray getIcon(const std::string& iconPath);
    std::string getIconPath() const
    {
        return _iconPath;
    }
    void setIcon(const QByteArray& icon)
    {
        _icon = icon;
    }
    void setIcon(const std::string& iconPath);
    bool hasIcon() const
    {
        return !_icon.isEmpty();
    }
    bool isReadOnly() const
    {
        return _readOnly;
    }
    void setReadOnly(bool readOnly)
    {
        _readOnly = readOnly;
    }
    bool isDisabled() const
    {
        return _disabled;
    }
    void setDisabled(bool disabled)
    {
        _disabled = disabled;
    }

    std::string getMaterialDirectory() const
    {
        return _materialDirectory;
    }
    void setMaterialDirectory(const std::string& directory)
    {
        _materialDirectory = cleanPath(directory);
    }

    std::string getModelDirectory() const
    {
        return _modelDirectory;
    }
    void setModelDirectory(const std::string& directory)
    {
        _modelDirectory = cleanPath(directory);
    }
    std::string getMaterialDirectoryPath() const;
    std::string getModelDirectoryPath() const;

    bool operator==(const ManagedLibrary& library) const;
    bool operator!=(const ManagedLibrary& library) const
    {
        return !operator==(library);
    }

    std::string getLocalPath(const std::string& path) const;
    std::string getRelativePath(const std::string& path) const;
    std::string getLibraryPath(const std::string& path, const std::string& filename) const;
    bool isRoot(const std::string& path) const;

    // Validate a remote library against this one (a local library)
    void validate(const ManagedLibrary& remote) const;

    static std::string cleanPath(const std::string path);
    static QString cleanPath(const QString& path);

protected:
    friend class ModelLibraryLocal;
    friend class MaterialLibraryLocal;

    std::shared_ptr<std::map<std::string, std::shared_ptr<Model>>> _modelPathMap;
    std::shared_ptr<std::map<std::string, std::shared_ptr<Material>>> _materialPathMap;

private:
    std::string _repositoryName;
    std::string _libraryName;
    std::string _materialDirectory;
    std::string _modelDirectory;
    QByteArray _icon;
    std::string _iconPath;
    bool _readOnly;
    bool _disabled;

    bool _local;
    bool _module;

    QByteArray loadByteArrayFromFile(const std::string& filePath) const;
};

}  // namespace Materials

#endif  // MATERIAL_MANAGEDLIBRARY_H
