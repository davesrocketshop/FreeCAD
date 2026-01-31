// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
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

#ifndef MATGUI_MATERIALTREEMODEL_H
#define MATGUI_MATERIALTREEMODEL_H

#include <memory>

#include <QStandardItemModel>
// #include <QDialog>
// #include <QList>
// #include <QStandardItem>
// #include <QVariant>

// #include <Mod/Material/App/Materials.h>
// #include <Mod/Material/App/Model.h>

namespace MatGui
{

class MaterialTreeModel: public QStandardItemModel
{
    Q_OBJECT

public:
    MaterialTreeModel(QObject* parent = nullptr);
    ~MaterialTreeModel() override = default;

    bool dropMimeData(
        const QMimeData* data,
        Qt::DropAction action,
        int row,
        int column,
        const QModelIndex& parent
    ) override;
    void decodeDataRecursive(QDataStream& stream, QStandardItem* item);

Q_SIGNALS:
    void itemDropped(Qt::DropAction action, QStandardItem* source, QStandardItem* destination);

private:
};

}  // namespace MatGui

#endif  // MATGUI_MATERIALTREEMODEL_H
