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
#include "Materials.h"
#include "Model.h"
#include "ModelLibrary.h"

using namespace Materials;

const QString Database::DB_MySQL = QLatin1String("MySQL");
const QString Database::DB_Maria = QLatin1String("MariaDB");
const QString Database::DB_Postgress = QLatin1String("Postgress");
const QString Database::DB_SQLServer = QLatin1String("SQL Server");
const QString Database::DB_SQLite = QLatin1String("SQLite");

// QMutex Database::_mutex;
// LRU::Cache<std::pair<int, const QString&>, int> Database::_folderCache(100);

Database::Database()
{
    // initCache();
    setup();
}

Database::~Database()
{
    // QSqlDatabase::removeDatabase(QLatin1String("material"));
    _db.close();
}

// void Database::initCache()
// {
//     QMutexLocker locker(&_mutex);

//     _cache.capacity(100);

//     // _cache.monitor();
// }

void Database::setup()
{
    getConnectionSettings();

    if (QSqlDatabase::contains(QLatin1String("material"))) {
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
        query.prepare(QLatin1String("SELECT library_id FROM library WHERE library_name = ?"));
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
                    query.prepare(QLatin1String(
                        "INSERT INTO library (library_name, library_icon, library_read_only) "
                        "VALUES (?, ?, ?)"));
                    query.addBindValue(name);
                    query.addBindValue(icon);
                    query.addBindValue(readOnly);
                    query.exec();
                }
                else {
                    query.prepare(
                        QLatin1String("INSERT INTO library (library_name, library_read_only) "
                                      "VALUES (?, ?)"));
                    query.addBindValue(name);
                    query.addBindValue(readOnly);
                    query.exec();
                }
            }
        }
        _db.commit();
        // _db.close();
    }
}

int Database::createPath(int libraryIndex,
                         int parentIndex,
                         int pathIndex,
                         const QStringList& pathList)
{
    // auto cacheIndex = std::pair<int, const QString &>(parentIndex, pathList[pathIndex]);
    // if (_folderCache.contains(cacheIndex)) {
    //     return _folderCache.lookup(uuid);
    // }

    int newId = 0;
    if (parentIndex == 0) {
        // No parent. Root folder
        if (_db.transaction()) {
            QSqlQuery query(_db);

            // First check if the folder exists
            query.prepare(QLatin1String(
                "SELECT folder_id FROM folder WHERE folder_name = ? AND library_id = ?"
                " AND parent_id IS NULL"));
            query.addBindValue(pathList[pathIndex]);
            query.addBindValue(libraryIndex);
            query.exec();

            if (query.next()) {
                newId = query.value(0).toInt();
                // Base::Console().Log("Found folder '%s' id=%d\n",
                // pathList[pathIndex].toStdString().c_str(), newId);
            }
            else {
                query.prepare(QLatin1String("INSERT INTO folder (folder_name, library_id) "
                                            "VALUES (?, ?)"));
                query.addBindValue(pathList[pathIndex]);
                query.addBindValue(libraryIndex);
                // query.addBindValue(parentIndex);
                if (query.exec()) {
                    newId = query.lastInsertId().toInt();
                }
                else {
                    Base::Console().Log("Error creating path?\n");
                    throw DBError(query.lastError());
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
                query.prepare(
                    QLatin1String("INSERT INTO folder (folder_name, library_id, parent_id) "
                                  "VALUES (?, ?, ?)"));
                query.addBindValue(pathList[pathIndex]);
                query.addBindValue(libraryIndex);
                query.addBindValue(parentIndex);
                if (query.exec()) {
                    newId = query.lastInsertId().toInt();
                }
                else {
                    Base::Console().Log("Error creating path?\n");
                    throw DBError(query.lastError());
                }
            }
        }
        _db.commit();
    }
    // _folderCache.emplace(cacheIndex, newId);

    // return newId;
    auto index = parentIndex + 1;
    if (index >= (pathList.size() - 1)) {
        return newId;
    }
    return createPath(libraryIndex, newId, index, pathList);
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
    }

    return newId;
}

/*
 * Temprorarily ignore foreign keys. Dont' forget to restore!
 */

void Database::foreignKeysIgnore()
{
    QSqlQuery query(_db);

    query.prepare(QLatin1String("SET FOREIGN_KEY_CHECKS=0"));
    query.exec();
}

void Database::foreignKeysRestore()
{
    QSqlQuery query(_db);

    query.prepare(QLatin1String("SET FOREIGN_KEY_CHECKS=1"));
    query.exec();
}

void Database::createInheritance(const QString& modelUUID, const QString& inheritUUID)
{
    QSqlQuery query(_db);

    // First check if the folder exists
    query.prepare(QLatin1String("SELECT model_inheritance_id FROM model_inheritance WHERE model_id "
                                "= ? AND inherits_id = ?"));
    query.addBindValue(modelUUID);
    query.addBindValue(inheritUUID);
    query.exec();

    if (!query.next()) {
        // Mass updates may insert models out of sequence creating a foreign key violation
        foreignKeysIgnore();
        query.prepare(QLatin1String("INSERT INTO model_inheritance (model_id, inherits_id) "
                                    "VALUES (?, ?)"));
        query.addBindValue(modelUUID);
        query.addBindValue(inheritUUID);
        if (!query.exec()) {
            auto error = query.lastError();
            Base::Console().Log("Error creating inheritance\n");
            foreignKeysRestore();
            throw DBError(error);
        }
        foreignKeysRestore();
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
            throw DBError(query.lastError());
        }
    }
}

void Database::createModelProperty(const QString& modelUUID, const ModelProperty& property)
{
    if (property.isInherited()) {
        // Property is inherited
        return;
    }
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
                                    "model_property_description) "
                                    "VALUES (?, ?, ?, ?, ?, ?, ?)"));
        query.addBindValue(modelUUID);
        query.addBindValue(property.getName());
        query.addBindValue(property.getDisplayName());
        query.addBindValue(property.getPropertyType());
        query.addBindValue(property.getUnits());
        query.addBindValue(property.getURL());
        query.addBindValue(property.getDescription());
        if (query.exec()) {
            propertyId = query.lastInsertId().toInt();

            auto columns = property.getColumns();
            for (auto column : columns) {
                // Base::Console().Log("Add column '%s', property %d\n",
                //                     column.getName().toStdString().c_str(),
                //                     propertyId);
                createModelPropertyColumn(propertyId, column);
            }
        }
        else {
            Base::Console().Log("Error creating model property\n");
            throw DBError(query.lastError());
        }
    }
}

void Database::createModel(int libraryIndex,
                           const QString& path,
                           const std::shared_ptr<Model>& model)
{
    if (_db.open()) {
        if (_db.transaction()) {
            try {
                // Base::Console().Log("Path = '%s'\n", path.toStdString().c_str());
                // Base::Console().Log("Directory = '%s'\n",
                //                     model->getDirectory().toStdString().c_str());
                auto pathIndex = createPath(libraryIndex, path);

                QSqlQuery query(_db);

                // First check if the folder exists
                query.prepare(QLatin1String("SELECT model_id FROM model WHERE model_id = ?"));
                query.addBindValue(model->getUUID());
                query.exec();

                if (!query.next()) {
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
                        // modelId = query.lastInsertId().toString();
                        auto id = query.lastInsertId();

                        auto inherits = model->getInheritance();
                        for (auto inherit : inherits) {
                            createInheritance(model->getUUID(), inherit);
                        }

                        for (auto property : *model) {
                            createModelProperty(model->getUUID(), property.second);
                        }
                    }
                    else {
                        Base::Console().Log("Error creating model\n");
                        Base::Console().Log("library %d, folder %d\n", libraryIndex, pathIndex);
                        throw DBError(query.lastError());
                    }
                }
                _db.commit();
            }
            catch (const DBError& error) {
                Base::Console().Log("Exception caught\n");
                _db.rollback();
            }
        }
        // _db.close();
    }
}

void Database::createMaterial(int libraryIndex,
                           const QString& path,
                           const std::shared_ptr<Material>& material)
{
    if (_db.open()) {
        if (_db.transaction()) {
            try {
                auto pathIndex = createPath(libraryIndex, path);

                QSqlQuery query(_db);

                // First check if the folder exists
                query.prepare(
                    QLatin1String("SELECT material_id FROM material WHERE material_id = ?"));
                query.addBindValue(material->getUUID());
                query.exec();

                if (!query.next()) {
                    // Mass updates may insert models out of sequence creating a foreign key
                    // violation
                    foreignKeysIgnore();

                    query.prepare(
                        QLatin1String("INSERT INTO material (material_id, library_id, folder_id, "
                                      "material_name, material_author, material_license, "
                                      "material_parent_uuid, material_description, material_url, "
                                      "material_reference) "
                                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
                    query.addBindValue(material->getUUID());
                    query.addBindValue(libraryIndex);
                    query.addBindValue(pathIndex == 0 ? QVariant() : pathIndex);
                    query.addBindValue(material->getName());
                    query.addBindValue(material->getAuthor());
                    query.addBindValue(material->getLicense());
                    query.addBindValue(material->getParentUUID());
                    query.addBindValue(material->getDescription());
                    query.addBindValue(material->getURL());
                    query.addBindValue(material->getReference());
                    if (query.exec()) {
                        auto id = query.lastInsertId();

                        // auto inherits = model->getInheritance();
                        // for (auto inherit : inherits) {
                        //     createInheritance(model->getUUID(), inherit);
                        // }

                        // for (auto property : *model) {
                        //     createModelProperty(model->getUUID(), property.second);
                        // }
                    }
                    else {
                        auto error = query.lastError();
                        Base::Console().Log("Error creating material\n");
                        Base::Console().Log("library %d, folder %d\n", libraryIndex, pathIndex);
                        foreignKeysRestore();
                        throw DBError(error);
                    }
                }
                foreignKeysRestore();
                _db.commit();
            }
            catch (const DBError& error) {
                Base::Console().Log("Exception caught\n");
                _db.rollback();
            }
        }
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

void Database::migrateMaterialLibrary(
    const QString& name,
    const std::unique_ptr<std::map<QString, std::shared_ptr<Material>>>& _materialPathMap)
{
    // _folderCache.clear();
    auto libraryIndex = findLibrary(name);

    for (const auto& [path, material] : *_materialPathMap) {
        createMaterial(libraryIndex, path, material);
    }
}

void Database::migrateModelLibrary(
    const QString& name,
    const std::unique_ptr<std::map<QString, std::shared_ptr<Model>>>& _modelPathMap)
{
    auto libraryIndex = findLibrary(name);

    for (const auto& [path, model] : *_modelPathMap) {
        createModel(libraryIndex, path, model);
    }
}

QString Database::getPath(int folderId)
{
    QString path;
    if (_db.open()) {
        QSqlQuery query(_db);

        query.prepare(
            QLatin1String("SELECT folder_name, parent_id FROM folder WHERE folder_id = ?"));
        query.addBindValue(folderId);
        query.exec();

        if (query.next()) {
            QString folder_name = query.value(0).toString();
            if (query.value(1).isNull()) {
                // path = QLatin1String("/") + folder_name;
                path = folder_name;
            }
            else {
                int parentId = query.value(1).toInt();
                path = getPath(parentId) + QLatin1String("/") + folder_name;
            }
        }
        else {
            Base::Console().Log("Error retrieving path %d\n", folderId);
            throw DBError(query.lastError());
        }
    }
    return path;
}

QStringList Database::getInherits(const QString &uuid)
{
    QStringList inherits;
    if (_db.open()) {
        QSqlQuery query(_db);

        query.prepare(QLatin1String("SELECT inherits_id FROM model_inheritance "
                                    "WHERE model_id = ?"));
        query.addBindValue(uuid);
        query.exec();

        if (!query.isActive()) {
            Base::Console().Log("Error retrieving inheritance for model '%s'\n",
                                uuid.toStdString().c_str());
            throw DBError(query.lastError());
        }
        while (query.next()) {
            QString inheritsUuid = query.value(0).toString();
            inherits.append(inheritsUuid);
        }
    }
    return inherits;
}

std::shared_ptr<std::vector<ModelProperty>> Database::getModelColumns(const QString &uuid, const QString &propertyName)
{
    auto columns = std::make_shared<std::vector<ModelProperty>>();
    if (_db.open()) {
        QSqlQuery query(_db);

        query.prepare(QLatin1String("SELECT model_property_id FROM model_property "
                                    "WHERE model_id = ? AND model_property_name = ?"));
        query.addBindValue(uuid);
        query.addBindValue(propertyName);
        query.exec();

        int propertyId = 0;
        if (query.next()) {
            propertyId = query.value(0).toInt();
        }
        else {
            Base::Console().Log("Error retrieving property columns for model '%s'\n",
                                uuid.toStdString().c_str());
            throw DBError(query.lastError());
        }

        query.prepare(QLatin1String("SELECT model_property_name, "
                                    "model_property_display_name, model_property_type, "
                                    "model_property_units, model_property_url, "
                                    "model_property_description FROM model_property_column "
                                    "WHERE model_property_id = ?"));
        query.addBindValue(propertyId);
        query.exec();

        if (!query.isActive()) {
            Base::Console().Log("Error retrieving property columns for property '%d'\n",
                                propertyId);
            throw DBError(query.lastError());
        }
        while (query.next()) {
            ModelProperty prop;
            QString propName = query.value(0).toString();
            QString propDisplayName = query.value(1).toString();
            QString propType = query.value(2).toString();
            QString propUnits = query.value(3).toString();
            QString propUrl = query.value(4).toString();
            QString propDescription = query.value(5).toString();
            prop.setName(propName);
            prop.setColumnHeader(propDisplayName);
            prop.setPropertyType(propType);
            prop.setUnits(propUnits);
            prop.setURL(propUrl);
            prop.setDescription(propDescription);

            columns->push_back(prop);
        }
    }
    return columns;
}

std::shared_ptr<std::vector<ModelProperty>> Database::getModelProperties(const QString& uuid)
{
    auto properties = std::make_shared<std::vector<ModelProperty>>();
    if (_db.open()) {
        QSqlQuery query(_db);

        query.prepare(QLatin1String("SELECT model_property_name, "
                                    "model_property_display_name, model_property_type, "
                                    "model_property_units, model_property_url, "
                                    "model_property_description FROM model_property "
                                    "WHERE model_id = ?"));
        query.addBindValue(uuid);
        query.exec();

        if (!query.isActive()) {
            Base::Console().Log("Error retrieving properties for model '%s'\n",
                                uuid.toStdString().c_str());
            throw DBError(query.lastError());
        }
        while (query.next()) {
            ModelProperty prop;
            QString propName = query.value(0).toString();
            QString propDisplayName = query.value(1).toString();
            QString propType = query.value(2).toString();
            QString propUnits = query.value(3).toString();
            QString propUrl = query.value(4).toString();
            QString propDescription = query.value(5).toString();
            prop.setName(propName);
            prop.setColumnHeader(propDisplayName);
            prop.setPropertyType(propType);
            prop.setUnits(propUnits);
            prop.setURL(propUrl);
            prop.setDescription(propDescription);

            properties->push_back(prop);
        }

        // This has to happen after the properties are retrieved as QSql doesn't support nested queries
        for (auto& property : *properties) {
            auto columns = getModelColumns(uuid, property.getName());
            for (auto column : *columns) {
                property.addColumn(column);
            }
        }
    }
    return properties;
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

            auto path = getPath(folderId) + QLatin1String("/") + modelName;
            // Base::Console().Log("path '%s'\n", path.toStdString().c_str());
            model->setDirectory(path);

            auto inherits = getInherits(uuid);
            for (auto& inherit : inherits) {
                model->addInheritance(inherit);
            }

            auto properties = getModelProperties(uuid);
            for (auto property : *properties) {
                model->addProperty(property);
            }
            return model;
        }
        // _db.close();
    }

    return nullptr;
}
