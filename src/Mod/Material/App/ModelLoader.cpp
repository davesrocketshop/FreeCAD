// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <QDirIterator>
#include <QFileInfo>
#include <QString>

#include <App/Application.h>
#include <Base/FileInfo.h>
#include <Base/Interpreter.h>
#include <Base/Stream.h>


#include "Model.h"
#include "ModelLoader.h"
#include "ModelManager.h"


using namespace Materials;

ModelEntry::ModelEntry(
    const std::shared_ptr<ModelLibraryLocal>& library,
    const QString& baseName,
    const QString& modelName,
    const QString& dir,
    const QString& modelUuid,
    const YAML::Node& modelData
)
    : _library(library)
    , _base(baseName)
    , _name(modelName)
    , _directory(Library::cleanPath(dir))
    , _uuid(modelUuid)
    , _model(modelData)
{}

std::unique_ptr<std::map<QString, std::shared_ptr<ModelEntry>>> ModelLoader::_modelEntryMap = nullptr;

ModelLoader::ModelLoader(
    std::shared_ptr<std::multimap<QString, std::shared_ptr<Model>>> modelMap,
    std::shared_ptr<std::list<std::shared_ptr<ModelLibraryLocal>>> libraryList
)
    : _modelMap(modelMap)
    , _libraryList(libraryList)
{
    loadLibraries();
}

void ModelLoader::addLibrary(std::shared_ptr<ModelLibraryLocal> model)
{
    _libraryList->push_back(model);
}

const QString ModelLoader::getUUIDFromPath(const QString& path)
{
    QFile file(Library::cleanPath(path));
    if (!file.exists()) {
        throw ModelNotFound();
    }

    try {
        Base::FileInfo fi(path.toStdString());
        Base::ifstream str(fi);
        YAML::Node yamlroot = YAML::Load(str);
        std::string base = "Model";
        if (yamlroot["AppearanceModel"]) {
            base = "AppearanceModel";
        }

        const QString uuid = QString::fromStdString(yamlroot[base]["UUID"].as<std::string>());
        return uuid;
    }
    catch (YAML::Exception&) {
        throw ModelNotFound();
    }
}

std::shared_ptr<ModelEntry> ModelLoader::getModelFromPath(
    std::shared_ptr<ModelLibrary> library,
    const QString& path
) const
{
    QFile file(Library::cleanPath(path));
    if (!file.exists()) {
        throw ModelNotFound();
    }

    YAML::Node yamlroot;
    std::string base = "Model";
    std::string uuid;
    std::string name;
    try {
        Base::FileInfo fi(path.toStdString());
        Base::ifstream str(fi);
        yamlroot = YAML::Load(str);
        if (yamlroot["AppearanceModel"]) {
            base = "AppearanceModel";
        }

        uuid = yamlroot[base]["UUID"].as<std::string>();
        name = yamlroot[base]["Name"].as<std::string>();
    }
    catch (YAML::Exception const&) {
        throw InvalidModel();
    }

    auto localLibrary = std::static_pointer_cast<ModelLibraryLocal>(library);
    std::shared_ptr<ModelEntry> model = std::make_shared<ModelEntry>(
        localLibrary,
        QString::fromStdString(base),
        QString::fromStdString(name),
        Library::cleanPath(path),
        QString::fromStdString(uuid),
        yamlroot
    );

    return model;
}

void ModelLoader::showYaml(const YAML::Node& yaml) const
{
    std::stringstream out;

    out << yaml;
    std::string logData = out.str();
    Base::Console().log("%s\n", logData.c_str());
}

QString ModelLoader::yamlValue(
    const YAML::Node& node,
    const std::string& key,
    const std::string& defaultValue
)
{
    if (node[key]) {
        return QString::fromStdString(node[key].as<std::string>());
    }
    return QString::fromStdString(defaultValue);
}

void ModelLoader::addToTree(std::shared_ptr<ModelEntry> model)
{
    std::set<QString> exclude;
    exclude.insert(QStringLiteral("Name"));
    exclude.insert(QStringLiteral("UUID"));
    exclude.insert(QStringLiteral("URL"));
    exclude.insert(QStringLiteral("Description"));
    exclude.insert(QStringLiteral("DOI"));
    exclude.insert(QStringLiteral("Inherits"));

    auto yamlModel = model->getModel();
    if (!model->getLibrary()->isLocal()) {
        throw InvalidLibrary();
    }
    auto library = model->getLibrary();
    auto base = model->getBase().toStdString();
    auto name = model->getName();
    auto directory = model->getDirectory();
    auto uuid = model->getUUID();

    QString description = yamlValue(yamlModel[base], "Description", "");
    QString url = yamlValue(yamlModel[base], "URL", "");
    QString doi = yamlValue(yamlModel[base], "DOI", "");

    Model::ModelType type = (base == "Model") ? Model::ModelType_Physical
                                              : Model::ModelType_Appearance;

    Model finalModel(library, type, name, directory, uuid, description, url, doi);

    // Add inheritance list
    if (yamlModel[base]["Inherits"]) {
        auto inherits = yamlModel[base]["Inherits"];
        for (auto it = inherits.begin(); it != inherits.end(); it++) {
            QString nodeName = QString::fromStdString((*it)["UUID"].as<std::string>());

            finalModel.addInheritance(nodeName);
        }
    }

    // Add property list
    auto yamlProperties = yamlModel[base];
    for (auto it = yamlProperties.begin(); it != yamlProperties.end(); it++) {
        std::string propName = it->first.as<std::string>();
        if (!exclude.contains(QString::fromStdString(propName))) {
            // showYaml(it->second);
            auto yamlProp = yamlProperties[propName];
            auto propDisplayName = yamlValue(yamlProp, "DisplayName", "");
            auto propType = yamlValue(yamlProp, "Type", "");
            auto propUnits = yamlValue(yamlProp, "Units", "");
            auto propURL = yamlValue(yamlProp, "URL", "");
            auto propDescription = yamlValue(yamlProp, "Description", "");
            // auto inherits = yamlValue(yamlProp, "Inherits", "");

            ModelProperty property(
                QString::fromStdString(propName),
                propDisplayName,
                propType,
                propUnits,
                propURL,
                propDescription
            );

            if (propType == QStringLiteral("2DArray") || propType == QStringLiteral("3DArray")) {
                // Read the columns
                auto cols = yamlProp["Columns"];
                for (const auto& col : cols) {
                    std::string colName = col.first.as<std::string>();

                    auto colProp = cols[colName];
                    auto colPropDisplayName = yamlValue(colProp, "DisplayName", "");
                    auto colPropType = yamlValue(colProp, "Type", "");
                    auto colPropUnits = yamlValue(colProp, "Units", "");
                    auto colPropURL = yamlValue(colProp, "URL", "");
                    auto colPropDescription = yamlValue(colProp, "Description", "");
                    ModelProperty colProperty(
                        QString::fromStdString(colName),
                        colPropDisplayName,
                        colPropType,
                        colPropUnits,
                        colPropURL,
                        colPropDescription
                    );

                    property.addColumn(colProperty);
                }
            }

            finalModel.addProperty(property);
        }
    }

    auto sharedModel = library->addModel(finalModel, directory);
    _modelMap->insert({uuid, sharedModel});
}

void ModelLoader::loadLibrary(std::shared_ptr<ModelLibraryLocal> library)
{
    if (_modelEntryMap == nullptr) {
        _modelEntryMap = std::make_unique<std::map<QString, std::shared_ptr<ModelEntry>>>();
    }

    QDirIterator it(library->getDirectory(), QDirIterator::Subdirectories);
    while (it.hasNext()) {
        auto pathname = it.next();
        QFileInfo file(pathname);
        if (file.isFile()) {
            if (file.suffix().toStdString() == "yml") {
                try {
                    auto model = getModelFromPath(library, file.canonicalFilePath());
                    (*_modelEntryMap)[model->getUUID()] = model;
                    // showYaml(model->getModel());
                }
                catch (InvalidModel const&) {
                    Base::Console().log("Invalid model '%s'\n", pathname.toStdString().c_str());
                }
            }
        }
    }

    for (auto it = _modelEntryMap->begin(); it != _modelEntryMap->end(); it++) {
        addToTree(it->second);
    }
    _modelEntryMap->clear();
}

void ModelLoader::loadLibraries()
{
    getModelLibraries();
    if (_libraryList) {
        for (auto it = _libraryList->begin(); it != _libraryList->end(); it++) {
            loadLibrary(*it);
        }
    }
}

void ModelLoader::getModelLibraries()
{
    auto localParam = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Resources/Local"
    );

    // Ensure the builtin libraries have a configuration
    if (!localParam->HasGroup("System")) {
        ModelManager::createSystemLibraryConfig();
    }
    if (!localParam->HasGroup("User")) {
        ModelManager::createUserLibraryConfig();
    }

    auto groups = localParam->GetGroups();
    for (auto& group : groups) {
        auto libName = QString::fromStdString(group->GetGroupName());
        auto libDir = QString::fromStdString(group->GetASCII("ModelDirectory", ""));
        auto libIcon = QString::fromStdString(group->GetASCII("IconPath", ""));
        auto libReadOnly = group->GetBool("ReadOnly", true);
        auto libDisabled = group->GetBool("Disabled", false);

        if (libDir.length() > 0) {
            QDir dir(libDir);
            if (dir.exists()) {
                auto libData = std::make_shared<ModelLibraryLocal>(
                    libName,
                    dir.canonicalPath(),
                    libIcon,
                    libReadOnly
                );
                libData->setDisabled(libDisabled);
                _libraryList->push_back(libData);
            }
        }
    }

    auto moduleParam = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Resources/Modules"
    );
    for (auto& group : moduleParam->GetGroups()) {
        auto moduleName = QString::fromStdString(group->GetGroupName());
        auto modelDir = QString::fromStdString(
            Library::cleanPath(group->GetASCII("ModuleModelDir", ""))
        );
        auto materialIcon = QString::fromStdString(group->GetASCII("ModuleIcon", ""));
        auto materialReadOnly = group->GetBool("ModuleReadOnly", true);
        auto materialDisabled = group->GetBool("ModuleMaterialDisabled", false);

        if (modelDir.length() > 0) {
            QDir dir(modelDir);
            if (dir.exists()) {
                auto libData = std::make_shared<ModelLibraryLocal>(
                    moduleName,
                    dir.canonicalPath(),
                    materialIcon,
                    materialReadOnly
                );
                libData->setModule(true);
                libData->setDisabled(materialDisabled);
                _libraryList->push_back(libData);
            }
        }
    }
}
