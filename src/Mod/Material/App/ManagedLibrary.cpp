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

#include <string>

#include <App/Application.h>

#include "Exceptions.h"
#include "ManagedLibrary.h"


using namespace Materials;

TYPESYSTEM_SOURCE(Materials::ManagedLibrary, Base::BaseClass)

ManagedLibrary::ManagedLibrary(const QString& libraryName, const QString& iconPath, bool readOnly)
    : _libraryName(libraryName)
    , _readOnly(readOnly)
    , _disabled(false)
    , _local(false)
    , _module(false)
{
    setIcon(iconPath);
}

ManagedLibrary::ManagedLibrary(const QString& libraryName, const QByteArray& icon, bool readOnly)
    : _libraryName(libraryName)
    , _icon(icon)
    , _readOnly(readOnly)
    , _disabled(false)
    , _local(false)
    , _module(false)
{}

ManagedLibrary::ManagedLibrary(const QString& libraryName, const QString& dir, const QString& iconPath, bool readOnly)
    : _libraryName(libraryName)
    , _materialDirectory(cleanPath(dir))
    , _readOnly(readOnly)
    , _disabled(false)
    , _local(false)
    , _module(false)
{
    setIcon(iconPath);
}

QByteArray ManagedLibrary::getIcon(const QString& iconPath)
{
    QFile file(iconPath);
    if (!file.open(QIODevice::ReadOnly)) {
        Base::Console().log("Failed to open icon file '%s'\n", iconPath.toStdString().c_str());
        return QByteArray();  // Return an empty QByteArray if file opening fails
    }

    QByteArray data = file.readAll();
    file.close();
    return data;
}

void ManagedLibrary::setIcon(const QString& iconPath)
{
    _iconPath = iconPath;
    _icon = getIcon(iconPath);
}

bool ManagedLibrary::isLocal() const
{
    return _local;
}

void ManagedLibrary::setLocal(bool local)
{
    _local = local;
}

bool ManagedLibrary::isModule() const
{
    return _module;
}

void ManagedLibrary::setModule(bool module)
{
    _module = module;
}

bool ManagedLibrary::operator==(const ManagedLibrary& library) const
{
    return (isRepositoryName(library.getRepositoryName()))
        && (isLibraryName(library.getLibraryName()))
        && (_materialDirectory == library._materialDirectory)
        && (_modelDirectory == library._modelDirectory);
}

void ManagedLibrary::validate(const ManagedLibrary& remote) const
{
    if (!isLibraryName(remote.getLibraryName())) {
        throw InvalidLibrary("Library names don't match");
    }
    if (getIcon() != remote.getIcon()) {
        throw InvalidLibrary("Library icons don't match");
    }

    // Local and remote paths will differ
    if (!remote.getMaterialDirectory().isEmpty()) {
        throw InvalidLibrary("Remote library should not have a material path");
    }

    // Local and remote paths will differ
    if (!remote.getModelDirectory().isEmpty()) {
        throw InvalidLibrary("Remote library should not have a model path");
    }

    if (isReadOnly() != remote.isReadOnly()) {
        throw InvalidLibrary("Library readonly settings don't match");
    }
}

QString ManagedLibrary::getLocalPath(const QString& path) const
{
    QString filePath = getMaterialDirectoryPath();
    if (!(filePath.endsWith(QStringLiteral("/")) || filePath.endsWith(QStringLiteral("\\")))) {
        filePath += QStringLiteral("/");
    }

    QString clean = cleanPath(path);
    QString prefix = QStringLiteral("/") + getLibraryName();
    if (clean.startsWith(prefix)) {
        // Remove the library name from the path
        filePath += clean.right(clean.length() - prefix.length());
    }
    else {
        filePath += clean;
    }

    return filePath;
}

bool ManagedLibrary::isRoot(const QString& path) const
{
    QString localPath = getLocalPath(cleanPath(path));
    QString clean = getLocalPath(QStringLiteral(""));
    return (clean == localPath);
}

QString ManagedLibrary::getRelativePath(const QString& path) const
{
    QString filePath;
    QString clean = cleanPath(path);
    QString prefix = QStringLiteral("/") + getLibraryName();
    if (clean.startsWith(prefix)) {
        // Remove the library name from the path
        filePath = clean.right(clean.length() - prefix.length());
    }
    else {
        filePath = clean;
    }

    prefix = getMaterialDirectoryPath();
    if (filePath.startsWith(prefix, Qt::CaseInsensitive)) {
        // Remove the library root from the path
        filePath = filePath.right(filePath.length() - prefix.length());
    }

    // Remove any leading '/'
    if (filePath.startsWith(QStringLiteral("/"))) {
        filePath.remove(0, 1);
    }

    return filePath;
}

QString ManagedLibrary::getLibraryPath(const QString& path, const QString& filename) const
{
    QString filePath(cleanPath(path));
    if (filePath.endsWith(filename)) {
        filePath = filePath.left(filePath.length() - filename.length());
    }
    if (filePath.endsWith(QStringLiteral("/"))) {
        filePath = filePath.left(filePath.length() - 1);
    }

    return filePath;
}

std::string ManagedLibrary::cleanPath(const std::string path)
{
    QString clean = QDir::cleanPath(QString::fromStdString(path));
    return clean.toStdString();
}

QString ManagedLibrary::cleanPath(const QString& path)
{
    QString clean = QDir::cleanPath(path);
    return clean;
}
