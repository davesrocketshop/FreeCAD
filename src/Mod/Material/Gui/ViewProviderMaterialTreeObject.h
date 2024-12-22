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

#ifndef MATGUI_VIEWPROVIDERMATERIALTREEOBJECT_H
#define MATGUI_VIEWPROVIDERMATERIALTREEOBJECT_H

#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/ViewProviderFeaturePython.h>

#include <Mod/Material/MaterialGlobal.h>

namespace MatGui
{

/** View provider associated with a MaterialTreeObject
 */
class MatGuiExport ViewProviderMaterialTreeObject: public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(MatGui::ViewProviderMaterialTreeObject);

public:
    ViewProviderMaterialTreeObject();
    ~ViewProviderMaterialTreeObject() override = default;

    bool isShow() const override { return true; }

    bool doubleClicked() override;

    void onFinished(int);

private:
    // std::unique_ptr<Dialog::DlgAddPropertyVarSet> dialog;
};

using ViewProviderMaterialTreeObjectPython =
    Gui::ViewProviderFeaturePythonT<MatGui::ViewProviderMaterialTreeObject>;

}  // namespace MatGui

#endif  // MATGUI_VIEWPROVIDERMATERIALTREEOBJECT_H
