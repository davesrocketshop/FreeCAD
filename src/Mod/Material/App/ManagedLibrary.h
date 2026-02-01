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

# include <QDir>
# include <QByteArray>
# include <QString>

# include <Base/BaseClass.h>

# include <Mod/Material/MaterialGlobal.h>

namespace Materials
{

class ModelLoader;
class ModelManagerLocal;
class MaterialManagerLocal;

class MaterialsExport ManagedLibrary: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    ManagedLibrary() = default;
    ManagedLibrary(const ManagedLibrary& other) = default;
    ManagedLibrary(const QString& libraryName, const QString& icon, bool readOnly = true);
    ManagedLibrary(const QString& libraryName, const QByteArray& icon, bool readOnly);
    ManagedLibrary(
        const QString& libraryName,
        const QString& dir,
        const QString& iconPath,
        bool readOnly = true
    );
    ~ManagedLibrary() override = default;

    bool isLocal() const;
    void setLocal(bool local);

    bool isModule() const;
    void setModule(bool module);

    QString getRepositoryName() const
    {
        return _repositoryName;
    }
    void setRepositoryName(const QString& newName)
    {
        _repositoryName = newName;
    }
    bool isRepositoryName(const QString& name) const
    {
        return (_repositoryName == name);
    }

    QString getLibraryName() const
    {
        return _libraryName;
    }
    void setLibraryName(const QString& newName)
    {
        _libraryName = newName;
    }
    bool isLibraryName(const QString& name) const
    {
        return (_libraryName == name);
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
    void setDisabled(bool disabled)
    {
        _disabled = disabled;
    }

    QString getMaterialDirectory() const
    {
        return _materialDirectory;
    }
    void setMaterialDirectory(const QString& directory)
    {
        _materialDirectory = cleanPath(directory);
    }

    QString getModelDirectory() const
    {
        return _modelDirectory;
    }
    void setModelDirectory(const QString& directory)
    {
        _modelDirectory = cleanPath(directory);
    }
    QString getMaterialDirectoryPath() const
    {
        return QDir(_materialDirectory).absolutePath();
    }
    QString getModelDirectoryPath() const
    {
        return QDir(_modelDirectory).absolutePath();
    }

    bool operator==(const ManagedLibrary& library) const;
    bool operator!=(const ManagedLibrary& library) const
    {
        return !operator==(library);
    }

    QString getLocalPath(const QString& path) const;
    QString getRelativePath(const QString& path) const;
    QString getLibraryPath(const QString& path, const QString& filename) const;
    bool isRoot(const QString& path) const;

    // Validate a remote library against this one (a local library)
    void validate(const ManagedLibrary& remote) const;

    static std::string cleanPath(const std::string path);
    static QString cleanPath(const QString& path);

private:
    QString _repositoryName;
    QString _libraryName;
    QString _materialDirectory;
    QString _modelDirectory;
    QByteArray _icon;
    QString _iconPath;
    bool _readOnly;
    bool _disabled;

    bool _local;
    bool _module;

    QByteArray loadByteArrayFromFile(const QString& filePath) const;
};

}  // namespace Materials

#endif  // MATERIAL_MANAGEDLIBRARY_H
