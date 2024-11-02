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

#include "PreCompiled.h"

#ifndef _PreComp_
# include <string>
#endif

#include <App/Document.h>
#include <Base/Exception.h>
#include <Base/Placement.h>

#include "FeatureMaterialRoot.h"
#include "FeatureMaterial.h"


#ifndef M_PI
# define M_PI       3.14159265358979323846
#endif

using namespace Part;


PROPERTY_SOURCE(Part::FeatureMaterialRoot, App::DocumentObject)

FeatureMaterialRoot::FeatureMaterialRoot()
    : extension(this)
{
    ADD_PROPERTY_TYPE(MaterialFeatures, (nullptr), 0, App::Prop_Hidden, "Material descriptions");

    setStatus(App::NoAutoExpand,true);
    extension.initExtension(this);
}


FeatureMaterialRoot::~FeatureMaterialRoot() = default;

FeatureMaterial* FeatureMaterialRoot::getFeatureMaterial(const char* role) const
{
    const auto & features = MaterialFeatures.getValues ();
    auto featIt = std::find_if (features.begin(), features.end(),
            [role] (App::DocumentObject *obj) {
                return obj->isDerivedFrom ( Part::FeatureMaterial::getClassTypeId () ) &&
                    strcmp (static_cast<Part::FeatureMaterial *>(obj)->Role.getValue(), role) == 0;
            } );
    if (featIt != features.end()) {
        return static_cast<Part::FeatureMaterial *>(*featIt);
    } else {

        std::stringstream err;
        err << "Origin \"" << getFullName () << "\" doesn't contain feature with role \""
            << role << '"';
        throw Base::RuntimeError ( err.str().c_str () );
    }
}

bool FeatureMaterialRoot::hasObject (const DocumentObject *obj) const {
    const auto & features = MaterialFeatures.getValues ();
    return std::find (features.begin(), features.end(), obj) != features.end ();
}

short FeatureMaterialRoot::mustExecute() const {
    if (MaterialFeatures.isTouched ()) {
        return 1;
    } else {
        return DocumentObject::mustExecute();
    }
}

App::DocumentObjectExecReturn *FeatureMaterialRoot::execute() {
    try { // try to find all base axis and planes in the origin
        // for (const char* role: AxisRoles) {
        //     App::Line *axis = getAxis (role);
        //     assert(axis);
        //     (void)axis;
        // }
        // for (const char* role: PlaneRoles) {
        //     App::Plane *plane = getPlane (role);
        //     assert(plane);
        //     (void)plane;
        // }
    } catch (const Base::Exception &ex) {
        setError ();
        return new App::DocumentObjectExecReturn ( ex.what () );
    }

    return App::DocumentObject::execute ();
}

void FeatureMaterialRoot::setupObject () {
    const static struct {
        const Base::Type type;
        const char *role;
	    const QString label;
        Base::Rotation rot;
    };
    // setupData [] = {
    //     // clang-format off
    //     {App::Line::getClassTypeId(),  AxisRoles[0],  tr("X-axis"),   Base::Rotation()},
    //     {App::Line::getClassTypeId(),  AxisRoles[1],  tr("Y-axis"),   Base::Rotation(Base::Vector3d(1,1,1), M_PI*2/3)},
    //     {App::Line::getClassTypeId(),  AxisRoles[2],  tr("Z-axis"),   Base::Rotation(Base::Vector3d(1,-1,1), M_PI*2/3)},
    //     {App::Plane::getClassTypeId(), PlaneRoles[0], tr("XY-plane"), Base::Rotation()},
    //     {App::Plane::getClassTypeId(), PlaneRoles[1], tr("XZ-plane"), Base::Rotation(1.0, 0.0, 0.0, 1.0 )},
    //     {App::Plane::getClassTypeId(), PlaneRoles[2], tr("YZ-plane"), Base::Rotation(Base::Vector3d(1,1,1), M_PI*2/3)}
    //     // clang-format on
    // };

    App::Document *doc = getDocument ();

    std::vector<App::DocumentObject *> links;
    // for (auto data: setupData) {
    //     std::string objName = doc->getUniqueObjectName ( data.role );
    //     App::DocumentObject *featureObj = doc->addObject ( data.type.getName(), objName.c_str () );

    //     assert ( featureObj && featureObj->isDerivedFrom ( Part::FeatureMaterial::getClassTypeId () ) );

	// QByteArray byteArray = data.label.toUtf8();
    //     featureObj->Label.setValue(byteArray.constData());

    //     Part::FeatureMaterial *feature = static_cast <Part::FeatureMaterial *> ( featureObj );
    //     feature->Placement.setValue ( Base::Placement ( Base::Vector3d (), data.rot ) );
    //     feature->Role.setValue ( data.role );

    //     links.push_back (feature);
    // }

    MaterialFeatures.setValues (links);
}

void FeatureMaterialRoot::unsetupObject () {
    const auto &objsLnk = MaterialFeatures.getValues ();
    // Copy to set to assert we won't call methode more then one time for each object
    std::set<App::DocumentObject *> objs (objsLnk.begin(), objsLnk.end());
    // Remove all controlled objects
    for (auto obj: objs ) {
        // Check that previous deletes wasn't inderectly removed one of our objects
        const auto &objsLnk = MaterialFeatures.getValues ();
        if ( std::find(objsLnk.begin(), objsLnk.end(), obj) != objsLnk.end()) {
            if ( ! obj->isRemoving() ) {
                obj->getDocument()->removeObject (obj->getNameInDocument());
            }
        }
    }
}

// ----------------------------------------------------------------------------

FeatureMaterialRoot::FeatureMaterialRootExtension::FeatureMaterialRootExtension(
    FeatureMaterialRoot* obj)
    : obj(obj)
{
    Group.setStatus(App::Property::Transient, true);
}

void FeatureMaterialRoot::FeatureMaterialRootExtension::initExtension(ExtensionContainer* obj) {
    App::GroupExtension::initExtension(obj);
}

bool FeatureMaterialRoot::FeatureMaterialRootExtension::extensionGetSubObject(App::DocumentObject *&ret, const char *subname,
                                                    PyObject **pyobj, Base::Matrix4D *mat, bool, int depth) const {
    if (!subname || subname[0] == '\0') {
        return false;
    }

    // mapping of object name to role name
    std::string name(subname);
    // for (int i=0; i<3; i++) {
    //     if (name.rfind(FeatureMaterialRoot::AxisRoles[i], 0) == 0) {
    //         name = FeatureMaterialRoot::AxisRoles[i];
    //         break;
    //     }
    //     if (name.rfind(FeatureMaterialRoot::PlaneRoles[i], 0) == 0) {
    //         name = FeatureMaterialRoot::PlaneRoles[i];
    //         break;
    //     }
    // }

    try {
        ret = obj->getFeatureMaterial(name.c_str());
        if (!ret)
            return false;
        const char *dot = strchr(subname, '.');
        if (dot)
            subname = dot+1;
        else
            subname = "";
        ret = ret->getSubObject(subname, pyobj, mat, true, depth+1);
        return true;
    }
    catch (const Base::Exception& e) {
        e.ReportException();
        return false;
    }
}
