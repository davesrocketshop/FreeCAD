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

#ifndef MATGUI_DLGINSPECTMODELS_H
#define MATGUI_DLGINSPECTMODELS_H

#include <memory>

#include <QStandardItem>
#include <QStandardItemModel>
#include <QTreeView>

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>

#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/ModelManager.h>

#include "ClipboardText.h"

namespace MatGui
{
class Ui_DlgInspectModels;

class DlgInspectModels: public QWidget, public ClipboardText
{
    Q_OBJECT

public:
    explicit DlgInspectModels(QWidget* parent = nullptr);
    ~DlgInspectModels() override;

private:
    std::unique_ptr<Ui_DlgInspectModels> ui;

    void onModel(int index);
    void onProperty(int index);

    void setupModels();
    void setupProperties(const Materials::Model& model);

    void addExpanded(QTreeView* tree, QStandardItemModel* parent, QStandardItem* child);
    void addExpanded(QTreeView* tree, QStandardItem* parent, QStandardItem* child);
    void updateModelTree(const Materials::Model& model);
    void addModel(QTreeView* tree, QStandardItemModel* parent, const Materials::Model& model);
    void updatePropertyTree(const Materials::ModelProperty& property);
    void addProperty(
        QTreeView* tree,
        QStandardItemModel* parent,
        const Materials::ModelProperty& property
    );
    void addProperty(QTreeView* tree, QStandardItem* parent, const Materials::ModelProperty& property);
    void addPropertyDetails(
        QTreeView* tree,
        QStandardItem* parent,
        const Materials::ModelProperty& property
    );
};


class TaskInspectModels: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskInspectModels();
    ~TaskInspectModels() override;

public:
    void open() override;
    bool accept() override;
    void clicked(int) override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok;
    }

private:
    DlgInspectModels* widget;
};

}  // namespace MatGui

#endif  // MATGUI_DLGINSPECTMODELS_H
