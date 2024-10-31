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

#include "FeatureMaterial.h"
#include <App/Document.h>
#include <App/Origin.h>


using namespace Part;

PROPERTY_SOURCE(Part::FeatureMaterial, App::GeoFeature)
// PROPERTY_SOURCE(Part::Plane, Part::FeatureMaterial)
// PROPERTY_SOURCE(Part::Line, Part::FeatureMaterial)

FeatureMaterial::FeatureMaterial()
{
    ADD_PROPERTY_TYPE ( Role, (""), 0, App::Prop_ReadOnly, "Role of the feature in the Origin" ) ;

    // Set placement to read-only
    Placement.setStatus(App::Property::Hidden, true);
}

FeatureMaterial::~FeatureMaterial() = default;

App::Origin* FeatureMaterial::getOrigin()
{
    App::Document *doc = getDocument();
    auto origins = doc->getObjectsOfType ( App::Origin::getClassTypeId() );

    auto originIt= std::find_if (origins.begin(), origins.end(), [this] (DocumentObject *origin) {
            assert ( origin->isDerivedFrom ( App::Origin::getClassTypeId() ) );
            return static_cast<App::Origin *> (origin)->hasObject (this);
        } );
    if (originIt == origins.end()) {
        return nullptr;
    } else {
        assert ( (*originIt)->isDerivedFrom ( App::Origin::getClassTypeId() ) );
        return static_cast<App::Origin *> (*originIt);
    }
}
