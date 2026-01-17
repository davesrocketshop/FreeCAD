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

#ifndef MATERIAL_SHAREDLIBRARY_H
#define MATERIAL_SHAREDLIBRARY_H

#include <QDir>
#include <QByteArray>
#include <QString>

#include <Base/BaseClass.h>

#include <Mod/Material/MaterialGlobal.h>

namespace Materials
{

class ModelLoader;
class ModelManagerLocal;
class MaterialManagerLocal;

class MaterialsExport SharedLibrary: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    SharedLibrary() = default;
    SharedLibrary(const SharedLibrary& other) = default;
    SharedLibrary(
        const QString& repositoryName,
        const QString& libraryName,
        const QString& icon,
        bool readOnly = true
    );
    SharedLibrary(
        const QString& repositoryName,
        const QString& libraryName,
        const QByteArray& icon,
        bool readOnly
    );
    SharedLibrary(
        const QString& repositoryName,
        const QString& libraryName,
        const QString& dir,
        const QString& iconPath,
        bool readOnly = true
    );
    ~SharedLibrary() override = default;

    bool isLocal() const;
    void setLocal(bool local);

    bool isModule() const;
    void setModule(bool module);

    QString getName() const
    {
        return _name;
    }
    void setName(const QString& newName)
    {
        _name = newName;
    }
    bool isName(const QString& name)
    {
        return (_name == name);
    }

    QString getRepository() const
    {
        return _name;
    }
    void setRepository(const QString& newName)
    {
        _name = newName;
    }
    bool isRepository(const QString& name)
    {
        return (_name == name);
    }

    QByteArray getIcon() const
    {
        return _icon;
    }
    static QByteArray getIcon(const QString& iconPath);
    QString getIconPath() const
    {
        return _iconPath;
    }
    void setIcon(const QByteArray& icon)
    {
        _icon = icon;
    }
    void setIcon(const QString& iconPath);
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

    QString getDirectory() const
    {
        return _directory;
    }
    QString getDirectoryPath() const
    {
        return QDir(_directory).absolutePath();
    }

    bool operator==(const SharedLibrary& library) const;
    bool operator!=(const SharedLibrary& library) const
    {
        return !operator==(library);
    }

    QString getLocalPath(const QString& path) const;
    QString getRelativePath(const QString& path) const;
    QString getLibraryPath(const QString& path, const QString& filename) const;
    bool isRoot(const QString& path) const;

    // Validate a remote library against this one (a local library)
    void validate(const SharedLibrary& remote) const;

    static std::string cleanPath(const std::string path);
    static QString cleanPath(const QString& path);

protected:
    // These should only be done through the MaterialManager or one of its subbordinates
    void setDisabled(bool disabled)
    {
        _disabled = disabled;
    }
    void setDirectory(const QString& directory)
    {
        _directory = cleanPath(directory);
    }

    friend class LibraryManager;
    friend class ModelLoader;
    friend class ModelManagerLocal;
    friend class MaterialManagerLocal;

private:
    QString _repository;
    QString _name;
    QString _directory;
    QByteArray _icon;
    QString _iconPath;
    bool _readOnly;
    bool _disabled;

    bool _local;
    bool _module;

    QByteArray loadByteArrayFromFile(const QString& filePath) const;
};

}  // namespace Materials

#endif  // MATERIAL_SHAREDLIBRARY_H
