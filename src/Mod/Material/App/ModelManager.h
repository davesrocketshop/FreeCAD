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

#ifndef MATERIAL_MODELMANAGER_H
#define MATERIAL_MODELMANAGER_H

#include <memory>

#include <Base/Parameter.h>
#include <Mod/Material/MaterialGlobal.h>

#include <QMutex>

#include "Exceptions.h"
#include "FolderTree.h"
#include "LibraryManager.h"
#include "Model.h"
#include "ModelLibrary.h"

namespace Materials
{
class ModelManagerLocal;
class ModelManagerExternal;
class MaterialManagerLocal;

class MaterialsExport ModelManager: public Base::BaseClass, ParameterGrp::ObserverType, LibraryManager::ObserverType
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    ~ModelManager() override;

    static ModelManager& getManager();

    static void cleanup();
    void refresh();

    // Library management
    std::shared_ptr<std::vector<std::shared_ptr<ModelLibrary>>> getLibraries(bool includeDisabled = false);
    std::shared_ptr<std::vector<std::shared_ptr<ModelLibrary>>> getLocalLibraries(bool includeDisabled = false);
    std::shared_ptr<ModelLibrary> getLibrary(const std::string& name) const;
    std::shared_ptr<std::vector<LibraryObject>>
    libraryModels(const std::string& libraryName);

    // Folder management

    // Tree management
    std::shared_ptr<std::map<std::string, std::shared_ptr<ModelTreeNode>>>
    getModelTree(std::shared_ptr<ModelLibrary> library, ModelFilter filter = ModelFilter_None) const
    {
        return library->getModelTree(filter);
    }

    // Model management
    std::shared_ptr<std::map<std::string, std::shared_ptr<Model>>> getModels();
    std::shared_ptr<std::map<std::string, std::shared_ptr<Model>>> getLocalModels();
    std::shared_ptr<Model> getModel(const std::string& uuid) const;
    std::shared_ptr<Model> getModel(const std::string& libraryName, const std::string& uuid) const;
    std::shared_ptr<Model> getModelByPath(const std::string& path) const;
    std::shared_ptr<Model> getModelByPath(const std::string& path, const std::string& lib) const;

    static void dereference(Model& model);
    static void dereference(const std::shared_ptr<Model>& model);

    static bool isModel(const std::string& file);
    static bool passFilter(ModelFilter filter, Model::ModelType modelType);

    /// Observer message from the ParameterGrp
    void OnChange(ParameterGrp::SubjectType& rCaller, ParameterGrp::MessageType Reason) override;
    /// Observer message from the LibraryManager
    void OnChange(LibraryManager::SubjectType& manager, LibraryManager::MessageType reason) override;

#if defined(BUILD_MATERIAL_EXTERNAL)
    void migrateToExternal(const std::shared_ptr<Materials::ModelLibrary>& library);
    void validateMigration(const std::shared_ptr<Materials::ModelLibrary>& library);

    // Cache functions
    static void resetCache();
    static double modelHitRate();
#endif

protected:

private:
    ModelManager();

    FC_DISABLE_COPY_MOVE(ModelManager);

    static void initManagers();

    static ModelManager* _manager;
    static std::unique_ptr<ModelManagerLocal> _localManager;
#if defined(BUILD_MATERIAL_EXTERNAL)
    static std::unique_ptr<ModelManagerExternal> _externalManager;
#endif
    static QMutex _mutex;
    static bool _useExternal;

    ParameterGrp::handle _hGrp;
};

}  // namespace Materials

#endif  // MATERIAL_MODELMANAGER_H