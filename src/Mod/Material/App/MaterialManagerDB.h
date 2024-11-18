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

#ifndef MATERIAL_MATERIALMANAGERDB_H
#define MATERIAL_MATERIALMANAGERDB_H

#include <lru/lru.hpp>
#include <memory>

#include <Mod/Material/MaterialGlobal.h>

// #include "FolderTree.h"
// #include "Materials.h"

// #include "MaterialLibrary.h"
// #include "MaterialFilter.h"

class QMutex;

namespace App
{
class Material;
}

namespace Materials
{
// Forward declarations
class Material;

class MaterialsExport MaterialManagerDB: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    MaterialManagerDB();
    ~MaterialManagerDB() override = default;

    static void refresh();

    std::shared_ptr<Material> getMaterial(const QString& uuid) const;

    // Cache stats
    static double materialHitRate();

private:
    static void initCache();

    static QMutex _mutex;
    static LRU::Cache<QString, std::shared_ptr<Material>> _cache;
};

}  // namespace Materials

#endif  // MATERIAL_MATERIALMANAGERDB_H
