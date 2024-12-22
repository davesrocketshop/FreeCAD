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
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Tools.h>

#include <Mod/Material/App/MaterialTreeObjectGroup.h>

#include "Body.h"
#include "BodyMaterialExtension.h"

using namespace PartDesign;

EXTENSION_PROPERTY_SOURCE(PartDesign::BodyMaterialExtension, App::DocumentObjectExtension)

BodyMaterialExtension::BodyMaterialExtension()
{
    initExtensionType(BodyMaterialExtension::getExtensionClassTypeId());

    // EXTENSION_ADD_PROPERTY_TYPE(Origin,
    //                             (nullptr),
    //                             0,
    //                             App::Prop_Hidden,
    //                             "Origin linked to the group");
    // Origin.setScope(LinkScope::Child);
}

BodyMaterialExtension::~BodyMaterialExtension() = default;

// App::Origin* BodyMaterialExtension::getOrigin() const
// {
//     App::DocumentObject* originObj = Origin.getValue();

//     if (!originObj) {
//         std::stringstream err;
//         err << "Can't find Origin for \"" << getExtendedObject()->getFullName() << "\"";
//         throw Base::RuntimeError(err.str().c_str());
//     }
//     else if (!originObj->isDerivedFrom(App::Origin::getClassTypeId())) {
//         std::stringstream err;
//         err << "Bad object \"" << originObj->getFullName() << "\"("
//             << originObj->getTypeId().getName() << ") linked to the Origin of \""
//             << getExtendedObject()->getFullName() << "\"";
//         throw Base::RuntimeError(err.str().c_str());
//     }
//     else {
//         return static_cast<App::Origin*>(originObj);
//     }
// }

short BodyMaterialExtension::extensionMustExecute()
{
    // if (Origin.isTouched()) {
    //     return 1;
    // }
    // else {
        return DocumentObjectExtension::extensionMustExecute();
    // }
}

App::DocumentObjectExecReturn* BodyMaterialExtension::extensionExecute()
{
    // try {  // try to find all base axis and planes in the origin
    //     getOrigin();
    // }
    // catch (const Base::Exception& ex) {
    //     // getExtendedObject()->setError ();
    //     return new App::DocumentObjectExecReturn(ex.what());
    // }

    return DocumentObjectExtension::extensionExecute();
}

App::DocumentObject* BodyMaterialExtension::getMaterialTreeObject(App::Document* doc)
{
    App::DocumentObject* groupObject =
        doc->addObject("Materials::MaterialTreeObjectGroup", "Material");
    App::DocumentObject* entryObject =
        doc->addObject("Materials::MaterialTreeObject", "Material");
    entryObject->Label.setValue("UUID: The world is a vampire");
    static_cast<Materials::MaterialTreeObjectGroup*>(groupObject)->addObject(entryObject);
    return groupObject;
}

App::DocumentObject* BodyMaterialExtension::getMaterialTreeObject()
{
    App::Document* doc = getExtendedObject()->getDocument();
    return getMaterialTreeObject(doc);
}

void BodyMaterialExtension::onExtendedSetupObject()
{
    App::Document* doc = getExtendedObject()->getDocument();

    App::DocumentObject* materialObj = getMaterialTreeObject(doc);

    assert(materialObj
           && materialObj->isDerivedFrom(Materials::MaterialTreeObjectGroup::getClassTypeId()));
    _materialTreeObjectGroup = materialObj;

    auto obj = getExtendedObject();
    auto bodyObj = dynamic_cast<Body *>(obj);
    if (bodyObj) {
        bodyObj->addObject(_materialTreeObjectGroup);
    }

    DocumentObjectExtension::onExtendedSetupObject();
}

void BodyMaterialExtension::onExtendedUnsetupObject()
{
    // App::DocumentObject* materialTreeObject = Origin.getValue();
    // if (materialTreeObject && !materialTreeObject->isRemoving()) {
    //     materialTreeObject->getDocument()->removeObject(materialTreeObject->getNameInDocument());
    // }

    DocumentObjectExtension::onExtendedUnsetupObject();
}

void BodyMaterialExtension::extensionOnChanged(const App::Property* property)
{
    // if (p == &Origin) {
    //     App::DocumentObject* owner = getExtendedObject();
    //     App::DocumentObject* origin = Origin.getValue();
    //     // Document::Importing indicates the object is being imported (i.e.
    //     // copied). So check the Origin ownership here to prevent copy without
    //     // dependency
    //     if (origin && owner && owner->getDocument()
    //         && owner->getDocument()->testStatus(Document::Importing)) {
    //         for (auto o : origin->getInList()) {
    //             if (o != owner
    //                 && o->hasExtension(App::BodyMaterialExtension::getExtensionClassTypeId())) {
    //                 App::Document* document = owner->getDocument();
    //                 // Temporarily reset 'Restoring' status to allow document to auto label new
    //                 // objects
    //                 Base::ObjectStatusLocker<Document::Status, Document> guard(Document::Restoring,
    //                                                                            document,
    //                                                                            false);
    //                 Origin.setValue(getLocalizedOrigin(document));
    //                 FC_WARN("Reset origin in " << owner->getFullName());
    //                 return;
    //             }
    //         }
    //     }
    // }
    DocumentObjectExtension::extensionOnChanged(property);
}


// Python feature ---------------------------------------------------------

namespace App
{
EXTENSION_PROPERTY_SOURCE_TEMPLATE(PartDesign::BodyMaterialExtensionPython,
                                   PartDesign::BodyMaterialExtension)

// explicit template instantiation
template class PartDesignExport ExtensionPythonT<PartDesign::BodyMaterialExtension>;
}  // namespace App
