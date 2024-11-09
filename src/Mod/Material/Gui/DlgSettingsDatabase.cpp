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

#include "PreCompiled.h"

#include <QSqlDatabase>
#include <QSqlError>

#include <App/Application.h>

#include "DlgSettingsDatabase.h"
#include "ui_DlgSettingsDatabase.h"


using namespace MatGui;

DlgSettingsDatabase::DlgSettingsDatabase(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsDatabase)
{
    ui->setupUi(this);

    connect(ui->buttonTest, &QPushButton::clicked, this, &DlgSettingsDatabase::testConnection);
}

void DlgSettingsDatabase::saveSettings()
{
    ui->comboDBType->onSave();
    ui->inputHostname->onSave();
    ui->inputDatabase->onSave();
    ui->inputUsername->onSave();
    ui->inputPassword->onSave();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Database");

    bool useDatabase = ui->groupDatabase->isChecked();
    hGrp->SetBool("UseDatabase", useDatabase);
}

void DlgSettingsDatabase::loadSettings()
{
    ui->inputHostname->onRestore();
    ui->inputDatabase->onRestore();
    ui->inputUsername->onRestore();
    ui->inputPassword->onRestore();

    // Database type - these are products so not translated
    ui->comboDBType->clear();
    ui->comboDBType->addItem(QLatin1String("ODBC"));
    ui->comboDBType->addItem(QLatin1String("MySQL/MariaDB"));
    ui->comboDBType->addItem(QLatin1String("Postgress"));
    ui->comboDBType->addItem(QLatin1String("SQL Server"));
    ui->comboDBType->addItem(QLatin1String("SQLite"));

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Database");
    auto dbType = hGrp->GetASCII("DBType", "QODBC");
    ui->comboDBType->setCurrentText(QString::fromStdString(dbType));

    bool useDatabase = hGrp->GetBool("UseDatabase", false);
    ui->groupDatabase->setChecked(useDatabase);
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsDatabase::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void DlgSettingsDatabase::testConnection(bool)
{
    QSqlDatabase db =
        QSqlDatabase::addDatabase(QLatin1String("QODBC"), QLatin1String("connectionTest"));
    db.setHostName(ui->inputHostname->text());
    db.setDatabaseName(ui->inputDatabase->text());
    db.setUserName(ui->inputUsername->text());
    db.setPassword(ui->inputPassword->text());
    bool ok = db.open();
    if (ok) {
        ui->inputStatus->setText(QLatin1String("Pass"));
        db.close();
    }
    else {
        ui->inputStatus->setText(QLatin1String("Fail"));
        auto error = db.lastError();
        Base::Console().Log(error.text().toStdString().c_str());
    }
    QSqlDatabase::removeDatabase(QLatin1String("connectionTest"));
}

#include "moc_DlgSettingsDatabase.cpp"
