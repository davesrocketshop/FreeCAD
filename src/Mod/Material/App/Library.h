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

#ifndef MATERIAL_LIBRARY_H
#define MATERIAL_LIBRARY_H

#include <memory>
#include <filesystem>

#include <QByteArray>
#include <QString>

#include <Base/BaseClass.h>

#include <Mod/Material/MaterialGlobal.h>

#include"ManagedLibrary.h"

namespace fs = std::filesystem;

namespace Materials
{

class ModelLoader;
class ModelManagerLocal;
class MaterialManagerLocal;
class LibraryManager;

class MaterialsExport Library: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    Library(const Library &other) = default;
    ~Library() override = default;

    bool isLocal() const;
    void setLocal(bool local);

    bool isModule() const;
    void setModule(bool module);

    std::string getRepositoryName() const
    {
        return _managedLibrary->getRepositoryName();
    }
    void setRepositoryName(const std::string& newName)
    {
        _managedLibrary->setRepositoryName(newName);
    }
    bool isRepositoryName(const std::string& name) const
    {
        return _managedLibrary->isRepositoryName(name);
    }

    std::string getName() const
    {
        return _managedLibrary->getLibraryName();
    }
    void setName(const std::string& newName)
    {
        _managedLibrary->setLibraryName(newName);
    }
    bool isName(const std::string& name)
    {
        return _managedLibrary->isLibraryName(name);
    }

    QByteArray getIcon() const
    {
        return _managedLibrary->getIcon();
    }
    void setIcon(const QByteArray& icon)
    {
        _managedLibrary->setIcon(icon);
    }
    void setIcon(const std::string& iconPath);
    bool hasIcon() const
    {
        return _managedLibrary->hasIcon();
    }
    bool isReadOnly() const
    {
        return _managedLibrary->isReadOnly();
    }
    void setReadOnly(bool readOnly)
    {
        _managedLibrary->setReadOnly(readOnly);
    }
    bool isDisabled() const
    {
        return _managedLibrary->isDisabled();
    }

    virtual std::string getDirectory() const = 0;
    virtual std::string getDirectoryPath() const = 0;

    std::string getMaterialDirectory() const
    {
        return _managedLibrary->getMaterialDirectory();
    }
    std::string getMaterialDirectoryPath() const
    {
        return fs::weakly_canonical(_managedLibrary->getMaterialDirectory()).string();
    }

    std::string getModelDirectory() const
    {
        return _managedLibrary->getModelDirectory();
    }
    std::string getModelDirectoryPath() const
    {
        return fs::weakly_canonical(_managedLibrary->getModelDirectory()).string();
    }

    bool operator==(const Library& library) const;
    bool operator!=(const Library& library) const
    {
        return !operator==(library);
    }

    std::string getRelativePath(const std::string& path) const;
    std::string getLibraryPath(const std::string& path, const std::string& filename) const;
    virtual bool isRoot(const std::string& path) const = 0;

    // Validate a remote library against this one (a local library)
    void validate(const Library& remote) const;

    static std::string cleanPath(const std::string path);
    static QString cleanPath(const QString& path);

protected:
    Library();
    Library(const std::shared_ptr<ManagedLibrary>& library);
    Library(const std::string& libraryName, const std::string& icon, bool readOnly = true);
    Library(const std::string& libraryName, const QByteArray& icon, bool readOnly);
    Library(const std::string& libraryName, const std::string& dir, const std::string& iconPath, bool readOnly = true);

    // These should only be done through the MaterialManager or one of its subbordinates
    void setDisabled(bool disabled)
    {
        _managedLibrary->setDisabled(disabled);
    }
    void setDirectory(const std::string& directory)
    {
        _managedLibrary->setMaterialDirectory(cleanPath(directory));
    }

    // friend class ModelLoader;
    // friend class ModelManagerLocal;
    // friend class MaterialManagerLocal;
    friend class LibraryManager;

    std::shared_ptr<ManagedLibrary> proxy() const {
        return _managedLibrary;
    }

    std::string getLocalPath(const std::string& directory, const std::string& path) const;

private:
    std::shared_ptr<ManagedLibrary> _managedLibrary;

    QByteArray loadByteArrayFromFile(const std::string& filePath) const;
};

class MaterialsExport LibraryObject
{
public:
    LibraryObject(const std::string& uuid, const std::string& path, const std::string& name)
        : _uuid(uuid)
        , _path(path)
        , _name(name)
    {}
    ~LibraryObject() = default;

    void setUUID(const std::string& uuid)
    {
        _uuid = uuid;
    }
    std::string getUUID() const
    {
        return _uuid;
    }

    void setPath(const std::string& path)
    {
        _path = path;
    }
    std::string getPath() const
    {
        return _path;
    }

    void setName(const std::string& name)
    {
        _name = name;
    }
    std::string getName() const
    {
        return _name;
    }

private:
    std::string _uuid;
    std::string _path;
    std::string _name;
};

}  // namespace Materials

#endif  // MATERIAL_LIBRARY_H
