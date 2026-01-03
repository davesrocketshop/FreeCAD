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

#include <QClipboard>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTreeView>


#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>

#include <Mod/Material/App/MaterialLibrary.h>
#include <Mod/Material/App/ModelManager.h>
#include <Mod/Material/App/PropertyMaterial.h>

#include "DlgInspectModels.h"
#include "ui_DlgInspectModels.h"


using namespace MatGui;

/* TRANSLATOR MatGui::DlgInspectModels */

DlgInspectModels::DlgInspectModels(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui_DlgInspectModels)
{
    ui->setupUi(this);

    auto tree = ui->treeModels;
    auto model = new QStandardItemModel();
    tree->setModel(model);

    tree->setHeaderHidden(true);

    tree = ui->treeProperties;
    model = new QStandardItemModel();
    tree->setModel(model);

    tree->setHeaderHidden(true);

    connect(ui->comboModel, &QComboBox::currentIndexChanged, this, &DlgInspectModels::onModel);
    connect(ui->comboProperty, &QComboBox::currentIndexChanged, this, &DlgInspectModels::onProperty);

    setupModels();
}

DlgInspectModels::~DlgInspectModels()
{
    // Gui::Selection().Detach(this);
}

void DlgInspectModels::setupModels()
{
    auto& manager = Materials::ModelManager::getManager();
    auto models = manager.getModels();
    ui->comboModel->clear();
    for (auto& it : *models) {
        auto model = it.second;
        ui->comboModel->addItem(model->getName(), QVariant::fromValue(model));
    }
}

void DlgInspectModels::onModel(int index)
{
    Q_UNUSED(index)

    auto value = ui->comboModel->currentData();
    auto model = value.value<std::shared_ptr<Materials::Model>>();
    // Base::Console().log("Model:  %s\n", model->getName().toStdString().c_str());
    resetClipboard();
    updateModelTree(*model);
    setupProperties(*model);
}

void DlgInspectModels::setupProperties(const Materials::Model& model)
{
    ui->comboProperty->clear();
    for (auto& it : model) {
        auto property = it.second;
        Base::Console().log("Property:  %s\n", property.getName().toStdString().c_str());
        ui->comboProperty->addItem(property.getName(), QVariant::fromValue(property));
    }
}

void DlgInspectModels::onProperty(int index)
{
    auto value = ui->comboProperty->currentData();
    if (!value.isNull() && value.isValid())
    {
        auto property = value.value<Materials::ModelProperty>();
        Base::Console()
            .log("Property selected %d:  %s\n", index, property.getDisplayName().toStdString().c_str());
        resetClipboard();
        updatePropertyTree(property);
    }
}

void DlgInspectModels::addExpanded(QTreeView* tree, QStandardItemModel* parent, QStandardItem* child)
{
    parent->appendRow(child);
    tree->setExpanded(child->index(), true);
}

void DlgInspectModels::addExpanded(QTreeView* tree, QStandardItem* parent, QStandardItem* child)
{
    parent->appendRow(child);
    tree->setExpanded(child->index(), true);
}

void DlgInspectModels::updateModelTree(const Materials::Model& model)
{
    Base::Console().log("Model '%s'\n", model.getName().toStdString().c_str());

    auto tree = ui->treeModels;
    auto treeModel = qobject_cast<QStandardItemModel*>(tree->model());
    treeModel->clear();

    addModel(tree, treeModel, model);
}

void DlgInspectModels::addModel(QTreeView* tree, QStandardItemModel* parent, const Materials::Model& model)
{
    auto card = clipItem(tr("Name: ") + model.getName());
    addExpanded(tree, parent, card);

    indent();
        auto item = clipItem(tr("Library: ") + model.getLibrary()->getName());
        addExpanded(tree, parent, item);
        item = clipItem(tr("Library directory: ") + model.getLibrary()->getDirectoryPath());
        addExpanded(tree, parent, item);
        item = clipItem(tr("Type: ") + model.getBase());
        addExpanded(tree, parent, item);
        item = clipItem(tr("Directory: ") + model.getDirectory());
        addExpanded(tree, parent, item);
        item = clipItem(tr("Filename: ") + model.getFilename());
        addExpanded(tree, parent, item);
        item = clipItem(tr("UUID: ") + model.getUUID());
        addExpanded(tree, parent, item);
        item = clipItem(tr("Description: ") + model.getDescription());
        addExpanded(tree, parent, item);
        item = clipItem(tr("URL: ") + model.getURL());
        addExpanded(tree, parent, item);
        item = clipItem(tr("DOI: ") + model.getDOI());
        addExpanded(tree, parent, item);
        auto inherits = clipItem(tr("Inherits: "));
        addExpanded(tree, parent, inherits);
        indent();
        for (auto const& uuid : model.getInheritance()) {
            auto superModel = Materials::ModelManager::getManager().getModel(uuid);
            item = clipItem(uuid + QStringLiteral(": ") + superModel->getName());
            addExpanded(tree, inherits, item);
        }
        unindent();
    unindent();
}

void DlgInspectModels::updatePropertyTree(const Materials::ModelProperty& property)
{
    Base::Console().log("Property '%s'\n", property.getName().toStdString().c_str());

    auto tree = ui->treeProperties;
    auto model = qobject_cast<QStandardItemModel*>(tree->model());
    model->clear();

    addProperty(tree, model, property);
}

void DlgInspectModels::addProperty(
    QTreeView* tree,
    QStandardItemModel* parent,
    const Materials::ModelProperty& property
)
{
    auto card = clipItem(tr("Name: ") + property.getName());
    addExpanded(tree, parent, card);

    indent();
    addPropertyDetails(tree, card, property);
    unindent();
}

void DlgInspectModels::addProperty(
    QTreeView* tree,
    QStandardItem* parent,
    const Materials::ModelProperty& property
)
{
    auto card = clipItem(tr("Name: ") + property.getName());
    addExpanded(tree, parent, card);

    indent();
    addPropertyDetails(tree, card, property);
    unindent();
}

void DlgInspectModels::addPropertyDetails(
    QTreeView* tree,
    QStandardItem* parent,
    const Materials::ModelProperty& property
)
{
    auto text = clipItem(tr("Display Name: ") + property.getDisplayName());
    addExpanded(tree, parent, text);
    text = clipItem(tr("Type: ") + property.getPropertyType());
    addExpanded(tree, parent, text);
    text = clipItem(tr("Units: ") + property.getUnits());
    addExpanded(tree, parent, text);
    text = clipItem(tr("URL: ") + property.getURL());
    addExpanded(tree, parent, text);
    text = clipItem(tr("Description: ") + property.getDescription());
    addExpanded(tree, parent, text);
    auto uuid = property.getInheritance();
    text = clipItem(tr("Inheritance: ") + property.getInheritance());
    addExpanded(tree, parent, text);
    if (!uuid.isEmpty()) {
        auto model = Materials::ModelManager::getManager().getModel(uuid);
        text = clipItem(QStringLiteral("- ") + model->getName());
        addExpanded(tree, parent, text);
    }
    // TODO: Column properties
}

/* TRANSLATOR MatGui::TaskInspectModels */

TaskInspectModels::TaskInspectModels()
{
    widget = new DlgInspectModels();
    addTaskBox(Gui::BitmapFactory().pixmap("Material_Edit"), widget);
}

TaskInspectModels::~TaskInspectModels() = default;

void TaskInspectModels::open()
{}

void TaskInspectModels::clicked(int)
{}

bool TaskInspectModels::accept()
{
    // return widget->accept();
    return true;
}

#include "moc_DlgInspectModels.cpp"
