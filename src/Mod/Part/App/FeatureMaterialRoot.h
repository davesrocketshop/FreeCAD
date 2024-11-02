/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
 *   Copyright (c) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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


#ifndef PART_FEATUREMATERIALROOT_H
#define PART_FEATUREMATERIALROOT_H

#include <App/GeoFeatureGroupExtension.h>
#include "FeatureMaterial.h"

#include "QCoreApplication"

namespace Part
{

/** Base class of all geometric document objects.
 */
class PartExport FeatureMaterialRoot: public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(App::FeatureMaterialRoot);
    Q_DECLARE_TR_FUNCTIONS(App::FeatureMaterialRoot)

public:
    /// Constructor
    FeatureMaterialRoot();
    ~FeatureMaterialRoot() override;

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override {
        return "PartGui::ViewProviderFeatureMaterialRoot";
    }

    /// Returns true if the given object is part of the FeatureMaterialRoot
    bool hasObject (const DocumentObject *obj) const;

    /// Returns true on changing FeatureMaterial set
    short mustExecute() const override;

    /// Returns an axis by it's name
    Part::FeatureMaterial* getFeatureMaterial(const char* role) const;

    /// Axis types
    static constexpr const char* AxisRoles[3] = {"X_Axis", "Y_Axis", "Z_Axis"};
    /// Baseplane types
    static constexpr const char* PlaneRoles[3] = {"XY_Plane", "XZ_Plane", "YZ_Plane"};

    // Axis links
    App::PropertyLinkList MaterialFeatures;

protected:
    /// Checks integrity of the FeatureMaterialRoot
    App::DocumentObjectExecReturn *execute() override;
    /// Creates all corresponding Axes and Planes objects for the FeatureMaterialRoot if they aren't linked yet
    void setupObject () override;
    /// Removes all planes and axis if they are still linked to the document
    void unsetupObject () override;

private:
    struct SetupData;
    void setupFeatureMaterialRoot(App::PropertyLink &featProp, const SetupData &data);

    class FeatureMaterialRootExtension : public App::GeoFeatureGroupExtension {
        FeatureMaterialRoot* obj;
    public:
        explicit FeatureMaterialRootExtension(FeatureMaterialRoot* obj);
        void initExtension(ExtensionContainer* obj) override;
        bool extensionGetSubObject(DocumentObject *&ret, const char *subname,
                PyObject **, Base::Matrix4D *, bool, int) const override;
    };
    FeatureMaterialRootExtension extension;
};

}  // namespace Part

#endif  // PART_FEATUREMATERIALROOT_H
