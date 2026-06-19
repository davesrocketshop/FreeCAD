/***************************************************************************
 *   Copyright (c) 2025 David Carter <dcarter@david.carter.ca>             *
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

#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include "PropertiesWidget.h"
#include "ui_PropertiesWidget.h"

using namespace MatGui;

/* TRANSLATOR MatGui::MaterialsEditor */

PropertiesWidget::PropertiesWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui_PropertiesWidget)
{
    ui->setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
PropertiesWidget::~PropertiesWidget()
{
    // no need to delete child widgets, Qt does it all for us
    // but we can't use default as Ui_PropertiesWidget is undefined
}

#include "moc_PropertiesWidget.cpp"
