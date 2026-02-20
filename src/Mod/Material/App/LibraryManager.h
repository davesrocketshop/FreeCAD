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
#include "ExternalManager.h"
#include "ManagedLibrary.h"
#include "MaterialLibrary.h"
#include "ModelLibrary.h"

namespace Materials
{

enum LibraryEventType
{
    LibraryEventType_Create,
    LibraryEventType_Rename,
    LibraryEventType_Remove,
    LibraryEventType_IconChange
};

struct LibraryEvent {
    std::shared_ptr<ManagedLibrary> library;
    LibraryEventType eventType;
};

class MaterialsExport LibraryManager
    : public Base::BaseClass
    , public ParameterGrp::ObserverType
    , public Base::Subject<LibraryEvent>
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

    std::shared_ptr<std::vector<std::shared_ptr<ManagedLibrary>>> getLibraries(
        bool includeDisabled = false
    );
    std::shared_ptr<std::vector<std::shared_ptr<ModelLibrary>>> getModelLibraries(
        bool includeDisabled = false
    );
    std::shared_ptr<std::vector<std::shared_ptr<MaterialLibrary>>> getMaterialLibraries(
        bool includeDisabled = false
    );
    std::shared_ptr<std::vector<std::shared_ptr<ManagedLibrary>>> getLocalLibraries(
        bool includeDisabled = false
    );
    std::shared_ptr<std::vector<std::shared_ptr<ModelLibrary>>> getLocalModelLibraries(
        bool includeDisabled = false
    );
    std::shared_ptr<std::vector<std::shared_ptr<MaterialLibrary>>> getLocalMaterialLibraries(
        bool includeDisabled = false
    );
#if defined(BUILD_MATERIAL_EXTERNAL)
    std::shared_ptr<std::vector<std::shared_ptr<ManagedLibrary>>> getRemoteLibraries(
        bool includeDisabled = false
    );
    std::shared_ptr<std::vector<std::shared_ptr<ModelLibrary>>> getRemoteModelLibraries(
        bool includeDisabled = false
    );
    std::shared_ptr<std::vector<std::shared_ptr<MaterialLibrary>>> getRemoteMaterialLibraries(
        bool includeDisabled = false
    );
#endif
    std::shared_ptr<ManagedLibrary> getLibrary(
        const std::string& repositoryName,
        const std::string& name
    ) const;
    std::shared_ptr<ManagedLibrary> getLibrary(
        const std::string& name
    ) const;
    std::shared_ptr<ModelLibrary> getModelLibrary(
        const std::string& repositoryName,
        const std::string& name
    ) const;
    std::shared_ptr<ModelLibrary> getModelLibrary(
        const std::string& name
    ) const;
    std::shared_ptr<MaterialLibrary> getMaterialLibrary(
        const std::string& repositoryName,
        const std::string& name
    ) const;
    std::shared_ptr<MaterialLibrary> getMaterialLibrary(
        const std::string& name
    ) const;

    // These are the 'Default' locations used for saving and other operations
    // when no library is explicitly specified
    std::shared_ptr<ManagedLibrary> getDefaultLibrary() const;
    std::shared_ptr<ModelLibrary> getDefaultModelLibrary() const;
    std::shared_ptr<MaterialLibrary> getDefaultMaterialLibrary() const;

    void createRemoteLibrary(
        const std::string& repositoryName,
        const std::string& libraryName,
        const std::string& iconPath,
        bool readOnly
    );
    void createRemoteLibrary(
        const std::string& repositoryName,
        const std::string& libraryName,
        const char* iconPath,
        bool readOnly
    )
    {
        createRemoteLibrary(repositoryName, libraryName, std::string(iconPath), readOnly);
    }
    void createRemoteLibrary(
        const std::string& repositoryName,
        const std::string& libraryName,
        const QByteArray& icon,
        bool readOnly
    );
    std::shared_ptr<MaterialLibrary> createLocalLibrary(
        const std::string& libraryName,
        const std::string& materialDirectory,
        const std::string& modelDirectory,
        const std::string& iconPath,
        bool readOnly
    );

    void renameLibrary(
        const std::string& repositoryName,
        const std::string& libraryName,
        const std::string& newName
    );
    void renameLibrary(
        const std::string& libraryName,
        const std::string& newName
    );
    void renameLibrary(
        const std::shared_ptr<ManagedLibrary>& library,
        const std::string& newName
    );
    void changeIcon(
        const std::string& repositoryName,
        const std::string& libraryName,
        const std::string& iconPath
    );
    void changeIcon(
        const std::string& libraryName,
        const std::string& iconPath
    );
    void changeIcon(
        const std::shared_ptr<ManagedLibrary>& library,
        const std::string& iconPath
    );
    void removeLibrary(const std::string& repositoryName, const std::string& libraryName);
    void removeLibrary(const std::string& libraryName);
    bool isLocalLibrary(const std::string& repositoryName, const std::string& libraryName);
    bool isLocalLibrary(const std::string& libraryName);

    void setDisabled(const std::string& repositoryName, const std::string& libraryName, bool disabled);
    void setDisabled(const std::string& libraryName, bool disabled);
    void setDisabled(Library& library, bool disabled);
    void setDisabled(const std::shared_ptr<ManagedLibrary>& library, bool disabled);
    bool isDisabled(const std::string& repositoryName, const std::string& libraryName) const;
    bool isDisabled(const Library& library) const;
    bool isDisabled(const std::shared_ptr<ManagedLibrary>& library) const;

    /// Observer message from the ParameterGrp
    void OnChange(ParameterGrp::SubjectType& rCaller, ParameterGrp::MessageType Reason) override;

    static std::shared_ptr<std::vector<std::shared_ptr<ManagedLibrary>>> getConfiguredLibraries(
        bool includeDisabled = false
    );

    static const std::string RepositoryLocal;
    static const std::string RepositoryRemote;

private:
    LibraryManager();

    FC_DISABLE_COPY_MOVE(LibraryManager);

    static void initManagers();
    static void convertConfiguration();

    static void createSystemLibraryConfig();
    static void createUserLibraryConfig();

    const char* getResourceRoot(const std::shared_ptr<ManagedLibrary>& library) const;
    static const char* getResourceRootLocal();
    static const char* getResourceRootModules();
    static const char* getResourceRootRemote();

    void updateLibraries();
    void updateLibraryMap();

    void renameLibraryLocal(const std::shared_ptr<ManagedLibrary>& library, const std::string& newName);
    void renameLibraryRemote(const std::shared_ptr<ManagedLibrary>& library, const std::string& newName);
    void changeIconLocal(const std::shared_ptr<ManagedLibrary>& library, const std::string& iconPath);
    void changeIconRemote(const std::shared_ptr<ManagedLibrary>& library, const std::string& iconPath);
    void removeLibraryLocal(const std::shared_ptr<ManagedLibrary>& library);
    void removeLibraryRemote(const std::shared_ptr<ManagedLibrary>& library);

#if defined(BUILD_MATERIAL_EXTERNAL)
    static ExternalManager* externalManager()
    {
        return ExternalManager::getManager();
    }
#endif


    static LibraryManager* _manager;
    static QMutex _mutex;
    static bool _useExternal;
    static std::shared_ptr<std::list<std::shared_ptr<ManagedLibrary>>> _libraryList;
    static std::shared_ptr<std::multimap<std::string, std::shared_ptr<ManagedLibrary>>> _libraryMap;

    ParameterGrp::handle _hGrp;
};

}  // namespace Materials

#endif  // MATERIAL_LIBRARYMANAGER_H