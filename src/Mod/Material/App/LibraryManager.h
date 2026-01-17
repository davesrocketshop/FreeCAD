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

#ifndef MATERIAL_LIBRARYMANAGER_H
#define MATERIAL_LIBRARYMANAGER_H

#include <memory>

#include <Base/Parameter.h>
#include <Mod/Material/MaterialGlobal.h>

#include <QMutex>

#include "Exceptions.h"
#include "FolderTree.h"
#include "Model.h"
#include "MaterialLibrary.h"
#include "ModelLibrary.h"
#include "SharedLibrary.h"

namespace Materials
{
class ModelManagerLocal;
class ModelManagerExternal;
class MaterialManagerLocal;

class MaterialsExport LibraryManager: public Base::BaseClass, ParameterGrp::ObserverType
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    ~LibraryManager() override;

    static LibraryManager& getManager();

    static void cleanup();
    void refresh();

    // Library management
    bool useExternal() const
    {
        return _useExternal;
    }
    void setUseExternal(bool useExternal);

    std::shared_ptr<std::list<std::shared_ptr<ModelLibrary>>> getLibraries(
        bool includeDisabled = false
    );
    std::shared_ptr<std::list<std::shared_ptr<ModelLibrary>>> getLocalLibraries(
        bool includeDisabled = false
    );
    std::shared_ptr<ModelLibrary> getModelLibrary(
        const QString& repositoryName,
        const QString& name
    ) const;
    std::shared_ptr<MaterialLibrary> getMaterialLibrary(
        const QString& repositoryName,
        const QString& name
    ) const;
    void createRemoteLibrary(
        const QString& repositoryName,
        const QString& libraryName,
        const QString& iconPath,
        bool readOnly
    );
    void createLocalLibrary(
        const QString& repositoryName,
        const QString& libraryName,
        const QString& materialDirectory,
        const QString& modelDirectory,
        const QString& icon,
        bool readOnly
    );
    void registerLibrary(
        const QString& repositoryName,
        const QString& libraryName,
        const QString& materialDirectory,
        const QString& modelDirectory,
        const QString& icon,
        bool readOnly,
        bool disabled
    ) {}
    void unregisterLibrary(
        const QString& repositoryName,
        const QString& libraryName
    ) {}

    void renameLibrary(const QString& repositoryName, const QString& libraryName, const QString& newName);
    void changeIcon(const QString& repositoryName, const QString& libraryName, const QString& icon);
    void removeLibrary(const QString& repositoryName, const QString& libraryName);
    bool isLocalLibrary(const QString& repositoryName, const QString& libraryName);

    /// Observer message from the ParameterGrp
    void OnChange(ParameterGrp::SubjectType& rCaller, ParameterGrp::MessageType Reason) override;

    static void createSystemLibraryConfig();
    static void createUserLibraryConfig();
    static std::shared_ptr<std::list<std::shared_ptr<SharedLibrary>>> getConfiguredLibraries(
        bool includeDisabled = false
    );

protected:
    void setDisabled(const QString& repositoryName, Library& library, bool disabled);

    friend class MaterialManager;

private:
    LibraryManager();

    FC_DISABLE_COPY_MOVE(LibraryManager);

    static void initManagers();
    static void convertConfiguration();

    static LibraryManager* _manager;
    static QMutex _mutex;
    static bool _useExternal;
    static std::shared_ptr<std::list<std::shared_ptr<SharedLibrary>>> _libraryList;

    ParameterGrp::handle _hGrp;
};

}  // namespace Materials

#endif  // MATERIAL_LIBRARYMANAGER_H