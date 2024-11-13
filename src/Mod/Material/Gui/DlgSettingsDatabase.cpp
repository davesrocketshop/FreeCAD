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
#ifndef _PreComp_
#include <QMessageBox>
#endif

// #include <QSqlDatabase>
// #include <QSqlError>

#include <App/Application.h>

#include <Mod/Material/App/Database.h>
#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/ModelManager.h>

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
    ui->spinModelCacheSize->onSave();
    ui->spinMaterialCacheSize->onSave();

    bool useDatabase = ui->groupDatabase->isChecked();
    hGrp->SetBool("UseDatabase", useDatabase);
}

QString DlgSettingsDatabase::toPerCent(double value) const
{
    int pc = value * 100.0;
    QString pcString;
    pcString.setNum(pc);
    pcString += QLatin1String("%");

    return pcString;
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
    // if (drivers.contains(QLatin1String("QPSQL"))) {
    //     ui->comboConnectionType->addItem(QLatin1String("Postgres"));
    // }

    // Database type - these are products so not translated
    ui->comboDBType->clear();
    ui->comboDBType->addItem(Materials::Database::DB_MySQL);
    ui->comboDBType->addItem(Materials::Database::DB_Maria);
    // ui->comboDBType->addItem(Materials::Database::DB_Postgress);
    // ui->comboDBType->addItem(Materials::Database::DB_SQLServer);
    // ui->comboDBType->addItem(Materials::Database::DB_SQLite);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Database");
    auto connectionType = hGrp->GetASCII("ConnectionType", "QODBC");
    if (connectionType == "QODBC") {
        ui->comboConnectionType->setCurrentText(QLatin1String("ODBC"));
    }
    else if (connectionType == "QMYSQL") {
        ui->comboConnectionType->setCurrentText(QLatin1String("MySQL"));
    }
    else if (connectionType == "QMARIADB") {
        ui->comboConnectionType->setCurrentText(QLatin1String("MariaDB"));
    }
    // else if (connectionType == "QPSQL") {
    //     ui->comboConnectionType->setCurrentText(QLatin1String("Postgres"));
    // }
    auto dbType = hGrp->GetASCII("DBType", Materials::Database::DB_MySQL.toStdString().c_str());
    ui->comboDBType->setCurrentText(QString::fromStdString(dbType));

    bool useDatabase = hGrp->GetBool("UseDatabase", false);
    ui->groupDatabase->setChecked(useDatabase);

    auto cacheSize = hGrp->GetInt("ModelCacheSize", 100);
    ui->spinModelCacheSize->setValue(cacheSize);
    cacheSize = hGrp->GetInt("MaterialCacheSize", 100);
    ui->spinMaterialCacheSize->setValue(cacheSize);

    // Cache stats
    auto hitRate = Materials::ModelManager::modelHitRate();
    ui->inputModelCacheHitRate->setText(toPerCent(hitRate));
    ui->inputMaterialCacheHitRate->setText(toPerCent(0.0));
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
    else if (ui->comboConnectionType->currentText() == QLatin1String("MySQL")) {
        db = QSqlDatabase::addDatabase(QLatin1String("QMYSQL"), QLatin1String("connectionTest"));
    }
    else if (ui->comboConnectionType->currentText() == QLatin1String("MariaDB")) {
        db = QSqlDatabase::addDatabase(QLatin1String("QMARIADB"), QLatin1String("connectionTest"));
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

// void DlgSettingsDatabase::initialize(bool)
// {
//     // Connection settings must be saved before connecting
//     saveSettings();

//     Materials::Database db;

//     if (!db.createTables()) {
//         Base::Console().Log("Fail\n");
//         auto error = db.lastError();
//         Base::Console().Log("%s\n", error.text().toStdString().c_str());
//         QMessageBox::critical(this, tr("Database Initialize"), error.text());
//     }
// }

#include "moc_DlgSettingsDatabase.cpp"
