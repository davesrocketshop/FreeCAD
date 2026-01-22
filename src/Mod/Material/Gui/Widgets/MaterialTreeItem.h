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

#ifndef MATGUI_MATERIALTREEITEM_H
#define MATGUI_MATERIALTREEITEM_H

#include <QStandardItem>

#include <Mod/Material/App/MaterialLibrary.h>

namespace MatGui
{

const int TreeDataRole = Qt::UserRole;
const int TreeFunctionRole = Qt::UserRole + 1;
const int TreeNameRole = Qt::UserRole + 2;

typedef enum
{
    TreeFunctionFavorites,
    TreeFunctionRecents,
    TreeFunctionLibrary,
    TreeFunctionFolder,
    TreeFunctionMaterial
} TreeFunctionType;

class MaterialTreeItem: public QStandardItem
{

public:
    explicit MaterialTreeItem();
    explicit MaterialTreeItem(const QString& text);
    explicit MaterialTreeItem(const QIcon& icon, const QString& text);
    ~MaterialTreeItem() override;

    MaterialTreeItem *parent() const {
        return static_cast<MaterialTreeItem*>(QStandardItem::parent());
    }
    MaterialTreeItem* child(int row, int column = 0) const
    {
        return static_cast<MaterialTreeItem*>(QStandardItem::child(row, column));
    }

    TreeFunctionType getItemFunction() const;
    QString getUniqueName(const QString& name, TreeFunctionType function) const;

    QString originalName() const;
    void setOriginalName(const QString& name);

private:

};

class MaterialTreeLibraryItem: public MaterialTreeItem
{
public:
    explicit MaterialTreeLibraryItem();
    explicit MaterialTreeLibraryItem(const QString& text);
    explicit MaterialTreeLibraryItem(const QIcon& icon, const QString& text);
    ~MaterialTreeLibraryItem() override;

    std::shared_ptr<Materials::MaterialLibrary> getLibrary() const;
    void setLibrary(const std::shared_ptr<Materials::MaterialLibrary>& library);
};

class MaterialTreeFolderItem: public MaterialTreeItem
{
public:
    explicit MaterialTreeFolderItem();
    explicit MaterialTreeFolderItem(const QString& text);
    explicit MaterialTreeFolderItem(const QIcon& icon, const QString& text);
    ~MaterialTreeFolderItem() override;
};

class MaterialTreeMaterialItem: public MaterialTreeItem
{
public:
    explicit MaterialTreeMaterialItem();
    explicit MaterialTreeMaterialItem(const QString& text);
    explicit MaterialTreeMaterialItem(const QIcon& icon, const QString& text, const QString& uuid);
    ~MaterialTreeMaterialItem() override;

    QString getUUID() const;
    void setUUID(const QString& uuid);
};

class MaterialTreeFavoriteItem: public MaterialTreeMaterialItem
{
public:
    explicit MaterialTreeFavoriteItem();
    explicit MaterialTreeFavoriteItem(const QString& text);
    explicit MaterialTreeFavoriteItem(const QIcon& icon, const QString& text, const QString& uuid);
    ~MaterialTreeFavoriteItem() override;
};

class MaterialTreeRecentItem: public MaterialTreeMaterialItem
{
public:
    explicit MaterialTreeRecentItem();
    explicit MaterialTreeRecentItem(const QString& text);
    explicit MaterialTreeRecentItem(const QIcon& icon, const QString& text, const QString& uuid);
    ~MaterialTreeRecentItem() override;
};

}  // namespace MatGui

#endif  // MATGUI_MATERIALTREEITEM_H
