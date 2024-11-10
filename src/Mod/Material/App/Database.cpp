/***************************************************************************
 *   Copyright (c) 2024 David Carter <dcarter@david.carter.ca>             *
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
#include <QSqlQuery>
#endif

#include <App/Application.h>

#include "Database.h"

using namespace Materials;

const QString Database::DB_MySQL = QLatin1String("MySQL");
const QString Database::DB_Maria = QLatin1String("MariaDB");
const QString Database::DB_Postgress = QLatin1String("Postgress");
const QString Database::DB_SQLServer = QLatin1String("SQL Server");
const QString Database::DB_SQLite = QLatin1String("SQLite");

Database::Database()
{
    setup();
}

Database::~Database()
{
    // QSqlDatabase::removeDatabase(QLatin1String("material"));
}

void Database::setup()
{
    getConnectionSettings();

    if (QSqlDatabase::contains(QLatin1String("material")))
    {
        // Get the database but don't open it yet
        _db = QSqlDatabase::database(QLatin1String("material"), false);
    }
    else {
        _db = QSqlDatabase::addDatabase(_connectionType, QLatin1String("material"));
        _db.setDatabaseName(_dbName);
        _db.setHostName(_hostname);
        _db.setUserName(_username);
        _db.setPassword(_password);
    }
}

void Database::getConnectionSettings()
{
    ParameterGrp::handle hGrp = getParameterGroup();
    _connectionType = getConnectionType(hGrp);
    _dbType = getDBType(hGrp);
    _dbName = getDBName(hGrp);
    _hostname = getHostname(hGrp);
    _username = getUsername(hGrp);
    _password = getPassword(hGrp);
}

QString Database::getConnectionType(ParameterGrp::handle hGrp)
{
    return QString::fromStdString(hGrp->GetASCII("ConnectionType", "QODBC"));
}

QString Database::getDBType(ParameterGrp::handle hGrp)
{
    return QString::fromStdString(
        hGrp->GetASCII("DBType", Database::DB_MySQL.toStdString().c_str()));
}

QString Database::getDBName(ParameterGrp::handle hGrp)
{
    return QString::fromStdString(hGrp->GetASCII("Database", ""));
}

QString Database::getHostname(ParameterGrp::handle hGrp)
{
    return QString::fromStdString(hGrp->GetASCII("Host", ""));
}

QString Database::getUsername(ParameterGrp::handle hGrp)
{
    return QString::fromStdString(hGrp->GetASCII("Username", ""));
}

QString Database::getPassword(ParameterGrp::handle hGrp)
{
    return QString::fromStdString(hGrp->GetASCII("Password", ""));
}

ParameterGrp::handle Database::getParameterGroup()
{
    return App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Database");
}

bool Database::useDatabase()
{
    ParameterGrp::handle hGrp = getParameterGroup();
    return hGrp->GetBool("UseDatabase", false);
}

bool Database::createTables()
{
    if (!_db.open()) {
        return false;
    }

    if (!_db.transaction()) {
        _db.close();
        return false;
    }

    try {
        if (_dbType == DB_MySQL || _dbType == DB_Maria) {
            createTablesMySQL();
        }
        else if (_dbType == DB_Postgress) {
            createTablesPostgress();
        }
        else if (_dbType == DB_SQLServer) {
            createTablesSQLServer();
        }
        else if (_dbType == DB_SQLite) {
            createTablesSQLite();
        }
    }
    catch (QSqlError error) {
        Base::Console().Log("%s\n", error.text().toStdString().c_str());
        _db.rollback();
        _db.close();
        return false;
    }

    if (!_db.commit()) {
        _db.close();
        return false;
    }

    _db.close();
    return true;
}

void Database::dropTable(const QString& table)
{
    // Ignore any errors
    QSqlQuery query(_db);
    query.exec(QLatin1String("DROP TABLE ") + table);
}

void Database::createTablesMySQL()
{
    QSqlQuery query(_db);

    dropTable(QLatin1String("library"));
    if (!query.exec(QLatin1String(
            "CREATE TABLE library (name VARCHAR(255), icon VARCHAR(20), readonly CHAR(1))"))) {
        auto error = lastError();
        if (error.type() != QSqlError::NoError) {
            throw error;
        }
    }
}

void Database::createTablesPostgress()
{
    createTablesMySQL();
}

void Database::createTablesSQLServer()
{
    createTablesMySQL();
}

void Database::createTablesSQLite()
{
    createTablesMySQL();
}
