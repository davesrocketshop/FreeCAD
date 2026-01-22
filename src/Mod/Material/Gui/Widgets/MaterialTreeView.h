// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 David Carter <dcarter@david.carter.ca>             *
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

#ifndef MATGUI_MATERIALTREEVIEW_H
#define MATGUI_MATERIALTREEVIEW_H

#include <QPoint>
#include <QStandardItemModel>
#include <QTreeView>

namespace MatGui
{

class MaterialTreeView: public QTreeView
{
    Q_OBJECT

public:
    MaterialTreeView(QWidget* parent = nullptr);
    ~MaterialTreeView() = default;

    // Reimplemented functions
    QStandardItemModel* model() const;

    void mousePressEvent(QMouseEvent* event) override;

private:
};

}  // namespace MatGui

#endif  // MATGUI_MATERIALTREEVIEW_H
