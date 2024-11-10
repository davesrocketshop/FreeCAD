/***************************************************************************
 *   Copyright (c) 2023-2024 David Carter <dcarter@david.carter.ca>        *
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

#ifndef PART_MATERIALGROUPEXTENSION_H
#define PART_MATERIALGROUPEXTENSION_H

#include <App/GroupExtension.h>
#include <QCoreApplication>

namespace Part {
class FeatureMaterialRoot;

/**
 * Represents an abstract placeable group of objects with an associated Origin
 */
class PartExport MaterialGroupExtension: public App::GroupExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(App::MaterialGroupExtension);
    Q_DECLARE_TR_FUNCTIONS(App::MaterialGroupExtension)

public:
    MaterialGroupExtension ();
    ~MaterialGroupExtension () override;

    /// Returns the origin link or throws an exception
    Part::FeatureMaterialRoot *getMaterialRoot () const;

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName () const {
        return "PartGui::ViewProviderMaterialGroup";
    }

    /**
     * Returns the FeatureMaterialRoot group which contains this object.
     * In case this object is not part of any geoFeatureGroup, 0 is returned.
     * @param obj       the object to search for
     */
    static App::DocumentObject* getGroupOfObject(const App::DocumentObject* obj);

    /// Returns true on changing FeatureMaterial set
    short extensionMustExecute () override;

    /// Origin linked to the group
    App::PropertyLink MaterialFeatures;

    // changes all links of obj to a origin to point to this groups origin
    void relinkToMaterial(App::DocumentObject* obj);

    std::vector<App::DocumentObject*> addObjects(std::vector<App::DocumentObject*> obj) override;
    bool hasObject(const App::DocumentObject* obj, bool recursive = false) const override;

    bool extensionGetSubObject(App::DocumentObject*& ret,
                               const char* subname,
                               PyObject** pyObj,
                               Base::Matrix4D* mat,
                               bool transform,
                               int depth) const override;

    void extensionOnChanged(const App::Property* p) override;

protected:
    /// Checks integrity of the Origin
    App::DocumentObjectExecReturn *extensionExecute () override;
    /// Creates the corresponding Origin object
    void onExtendedSetupObject () override;
    /// Removes all planes and axis if they are still linked to the document
    void onExtendedUnsetupObject () override;

private:
    /// Creates a localized Origin object
    App::DocumentObject *getLocalizedOrigin(App::Document *doc);
};

using MaterialGroupExtensionPython = App::ExtensionPythonT<App::GroupExtensionPythonT<MaterialGroupExtension>>;

}  // namespace Part

#endif  // PART_MATERIALGROUPEXTENSION_H
