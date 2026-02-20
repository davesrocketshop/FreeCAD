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

#include <QMetaType>
#include <QRegularExpression>
#include <QString>


#include <App/Application.h>
#include <Base/FileInfo.h>
#include <Base/Interpreter.h>
#include <Base/Stream.h>
#include <Gui/MetaTypes.h>

#include "Materials.h"

#include "MaterialConfigLoader.h"
#include "MaterialLibrary.h"
#include "MaterialLoader.h"
#include "Model.h"
#include "ModelManager.h"

namespace fs = std::filesystem;

using namespace Materials;

MaterialEntry::MaterialEntry(
    const std::shared_ptr<MaterialLibraryLocal>& library,
    const std::string& modelName,
    const std::string& dir,
    const std::string& modelUuid
)
    : _library(library)
    , _name(modelName)
    , _directory(Library::cleanPath(dir))
    , _uuid(modelUuid)
{}

MaterialYamlEntry::MaterialYamlEntry(
    const std::shared_ptr<MaterialLibraryLocal>& library,
    const std::string& modelName,
    const std::string& dir,
    const std::string& modelUuid,
    const YAML::Node& modelData
)
    : MaterialEntry(library, modelName, dir, modelUuid)
    , _model(modelData)
{}

std::string MaterialYamlEntry::yamlValue(
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

std::shared_ptr<std::vector<QVariant>> MaterialYamlEntry::readList(const YAML::Node& node, bool isImageList)
{
    auto list = std::make_shared<std::vector<QVariant>>();
    for (auto it = node.begin(); it != node.end(); it++) {
        QVariant nodeValue;
        if (isImageList) {
            nodeValue = QString::fromStdString(it->as<std::string>())
                            .remove(QRegularExpression(QStringLiteral("[\r\n]")));
        }
        else {
            nodeValue = QString::fromStdString(it->as<std::string>());
        }
        list->push_back(nodeValue);
    }

    return list;
}

std::shared_ptr<std::vector<QVariant>> MaterialYamlEntry::readImageList(const YAML::Node& node)
{
    return readList(node, true);
}

std::shared_ptr<Array2D> MaterialYamlEntry::read2DArray(const YAML::Node& node, int columns)
{
    auto array2d = std::make_shared<Array2D>();
    array2d->setColumns(columns);

    if (node.size() == 1 || node.size() == 2) {
        // There used to be a default value. Ignore it.
        auto yamlArray = node[0];
        if (node.size() == 2) {
            yamlArray = node[1];
        }

        for (std::size_t i = 0; i < yamlArray.size(); i++) {
            auto yamlRow = yamlArray[i];

            auto row = std::make_shared<std::vector<QVariant>>();
            for (std::size_t j = 0; j < yamlRow.size(); j++) {
                Base::Quantity qq = Base::Quantity::parse(yamlRow[j].as<std::string>());
                qq.setFormat(MaterialValue::getQuantityFormat());
                row->push_back(QVariant::fromValue(qq));
            }
            array2d->addRow(row);
        }
    }

    return array2d;
}

std::shared_ptr<Array3D> MaterialYamlEntry::read3DArray(const YAML::Node& node, int columns)
{
    auto array3d = std::make_shared<Array3D>();
    array3d->setColumns(columns - 1);  // First column is third dimension

    if (node.size() == 1 || node.size() == 2) {
        // There used to be a default value. Ignore it.
        auto yamlArray = node[0];
        if (node.size() == 2) {
            yamlArray = node[1];
        }

        for (std::size_t depth = 0; depth < yamlArray.size(); depth++) {
            auto yamlDepth = yamlArray[depth];
            for (auto it = yamlDepth.begin(); it != yamlDepth.end(); it++) {
                auto depthValue = Base::Quantity::parse(it->first.as<std::string>());
                depthValue.setFormat(MaterialValue::getQuantityFormat());
                array3d->addDepth(depth, depthValue);

                auto yamlTable = it->second;
                for (std::size_t i = 0; i < yamlTable.size(); i++) {
                    auto yamlRow = yamlTable[i];

                    auto row = std::make_shared<std::vector<Base::Quantity>>();
                    for (std::size_t j = 0; j < yamlRow.size(); j++) {
                        auto qq = Base::Quantity::parse(yamlRow[j].as<std::string>());
                        qq.setFormat(MaterialValue::getQuantityFormat());
                        row->push_back(qq);
                    }
                    array3d->addRow(depth, row);
                }
            }
        }
    }

    return array3d;
}

std::shared_ptr<Material> MaterialYamlEntry::toMaterial() const
{
    std::set<std::string> exclude;
    exclude.insert("General");
    exclude.insert("Inherits");

    auto yamlModel = getModel();
    auto library = getLibrary();
    auto name = getName();
    auto directory = getDirectory();
    std::string uuid = getUUID();

    std::string author = yamlValue(yamlModel["General"], "Author", "");
    std::string license = yamlValue(yamlModel["General"], "License", "");
    std::string description = yamlValue(yamlModel["General"], "Description", "");
    std::string sourceReference = yamlValue(yamlModel["General"], "ReferenceSource", "");
    std::string sourceURL = yamlValue(yamlModel["General"], "SourceURL", "");

    std::shared_ptr<Material> finalModel = std::make_shared<Material>(
        library,
        directory,
        uuid,
        name
    );
    finalModel->setAuthor(author);
    finalModel->setLicense(license);
    finalModel->setDescription(description);
    finalModel->setReference(sourceReference);
    finalModel->setURL(sourceURL);

    if (yamlModel["General"]["Tags"]) {
        auto tags = readList(yamlModel["General"]["Tags"]);
        for (auto tag : *tags) {
            finalModel->addTag(tag.toString().toStdString());
        }
    }

    // Add inheritance list
    if (yamlModel["Inherits"]) {
        auto inherits = yamlModel["Inherits"];
        for (auto it = inherits.begin(); it != inherits.end(); it++) {
            auto nodeName = it->second["UUID"].as<std::string>();

            finalModel->setParentUUID(nodeName);  // Should only be one.
                                                                          // Need to check
        }
    }

    // Add material models
    if (yamlModel["Models"]) {
        auto models = yamlModel["Models"];
        for (auto it = models.begin(); it != models.end(); it++) {
            auto modelName = (it->first).as<std::string>();

            // Add the model uuid
            auto modelNode = models[modelName];
            auto modelUUID = modelNode["UUID"].as<std::string>();
            finalModel->addPhysical(modelUUID);

            // Add the property values
            auto properties = yamlModel["Models"][modelName];
            for (auto itp = properties.begin(); itp != properties.end(); itp++) {
                auto propertyName = (itp->first).as<std::string>();
                if (finalModel->hasPhysicalProperty(propertyName)) {
                    auto prop = finalModel->getPhysicalProperty(propertyName);
                    auto type = prop->getType();

                    try {
                        if (type == MaterialValue::List || type == MaterialValue::FileList) {
                            auto list = readList(itp->second);
                            finalModel->setPhysicalValue(propertyName, list);
                        }
                        else if (type == MaterialValue::ImageList) {
                            auto list = readImageList(itp->second);
                            finalModel->setPhysicalValue(propertyName, list);
                        }
                        else if (type == MaterialValue::Array2D) {
                            auto array2d = read2DArray(itp->second, prop->columns());
                            finalModel->setPhysicalValue(propertyName, array2d);
                        }
                        else if (type == MaterialValue::Array3D) {
                            auto array3d = read3DArray(itp->second, prop->columns());
                            finalModel->setPhysicalValue(propertyName, array3d);
                        }
                        else {
                            QString propertyValue = QString::fromStdString(
                                (itp->second).as<std::string>()
                            );
                            if (type == MaterialValue::Image) {
                                propertyValue = propertyValue.remove(
                                    QRegularExpression(QStringLiteral("[\r\n]"))
                                );
                            }
                            try {
                                finalModel->setPhysicalValue(
                                    propertyName,
                                    propertyValue.toStdString()
                                );
                            }
                            catch (const Base::ValueError&) {
                                // Units mismatch
                                Base::Console().log(
                                    "Units mismatch in material '%s':'%s' = '%s', "
                                    "setting to default property units '%s'\n",
                                    name.c_str(),
                                    propertyName.c_str(),
                                    propertyValue.toStdString().c_str(),
                                    prop->getUnits().c_str()
                                );
                                auto quantity = Base::Quantity::parse(propertyValue.toStdString());
                                finalModel->setPhysicalValue(
                                    propertyName,
                                    Base::Quantity(quantity.getValue(), prop->getUnits())
                                );
                            }
                        }
                    }
                    catch (const YAML::BadConversion& e) {
                        Base::Console().log(
                            "Exception %s <%s:%s> - ignored\n",
                            e.what(),
                            name.c_str(),
                            propertyName.c_str()
                        );
                    }
                }
                else if (propertyName != "UUID") {
                    Base::Console().log(
                        "\tProperty '%s' is not described by any model. Ignored\n",
                        propertyName.c_str()
                    );
                }
            }
        }
    }

    // Add appearance models
    if (yamlModel["AppearanceModels"]) {
        auto models = yamlModel["AppearanceModels"];
        for (auto it = models.begin(); it != models.end(); it++) {
            auto modelName = (it->first).as<std::string>();

            // Add the model uuid
            auto modelNode = models[modelName];
            auto modelUUID = modelNode["UUID"].as<std::string>();
            finalModel->addAppearance(modelUUID);

            // Add the property values
            auto properties = yamlModel["AppearanceModels"][modelName];
            for (auto itp = properties.begin(); itp != properties.end(); itp++) {
                auto propertyName = (itp->first).as<std::string>();
                if (finalModel->hasAppearanceProperty(propertyName)) {
                    auto prop = finalModel->getAppearanceProperty(propertyName);
                    auto type = prop->getType();

                    try {
                        if (type == MaterialValue::List || type == MaterialValue::FileList) {
                            auto list = readList(itp->second);
                            finalModel->setAppearanceValue(propertyName, QVariant::fromValue(list));
                        }
                        else if (type == MaterialValue::ImageList) {
                            auto list = readImageList(itp->second);
                            finalModel->setAppearanceValue(propertyName, QVariant::fromValue(list));
                        }
                        else if (type == MaterialValue::Array2D) {
                            auto array2d = read2DArray(itp->second, prop->columns());
                            finalModel->setAppearanceValue(propertyName, array2d);
                        }
                        else if (type == MaterialValue::Array3D) {
                            auto array3d = read3DArray(itp->second, prop->columns());
                            finalModel->setAppearanceValue(propertyName, array3d);
                        }
                        else {
                            auto propertyValue = QString::fromStdString(
                                (itp->second).as<std::string>()
                            );
                            if (type == MaterialValue::Image) {
                                propertyValue = propertyValue.remove(
                                    QRegularExpression(QStringLiteral("[\r\n]"))
                                );
                            }
                            finalModel->setAppearanceValue(
                                propertyName,
                                propertyValue.toStdString()
                            );
                        }
                    }
                    catch (const YAML::BadConversion& e) {
                        Base::Console().log(
                            "Exception %s <%s:%s> - ignored\n",
                            e.what(),
                            name.c_str(),
                            propertyName.c_str()
                        );
                    }
                }
                else if (propertyName != "UUID") {
                    Base::Console().log(
                        "\tProperty '%s' is not described by any model. Ignored\n",
                        propertyName.c_str()
                    );
                }
            }
        }
    }

    return finalModel;
}

void MaterialYamlEntry::addToTree(
    std::shared_ptr<std::map<std::string, std::shared_ptr<Material>>> materialMap
)
{
    auto material = toMaterial();
    (*materialMap)[material->getUUID()] = getLibrary()->addMaterial(material, material->getDirectory());
}

//===

std::unique_ptr<std::map<std::string, std::shared_ptr<MaterialEntry>>> MaterialLoader::_materialEntryMap
    = nullptr;

MaterialLoader::MaterialLoader(
    const std::shared_ptr<std::map<std::string, std::shared_ptr<Material>>>& materialMap,
    const std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>>& libraryList
)
    : _materialMap(materialMap)
    , _libraryList(libraryList)
{
    loadLibraries(libraryList);
}

void MaterialLoader::addLibrary(const std::shared_ptr<MaterialLibraryLocal>& model)
{
    _libraryList->push_back(model);
}

std::shared_ptr<MaterialYamlEntry> MaterialLoader::getMaterialFromYAML(
    const std::shared_ptr<MaterialLibraryLocal>& library,
    YAML::Node& yamlroot,
    const std::string& path
)
{
    std::shared_ptr<MaterialYamlEntry> material = nullptr;

    try {
        auto uuid = yamlroot["General"]["UUID"].as<std::string>();

        // Always get the name from the filename
        std::string clean = Library::cleanPath(path);
        Base::FileInfo filepath(clean);
        std::string name = filepath.fileNamePure();

        material = std::make_shared<MaterialYamlEntry>(library, name, clean, uuid, yamlroot);
    }
    catch (YAML::Exception const& e) {
        Base::Console().error("YAML parsing error: '%s'\n", path.c_str());
        Base::Console().error("\t'%s'\n", e.what());
        showYaml(yamlroot);
    }


    return material;
}

std::shared_ptr<Material>
MaterialLoader::getMaterialFromPath(const std::shared_ptr<MaterialLibraryLocal>& library, const std::string& path)
{
    std::string pathName = Library::cleanPath(path);
    std::shared_ptr<MaterialYamlEntry> material = nullptr;

    if (MaterialConfigLoader::isConfigStyle(path)) {
        auto material = MaterialConfigLoader::getMaterialFromPath(library, pathName);
        return material;
    }

    Base::FileInfo info(pathName);
    Base::ifstream fin(info);
    if (!fin) {
        Base::Console().error("YAML file open error: '%s'\n", pathName.c_str());
        return nullptr;
    }

    YAML::Node yamlroot;
    try {
        yamlroot = YAML::Load(fin);

        material = getMaterialFromYAML(library, yamlroot, pathName);
    }
    catch (YAML::Exception const& e) {
        Base::Console().error("YAML parsing error: '%s'\n", pathName.c_str());
        Base::Console().error("\t'%s'\n", e.what());
        showYaml(yamlroot);
    }


    return material->toMaterial();

}

std::shared_ptr<MaterialYamlEntry> MaterialLoader::getMaterialEntryFromPath(
    const std::shared_ptr<MaterialLibraryLocal>& library,
    const std::string& path
) const
{
    std::string pathName = Library::cleanPath(path);
    std::shared_ptr<MaterialYamlEntry> material = nullptr;

    if (MaterialConfigLoader::isConfigStyle(path)) {
        auto material = MaterialConfigLoader::getMaterialFromPath(library, pathName);
        if (material) {
            (*_materialMap)[material->getUUID()] = library->addMaterial(material, pathName);
        }

        // Return the nullptr as there are no intermediate steps to take, such
        // as checking inheritance
        return nullptr;
    }

    Base::FileInfo info(pathName);
    Base::ifstream fin(info);
    if (!fin) {
        Base::Console().error("YAML file open error: '%s'\n", pathName.c_str());
        return material;
    }

    YAML::Node yamlroot;
    try {
        yamlroot = YAML::Load(fin);

        material = getMaterialFromYAML(library, yamlroot, pathName);
    }
    catch (YAML::Exception const& e) {
        Base::Console().error("YAML parsing error: '%s'\n", pathName.c_str());
        Base::Console().error("\t'%s'\n", e.what());
        showYaml(yamlroot);
    }


    return material;
}

void MaterialLoader::showYaml(const YAML::Node& yaml)
{
    std::stringstream out;

    out << yaml;
    std::string logData = out.str();
    Base::Console().log("%s\n", logData.c_str());
}


void MaterialLoader::dereference(
    const std::shared_ptr<std::map<std::string, std::shared_ptr<Material>>>& materialMap,
    const std::shared_ptr<Material>& material
)
{
    // Avoid recursion
    if (material->getDereferenced()) {
        return;
    }

    auto parentUUID = material->getParentUUID();
    if (parentUUID.size() > 0) {
        std::shared_ptr<Material> parent;
        try {
            parent = materialMap->at(parentUUID);
        }
        catch (std::out_of_range&) {
            Base::Console().log(
                "Unable to apply inheritance for material '%s', parent '%s' not found.\n",
                material->getName().c_str(),
                parentUUID.c_str()
            );
            return;
        }

        // Ensure the parent has been dereferenced
        dereference(materialMap, parent);

        // Add physical models
        auto modelVector = parent->getPhysicalModels();
        for (auto& model : *modelVector) {
            if (!material->hasPhysicalModel(model)) {
                material->addPhysical(model);
            }
        }

        // Add appearance models
        modelVector = parent->getAppearanceModels();
        for (auto& model : *modelVector) {
            if (!material->hasAppearanceModel(model)) {
                material->addAppearance(model);
            }
        }

        // Add values
        auto properties = parent->getPhysicalProperties();
        for (auto& itp : properties) {
            auto name = itp.first;
            auto property = itp.second;

            if (material->getPhysicalProperty(name)->isNull()) {
                material->getPhysicalProperty(name)->setValue(property->getValue());
            }
        }

        properties = parent->getAppearanceProperties();
        for (auto& itp : properties) {
            auto name = itp.first;
            auto property = itp.second;

            if (material->getAppearanceProperty(name)->isNull()) {
                material->getAppearanceProperty(name)->setValue(property->getValue());
            }
        }
    }

    material->markDereferenced();
}

void MaterialLoader::dereference(const std::shared_ptr<Material>& material)
{
    dereference(_materialMap, material);
}

void MaterialLoader::loadLibrary(const std::shared_ptr<MaterialLibraryLocal>& library)
{
    if (_materialEntryMap == nullptr) {
        _materialEntryMap = std::make_unique<std::map<std::string, std::shared_ptr<MaterialEntry>>>();
    }

    Base::FileInfo dirInfo(library->getDirectory());
    auto dirList = dirInfo.getDirectoryContentRecursive(); // This needs to be recursive
    for (auto file : dirList) {
        if (file.isFile()) {
            if (file.hasExtension("FCMat")) {
                try {
                    auto material = getMaterialEntryFromPath(
                        library,
                        ManagedLibrary::cleanPath(file.filePath())
                    );
                    if (material) {
                        (*_materialEntryMap)[material->getUUID()] = material;
                    }
                }
                catch (const MaterialReadError&) {
                    // Ignore the file. Error messages should have already been logged
                }
                catch (fs::filesystem_error const& ex) {
                }
            }
        }
    }

    for (auto& it : *_materialEntryMap) {
        it.second->addToTree(_materialMap);
    }
}

void MaterialLoader::loadLibraries(
    const std::shared_ptr<std::list<std::shared_ptr<MaterialLibrary>>>& libraryList
)
{
    if (libraryList) {
        for (auto& it : *libraryList) {
            if (it->isLocal() && !it->isDisabled()) {
                auto materialLibrary = std::make_shared<MaterialLibraryLocal>(*it);
                loadLibrary(materialLibrary);
            }
        }
    }

    for (auto& it : *_materialMap) {
        dereference(it.second);
        it.second->resetEditState();
    }
}

std::shared_ptr<std::list<std::string>> MaterialLoader::getMaterialFolders(
    const MaterialLibraryLocal& library
)
{
    std::shared_ptr<std::list<std::string>> pathList = std::make_shared<std::list<std::string>>();
    Base::FileInfo dirInfo(library.getDirectory());
    for (auto file : dirInfo.getDirectoryContentRecursive()) {
        if (file.isDir()) {
            auto libraryPath = Base::FileInfo::stringToPath(library.getDirectory());
            auto filePath = Base::FileInfo::stringToPath(file.filePath());
            std::string path = Base::FileInfo::pathToString(fs::relative(filePath, libraryPath));
            if (!path.starts_with(".")) {
                pathList->push_back(Library::cleanPath(path));
            }
        }
    }

    return pathList;
}
