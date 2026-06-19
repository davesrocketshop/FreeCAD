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

#include <Mod/Material/Gui/Models/MaterialTreeModel.h>
#include <Mod/Material/Gui/Widgets/MaterialTreeItem.h>

#include "MaterialTreeView.h"

using namespace MatGui;

/* TRANSLATOR MatGui::MaterialTreeView */

MaterialTreeView::MaterialTreeView(QWidget* parent)
    : QTreeView(parent)
{
    setDragDropMode(QAbstractItemView::DragDrop);
    setAcceptDrops(true);
    setDefaultDropAction(Qt::MoveAction);
    setModel(new MaterialTreeModel(this));
}

// Reimplemented functions
MaterialTreeModel* MaterialTreeView::model() const
{
    return static_cast<MaterialTreeModel*>(QTreeView::model());
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

void MaterialTreeView::startDrag(Qt::DropActions supportedActions)
{
    Base::Console().log("startDrag()\n");
    auto itemActions = supportedActions;

    QModelIndexList indexes = selectedDraggableIndexes();
    if (indexes.size() > 0) {
        if (indexes.size() > 1) {
            Base::Console().log("Too many indexes selected: %d\n", indexes.size());
            return;
        }

        auto item = model()->itemFromIndex(indexes[0]);
        auto library = getLibraryForItem(item);
        if (library && library->isReadOnly()) {
                itemActions &= ~Qt::MoveAction;
        }

        QTreeView::startDrag(itemActions);
    }
}

QModelIndexList MaterialTreeView::selectedDraggableIndexes() const
{
    QModelIndexList indexes = selectedIndexes();
    auto isNotDragEnabled = [this](const QModelIndex& index) {
        return !isIndexDragEnabled(index);
    };
    indexes.removeIf(isNotDragEnabled);
    return indexes;
}

std::shared_ptr<Materials::MaterialLibrary> MaterialTreeView::getLibraryForItem(
    const QStandardItem* item
) const
{
    auto parent = static_cast<MaterialTreeItem*>(item->parent());
    while (parent) {
        if (parent->getItemFunction() == TreeFunctionLibrary) {
            return getItemAsLibrary(parent);
        }
        parent = parent->parent();
    }

    // Not found
    return nullptr;
}

std::shared_ptr<Materials::MaterialLibrary> MaterialTreeView::getItemAsLibrary(
    const QStandardItem* item
) const
{
    auto materialItem = static_cast<const MaterialTreeItem*>(item);
    if (materialItem && materialItem->getItemFunction() == TreeFunctionLibrary) {
        auto libraryItem = static_cast<const MaterialTreeLibraryItem*>(item);
        return libraryItem->getLibrary();
    }
    return nullptr;
}

#include "moc_MaterialTreeView.cpp"
