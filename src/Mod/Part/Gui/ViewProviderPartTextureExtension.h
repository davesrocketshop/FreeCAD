// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 David Carter <dcarter@david.carter.ca>             *
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

#include <Gui/ViewProviderTextureExtension.h>

class SoCoordinate3;
class SoGroup;
class SoIndexedFaceSet;
class SoMaterial;
class SoSwitch;
class SoTexture2;
class SoTexture3;
class SoTextureCoordinateEnvironment;
class SoSeparator;
class SoSphere;

namespace App
{
class Material;
}

namespace PartGui
{

class PartGuiExport ViewProviderPartTextureExtension: public Gui::ViewProviderFaceTexture
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProviderFaceTexture);

public:
    /// Constructor
    ViewProviderPartTextureExtension();
    ~ViewProviderPartTextureExtension() override;
    void setup(SoMaterial*);

private:
    // Used to support per face textures
    // SoTexture3* pcShapeTexture3D;
    // SoCoordinate3* pcShapeCoordinates;
    // SoIndexedFaceSet* pcShapeFaceset;
};

}  // namespace Gui
