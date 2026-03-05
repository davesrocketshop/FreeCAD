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

#include <QIODevice>
#include <QMetaType>
#include <QMimeData>

#include <Base/Console.h>

#include "MaterialTreeModel.h"


using namespace MatGui;

/* TRANSLATOR MatGui::MaterialTreeModel */

static inline QString qStandardItemModelDataListMimeType()
{
    return QStringLiteral("application/x-qstandarditemmodeldatalist");
}

MaterialTreeModel::MaterialTreeModel(QObject* parent)
    : QStandardItemModel(parent)
{}

bool MaterialTreeModel::dropMimeData(
    const QMimeData* data,
    Qt::DropAction action,
    int row,
    int column,
    const QModelIndex& parent
)
{
    Base::Console().log("dropMimeData()\n");

    // check if the action is supported
    if (!data || !(action == Qt::CopyAction || action == Qt::MoveAction)) {
        return false;
    }
    // check if the format is supported
    const QString format = qStandardItemModelDataListMimeType();
    if (!data->hasFormat(format)) {
        return QAbstractItemModel::dropMimeData(data, action, row, column, parent);
    }

    if (row > rowCount(parent)) {
        row = rowCount(parent);
    }
    if (row == -1) {
        row = rowCount(parent);
    }
    if (column == -1) {
        column = 0;
    }
    auto parentItem = itemFromIndex(parent);
    if (!parentItem) {
        return false;
    }

    // decode and insert
    QByteArray encoded = data->data(format);
    QDataStream stream(&encoded, QIODevice::ReadOnly);


    // code based on QAbstractItemModel::decodeData
    //  adapted to work with QStandardItem
    QList<QStandardItem*> items;
    while (!stream.atEnd()) {
        int r, c;
        QStandardItem* item = new QStandardItem();
        stream >> r >> c;
        decodeDataRecursive(stream, item);

        items.append(item);
    }

    if (items.size() != 1) {
        if (items.size() == 0) {
            Base::Console().log("Attempt to drop 0 items, ignored\n");
        }
        else {
            Base::Console().log("Attempt to drop multiple items, ignored\n");
        }
        return false;
    }

    // Pass the source and destination items
    Q_EMIT itemDropped(action, items[0], parentItem);
    return true;
}

/*
    Copied from QStandardItemModelPrivate

    Used by QStandardItemModel::dropMimeData
    stream out an item and his children
 */
void MaterialTreeModel::decodeDataRecursive(QDataStream& stream, QStandardItem* item)
{
    int colCount, childCount;
    stream >> *item;
    stream >> colCount >> childCount;
    item->setColumnCount(colCount);

    int childPos = childCount;

    while (childPos > 0) {
        childPos--;
        QStandardItem* child = new QStandardItem();
        decodeDataRecursive(stream, child);
        item->setChild(childPos / colCount, childPos % colCount, child);
    }
}


#include "moc_MaterialTreeModel.cpp"
