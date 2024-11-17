/***************************************************************************
 *   Copyright (c) 2024 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <QPushButton>

#include <Base/Console.h>

#include "TaskMigrateDatabase.h"
#include "ui_TaskMigrateDatabase.h"


using namespace MatGui;

/* TRANSLATOR MatGui::DlgMigrateDatabase */

DlgMigrateDatabase::DlgMigrateDatabase(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui_TaskMigrateDatabase)
{
    ui->setupUi(this);

    showLibraries();
}

void DlgMigrateDatabase::showLibraries()
{
    auto materialLibraries = _materialManager.getLocalMaterialLibraries();
    for (auto library : *materialLibraries) {
        if (library->getName() != QLatin1String("User")) {
            auto item = new QListWidgetItem(library->getName());
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Checked);
            item->setData(Qt::UserRole, QVariant::fromValue(library));
            ui->listMaterialLibraries->addItem(item);
        }
    }

    auto modelLibraries = _modelManager.getLocalModelLibraries();
    for (auto library : *modelLibraries) {
        if (library->getName() != QLatin1String("User")) {
            auto item = new QListWidgetItem(library->getName());
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Checked);
            item->setData(Qt::UserRole, QVariant::fromValue(library));
            ui->listModelLibraries->addItem(item);
        }
    }
}

void DlgMigrateDatabase::accept()
{
    Base::Console().Log("Migrating Models...\n");
    for (int row = 0; row < ui->listModelLibraries->count(); row++) {
        auto item = ui->listModelLibraries->item(row);
        if (item->checkState() == Qt::Checked) {
            auto library = item->data(Qt::UserRole).value<std::shared_ptr<Materials::ModelLibrary>>();
            Base::Console().Log("\tLibrary: %s...", library->getName().toStdString().c_str());
            _modelManager.migrateToDatabase(library);
            Base::Console().Log("done\n");
        }
    }
    Base::Console().Log("done\n");

    Base::Console().Log("Migrating Materials...\n");
    for (int row = 0; row < ui->listMaterialLibraries->count(); row++) {
        auto item = ui->listMaterialLibraries->item(row);
        if (item->checkState() == Qt::Checked) {
            auto library =
                item->data(Qt::UserRole).value<std::shared_ptr<Materials::MaterialLibrary>>();
            Base::Console().Log("\tLibrary: %s...", library->getName().toStdString().c_str());
            _materialManager.migrateToDatabase(library);
            Base::Console().Log("done\n");
        }
    }
    Base::Console().Log("done\n");
}

/* TRANSLATOR MatGui::TaskMigrateDatabase */

TaskMigrateDatabase::TaskMigrateDatabase()
{
    _widget = new DlgMigrateDatabase();
    addTaskBox(_widget);
}

QDialogButtonBox::StandardButtons TaskMigrateDatabase::getStandardButtons() const
{
    return QDialogButtonBox::Close | QDialogButtonBox::Ok;
}

void TaskMigrateDatabase::modifyStandardButtons(QDialogButtonBox* box)
{
    QPushButton* btn = box->button(QDialogButtonBox::Ok);
    btn->setText(QApplication::translate("MatGui::TaskMigrateDatabase", "&Migrate"));
}

bool TaskMigrateDatabase::accept()
{
    _widget->accept();
    return true;
}

bool TaskMigrateDatabase::reject()
{
    return true;
}

#include "moc_TaskMigrateDatabase.cpp"
