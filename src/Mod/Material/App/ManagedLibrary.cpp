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

#include <QDir>

#include <App/Application.h>
#include <Base/FileInfo.h>

#include "Exceptions.h"
#include "LibraryManager.h"
#include "ManagedLibrary.h"


using namespace Materials;

TYPESYSTEM_SOURCE(Materials::ManagedLibrary, Base::BaseClass)

ManagedLibrary::ManagedLibrary(const std::string& libraryName, const std::string& iconPath, bool readOnly)
    : _repositoryName(LibraryManager::RepositoryRemote)
    , _libraryName(libraryName)
    , _readOnly(readOnly)
    , _disabled(false)
    , _modelCaseSensitive(true)
    , _materialCaseSensitive(true)
    , _local(false)
    , _module(false)
{
    setIcon(iconPath);

    _modelPathMap = std::make_shared<std::map<std::string, std::shared_ptr<Model>>>();
}

ManagedLibrary::ManagedLibrary(const std::string& libraryName, const QByteArray& icon, bool readOnly)
    : _repositoryName(LibraryManager::RepositoryRemote)
    , _libraryName(libraryName)
    , _icon(icon)
    , _readOnly(readOnly)
    , _disabled(false)
    , _modelCaseSensitive(true)
    , _materialCaseSensitive(true)
    , _local(false)
    , _module(false)
{
    _modelPathMap = std::make_shared<std::map<std::string, std::shared_ptr<Model>>>();
}

ManagedLibrary::ManagedLibrary(
    const std::string& libraryName,
    const std::string& dir,
    const std::string& iconPath,
    bool readOnly
)
    : _repositoryName(LibraryManager::RepositoryRemote)
    , _libraryName(libraryName)
    , _materialDirectory(cleanPath(dir))
    , _readOnly(readOnly)
    , _disabled(false)
    , _modelCaseSensitive(true)
    , _materialCaseSensitive(true)
    , _local(false)
    , _module(false)
{
    setIcon(iconPath);
    setMaterialCaseSensitivity();

    _modelPathMap = std::make_shared<std::map<std::string, std::shared_ptr<Model>>>();
}

QByteArray ManagedLibrary::getIcon(const std::string& iconPath)
{
    QFile file(QString::fromStdString(iconPath));
    if (!file.open(QIODevice::ReadOnly)) {
        Base::Console().log("Failed to open icon file '%s'\n", iconPath.c_str());
        return QByteArray();  // Return an empty QByteArray if file opening fails
    }

    QByteArray data = file.readAll();
    file.close();
    return data;
}

void ManagedLibrary::setIcon(const std::string& iconPath)
{
    _iconPath = iconPath;
    _icon = getIcon(iconPath);
}

bool ManagedLibrary::isLocal() const
{
    return _local;
}

bool ManagedLibrary::isRemote() const
{
    return !_local;
}

void ManagedLibrary::setLocal(bool local)
{
    _local = local;
    if (local) {
        setRepositoryName(LibraryManager::RepositoryLocal);
    }
    else {
        setRepositoryName(LibraryManager::RepositoryRemote);
    }
}

bool ManagedLibrary::isModule() const
{
    return _module;
}

void ManagedLibrary::setModule(bool module)
{
    _module = module;
}

std::string ManagedLibrary::getMaterialDirectoryPath() const
{
    return cleanPath(_materialDirectory);
}

std::string ManagedLibrary::getModelDirectoryPath() const
{
    return cleanPath(_modelDirectory);
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
    if (!remote.getMaterialDirectory().empty()) {
        throw InvalidLibrary("Remote library should not have a material path");
    }

    // Local and remote paths will differ
    if (!remote.getModelDirectory().empty()) {
        throw InvalidLibrary("Remote library should not have a model path");
    }

    if (isReadOnly() != remote.isReadOnly()) {
        throw InvalidLibrary("Library readonly settings don't match");
    }
}

std::string ManagedLibrary::getLocalPath(const std::string& path) const
{
    std::string filePath = getMaterialDirectoryPath();
    if (!(filePath.ends_with("/") || filePath.ends_with("\\"))) {
        filePath += "/";
    }

    std::string clean = cleanPath(path);
    std::string prefix = "/" + getLibraryName();
    if (clean.starts_with(prefix)) {
        // Remove the library name from the path
        filePath += clean.erase(clean.length() - prefix.length());
    }
    else {
        filePath += clean;
    }

    return filePath;
}

bool ManagedLibrary::isRoot(const std::string& path) const
{
    std::string localPath = getLocalPath(cleanPath(path));
    std::string clean = getLocalPath("");
    return (clean == localPath);
}

std::string ManagedLibrary::getRelativePath(const std::string& path) const
{
    std::string filePath;
    std::string clean = cleanPath(path);
    std::string prefix = "/" + getLibraryName();
    if (clean.starts_with(prefix)) {
        // Remove the library name from the path
        filePath = clean.erase(clean.length() - prefix.length());
    }
    else {
        filePath = clean;
    }

    prefix = getMaterialDirectoryPath();
    if (filePath.starts_with(prefix)) {
        // Remove the library root from the path
        filePath = filePath.erase(filePath.length() - prefix.length());
    }

    // Remove any leading '/'
    if (filePath.starts_with("/")) {
        filePath.erase(0, 1);
    }

    return filePath;
}

std::string ManagedLibrary::getLibraryPath(const std::string& path, const std::string& filename) const
{
    std::string filePath(cleanPath(path));
    if (filePath.ends_with(filename)) {
        filePath = filePath.erase(0, filePath.length() - filename.length());
    }
    if (filePath.ends_with("/")) {
        filePath = filePath.erase(0, filePath.length() - 1);
    }

    return filePath;
}

std::string ManagedLibrary::cleanPath(const std::string path)
{
    // QDir just does it better and simpler
    QString clean = QDir::cleanPath(QString::fromStdString(path));
    return clean.toStdString();
}

QString ManagedLibrary::cleanPath(const QString& path)
{
    QString clean = QDir::cleanPath(path);
    return clean;
}

void ManagedLibrary::setMaterialDirectory(const std::string& directory)
{
    _materialDirectory = cleanPath(directory);
    setMaterialCaseSensitivity();
}

void ManagedLibrary::setModelDirectory(const std::string& directory)
{
    _modelDirectory = cleanPath(directory);
    setModelCaseSensitivity();
}

void ManagedLibrary::setModelCaseSensitivity()
{
    _modelCaseSensitive = isPathCaseSensitive(_modelDirectory);
}

void ManagedLibrary::setMaterialCaseSensitivity()
{
    _materialCaseSensitive = isPathCaseSensitive(_materialDirectory);
}

bool ManagedLibrary::isPathCaseSensitive(const std::string& path) const
{
    bool caseSensitive = true;
    auto qPath = QString::fromStdString(path);
    if (QDir(qPath).exists()) {
        auto upper = qPath.toUpper();
        auto lower = qPath.toLower();
        if ((qPath != upper) && QDir(upper).exists()) {
            caseSensitive = false;
        }
        else if ((qPath != lower) && QDir(lower).exists()) {
            caseSensitive = false;
        }
    }
    return caseSensitive;
}

bool ManagedLibrary::ciModelStartsWith(const std::string& path, const std::string& prefix) const
{
    return ciStartsWith(path, prefix, _modelCaseSensitive);
}

bool ManagedLibrary::ciMaterialStartsWith(const std::string& path, const std::string& prefix) const
{
    return ciStartsWith(path, prefix, _materialCaseSensitive);
}

bool ManagedLibrary::ciStartsWith(const std::string& path, const std::string& prefix, bool caseSensitive) const
{
    if (caseSensitive) {
        return path.starts_with(prefix);
    }
    else {
        return QString::fromStdString(path).startsWith(
                    QString::fromStdString(prefix),
                    Qt::CaseInsensitive
                );
    }
}
