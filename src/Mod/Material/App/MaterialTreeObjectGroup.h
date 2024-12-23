/****************************************************************************
 *   Copyright (c) 2024 David Carter <dcarter@david.carter.ca>              *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#ifndef MATERIAL_MATERIALTREEOBJECTGROUP_H
#define MATERIAL_MATERIALTREEOBJECTGROUP_H

#include <App/DocumentObject.h>
#include <App/GroupExtension.h>

#include <Mod/Material/MaterialGlobal.h>

#include "PropertyMaterial.h"

namespace Materials
{

/** A GroupExtension class to support material tree views
 */
class MaterialsExport MaterialTreeObjectGroup: public App::DocumentObject, public App::GroupExtension
{
    // using inherited = App::GroupExtension;
    // EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(Materials::MaterialTreeObjectGroup);
    PROPERTY_HEADER_WITH_OVERRIDE(Materials::MaterialTreeObjectGroup);

public:
    MaterialTreeObjectGroup();
    ~MaterialTreeObjectGroup() override = default;

    PropertyMaterial MaterialFeature;

    const char* getViewProviderName() const override;

    /**
     * Add the feature into the group
     */
    std::vector<App::DocumentObject*> addObject(App::DocumentObject* feature) override;
    std::vector<DocumentObject*> addObjects(std::vector<DocumentObject*> obj) override;

    void
    insertObject(App::DocumentObject* feature, App::DocumentObject* target, bool after = false);

    /// Remove the feature from the group
    std::vector<DocumentObject*> removeObject(DocumentObject* obj) override;

    /**
     * Return true if the given feature is allowed in the group
     */
    static bool isAllowed(const App::DocumentObject* obj);
    bool allowObject(DocumentObject* obj) override
    {
        return isAllowed(obj);
    }

    // PyObject* getPyObject() override;

    void setMaterial(const PropertyMaterial* property);
};

}  // namespace Materials

#endif  // MATERIAL_MATERIALTREEOBJECTGROUP_H
