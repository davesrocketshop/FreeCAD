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
#include <QVariant>
#endif

#include <App/Application.h>

#include "Database.h"
#include "Model.h"
#include "ModelLibrary.h"

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

int Database::findLibrary(const QString& name)
{
    int result = 0;
    if (_db.open()) {
        QSqlQuery query(_db);
        query.prepare(
            QLatin1String("SELECT library_id FROM library WHERE library_name = ?"));
        query.addBindValue(name);
        query.exec();

        if (query.next()) {
            result = query.value(0).toInt();
        }

        _db.close();
    }
    return result;
}

void Database::createLibrary(const QString& name, const QString& icon, bool readOnly)
{
    if (_db.open()) {
        if (_db.transaction()) {
            QSqlQuery query(_db);

            // First check if the library exists
            query.prepare(QLatin1String("SELECT library_id FROM library WHERE library_name = ?"));
            query.addBindValue(name);
            query.exec();

            if (!query.next()) {
                if (!icon.isEmpty()) {
                    query.prepare(
                        QLatin1String("INSERT INTO library (library_name, library_icon, library_read_only) "
                                    "VALUES (?, ?, ?)"));
                    query.addBindValue(name);
                    query.addBindValue(icon);
                    query.addBindValue(readOnly);
                    query.exec();
                }
                else {
                    query.prepare(QLatin1String(
                        "INSERT INTO library (library_name, library_read_only) "
                        "VALUES (?, ?)"));
                    query.addBindValue(name);
                    query.addBindValue(readOnly);
                    query.exec();
                }
            }
        }
        _db.commit();
        _db.close();
    }
}

int Database::createPath(int libraryIndex, int parentIndex, int pathIndex, const QStringList &pathList)
{
    int newId = 0;
    if (parentIndex == 0) {
        // No parent. Root folder
        if (_db.transaction()) {
            QSqlQuery query(_db);

            // First check if the folder exists
            query.prepare(
                QLatin1String("SELECT folder_id FROM folder WHERE folder_name = ? AND library_id = ?"
                    " AND parent_id IS NULL"));
            query.addBindValue(pathList[pathIndex]);
            query.addBindValue(libraryIndex);
            query.exec();

            if (query.next()) {
                newId = query.value(0).toInt();
                // Base::Console().Log("Found folder '%s' id=%d\n", pathList[pathIndex].toStdString().c_str(), newId);
            }
            else {
                query.prepare(
                    QLatin1String("INSERT INTO folder (folder_name, library_id) "
                                    "VALUES (?, ?)"));
                query.addBindValue(pathList[pathIndex]);
                query.addBindValue(libraryIndex);
                // query.addBindValue(parentIndex);
                if (query.exec()) {
                    newId = query.lastInsertId().toInt();
                }
                else {
                    Base::Console().Log("Error creating path?\n");
                }
            }
        }
        _db.commit();
    }
    else {
        if (_db.transaction()) {
            QSqlQuery query(_db);

            // First check if the folder exists
            query.prepare(QLatin1String(
                "SELECT folder_id FROM folder WHERE folder_name = ? AND library_id = ?"
                " AND parent_id = ?"));
            query.addBindValue(pathList[pathIndex]);
            query.addBindValue(libraryIndex);
            query.addBindValue(parentIndex);
            query.exec();

            if (query.next()) {
                newId = query.value(0).toInt();
                // Base::Console().Log("Found folder '%s' id=%d\n",
                //                     pathList[pathIndex].toStdString().c_str(),
                //                     newId);
            }
            else {
                query.prepare(QLatin1String("INSERT INTO folder (folder_name, library_id, parent_id) "
                                            "VALUES (?, ?, ?)"));
                query.addBindValue(pathList[pathIndex]);
                query.addBindValue(libraryIndex);
                query.addBindValue(parentIndex);
                if (query.exec()) {
                    newId = query.lastInsertId().toInt();
                }
                else {
                    Base::Console().Log("Error creating path?\n");
                }
            }
        }
        _db.commit();
    }

    // return newId;
    auto index = parentIndex + 1;
    if (index >= (pathList.size() - 1)) {
        return newId;
    }
    return createPath(libraryIndex, index, newId, pathList);
}

int Database::createPath(int libraryIndex, const QString& path)
{
    int newId = 0;
    QStringList pathList = path.split(QLatin1Char('/'));
    if (pathList.isEmpty() || pathList.size() < 2) {
        return 0;
    }
    if (_db.open()) {
        newId = createPath(libraryIndex, 0, 0, pathList);
        _db.close();
    }
    // for (auto p : pathList) {
    //     Base::Console().Log("\t'%s'\n", p.toStdString().c_str());
    // }

    return newId;
}

void Database::createInheritance(const QString& modelUUID, const QString& inheritUUID)
{
    QSqlQuery query(_db);

    // First check if the folder exists
    query.prepare(QLatin1String(
        "SELECT model_inheritance_id FROM model_inheritance WHERE model_id = ? AND inherits_id = ?"));
    query.addBindValue(modelUUID);
    query.addBindValue(inheritUUID);
    query.exec();

    if (!query.next()) {
        query.prepare(
            QLatin1String("INSERT INTO model_inheritance (model_id, inherits_id) "
                          "VALUES (?, ?)"));
        query.addBindValue(modelUUID);
        query.addBindValue(inheritUUID);
        if (!query.exec()) {
            Base::Console().Log("Error creating inheritance\n");
        }
    }
}

void Database::createModelPropertyColumn(int propertyId, const ModelProperty& property)
{
    QSqlQuery query(_db);

    // First check if the property exists
    query.prepare(QLatin1String(
        "SELECT model_property_column_id FROM model_property_column WHERE model_property_id "
        "= ? AND model_property_name = ?"));
    query.addBindValue(propertyId);
    query.addBindValue(property.getName());
    query.exec();

    if (!query.next()) {
        query.prepare(QLatin1String(
            "INSERT INTO model_property_column (model_property_id, model_property_name, "
            "model_property_display_name, model_property_type, "
            "model_property_units, model_property_url, "
            "model_property_description) "
            "VALUES (?, ?, ?, ?, ?, ?, ?)"));
        query.addBindValue(propertyId);
        query.addBindValue(property.getName());
        query.addBindValue(property.getDisplayName());
        query.addBindValue(property.getPropertyType());
        query.addBindValue(property.getUnits());
        query.addBindValue(property.getURL());
        query.addBindValue(property.getDescription());
        if (!query.exec()) {
            Base::Console().Log("Error creating model property column\n");
        }
    }
}

void Database::createModelProperty(const QString& modelUUID, const ModelProperty& property)
{
    QSqlQuery query(_db);

    int propertyId = 0;

    // First check if the property exists
    query.prepare(QLatin1String("SELECT model_property_id FROM model_property WHERE model_id "
                                "= ? AND model_property_name = ?"));
    query.addBindValue(modelUUID);
    query.addBindValue(property.getName());
    query.exec();

    if (query.next()) {
        propertyId = query.value(0).toInt();
    }
    else {
        query.prepare(QLatin1String("INSERT INTO model_property (model_id, model_property_name, "
                                    "model_property_display_name, model_property_type, "
                                    "model_property_units, model_property_url, "
                                    "model_property_description, model_property_inheritance_id) "
                                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"));
        query.addBindValue(modelUUID);
        query.addBindValue(property.getName());
        query.addBindValue(property.getDisplayName());
        query.addBindValue(property.getPropertyType());
        query.addBindValue(property.getUnits());
        query.addBindValue(property.getURL());
        query.addBindValue(property.getDescription());
        query.addBindValue(property.getInheritance());
        if (query.exec()) {
            propertyId = query.value(0).toInt();
        }
        else {
            Base::Console().Log("Error creating model property\n");
        }
    }
    auto columns = property.getColumns();
    for (auto column : columns) {
        // Base::Console().Log("Add column '%s'\n", column.getName().toStdString().c_str());
        createModelPropertyColumn(propertyId, column);
    }
}

void Database::createModel(int libraryIndex,
                           const QString& path,
                           const std::shared_ptr<Model>& model)
{
    // TODO
    // Base::Console().Log("Path = '%s'\n", path.toStdString().c_str());
    auto pathIndex = createPath(libraryIndex, path);

    QString modelId;
    if (_db.open()) {
        if (_db.transaction()) {
            QSqlQuery query(_db);

            // First check if the folder exists
            query.prepare(
                QLatin1String("SELECT model_id FROM model WHERE model_id = ?"));
            query.addBindValue(model->getUUID());
            query.exec();

            if (query.next()) {
                modelId = query.value(0).toString();
            }
            else {
                query.prepare(QLatin1String(
                    "INSERT INTO model (model_id, library_id, folder_id, "
                    "model_name, model_type, model_url, model_description, model_doi) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"));
                query.addBindValue(model->getUUID());
                query.addBindValue(libraryIndex);
                query.addBindValue(pathIndex == 0 ? QVariant() : pathIndex);
                query.addBindValue(model->getName());
                query.addBindValue(model->getBase());
                query.addBindValue(model->getURL());
                query.addBindValue(model->getDescription());
                query.addBindValue(model->getDOI());
                if (query.exec()) {
                    modelId = query.lastInsertId().toString();
                }
                else {
                    Base::Console().Log("Error creating model\n");
                    Base::Console().Log("library %d, folder %d\n", libraryIndex, pathIndex);
                }
            }
        }
        auto inherits = model->getInheritance();
        for (auto inherit : inherits) {
            createInheritance(model->getUUID(), inherit);
        }

        for (auto property : *model) {
            createModelProperty(model->getUUID(), property.second);
        }
        _db.commit();
        _db.close();
    }
}

std::shared_ptr<ModelLibrary> Database::getLibrary(int libraryId)
{
    if (_db.open()) {
        QSqlQuery query(_db);

        // First check if the library exists
        query.prepare(QLatin1String("SELECT library_name, library_icon, library_read_only FROM "
                                    "library WHERE library_id = ?"));
        query.addBindValue(libraryId);
        query.exec();

        if (query.next()) {
            QString libraryName = query.value(0).toString();
            QString libraryIcon = query.value(1).toString();
            bool libraryReadOnly = query.value(2).toBool();

            auto library = std::make_shared<ModelLibrary>(libraryName,
                                                          QString(),
                                                          libraryIcon,
                                                          libraryReadOnly);
            return library;
        }
    }
    _db.close();
    return nullptr;
}

QString Database::getPath(int folderId)
{
    return QString();
}

std::shared_ptr<Model> Database::getModel(const QString& uuid)
{
    if (_db.open()) {
        QSqlQuery query(_db);

        query.prepare(QLatin1String(
            "SELECT library_id, folder_id, model_type, "
            "model_name, model_url, model_description, model_doi FROM model WHERE model_id = ?"));
        query.addBindValue(uuid);
        query.exec();

        if (query.next()) {
            int libraryId = query.value(0).toInt();
            int folderId = query.value(1).toInt();
            QString modelType = query.value(2).toString();
            QString modelName = query.value(3).toString();
            QString modelUrl = query.value(4).toString();
            QString modelDescription = query.value(5).toString();
            QString modelDoi = query.value(6).toString();

            auto model = std::make_shared<Model>();
            model->setUUID(uuid);
            model->setType(modelType);
            model->setName(modelName);
            model->setURL(modelUrl);
            model->setDescription(modelDescription);
            model->setDOI(modelDoi);

            model->setLibrary(getLibrary(libraryId));
            model->setDirectory(getPath(folderId));
        }
        _db.close();
    }

    return nullptr;
}
