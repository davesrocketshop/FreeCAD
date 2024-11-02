/***************************************************************************
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

#include <App/Document.h>
#include <App/GeoFeature.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Tools.h>

#include "FeatureMaterial.h"
#include "FeatureMaterialRoot.h"
#include "MaterialGroupExtension.h"


FC_LOG_LEVEL_INIT("App", true, true)

using namespace Part;

EXTENSION_PROPERTY_SOURCE(Part::MaterialGroupExtension, App::GeoFeatureGroupExtension)

MaterialGroupExtension::MaterialGroupExtension () {

    initExtensionType(MaterialGroupExtension::getExtensionClassTypeId());

    EXTENSION_ADD_PROPERTY_TYPE(MaterialFeatures,
                                (nullptr),
                                0,
                                App::Prop_Hidden,
                                "Origin linked to the group");
    MaterialFeatures.setScope(App::LinkScope::Child);
}

MaterialGroupExtension::~MaterialGroupExtension () = default;

Part::FeatureMaterialRoot *MaterialGroupExtension::getMaterialRoot () const {
    App::DocumentObject *originObj = MaterialFeatures.getValue ();

    if ( !originObj ) {
        std::stringstream err;
        err << "Can't find Origin for \"" << getExtendedObject()->getFullName () << "\"";
        throw Base::RuntimeError ( err.str().c_str () );

    } else if (! originObj->isDerivedFrom ( Part::FeatureMaterialRoot::getClassTypeId() ) ) {
        std::stringstream err;
        err << "Bad object \"" << originObj->getFullName () << "\"(" << originObj->getTypeId().getName()
            << ") linked to the Origin of \"" << getExtendedObject()->getFullName () << "\"";
        throw Base::RuntimeError ( err.str().c_str () );
    } else {
            return static_cast<Part::FeatureMaterialRoot *> ( originObj );
    }
}

bool MaterialGroupExtension::extensionGetSubObject(App::DocumentObject *&ret, const char *subname,
        PyObject **pyObj, Base::Matrix4D *mat, bool transform, int depth) const
{
    App::DocumentObject *originObj = MaterialFeatures.getValue ();
    const char *dot;
    if(originObj && originObj->isAttachedToDocument() &&
       subname && (dot=strchr(subname,'.')))
    {
        bool found;
        if(subname[0] == '$')
            found = std::string(subname+1,dot)==originObj->Label.getValue();
        else
            found = std::string(subname,dot)==originObj->getNameInDocument();
        if(found) {
            if(mat && transform)
                *mat *= const_cast<MaterialGroupExtension*>(this)->placement().getValue().toMatrix();
            ret = originObj->getSubObject(dot+1,pyObj,mat,true,depth+1);
            return true;
        }
    }
    return GeoFeatureGroupExtension::extensionGetSubObject(ret,subname,pyObj,mat,transform,depth);
}

App::DocumentObject *MaterialGroupExtension::getGroupOfObject (const App::DocumentObject* obj) {

    if(!obj)
        return nullptr;

    bool isFeatureMaterial = obj->isDerivedFrom(Part::FeatureMaterial::getClassTypeId());

    auto list = obj->getInList();
    for (auto o : list) {
        if(o->hasExtension(Part::MaterialGroupExtension::getExtensionClassTypeId()))
            return o;
        else if (isFeatureMaterial && o->isDerivedFrom(Part::FeatureMaterialRoot::getClassTypeId())) {
            auto result = getGroupOfObject(o);
            if(result)
                return result;
        }
    }

    return nullptr;
}

short MaterialGroupExtension::extensionMustExecute() {
    if (MaterialFeatures.isTouched ()) {
        return 1;
    } else {
        return GeoFeatureGroupExtension::extensionMustExecute();
    }
}

App::DocumentObjectExecReturn *MaterialGroupExtension::extensionExecute() {
    try { // try to find all base axis and planes in the origin
        getMaterialRoot ();
    } catch (const Base::Exception &ex) {
        //getExtendedObject()->setError ();
        return new App::DocumentObjectExecReturn ( ex.what () );
    }

    return GeoFeatureGroupExtension::extensionExecute ();
}

App::DocumentObject *MaterialGroupExtension::getLocalizedOrigin(App::Document *doc) {
    App::DocumentObject *originObject = doc->addObject ( "Part::FeatureMaterialRoot", "Origin" );
    QByteArray byteArray = tr("Origin").toUtf8();
    originObject->Label.setValue(byteArray.constData());
    return originObject;
}

void MaterialGroupExtension::onExtendedSetupObject () {
    App::Document *doc = getExtendedObject()->getDocument ();

    App::DocumentObject *originObj = getLocalizedOrigin(doc);

    assert ( originObj && originObj->isDerivedFrom ( Part::FeatureMaterialRoot::getClassTypeId () ) );
    MaterialFeatures.setValue (originObj);

    GeoFeatureGroupExtension::onExtendedSetupObject ();
}

void MaterialGroupExtension::onExtendedUnsetupObject () {
    App::DocumentObject *origin = MaterialFeatures.getValue ();
    if (origin && !origin->isRemoving ()) {
        origin->getDocument ()->removeObject (origin->getNameInDocument());
    }

    GeoFeatureGroupExtension::onExtendedUnsetupObject ();
}

void MaterialGroupExtension::extensionOnChanged(const App::Property* p)
{
    if (p == &MaterialFeatures) {
        App::DocumentObject *owner = getExtendedObject();
        App::DocumentObject *origin = MaterialFeatures.getValue();
        // Document::Importing indicates the object is being imported (i.e.
        // copied). So check the Origin ownership here to prevent copy without
        // dependency
        if (origin && owner && owner->getDocument()
            && owner->getDocument()->testStatus(App::Document::Importing)) {
            for (auto o : origin->getInList()) {
                if(o != owner && o->hasExtension(Part::MaterialGroupExtension::getExtensionClassTypeId())) {
                    App::Document *document = owner->getDocument();
                    // Temporarily reset 'Restoring' status to allow document to auto label new objects
                    Base::ObjectStatusLocker<App::Document::Status, App::Document> guard(
                        App::Document::Restoring,
                        document,
                        false);
                    MaterialFeatures.setValue(getLocalizedOrigin(document));
                    FC_WARN("Reset material information in " << owner->getFullName());
                    return;
                }
            }
        }
    }
    GeoFeatureGroupExtension::extensionOnChanged(p);
}

void MaterialGroupExtension::relinkToMaterial(App::DocumentObject* obj)
{
    //we get all links and replace the origin objects if needed (subnames need not to change, they
    //would stay the same)
    std::vector< App::DocumentObject* > result;
    std::vector<App::Property*> list;
    obj->getPropertyList(list);
    for(App::Property* prop : list) {
        if(prop->isDerivedFrom<App::PropertyLink>()) {

            auto p = static_cast<App::PropertyLink*>(prop);
            if(!p->getValue() || !p->getValue()->isDerivedFrom(Part::FeatureMaterial::getClassTypeId()))
                continue;

            p->setValue(getMaterialRoot()->getFeatureMaterial(static_cast<Part::FeatureMaterial*>(p->getValue())->Role.getValue()));
        }
        else if(prop->isDerivedFrom<App::PropertyLinkList>()) {
            auto p = static_cast<App::PropertyLinkList*>(prop);
            auto vec = p->getValues();
            std::vector<App::DocumentObject*> result;
            bool changed = false;
            for(App::DocumentObject* o : vec) {
                if(!o || !o->isDerivedFrom(Part::FeatureMaterial::getClassTypeId()))
                    result.push_back(o);
                else {
                    result.push_back(getMaterialRoot()->getFeatureMaterial(static_cast<Part::FeatureMaterial*>(o)->Role.getValue()));
                    changed = true;
                }
            }
            if(changed)
                static_cast<App::PropertyLinkList*>(prop)->setValues(result);
        }
        else if(prop->isDerivedFrom<App::PropertyLinkSub>()) {
            auto p = static_cast<App::PropertyLinkSub*>(prop);
            if(!p->getValue() || !p->getValue()->isDerivedFrom(Part::FeatureMaterial::getClassTypeId()))
                continue;

            std::vector<std::string> subValues = p->getSubValues();
            p->setValue(getMaterialRoot()->getFeatureMaterial(static_cast<Part::FeatureMaterial*>(p->getValue())->Role.getValue()), subValues);
        }
        else if(prop->isDerivedFrom<App::PropertyLinkSubList>()) {
            auto p = static_cast<App::PropertyLinkSubList*>(prop);
            auto vec = p->getSubListValues();
            bool changed = false;
            for(auto &v : vec) {
                if(v.first && v.first->isDerivedFrom(Part::FeatureMaterial::getClassTypeId())) {
                    v.first = getMaterialRoot()->getFeatureMaterial(static_cast<Part::FeatureMaterial*>(v.first)->Role.getValue());
                    changed = true;
                }
            }
            if(changed)
                p->setSubListValues(vec);
        }
    }
}

std::vector<App::DocumentObject*>
MaterialGroupExtension::addObjects(std::vector<App::DocumentObject*> objs)
{

    for(auto obj : objs)
        relinkToMaterial(obj);

    return App::GeoFeatureGroupExtension::addObjects(objs);
}

bool MaterialGroupExtension::hasObject(const App::DocumentObject* obj, bool recursive) const
{

    if(MaterialFeatures.getValue() && (obj == getMaterialRoot() || getMaterialRoot()->hasObject(obj)))
        return true;

    return App::GroupExtension::hasObject(obj, recursive);
}


// Python feature ---------------------------------------------------------

namespace App {
EXTENSION_PROPERTY_SOURCE_TEMPLATE(Part::MaterialGroupExtensionPython, Part::MaterialGroupExtension)

// explicit template instantiation
template class PartExport App::ExtensionPythonT<App::GroupExtensionPythonT<Part::MaterialGroupExtension>>;
}
