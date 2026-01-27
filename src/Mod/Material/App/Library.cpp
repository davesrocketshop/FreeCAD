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
#include "Library.h"
#include "ManagedLibrary.h"


using namespace Materials;

TYPESYSTEM_SOURCE_ABSTRACT(Materials::Library, Base::BaseClass)

Library::Library()
{
    _managedLibrary = std::make_shared<ManagedLibrary>();
}

Library::Library(const std::shared_ptr<ManagedLibrary>& library)
    : _managedLibrary(library)
{}

Library::Library(const QString& libraryName, const QString& iconPath, bool readOnly)
{
    _managedLibrary = std::make_shared<ManagedLibrary>(libraryName, iconPath, readOnly);
}

Library::Library(const QString& libraryName, const QByteArray& icon, bool readOnly)
{
    _managedLibrary = std::make_shared<ManagedLibrary>(libraryName, icon, readOnly);
}

Library::Library(const QString& libraryName, const QString& dir, const QString& iconPath, bool readOnly)
{
    _managedLibrary = std::make_shared<ManagedLibrary>(libraryName, dir, iconPath, readOnly);
}

void Library::setIcon(const QString& iconPath)
{
    _managedLibrary->setIcon(iconPath);
}

bool Library::isLocal() const
{
    return _managedLibrary->isLocal();
}

void Library::setLocal(bool local)
{
    _managedLibrary->setLocal(local);
}

bool Library::isModule() const
{
    return _managedLibrary->isModule();
}

void Library::setModule(bool module)
{
    _managedLibrary->setModule(module);
}

bool Library::operator==(const Library& library) const
{
    return _managedLibrary->operator==(*library._managedLibrary);
}

void Library::validate(const Library& remote) const
{
    _managedLibrary->validate(*remote._managedLibrary);
}

QString Library::getLocalPath(const QString& directoryPath, const QString& path) const
{
    QString filePath = directoryPath;
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

QString Library::getRelativePath(const QString& path) const
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

QString Library::getLibraryPath(const QString& path, const QString& filename) const
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

std::string Library::cleanPath(const std::string path)
{
    return ManagedLibrary::cleanPath(path);
}

QString Library::cleanPath(const QString& path)
{
    return ManagedLibrary::cleanPath(path);
}
