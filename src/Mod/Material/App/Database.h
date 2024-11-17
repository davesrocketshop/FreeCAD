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

#ifndef MATERIAL_DATABASE_H
#define MATERIAL_DATABASE_H

#include <QSqlDatabase>
#include <QSqlError>

#include <Base/Parameter.h>

#include <Mod/Material/MaterialGlobal.h>

namespace Materials
{

class Model;
class ModelProperty;
class ModelLibrary;

class MaterialsExport DBError
{
public:
    DBError() {}
    DBError(const QSqlError &error)
        : _error(error)
    {}
    virtual ~DBError() = default;

    QSqlError getError() const
    {
        return _error;
    }
private:
    QSqlError _error;
};

class MaterialsExport Database
{

public:
    Database();
    virtual ~Database();

    int findLibrary(const QString &name);
    void createLibrary(const QString &name, const QString& icon, bool readOnly=false);
    int createPath(int libraryIndex, const QString& path);
    void createModel(int libraryIndex, const QString& path, const std::shared_ptr<Model>& model);

    std::shared_ptr<Model> getModel(const QString& uuid);
    QStringList getInherits(const QString &uuid);
    std::shared_ptr<std::vector<ModelProperty>> getModelColumns(const QString& uuid,
                                                                const QString& propertyName);
    std::shared_ptr<std::vector<ModelProperty>> getModelProperties(const QString& uuid);
    std::shared_ptr<ModelLibrary> getLibrary(int libraryId);
    QString getPath(int folderId);

    bool createTables();
    static bool useDatabase();

    QSqlError lastError() const
    {
        return _db.lastError();
    }

    // Types of databases
    static const QString DB_MySQL;
    static const QString DB_Maria;
    static const QString DB_Postgress;
    static const QString DB_SQLServer;
    static const QString DB_SQLite;

    /*
     * Temprorarily ignore foreign keys. Dont' forget to restore!
     */
    void foreignKeysIgnore();
    void foreignKeysRestore();

    /*
     * Nigration routines
     */
    void migrateModelLibrary(const QString &name,
    const std::unique_ptr<std::map<QString, std::shared_ptr<Model>>>& _modelPathMap);

protected:
    void setup();
    void getConnectionSettings();
    static ParameterGrp::handle getParameterGroup();

    QString getConnectionType(ParameterGrp::handle hGrp);
    QString getDBType(ParameterGrp::handle hGrp);
    QString getDBName(ParameterGrp::handle hGrp);
    QString getHostname(ParameterGrp::handle hGrp);
    QString getUsername(ParameterGrp::handle hGrp);
    QString getPassword(ParameterGrp::handle hGrp);

    void createTablesMySQL();
    void createTablesPostgress();
    void createTablesSQLServer();
    void createTablesSQLite();

    void dropTable(const QString& table);

    int createPath(int libraryIndex, int parentIndex, int pathIndex, const QStringList &pathList);
    void createInheritance(const QString& modelUUID, const QString& inheritUUID);
    void createModelProperty(const QString& modelUUID, const ModelProperty& property);
    void createModelPropertyColumn(int propertyId, const ModelProperty& property);

private:
    QSqlDatabase _db;
    QString _connectionType;
    QString _dbType;
    QString _dbName;
    QString _hostname;
    QString _username;
    QString _password;
};

}  // namespace Materials

#endif  // MATERIAL_DATABASE_H
