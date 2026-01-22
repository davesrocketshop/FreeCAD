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

#include <Base/UniqueNameManager.h>

#include "MaterialTreeItem.h"

using namespace MatGui;

/* TRANSLATOR MatGui::MaterialTreeItem */

MaterialTreeItem::MaterialTreeItem()
    : QStandardItem()
{}

MaterialTreeItem::MaterialTreeItem(const QString& text)
    : QStandardItem(text)
{
    setOriginalName(text);
}

MaterialTreeItem::MaterialTreeItem(const QIcon& icon, const QString& text)
    : QStandardItem(icon, text)
{
    setOriginalName(text);
}

/*
 *  Destroys the object and frees any allocated resources
 */
MaterialTreeItem::~MaterialTreeItem()
{
    // no need to delete child widgets, Qt does it all for us
    // but we can't use default as Ui_PropertiesWidget is undefined
}

TreeFunctionType MaterialTreeItem::getItemFunction() const
{
    auto typeVariant = data(TreeFunctionRole);
    return typeVariant.value<TreeFunctionType>();
}

QString MaterialTreeItem::originalName() const
{
    return data(TreeNameRole).toString();
}

void MaterialTreeItem::setOriginalName(const QString& name)
{
    setData(QVariant(name), TreeNameRole);
}

QString MaterialTreeItem::getUniqueName(const QString& name, TreeFunctionType function) const
{
    Base::UniqueNameManager manager;
    for (int row = 0; row < this->rowCount(); row++) {
        auto child = this->child(row);
        if (child) {
            if (child->getItemFunction() == function) {
                manager.addExactName(child->text().toStdString());
            }
        }
    }
    auto uniqueName = manager.makeUniqueName(name.toStdString(), 1);
    return QString::fromStdString(uniqueName);
}

//===
//
// MaterialTreeLibraryItem
//
//===

MaterialTreeLibraryItem::MaterialTreeLibraryItem()
    : MaterialTreeItem()
{
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);
    setData(QVariant(TreeFunctionType::TreeFunctionLibrary), TreeFunctionRole);
}

MaterialTreeLibraryItem::MaterialTreeLibraryItem(const QString& text)
    : MaterialTreeItem(text)
{
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);
    setData(QVariant(TreeFunctionType::TreeFunctionLibrary), TreeFunctionRole);
}

MaterialTreeLibraryItem::MaterialTreeLibraryItem(
    const QIcon& icon,
    const QString& text
)
    : MaterialTreeItem(icon, text)
{
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);
    setData(QVariant(TreeFunctionType::TreeFunctionLibrary), TreeFunctionRole);
}

MaterialTreeLibraryItem::~MaterialTreeLibraryItem()
{}

std::shared_ptr<Materials::MaterialLibrary> MaterialTreeLibraryItem::getLibrary() const
{
    return data(TreeDataRole).value<std::shared_ptr<Materials::MaterialLibrary>>();
}

void MaterialTreeLibraryItem::setLibrary(const std::shared_ptr<Materials::MaterialLibrary>& library)
{
    setData(QVariant::fromValue(library), TreeDataRole);
    setData(QVariant(library->getName()), TreeNameRole);
}

//===
//
// MaterialTreeFolderItem
//
//===

MaterialTreeFolderItem::MaterialTreeFolderItem()
    : MaterialTreeItem()
{
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);
    setData(QVariant(TreeFunctionType::TreeFunctionFolder), TreeFunctionRole);
}

MaterialTreeFolderItem::MaterialTreeFolderItem(const QString& text)
    : MaterialTreeItem(text)
{
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);
    setData(QVariant(TreeFunctionType::TreeFunctionFolder), TreeFunctionRole);
}

MaterialTreeFolderItem::MaterialTreeFolderItem(const QIcon& icon, const QString& text)
    : MaterialTreeItem(icon, text)
{
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);
    setData(QVariant(TreeFunctionType::TreeFunctionFolder), TreeFunctionRole);
}

MaterialTreeFolderItem::~MaterialTreeFolderItem()
{}

//===
//
// MaterialTreeMaterialItem
//
//===

MaterialTreeMaterialItem::MaterialTreeMaterialItem()
    : MaterialTreeItem()
{
    setFlags(
        Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
        | Qt::ItemIsDropEnabled
    );
    setData(QVariant(TreeFunctionType::TreeFunctionMaterial), TreeFunctionRole);
}

MaterialTreeMaterialItem::MaterialTreeMaterialItem(const QString& text)
    : MaterialTreeItem(text)
{
    setFlags(
        Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
        | Qt::ItemIsDropEnabled | Qt::ItemNeverHasChildren
    );
    setData(QVariant(TreeFunctionType::TreeFunctionMaterial), TreeFunctionRole);
}

MaterialTreeMaterialItem::MaterialTreeMaterialItem(
    const QIcon& icon,
    const QString& text,
    const QString& uuid
)
    : MaterialTreeItem(icon, text)
{
    setFlags(
        Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
        | Qt::ItemIsDropEnabled | Qt::ItemNeverHasChildren
    );
    setData(QVariant(TreeFunctionType::TreeFunctionMaterial), TreeFunctionRole);
    setData(QVariant(uuid), TreeDataRole);
}

MaterialTreeMaterialItem::~MaterialTreeMaterialItem()
{}

QString MaterialTreeMaterialItem::getUUID() const
{
    return data(TreeDataRole).toString();
}

void MaterialTreeMaterialItem::setUUID(const QString& uuid)
{
    setData(QVariant(uuid), TreeDataRole);
}

//===
//
// MaterialTreeFavoriteItem
//
//===

MaterialTreeFavoriteItem::MaterialTreeFavoriteItem()
    : MaterialTreeMaterialItem()
{
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    setData(QVariant(TreeFunctionType::TreeFunctionFavorites), TreeFunctionRole);
}

MaterialTreeFavoriteItem::MaterialTreeFavoriteItem(const QString& text)
    : MaterialTreeMaterialItem(text)
{
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    setData(QVariant(TreeFunctionType::TreeFunctionFavorites), TreeFunctionRole);
}

MaterialTreeFavoriteItem::MaterialTreeFavoriteItem(
    const QIcon& icon,
    const QString& text,
    const QString& uuid
)
    : MaterialTreeMaterialItem(icon, text, uuid)
{
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    setData(QVariant(TreeFunctionType::TreeFunctionFavorites), TreeFunctionRole);
}

MaterialTreeFavoriteItem::~MaterialTreeFavoriteItem()
{}

//===
//
// MaterialTreeRecentItem
//
//===

MaterialTreeRecentItem::MaterialTreeRecentItem()
    : MaterialTreeMaterialItem()
{
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    setData(QVariant(TreeFunctionType::TreeFunctionRecents), TreeFunctionRole);
}

MaterialTreeRecentItem::MaterialTreeRecentItem(const QString& text)
    : MaterialTreeMaterialItem(text)
{
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    setData(QVariant(TreeFunctionType::TreeFunctionRecents), TreeFunctionRole);
}

MaterialTreeRecentItem::MaterialTreeRecentItem(const QIcon& icon, const QString& text, const QString& uuid)
    : MaterialTreeMaterialItem(icon, text, uuid)
{
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    setData(QVariant(TreeFunctionType::TreeFunctionRecents), TreeFunctionRole);
}

MaterialTreeRecentItem::~MaterialTreeRecentItem()
{}
