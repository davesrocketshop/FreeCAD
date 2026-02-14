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

#include <string>


#include <App/Application.h>

#include "Exceptions.h"
#include "Model.h"
#include "ModelLibrary.h"
#include "ModelManager.h"


using namespace Materials;

TYPESYSTEM_SOURCE(Materials::ModelProperty, Base::BaseClass)

ModelProperty::ModelProperty()
{}

ModelProperty::ModelProperty(const std::string& name,
                             const std::string& header,
                             const std::string& type,
                             const std::string& units,
                             const std::string& url,
                             const std::string& description)
    : _name(name)
    , _displayName(header)
    , _propertyType(type)
    , _units(units)
    , _url(url)
    , _description(description)
{}

ModelProperty::ModelProperty(const ModelProperty& other)
    : _name(other._name)
    , _displayName(other._displayName)
    , _propertyType(other._propertyType)
    , _units(other._units)
    , _url(other._url)
    , _description(other._description)
    , _inheritance(other._inheritance)
    , _columns(other._columns)
{}

const std::string ModelProperty::getDisplayName() const
{
    if (_displayName.empty()) {
        return getName();
    }
    return _displayName;
}

ModelProperty& ModelProperty::operator=(const ModelProperty& other)
{
    if (this == &other) {
        return *this;
    }

    _name = other._name;
    _displayName = other._displayName;
    _propertyType = other._propertyType;
    _units = other._units;
    _url = other._url;
    _description = other._description;
    _inheritance = other._inheritance;
    _columns = other._columns;

    return *this;
}

bool ModelProperty::operator==(const ModelProperty& other) const
{
    if (this == &other) {
        return true;
    }

    return (_name == other._name) && (_displayName == other._displayName)
        && (_propertyType == other._propertyType) && (_units == other._units)
        && (_url == other._url) && (_description == other._description)
        && (_inheritance == other._inheritance);
}

void ModelProperty::validate(const ModelProperty& other) const
{
    if (_name != other._name) {
        throw InvalidProperty("Model names don't match");
    }
    if (getDisplayName() != other.getDisplayName()) {
        Base::Console().log("Local display name '%s'\n", getDisplayName().c_str());
        Base::Console().log("Remote display name '%s'\n",
                            other.getDisplayName().c_str());
        throw InvalidProperty("Model display names don't match");
    }
    if (_propertyType != other._propertyType) {
        throw InvalidProperty("Model property types don't match");
    }
    if (_units != other._units) {
        throw InvalidProperty("Model units don't match");
    }
    if (_url != other._url) {
        throw InvalidProperty("Model URLs don't match");
    }
    if (_description != other._description) {
        throw InvalidProperty("Model descriptions don't match");
    }
    if (_inheritance != other._inheritance) {
        throw InvalidProperty("Model inheritance don't match");
    }

    if (_columns.size() != other._columns.size()) {
        throw InvalidProperty("Model property column counts don't match");
    }
    for (size_t i = 0; i < _columns.size(); i++) {
        _columns[i].validate(other._columns[i]);
    }
}

TYPESYSTEM_SOURCE(Materials::Model, Base::BaseClass)

Model::Model()
    : _dereferenced(false)
    , _dereferencing(false)
{}

Model::Model(
    std::shared_ptr<ModelLibrary> library,
    ModelType type,
    const std::string& name,
    const std::string& directory,
    const std::string& uuid,
    const std::string& description,
    const std::string& url,
    const std::string& doi
)
    : _library(library)
    , _type(type)
    , _name(name)
    , _directory(directory)
    , _uuid(uuid)
    , _description(description)
    , _url(url)
    , _doi(doi)
    , _dereferenced(false)
    , _dereferencing(false)
{}

bool Model::isDisabled() const
{
    return _library->isDisabled();
}

std::string Model::getDirectory() const
{
    return _directory;
}

void Model::setDirectory(const std::string& directory)
{
    _directory = directory;
}

std::string Model::getFilename() const
{
    return _filename;
}

void Model::setFilename(const std::string& filename)
{
    _filename = filename;
}

std::string Model::getFilePath() const
{
    return QDir(QString::fromStdString(_directory + "/" + _filename)).absolutePath().toStdString();
}

bool Model::hasProperty(const std::string& name) const
{
    return _properties.contains(name);
}

ModelProperty& Model::operator[](const std::string& key)
{
    try {
        return _properties.at(key);
    }
    catch (std::out_of_range const&) {
        throw PropertyNotFound();
    }
}

void Model::validate(Model& other) const
{
    try {
        _library->validate(*(other._library));
    }
    catch (const InvalidLibrary& e)
    {
        throw InvalidModel(e.what());
    }

    // std::map<std::string, ModelProperty> _properties;
    if (_type != other._type) {
        throw InvalidModel("Model types don't match");
    }
    if (_name != other._name) {
        throw InvalidModel("Model names don't match");
    }
    if (_directory != other._directory) {
        throw InvalidModel("Model directories don't match");
    }
    if (!other._filename.empty()) {
        throw InvalidModel("Remote filename is not empty");
    }
    if (_uuid != other._uuid) {
        throw InvalidModel("Model UUIDs don't match");
    }
    if (_description != other._description) {
        throw InvalidModel("Model descriptions don't match");
    }
    if (_url != other._url) {
        throw InvalidModel("Model URLs don't match");
    }
    if (_doi != other._doi) {
        throw InvalidModel("Model DOIs don't match");
    }
    if (_inheritedUuids != other._inheritedUuids) {
        throw InvalidModel("Model inherited UUIDs don't match");
    }

    // Need to compare properties
    if (_properties.size() != other._properties.size()) {
        throw InvalidModel("Model property counts don't match");
    }
    for (auto& property : _properties) {
        auto& remote = other._properties[property.first];
        property.second.validate(remote);
    }
}

void Model::save(Base::TextOutputStream& stream)
{
    stream << "---\n";
    stream << "# File created by " << App::Application::Config()["ExeName"]
           << " " << App::Application::Config()["ExeVersion"]
           << " Revision: " << App::Application::Config()["BuildRevision"]
           << "\n";
    saveGeneral(stream);
    saveInherits(stream);
    saveProperties(stream);
}

void Model::saveGeneral(Base::TextOutputStream& stream) const
{
    stream << "General:\n";
    stream << "  UUID: \"" << _uuid << "\"\n";
    stream << "  Name: \"" << MaterialValue::escapeString(_name) << "\"\n";
    if (!_description.empty()) {
        stream << "  Description: \"" << MaterialValue::escapeString(_description) << "\"\n";
    }
    if (!_url.empty()) {
        stream << "  URL: \"" << MaterialValue::escapeString(_url) << "\"\n";
    }
    if (!_doi.empty()) {
        stream << "  DOI: \"" << MaterialValue::escapeString(_doi) << "\"\n";
    }
}

void Model::saveInherits(Base::TextOutputStream& stream) const
{
    if (!_inheritedUuids.empty()) {
        stream << "Inherits:\n";
        for (auto const& uuid : _inheritedUuids) {
            auto model = ModelManager::getManager().getModel(uuid);
            stream << "  - " << model->getName() << ":\n";
            stream << "    UUID: \"" << uuid << "\"\n";
        }
    }
}

void Model::saveProperties(Base::TextOutputStream& stream) const
{
    for (auto& it : _properties) {
        // auto& name = it.first;
        auto& property = it.second;
        stream << property.getName() << ":\n";
        if (!property.getDisplayName().empty()) {
            stream << "    DisplayName: \""
                   << MaterialValue::escapeString(property.getDisplayName()) << "\"\n";
        }
        if (!property.getPropertyType().empty()) {
            stream << "    Type: \"" << MaterialValue::escapeString(property.getPropertyType())
                   << "\"\n";
        }
        if (!property.getUnits().empty()) {
            stream << "    Units: \"" << MaterialValue::escapeString(property.getUnits()) << "\"\n";
        }
        if (!property.getURL().empty()) {
            stream << "    URL: \"" << MaterialValue::escapeString(property.getURL()) << "\"\n";
        }
        if (!property.getDescription().empty()) {
            stream << "    Description: \""
                   << MaterialValue::escapeString(property.getDescription()) << "\"\n";
        }
    }
}
