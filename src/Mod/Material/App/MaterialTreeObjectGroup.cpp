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

#include "PreCompiled.h"

#ifndef _PreComp_
#include <iostream>
#endif

#include "MaterialTreeObject.h"
#include "MaterialTreeObjectGroup.h"

using namespace Materials;

// EXTENSION_PROPERTY_SOURCE(Materials::MaterialTreeObjectGroup, App::GroupExtension)
PROPERTY_SOURCE(Materials::MaterialTreeObjectGroup, App::DocumentObject)

MaterialTreeObjectGroup::MaterialTreeObjectGroup()
{
    App::GroupExtension::initExtension(this);
}

const char* MaterialTreeObjectGroup::getViewProviderName() const
{
    return "MatGui::ViewProviderMaterialTreeObjectGroup";
}

bool MaterialTreeObjectGroup::isAllowed(const App::DocumentObject* obj)
{
    if (!obj) {
        return false;
    }

    return obj->isDerivedFrom<Materials::MaterialTreeObject>();
}

std::vector<App::DocumentObject*> MaterialTreeObjectGroup::addObject(App::DocumentObject* feature)
{
    if (!isAllowed(feature)) {
        throw Base::ValueError("MaterialTreeObjectGroup: object is not allowed");
    }

    // only one group per object. If it is in a body the single feature will be removed
    auto* group = getGroupOfObject(feature);
    if (group && group != getExtendedObject()) {
        group->getExtensionByType<GroupExtension>()->removeObject(feature);
    }

    insertObject(feature, nullptr, false);

    std::vector<App::DocumentObject*> result = {feature};
    return result;
}

std::vector<App::DocumentObject*>
MaterialTreeObjectGroup::addObjects(std::vector<App::DocumentObject*> objs)
{

    for (auto obj : objs) {
        addObject(obj);
    }

    return objs;
}

void MaterialTreeObjectGroup::insertObject(App::DocumentObject* feature,
                                           App::DocumentObject* target,
                                           bool after)
{
    if (target && !hasObject(target)) {
        throw Base::ValueError("MaterialTreeObjectGroup: the feature we should insert relative to "
                               "is not part of that body");
    }

    // ensure that all origin links are ok
    // relinkToOrigin(feature);

    std::vector<App::DocumentObject*> model = Group.getValues();
    std::vector<App::DocumentObject*>::iterator insertInto;

    // Find out the position there to insert the feature
    if (!target) {
        if (after) {
            insertInto = model.begin();
        }
        else {
            insertInto = model.end();
        }
    }
    else {
        std::vector<App::DocumentObject*>::iterator targetIt =
            std::find(model.begin(), model.end(), target);
        assert(targetIt != model.end());
        if (after) {
            insertInto = targetIt + 1;
        }
        else {
            insertInto = targetIt;
        }
    }

    // Insert the new feature after the given
    model.insert(insertInto, feature);

    Group.setValues(model);
}

std::vector<App::DocumentObject*>
MaterialTreeObjectGroup::removeObject(App::DocumentObject* feature)
{
    std::vector<App::DocumentObject*> model = Group.getValues();
    std::vector<App::DocumentObject*>::iterator it = std::find(model.begin(), model.end(), feature);

    // Erase feature from Group
    if (it != model.end()) {
        model.erase(it);
        Group.setValues(model);
    }
    std::vector<App::DocumentObject*> result = {feature};
    return result;
}

// PyObject* MaterialTreeObjectGroup::getPyObject()
// {
//     if (PythonObject.is(Py::_None())) {
//         // ref counter is set to 1
//         PythonObject = Py::Object(new MaterialTreeObjectGroupPy(this), true);
//     }
//     return Py::new_reference_to(PythonObject);
// }
