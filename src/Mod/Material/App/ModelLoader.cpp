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

#include <QString>

#include <App/Application.h>
#include <Base/FileInfo.h>
#include <Base/Interpreter.h>
#include <Base/Stream.h>

#include "LibraryManager.h"
#include "Model.h"
#include "ModelLoader.h"
#include "ModelManager.h"


using namespace Materials;

ModelEntry::ModelEntry(
    const std::shared_ptr<ModelLibraryLocal>& library,
    const std::string& baseName,
    const std::string& modelName,
    const std::string& dir,
    const std::string& modelUuid,
    const YAML::Node& modelData
)
    : _library(library)
    , _base(baseName)
    , _name(modelName)
    , _directory(Library::cleanPath(dir))
    , _uuid(modelUuid)
    , _model(modelData)
{}

std::unique_ptr<std::map<std::string, std::shared_ptr<ModelEntry>>> ModelLoader::_modelEntryMap = nullptr;

ModelLoader::ModelLoader(std::shared_ptr<std::multimap<std::string, std::shared_ptr<Model>>> modelMap)
    : _modelMap(modelMap)
{
    loadLibraries();
}

const std::string ModelLoader::getUUIDFromPath(const std::string& path)
{
    QFile file(Library::cleanPath(QString::fromStdString(path)));
    if (!file.exists()) {
        throw ModelNotFound();
    }

    try {
        Base::FileInfo fi(path);
        Base::ifstream str(fi);
        YAML::Node yamlroot = YAML::Load(str);
        std::string base = "Model";
        if (yamlroot["AppearanceModel"]) {
            base = "AppearanceModel";
        }

        const std::string uuid = yamlroot[base]["UUID"].as<std::string>();
        return uuid;
    }
    catch (YAML::Exception&) {
        throw ModelNotFound();
    }
}

std::shared_ptr<ModelEntry> ModelLoader::getModelFromPath(
    std::shared_ptr<ModelLibrary> library,
    const std::string& path
) const
{
    QFile file(Library::cleanPath(QString::fromStdString(path)));
    if (!file.exists()) {
        throw ModelNotFound();
    }

    YAML::Node yamlroot;
    std::string base = "Model";
    std::string uuid;
    std::string name;
    try {
        Base::FileInfo fi(path);
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
    std::shared_ptr<ModelEntry> model
        = std::make_shared<ModelEntry>(localLibrary, base, name, Library::cleanPath(path), uuid, yamlroot);

    return model;
}

void ModelLoader::showYaml(const YAML::Node& yaml) const
{
    std::stringstream out;

    out << yaml;
    std::string logData = out.str();
    Base::Console().log("%s\n", logData.c_str());
}

std::string ModelLoader::yamlValue(
    const YAML::Node& node,
    const std::string& key,
    const std::string& defaultValue
)
{
    if (node[key]) {
        return node[key].as<std::string>();
    }
    return defaultValue;
}

void ModelLoader::addToTree(std::shared_ptr<ModelEntry> model)
{
    std::set<std::string> exclude;
    exclude.insert("Name");
    exclude.insert("UUID");
    exclude.insert("URL");
    exclude.insert("Description");
    exclude.insert("DOI");
    exclude.insert("Inherits");

    auto yamlModel = model->getModel();
    if (!model->getLibrary()->isLocal()) {
        throw InvalidLibrary();
    }
    auto library = model->getLibrary();
    auto base = model->getBase();
    auto name = model->getName();
    auto directory = model->getDirectory();
    auto uuid = model->getUUID();

    std::string description = yamlValue(yamlModel[base], "Description", "");
    std::string url = yamlValue(yamlModel[base], "URL", "");
    std::string doi = yamlValue(yamlModel[base], "DOI", "");

    Model::ModelType type = (base == "Model") ? Model::ModelType_Physical
                                              : Model::ModelType_Appearance;

    Model finalModel(
        library,
        type,
        QString::fromStdString(name),
        QString::fromStdString(directory),
        QString::fromStdString(uuid),
        QString::fromStdString(description),
        QString::fromStdString(url),
        QString::fromStdString(doi)
    );

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
        if (!exclude.contains(propName)) {
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
                QString::fromStdString(propDisplayName),
                QString::fromStdString(propType),
                QString::fromStdString(propUnits),
                QString::fromStdString(propURL),
                QString::fromStdString(propDescription)
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
                        QString::fromStdString(colPropDisplayName),
                        QString::fromStdString(colPropType),
                        QString::fromStdString(colPropUnits),
                        QString::fromStdString(colPropURL),
                        QString::fromStdString(colPropDescription)
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
        _modelEntryMap = std::make_unique<std::map<std::string, std::shared_ptr<ModelEntry>>>();
    }

    Base::FileInfo dirInfo(library->getDirectory());
    for (auto file: dirInfo.getDirectoryContentRecursive()) {
        if (file.isFile()) {
            if (file.hasExtension("yml")) {
                try {
                    auto model = getModelFromPath(library, file.filePath());
                    (*_modelEntryMap)[model->getUUID()] = model;
                    // showYaml(model->getModel());
                }
                catch (InvalidModel const&) {
                    Base::Console().log("Invalid model '%s'\n", file.filePath().c_str());
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
    auto libraries = LibraryManager::getManager().getLocalModelLibraries(false);
    if (libraries) {
        for (auto it = libraries->begin(); it != libraries->end(); it++) {
            auto local = std::dynamic_pointer_cast<ModelLibraryLocal>(*it);
            if (local) {
                loadLibrary(local);
            }
        }
    }
}
