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

#include "Materials.h"
#include "MaterialTreeObject.h"

using namespace Materials;

const char* MaterialTreeObject::RoleEnums[] = {"Path", "UUID", "Inherits", nullptr};

PROPERTY_SOURCE(Materials::MaterialTreeObject, App::DocumentObject)

MaterialTreeObject::MaterialTreeObject()
{
    ADD_PROPERTY(Role, ((long)0));
    Role.setEnums(RoleEnums);
}

const char* MaterialTreeObject::getViewProviderName() const
{
    return "MatGui::ViewProviderMaterialTreeObject";
}

void MaterialTreeObject::onChanged(const App::Property* prop)
{
    if (prop == &Role) {
        if (Role.getValue() == 0) {
            Label.setValue("Path:");
        }
        else if (Role.getValue() == 1) {
            Label.setValue("UUID:");
        }
        else if (Role.getValue() == 2) {
            Label.setValue("Inherits:");
        }
    }
}

void MaterialTreeObject::setMaterial(const Material& material)
{
    Base::Console().Log("MaterialTreeObject::setMaterial('%s')\n",
                        material.getName().toStdString().c_str());
    if (Role.getValue() == 0) {
        QString label = QLatin1String("Path: ") + material.getDirectory() + QLatin1String("/")
            + material.getName();
        Label.setValue(label.toStdString());
    }
    else if (Role.getValue() == 1) {
        QString label = QLatin1String("UUID: ") + material.getUUID();
        Label.setValue(label.toStdString());
    }
    else if (Role.getValue() == 2) {
        QString label = QLatin1String("Inherits: ") + material.getParentUUID();
        Label.setValue(label.toStdString());
    }
}
