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

#include <Mod/Material/App/Database.h>

#include "DlgSettingsDatabase.h"
#include "ui_DlgSettingsDatabase.h"


using namespace MatGui;

DlgSettingsDatabase::DlgSettingsDatabase(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsDatabase)
{
    ui->setupUi(this);

    connect(ui->buttonTest, &QPushButton::clicked, this, &DlgSettingsDatabase::testConnection);
    connect(ui->buttonInitialize, &QPushButton::clicked, this, &DlgSettingsDatabase::initialize);
    connect(ui->buttonMigrate, &QPushButton::clicked, this, &DlgSettingsDatabase::migrate);
}

DlgSettingsDatabase::~DlgSettingsDatabase()
{
    if (QSqlDatabase::contains(QLatin1String("connectionTest"))) {
        QSqlDatabase::removeDatabase(QLatin1String("connectionTest"));
    }
}

void DlgSettingsDatabase::saveSettings()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Database");

    if (ui->comboConnectionType->currentText() == QLatin1String("ODBC")) {
        hGrp->SetASCII("ConnectionType", "QODBC");
    }
    else if (ui->comboConnectionType->currentText() == QLatin1String("MySQL")) {
        hGrp->SetASCII("ConnectionType", "QMYSQL");
    }
    else if (ui->comboConnectionType->currentText() == QLatin1String("MariaDB")) {
        hGrp->SetASCII("ConnectionType", "QMARIADB");
    }
    else if (ui->comboConnectionType->currentText() == QLatin1String("Postgres")) {
        hGrp->SetASCII("ConnectionType", "QPSQL");
    }
    ui->comboDBType->onSave();
    ui->inputHostname->onSave();
    ui->inputDatabase->onSave();
    ui->inputUsername->onSave();
    ui->inputPassword->onSave();

    bool useDatabase = ui->groupDatabase->isChecked();
    hGrp->SetBool("UseDatabase", useDatabase);
}

void DlgSettingsDatabase::loadSettings()
{
    ui->inputHostname->onRestore();
    ui->inputDatabase->onRestore();
    ui->inputUsername->onRestore();
    ui->inputPassword->onRestore();

    // Connection type - these are products so not translated
    ui->comboConnectionType->clear();
    auto drivers = QSqlDatabase::drivers();
    if (drivers.contains(QLatin1String("QODBC"))) {
        ui->comboConnectionType->addItem(QLatin1String("ODBC"));
    }
    if (drivers.contains(QLatin1String("QMYSQL"))) {
        ui->comboConnectionType->addItem(QLatin1String("MySQL"));
    }
    if (drivers.contains(QLatin1String("QMARIADB"))) {
        ui->comboConnectionType->addItem(QLatin1String("MariaDB"));
    }
    if (drivers.contains(QLatin1String("QPSQL"))) {
        ui->comboConnectionType->addItem(QLatin1String("Postgres"));
    }

    // Database type - these are products so not translated
    ui->comboDBType->clear();
    ui->comboDBType->addItem(Materials::Database::DB_MySQL);
    ui->comboDBType->addItem(Materials::Database::DB_Maria);
    ui->comboDBType->addItem(Materials::Database::DB_Postgress);
    ui->comboDBType->addItem(Materials::Database::DB_SQLServer);
    ui->comboDBType->addItem(Materials::Database::DB_SQLite);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Database");
    auto connectionType = hGrp->GetASCII("DBType", "QODBC");
    if (connectionType == "OODBC") {
        ui->comboConnectionType->setCurrentText(QLatin1String("ODBC"));
    }
    auto dbType = hGrp->GetASCII("DBType", Materials::Database::DB_MySQL.toStdString().c_str());
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
    QSqlDatabase db;

    // Always add a new database with the same name in case the connection type has changed
    if (ui->comboConnectionType->currentText() == QLatin1String("ODBC")) {
        db = QSqlDatabase::addDatabase(QLatin1String("QODBC"), QLatin1String("connectionTest"));
    }

    db.setHostName(ui->inputHostname->text());
    db.setDatabaseName(ui->inputDatabase->text());
    db.setUserName(ui->inputUsername->text());
    db.setPassword(ui->inputPassword->text());
    bool ok = db.open();
    if (ok) {
        // ui->inputStatus->setText(QLatin1String("Pass"));
        Base::Console().Log("Connection pass\n");
        db.close();
        QMessageBox::information(this, tr("Database Connection Test"), tr("Success!"));
    }
    else {
        // ui->inputStatus->setText(QLatin1String("Fail"));
        Base::Console().Log("Connection fail\n");
        auto error = db.lastError();
        Base::Console().Log("%s\n", error.text().toStdString().c_str());
        QMessageBox::critical(this, tr("Database Connection Test"), error.text());
    }
}

void DlgSettingsDatabase::initialize(bool)
{
    // Connection settings must be saved before connecting
    saveSettings();

    Materials::Database db;

    if (!db.createTables()) {
        Base::Console().Log("Fail\n");
        auto error = db.lastError();
        Base::Console().Log("%s\n", error.text().toStdString().c_str());
        QMessageBox::critical(this, tr("Database Initialize"), error.text());
    }
}

void DlgSettingsDatabase::migrate(bool)
{
}

#include "moc_DlgSettingsDatabase.cpp"
