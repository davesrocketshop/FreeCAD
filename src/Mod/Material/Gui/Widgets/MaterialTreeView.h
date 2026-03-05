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

#include <memory>

#include <QTreeView>

#include <Mod/Material/App/MaterialLibrary.h>
#include <Mod/Material/Gui/Models/MaterialTreeModel.h>

namespace MatGui
{

class MaterialTreeView: public QTreeView
{
    Q_OBJECT

public:
    MaterialTreeView(QWidget* parent = nullptr);
    ~MaterialTreeView() = default;

    // Reimplemented functions
    MaterialTreeModel* model() const;

    void mousePressEvent(QMouseEvent* event) override;
    void startDrag(Qt::DropActions supportedActions) override;
    QModelIndexList selectedDraggableIndexes() const;

    inline bool isIndexDragEnabled(const QModelIndex& index) const
    {
        return (model()->flags(index) & Qt::ItemIsDragEnabled);
    }

    std::shared_ptr<Materials::MaterialLibrary> getItemAsLibrary(const QStandardItem* item) const;
    std::shared_ptr<Materials::MaterialLibrary> getLibraryForItem(const QStandardItem* item) const;

private:
};

}  // namespace MatGui

#endif  // MATGUI_MATERIALTREEVIEW_H
