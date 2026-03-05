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

#pragma once

#include <memory>

#include <Base/BaseClass.h>
#include <Base/Quantity.h>

#include <Mod/Material/MaterialGlobal.h>

#include "Library.h"
#include "MaterialValue.h"
#include "Model.h"

namespace Materials
{

class LibraryManager;

class MaterialsExport ModelLibrary: public Library,
                                    public std::enable_shared_from_this<ModelLibrary>
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    ModelLibrary(const std::shared_ptr<ManagedLibrary>& library);
    ModelLibrary(const Library& library);
    ~ModelLibrary() override = default;

    bool isRoot(const std::string& path) const override;
    std::string getDirectory() const override
    {
        return getModelDirectory();
    }
    std::string getDirectoryPath() const override
    {
        return getModelDirectoryPath();
    }
    std::string getLocalPath(const std::string& path) const;

    std::shared_ptr<std::map<std::string, std::shared_ptr<ModelTreeNode>>>
    getModelTree(ModelFilter filter) const;

    // Use this to get a shared_ptr for *this
    std::shared_ptr<ModelLibrary> getptr()
    {
        return shared_from_this();
    }

protected:
    ModelLibrary();
    // ModelLibrary(
    //     const std::string& libraryName,
    //     const std::string& dir,
    //     const std::string& iconPath,
    //     bool readOnly = true
    // );

    friend class LibraryManager;
};

class MaterialsExport ModelLibraryLocal: public ModelLibrary
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    ModelLibraryLocal(const std::shared_ptr<ManagedLibrary>& library);
    ModelLibraryLocal(const Library& other);
    ModelLibraryLocal(const ModelLibraryLocal& other) = default;
    ~ModelLibraryLocal() override = default;

    bool operator==(const ModelLibrary& library) const
    {
        return Library::operator==(library);
    }
    bool operator!=(const ModelLibrary& library) const
    {
        return !operator==(library);
    }
    std::shared_ptr<Model> getModelByPath(const std::string& path) const;

    std::shared_ptr<Model> addModel(const Model& model, const std::string& path);

private:
    ModelLibraryLocal();
};

}  // namespace Materials

Q_DECLARE_METATYPE(std::shared_ptr<Materials::ModelLibrary>)
Q_DECLARE_METATYPE(std::shared_ptr<Materials::ModelLibraryLocal>)