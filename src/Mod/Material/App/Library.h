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

#include <QDir>
#include <QByteArray>
#include <QString>

#include <Base/BaseClass.h>

#include <Mod/Material/MaterialGlobal.h>

#include"ManagedLibrary.h"

namespace Materials
{

class ModelLoader;
class ModelManagerLocal;
class MaterialManagerLocal;

class MaterialsExport Library: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    Library();
    Library(const Library &other) = default;
    Library(const std::shared_ptr<ManagedLibrary>& library);
    Library(const QString& libraryName, const QString& icon, bool readOnly = true);
    Library(const QString& libraryName, const QByteArray& icon, bool readOnly);
    Library(const QString& libraryName,
            const QString& dir,
            const QString& iconPath,
            bool readOnly = true);
    ~Library() override = default;

    bool isLocal() const;
    void setLocal(bool local);

    bool isModule() const;
    void setModule(bool module);

    QString getName() const
    {
        return _managedLibrary->getLibraryName();
    }
    void setName(const QString& newName)
    {
        _managedLibrary->setLibraryName(newName);
    }
    bool isName(const QString& name)
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
    void setIcon(const QString& iconPath);
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

    virtual QString getDirectory() const = 0;
    virtual QString getDirectoryPath() const = 0;

    QString getMaterialDirectory() const
    {
        return _managedLibrary->getMaterialDirectory();
    }
    QString getMaterialDirectoryPath() const
    {
        return QDir(_managedLibrary->getMaterialDirectory()).absolutePath();
    }

    QString getModelDirectory() const
    {
        return _managedLibrary->getModelDirectory();
    }
    QString getModelDirectoryPath() const
    {
        return QDir(_managedLibrary->getModelDirectory()).absolutePath();
    }

    bool operator==(const Library& library) const;
    bool operator!=(const Library& library) const
    {
        return !operator==(library);
    }

    QString getRelativePath(const QString& path) const;
    QString getLibraryPath(const QString& path, const QString& filename) const;
    virtual bool isRoot(const QString& path) const = 0;

    // Validate a remote library against this one (a local library)
    void validate(const Library& remote) const;

    static std::string cleanPath(const std::string path);
    static QString cleanPath(const QString& path);

protected:
    // These should only be done through the MaterialManager or one of its subbordinates
    void setDisabled(bool disabled)
    {
        _managedLibrary->setDisabled(disabled);
    }
    void setDirectory(const QString& directory)
    {
        _managedLibrary->setMaterialDirectory(cleanPath(directory));
    }

    friend class ModelLoader;
    friend class ModelManagerLocal;
    friend class MaterialManagerLocal;

    std::shared_ptr<ManagedLibrary> proxy() const {
        return _managedLibrary;
    }

    QString getLocalPath(const QString& directory, const QString& path) const;

private:
    std::shared_ptr<ManagedLibrary> _managedLibrary;

    QByteArray loadByteArrayFromFile(const QString& filePath) const;
};

class MaterialsExport LibraryObject
{
public:
    LibraryObject(const QString& uuid, const QString& path, const QString& name)
        : _uuid(uuid)
        , _path(path)
        , _name(name)
    {}
    LibraryObject(const std::string& uuid, const std::string& path, const std::string& name)
        : _uuid(QString::fromStdString(uuid))
        , _path(QString::fromStdString(path))
        , _name(QString::fromStdString(name))
    {}
    ~LibraryObject() = default;

    void setUUID(const QString& uuid)
    {
        _uuid = uuid;
    }
    void setUUID(const std::string& uuid)
    {
        _uuid = QString::fromStdString(uuid);
    }
    QString getUUID() const
    {
        return _uuid;
    }

    void setPath(const QString& path)
    {
        _path = path;
    }
    void setPath(const std::string& path)
    {
        _path = QString::fromStdString(path);
    }
    QString getPath() const
    {
        return _path;
    }

    void setName(const QString& name)
    {
        _name = name;
    }
    void setName(const std::string& name)
    {
        _name = QString::fromStdString(name);
    }
    QString getName() const
    {
        return _name;
    }

private:
    QString _uuid;
    QString _path;
    QString _name;
};

}  // namespace Materials

#endif  // MATERIAL_LIBRARY_H
