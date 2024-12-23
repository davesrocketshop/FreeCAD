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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <memory>
#endif

#include <Mod/Material/App/MaterialTreeObject.h>

// #include "MainWindow.h"
#include "ViewProviderMaterialTreeObjectGroup.h"

using namespace MatGui;

PROPERTY_SOURCE(MatGui::ViewProviderMaterialTreeObjectGroup, Gui::ViewProviderDocumentObjectGroup)

ViewProviderMaterialTreeObjectGroup::ViewProviderMaterialTreeObjectGroup()
{
    sPixmap = "material_green";
}

bool ViewProviderMaterialTreeObjectGroup::doubleClicked()
{
    // if (!dialog) {
    //     dialog = std::make_unique<DlgAddPropertyVarSet>(getMainWindow(), this);
    // }

    // // Do not use exec() here because it blocks and prevents command Std_VarSet
    // // to commit the autotransaction.  This in turn prevents the dialog to
    // // handle transactions well.
    // dialog->setWindowModality(Qt::ApplicationModal);
    // dialog->show();
    // dialog->raise();
    // dialog->activateWindow();

    return true;
}

void ViewProviderMaterialTreeObjectGroup::onFinished(int /*result*/)
{
    // dialog = nullptr;
}

void ViewProviderMaterialTreeObjectGroup::onChanged(const App::Property* property)
{
    Base::Console().Log("ViewProviderMaterialTreeObjectGroup::onChanged('%s')\n",
                        property->getName());
}

namespace Gui
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(MatGui::ViewProviderMaterialTreeObjectGroupPython,
                         MatGui::ViewProviderMaterialTreeObjectGroup)
/// @endcond

// explicit template instantiation
template class MatGuiExport ViewProviderFeaturePythonT<MatGui::ViewProviderMaterialTreeObjectGroup>;
}  // namespace Gui
