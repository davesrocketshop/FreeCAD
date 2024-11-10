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

class MaterialsExport Database
{

public:
    Database();
    virtual ~Database();

    int findLibrary(const QString &name);
    void createLibrary(const QString &name, const QString& icon, bool readOnly=false);
    int createPath(int libraryIndex, const QString& path);
    void createModel(int libraryIndex, const QString& path, const std::shared_ptr<Model>& model);

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
