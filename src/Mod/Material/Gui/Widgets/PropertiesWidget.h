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

#ifndef MATGUI_PROPERTIESWIDGET_H
#define MATGUI_PROPERTIESWIDGET_H

#include <QWidget>

namespace MatGui
{

class Ui_PropertiesWidget;

class PropertiesWidget: public QWidget
{
    Q_OBJECT

public:
    explicit PropertiesWidget(QWidget* parent = nullptr);
    ~PropertiesWidget() override;

private:
    std::unique_ptr<Ui_PropertiesWidget> ui;

};

}  // namespace MatGui

#endif  // MATGUI_PROPERTIESWIDGET_H
