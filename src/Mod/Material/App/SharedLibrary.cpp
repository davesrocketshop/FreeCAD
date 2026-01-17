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

#include <string>

#include <App/Application.h>

#include "Exceptions.h"
#include "SharedLibrary.h"


using namespace Materials;

TYPESYSTEM_SOURCE(Materials::SharedLibrary, Base::BaseClass)

SharedLibrary::SharedLibrary(
    const QString& repositoryName,
    const QString& libraryName,
    const QString& iconPath,
    bool readOnly
)
    : _repository(repositoryName)
    , _name(libraryName)
    , _readOnly(readOnly)
    , _disabled(false)
    , _local(false)
    , _module(false)
{
    setIcon(iconPath);
}

SharedLibrary::SharedLibrary(
    const QString& repositoryName,
    const QString& libraryName,
    const QByteArray& icon,
    bool readOnly
)
    : _repository(repositoryName)
    , _name(libraryName)
    , _icon(icon)
    , _readOnly(readOnly)
    , _disabled(false)
    , _local(false)
    , _module(false)
{}

SharedLibrary::SharedLibrary(
    const QString& repositoryName,
    const QString& libraryName,
    const QString& dir,
    const QString& iconPath,
    bool readOnly
)
    : _repository(repositoryName)
    , _name(libraryName)
    , _directory(cleanPath(dir))
    , _readOnly(readOnly)
    , _disabled(false)
    , _local(false)
    , _module(false)
{
    setIcon(iconPath);
}

QByteArray SharedLibrary::getIcon(const QString& iconPath)
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

void SharedLibrary::setIcon(const QString& iconPath)
{
    _iconPath = iconPath;
    _icon = getIcon(iconPath);
}

bool SharedLibrary::isLocal() const
{
    return _local;
}

void SharedLibrary::setLocal(bool local)
{
    _local = local;
}

bool SharedLibrary::isModule() const
{
    return _module;
}

void SharedLibrary::setModule(bool module)
{
    _module = module;
}

bool SharedLibrary::operator==(const SharedLibrary& library) const
{
    return (getName() == library.getName()) && (_directory == library._directory);
}

void SharedLibrary::validate(const SharedLibrary& remote) const
{
    if (getName() != remote.getName()) {
        throw InvalidLibrary("Library names don't match");
    }
    if (getIcon() != remote.getIcon()) {
        throw InvalidLibrary("Library icons don't match");
    }

    // Local and remote paths will differ
    if (!remote.getDirectory().isEmpty()) {
        throw InvalidLibrary("Remote library should not have a path");
    }

    if (isReadOnly() != remote.isReadOnly()) {
        throw InvalidLibrary("Library readonly settings don't match");
    }
}

QString SharedLibrary::getLocalPath(const QString& path) const
{
    QString filePath = getDirectoryPath();
    if (!(filePath.endsWith(QStringLiteral("/")) || filePath.endsWith(QStringLiteral("\\")))) {
        filePath += QStringLiteral("/");
    }

    QString clean = cleanPath(path);
    QString prefix = QStringLiteral("/") + getName();
    if (clean.startsWith(prefix)) {
        // Remove the library name from the path
        filePath += clean.right(clean.length() - prefix.length());
    }
    else {
        filePath += clean;
    }

    return filePath;
}

bool SharedLibrary::isRoot(const QString& path) const
{
    QString localPath = getLocalPath(cleanPath(path));
    QString clean = getLocalPath(QStringLiteral(""));
    return (clean == localPath);
}

QString SharedLibrary::getRelativePath(const QString& path) const
{
    QString filePath;
    QString clean = cleanPath(path);
    QString prefix = QStringLiteral("/") + getName();
    if (clean.startsWith(prefix)) {
        // Remove the library name from the path
        filePath = clean.right(clean.length() - prefix.length());
    }
    else {
        filePath = clean;
    }

    prefix = getDirectoryPath();
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

QString SharedLibrary::getLibraryPath(const QString& path, const QString& filename) const
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

std::string SharedLibrary::cleanPath(const std::string path)
{
    QString clean = QDir::cleanPath(QString::fromStdString(path));
    return clean.toStdString();
}

QString SharedLibrary::cleanPath(const QString& path)
{
    QString clean = QDir::cleanPath(path);
    return clean;
}
