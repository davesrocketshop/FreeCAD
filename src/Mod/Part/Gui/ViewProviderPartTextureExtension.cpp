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


#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTexture3.h>
#include <Inventor/nodes/SoTextureCoordinateEnvironment.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSphere.h>


#include "ViewProviderPartTextureExtension.h"
// #include <App/Material.h>
// #include <Base/Console.h>
// #include <Gui/BitmapFactory.h>
// #include <Gui/ViewProviderDocumentObject.h>

using namespace PartGui;

EXTENSION_PROPERTY_SOURCE(PartGui::ViewProviderPartTextureExtension, Gui::ViewProviderFaceTexture)


ViewProviderPartTextureExtension::ViewProviderPartTextureExtension()
{
    initExtensionType(ViewProviderPartTextureExtension::getExtensionClassTypeId());

    // Support for textured faces
    // pcShapeTexture3D = new SoTexture3;
    // pcShapeTexture3D->ref();
    // pcShapeTexture3D->setName("ShapeTexture3D");
    // pcShapeCoordinates = new SoCoordinate3;
    // pcShapeCoordinates->ref();
    // pcShapeCoordinates->setName("ShapeCoordinates");
    // pcShapeFaceset = new SoIndexedFaceSet;
    // pcShapeFaceset->ref();
    // pcShapeFaceset->setName("ShapeFaceset");
}

ViewProviderPartTextureExtension::~ViewProviderPartTextureExtension()
{
    // pcShapeTexture3D->unref();
    // pcShapeCoordinates->unref();
    // pcShapeFaceset->unref();
}

void ViewProviderPartTextureExtension::setup(SoMaterial* mat)
{
    Gui::ViewProviderFaceTexture::setup(mat);

    // getTextureGroup3D()->addChild(pcShapeTexture3D);
    // getTextureGroup3D()->addChild(pcShapeCoordinates);
    // getTextureGroup3D()->addChild(pcShapeFaceset);
}
