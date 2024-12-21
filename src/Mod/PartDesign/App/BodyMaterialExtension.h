/***************************************************************************
 *   Copyright (c) 2024 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef PARTDESIGN_BODYMATERIALEXTENSION_H
#define PARTDESIGN_BODYMATERIALEXTENSION_H

#include <App/DocumentObjectExtension.h>
#include <App/ExtensionPython.h>

#include <Mod/PartDesign/PartDesignGlobal.h>

namespace PartDesign
{

/**
 * Represents the material status of an object
 */
class PartDesignExport BodyMaterialExtension: public App::DocumentObjectExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::BodyMaterialExtension);

public:
    BodyMaterialExtension();
    ~BodyMaterialExtension() override;

    /// Returns true on changing OriginFeature set
    short extensionMustExecute() override;

    App::DocumentObject* getMaterialTreeObject();

protected:
    /// Checks integrity
    App::DocumentObjectExecReturn* extensionExecute() override;

    void extensionOnChanged(const App::Property* property) override;

    /// Creates the corresponding MaterialTreeObject
    void onExtendedSetupObject() override;
    /// Removes the MaterialTreeObject
    void onExtendedUnsetupObject() override;

private:
    App::DocumentObject* getMaterialTreeObject(App::Document* doc);

    App::DocumentObject* _materialTreeObject;
};

using BodyMaterialExtensionPython = App::ExtensionPythonT<BodyMaterialExtension>;

}  // namespace PartDesign

#endif  // PARTDESIGN_BODYMATERIALEXTENSION_H
