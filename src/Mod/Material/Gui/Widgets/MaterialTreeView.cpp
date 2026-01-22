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

#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <Base/Console.h>

#include "MaterialTreeView.h"

using namespace MatGui;

/* TRANSLATOR MatGui::MaterialTreeView */

MaterialTreeView::MaterialTreeView(QWidget* parent)
    : QTreeView(parent)
{
    setDragDropMode(QAbstractItemView::DragDrop);
    setAcceptDrops(true);
}

// Reimplemented functions
QStandardItemModel* MaterialTreeView::model() const
{
    return static_cast<QStandardItemModel *>(QTreeView::model());
}

void MaterialTreeView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        // We want the context menu, but we don't want selection
        // The default behaviour is to select the item when right clicking,
        // such as when context menus are desired. The contect menus are
        // initiated on release, with the selection initiated on press.
        // Ignoring the press gets us what we want.
        return;
    }

    QTreeView::mousePressEvent(event);
}
