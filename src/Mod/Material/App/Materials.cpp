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
#include <QUuid>
#include <QDir>

#include <App/Application.h>
#include <Gui/MetaTypes.h>

#include "Materials.h"

#include "MaterialLibrary.h"
#include "MaterialProperty.h"
#include "MaterialManager.h"
#include "ModelManager.h"
#include "ModelUuids.h"
#include "StringUtility.h"


using namespace Materials;

/* TRANSLATOR Material::Materials */

TYPESYSTEM_SOURCE(Materials::Material, Base::BaseClass)

Material::Material()
    : _dereferenced(false)
    , _oldFormat(false)
    , _editState(MaterialEdit_None)
{
    // Create an initial UUID
    newUuid();
}

Material::Material(const std::shared_ptr<MaterialLibrary>& library,
                   const std::string& directory,
                   const std::string& uuid,
                   const std::string& name)
    : _library(library)
    , _directory(directory)
    , _uuid(uuid)
    , _name(name)
    , _dereferenced(false)
    , _oldFormat(false)
    , _editState(MaterialEdit_None)
{
    setDirectory(directory);
}

Material::Material(const Material& other)
    : _library(other._library)
    , _directory(other._directory)
    , _uuid(other._uuid)
    , _name(other._name)
    , _author(other._author)
    , _license(other._license)
    , _parentUuid(other._parentUuid)
    , _description(other._description)
    , _url(other._url)
    , _reference(other._reference)
    , _dereferenced(other._dereferenced)
    , _oldFormat(other._oldFormat)
    , _editState(other._editState)
{
    for (auto& it : other._tags) {
        _tags.insert(it);
    }
    for (auto& it : other._physicalUuids) {
        _physicalUuids.insert(it);
    }
    for (auto& it : other._appearanceUuids) {
        _appearanceUuids.insert(it);
    }
    for (auto& it : other._allUuids) {
        _allUuids.insert(it);
    }
    for (auto& it : other._physical) {
        MaterialProperty prop(it.second);
        _physical[it.first] = std::make_shared<MaterialProperty>(prop);
    }
    for (auto& it : other._appearance) {
        MaterialProperty prop(it.second);
        _appearance[it.first] = std::make_shared<MaterialProperty>(prop);
    }
    for (auto& it : other._legacy) {
        _legacy[it.first] = it.second;
    }
}

bool Material::isDisabled() const
{
    return _library->isDisabled();
}

void Material::setLibrary(const std::shared_ptr<MaterialLibrary>& library)
{
    _library = library;
    setEditStateChanged();
}

std::string Material::getDirectory() const
{
    return _directory;
}

void Material::setDirectory(const std::string& directory)
{
    _directory = directory;
    setEditStateChanged();
}

std::string Material::getFilePath() const
{
    return QDir(QString::fromStdString(_directory + "/" + _name + ".FCMat"))
        .absolutePath()
        .toStdString();
}

std::string Material::getLibraryPath() const
{
    std::string path;
    auto library = getLibrary();
    auto directory = getDirectory();
    if (directory.starts_with("/")) {
        // Remove leading '/' for consistent handling
        directory.erase(0, 1);
    }
    if (library) {
        if (!directory.empty()) {
            path ="[" + library->getName() + "]/" + directory + "/" + getName();
        }
        else {
            path = "[" + library->getName() + "]/" + getName();
        }
        return path;
    }

    if (!directory.empty()) {
        path = "/" + directory + "/" + getName();
    }
    else {
        path = "/" + getName();
    }
    return path;
}

std::string Material::getAuthorAndLicense() const
{
    std::string authorAndLicense;

    // Combine the author and license field for backwards compatibility
    if (!_author.empty()) {
        authorAndLicense = _author;
        if (!_license.empty()) {
            authorAndLicense += " " + _license;
        }
    }
    else if (!_license.empty()) {
        authorAndLicense = _license;
    }

    return authorAndLicense;
}

void Material::addModel(const std::string& uuid)
{
    for (const auto& modelUUID : std::as_const(_allUuids)) {
        if (modelUUID == uuid) {
            return;
        }
    }

    _allUuids << uuid;

    auto& manager = ModelManager::getManager();

    try {
        auto model = manager.getModel(uuid);
        auto inheritance = model->getInheritance();
        for (auto& inherits : inheritance) {
            addModel(inherits);
        }
    }
    catch (ModelNotFound const&) {
    }
}

void Material::clearModels()
{
    _physicalUuids.clear();
    _appearanceUuids.clear();
    _allUuids.clear();
    _physical.clear();
    _appearance.clear();
}

void Material::clearInherited()
{
    _allUuids.clear();

    // Rebuild the UUID lists without the inherited UUIDs
    for (auto& uuid : _physicalUuids) {
        _allUuids << uuid;
    }
    for (auto& uuid : _appearanceUuids) {
        _allUuids << uuid;
    }
}

void Material::setName(const std::string& name)
{
    _name = name;
    setEditStateChanged();
}

void Material::setAuthor(const std::string& author)
{
    _author = author;
    setEditStateChanged();
}

void Material::setLicense(const std::string& license)
{
    _license = license;
    setEditStateChanged();
}

void Material::setParentUUID(const std::string& uuid)
{
    _parentUuid = uuid;
    setEditStateChanged();
}

void Material::setDescription(const std::string& description)
{
    _description = trim_copy(description);
    setEditStateChanged();
}

void Material::setURL(const std::string& url)
{
    _url = url;
    setEditStateChanged();
}

void Material::setReference(const std::string& reference)
{
    _reference = reference;
    setEditStateChanged();
}

void Material::setEditState(MaterialEdit newState)
{
    if (_editState == MaterialEdit_New) {
        return;
    }
    if (newState == MaterialEdit_Changed) {
        if (_editState != MaterialEdit_InvariantChanged) {
            _editState = newState;
        }
    }
    else if (newState == MaterialEdit_InvariantChanged || newState == MaterialEdit_New) {
        _editState = newState;
    }
}

void Material::removeUUID(QSet<std::string>& uuidList, const std::string& uuid)
{
    uuidList.remove(uuid);
}

void Material::addTag(const std::string& tag)
{
    auto trimmed = tag;
    trim(trimmed);
    if (!trimmed.empty()) {
        _tags.insert(trimmed);
        setEditStateChanged();
    }
}

void Material::removeTag(const std::string& tag)
{
    _tags.remove(tag);
    setEditStateChanged();
}

void Material::addPhysical(const std::string& uuid)
{
    if (hasPhysicalModel(uuid)) {
        return;
    }

    auto& manager = ModelManager::getManager();

    try {
        auto model = manager.getModel(uuid);

        auto& inheritance = model->getInheritance();
        for (auto& it : inheritance) {
            // Inherited models may already have the properties, so just
            // remove the uuid
            removeUUID(_physicalUuids, it);
        }

        _physicalUuids.insert(uuid);
        addModel(uuid);
        setEditStateChanged();

        for (auto& it : *model) {
            std::string propertyName = it.first;
            if (!hasPhysicalProperty(propertyName)) {
                ModelProperty property = static_cast<ModelProperty>(it.second);

                try {
                    _physical[propertyName] = std::make_shared<MaterialProperty>(property, uuid);
                }
                catch (const UnknownValueType&) {
                    Base::Console().error("Property '%s' has unknown type '%s'. Ignoring\n",
                                          property.getName().c_str(),
                                          property.getPropertyType().c_str());
                }
            }
        }
    }
    catch (ModelNotFound const&) {
    }
}

void Material::removePhysical(const std::string& uuid)
{
    if (!hasPhysicalModel(uuid)) {
        return;
    }

    // If it's an inherited model, do nothing
    if (isInherited(uuid)) {
        return;
    }

    auto& manager = ModelManager::getManager();

    try {
        auto model = manager.getModel(uuid);

        auto& inheritance = model->getInheritance();
        for (auto& it : inheritance) {
            removeUUID(_physicalUuids, it);
            removeUUID(_allUuids, it);
        }
        removeUUID(_physicalUuids, uuid);
        removeUUID(_allUuids, uuid);

        for (auto& it : *model) {
            _physical.erase(it.first);
        }

        setEditStateInvariantChanged();
    }
    catch (ModelNotFound const&) {
        Base::Console().log("Physical model not found '%s'\n", uuid.c_str());
    }
}

void Material::addAppearance(const std::string& uuid)
{
    if (hasAppearanceModel(uuid)) {
        return;
    }

    auto& manager = ModelManager::getManager();

    try {
        auto model = manager.getModel(uuid);

        auto& inheritance = model->getInheritance();
        for (auto& it : inheritance) {
            // Inherited models may already have the properties, so just
            // remove the uuid
            removeUUID(_appearanceUuids, it);
        }

        _appearanceUuids.insert(uuid);
        addModel(uuid);
        setEditStateChanged();

        for (auto& it : *model) {
            std::string propertyName = it.first;
            if (!hasAppearanceProperty(propertyName)) {
                ModelProperty property = static_cast<ModelProperty>(it.second);

                _appearance[propertyName] = std::make_shared<MaterialProperty>(property, uuid);
            }
        }
    }
    catch (ModelNotFound const&) {
        Base::Console().log("Appearance model not found '%s'\n", uuid.c_str());
    }
}

void Material::removeAppearance(const std::string& uuid)
{
    if (!hasAppearanceModel(uuid)) {
        return;
    }

    // If it's an inherited model, do nothing
    if (isInherited(uuid)) {
        return;
    }

    auto& manager = ModelManager::getManager();

    try {
        auto model = manager.getModel(uuid);

        auto& inheritance = model->getInheritance();
        for (auto& it : inheritance) {
            removeUUID(_appearanceUuids, it);
            removeUUID(_allUuids, it);
        }
        removeUUID(_appearanceUuids, uuid);
        removeUUID(_allUuids, uuid);

        for (auto& it : *model) {
            _appearance.erase(it.first);
        }

        setEditStateInvariantChanged();
    }
    catch (ModelNotFound const&) {
    }
}

void Material::setPropertyEditState(const std::string& name)
{
    try {
        if (hasPhysicalProperty(name)) {
            setPhysicalEditState(name);
        }
        else if (hasAppearanceProperty(name)) {
            setAppearanceEditState(name);
        }
    }
    catch (const PropertyNotFound&) {
    }
}

void Material::setPhysicalEditState(const std::string& name)
{
    if (getPhysicalProperty(name)->isNull()) {
        setEditStateChanged();
    }
    else {
        setEditStateInvariantChanged();
    }
}

void Material::setAppearanceEditState(const std::string& name)
{
    try {
        if (getAppearanceProperty(name)->isNull()) {
            setEditStateChanged();
        }
        else {
            setEditStateInvariantChanged();
        }
    }
    catch (const PropertyNotFound&) {
    }
}

void Material::setPhysicalValue(const std::string& name, const std::string& value)
{
    setPhysicalEditState(name);

    if (hasPhysicalProperty(name)) {
        _physical[name]->setValue(value);  // may not be a string type, conversion may be required
    }
}

void Material::setPhysicalValue(const std::string& name, int value)
{
    setPhysicalEditState(name);

    if (hasPhysicalProperty(name)) {
        _physical[name]->setInt(value);
    }
}

void Material::setPhysicalValue(const std::string& name, double value)
{
    setPhysicalEditState(name);

    if (hasPhysicalProperty(name)) {
        _physical[name]->setFloat(value);
    }
}

void Material::setPhysicalValue(const std::string& name, const Base::Quantity& value)
{
    setPhysicalEditState(name);

    if (hasPhysicalProperty(name)) {
        _physical[name]->setQuantity(value);
    }
}

void Material::setPhysicalValue(const std::string& name, const std::shared_ptr<MaterialValue>& value)
{
    setPhysicalEditState(name);

    if (hasPhysicalProperty(name)) {
        _physical[name]->setValue(value);
    }
}

void Material::setPhysicalValue(const std::string& name, const std::shared_ptr<std::vector<QVariant>>& value)
{
    setPhysicalEditState(name);

    if (hasPhysicalProperty(name)) {
        _physical[name]->setList(*value);
    }
}

void Material::setPhysicalValue(const std::string& name, const QVariant& value)
{
    setPhysicalEditState(name);

    if (hasPhysicalProperty(name)) {
        _physical[name]->setValue(value);
    }
}

void Material::setAppearanceValue(const std::string& name, const std::string& value)
{
    setAppearanceEditState(name);

    if (hasAppearanceProperty(name)) {
        _appearance[name]->setValue(value);  // may not be a string type, conversion may be required
    }
}

void Material::setAppearanceValue(const std::string& name, const std::shared_ptr<MaterialValue>& value)
{
    setAppearanceEditState(name);

    if (hasAppearanceProperty(name)) {
        _appearance[name]->setValue(value);
    }
}

void Material::setAppearanceValue(const std::string& name,
                                  const std::shared_ptr<std::vector<QVariant>>& value)
{
    setAppearanceEditState(name);

    if (hasAppearanceProperty(name)) {
        _appearance[name]->setList(*value);
    }
}

void Material::setAppearanceValue(const std::string& name, const QVariant& value)
{
    setAppearanceEditState(name);

    if (hasAppearanceProperty(name)) {
        _appearance[name]->setValue(value);
    }
}

void Material::setValue(const std::string& name, const std::string& value)
{
    if (hasPhysicalProperty(name)) {
        setPhysicalValue(name, value);
    }
    else if (hasAppearanceProperty(name)) {
        setAppearanceValue(name, value);
    }
    else {
        throw PropertyNotFound();
    }
}

void Material::setValue(const std::string& name, const QVariant& value)
{
    if (hasPhysicalProperty(name)) {
        setPhysicalValue(name, value);
    }
    else if (hasAppearanceProperty(name)) {
        setAppearanceValue(name, value);
    }
    else {
        throw PropertyNotFound();
    }
}

void Material::setValue(const std::string& name, const std::shared_ptr<MaterialValue>& value)
{
    if (hasPhysicalProperty(name)) {
        setPhysicalValue(name, value);
    }
    else if (hasAppearanceProperty(name)) {
        setAppearanceValue(name, value);
    }
    else {
        throw PropertyNotFound();
    }
}

void Material::setLegacyValue(const std::string& name, const std::string& value)
{
    setEditStateInvariantChanged();

    _legacy[name] = value;
}

std::shared_ptr<MaterialProperty> Material::getPhysicalProperty(const std::string& name)
{
    try {
        return _physical.at(name);
    }
    catch (std::out_of_range const&) {
        throw PropertyNotFound();
    }
}

std::shared_ptr<MaterialProperty> Material::getPhysicalProperty(const std::string& name) const
{
    try {
        return _physical.at(name);
    }
    catch (std::out_of_range const&) {
        throw PropertyNotFound();
    }
}

std::shared_ptr<MaterialProperty> Material::getAppearanceProperty(const std::string& name)
{
    try {
        return _appearance.at(name);
    }
    catch (std::out_of_range const&) {
        throw PropertyNotFound();
    }
}

std::shared_ptr<MaterialProperty> Material::getAppearanceProperty(const std::string& name) const
{
    try {
        return _appearance.at(name);
    }
    catch (std::out_of_range const&) {
        throw PropertyNotFound();
    }
}

std::shared_ptr<MaterialProperty> Material::getProperty(const std::string& name)
{
    if (hasPhysicalProperty(name)) {
        return getPhysicalProperty(name);
    }
    if (hasAppearanceProperty(name)) {
        return getAppearanceProperty(name);
    }
    throw PropertyNotFound();
}

std::shared_ptr<MaterialProperty> Material::getProperty(const std::string& name) const
{
    if (hasPhysicalProperty(name)) {
        return getPhysicalProperty(name);
    }
    if (hasAppearanceProperty(name)) {
        return getAppearanceProperty(name);
    }
    throw PropertyNotFound();
}

QVariant
Material::getValue(const std::map<std::string, std::shared_ptr<MaterialProperty>>& propertyList,
                   const std::string& name)
{
    try {
        return propertyList.at(name)->getValue();
    }
    catch (std::out_of_range const&) {
        throw PropertyNotFound();
    }
}

std::string
Material::getValueString(const std::map<std::string, std::shared_ptr<MaterialProperty>>& propertyList,
                         const std::string& name)
{
    try {
        const auto& property = propertyList.at(name);
        if (property->isNull()) {
            return {};
        }
        if (property->getType() == MaterialValue::Quantity) {
            auto value = property->getValue();
            if (value.isNull()) {
                return {};
            }
            return value.value<Base::Quantity>().getUserString();
        }
        if (property->getType() == MaterialValue::Float) {
            auto value = property->getValue();
            if (value.isNull()) {
                return {};
            }
            return QString(QStringLiteral("%L1"))
                .arg(value.toFloat(), 0, 'g', MaterialValue::PRECISION)
                .toStdString();
        }
        return property->getValue().toString().toStdString();
    }
    catch (std::out_of_range const&) {
        throw PropertyNotFound();
    }
}

QVariant Material::getPhysicalValue(const std::string& name) const
{
    return getValue(_physical, name);
}

Base::Quantity Material::getPhysicalQuantity(const std::string& name) const
{
    return getValue(_physical, name).value<Base::Quantity>();
}

std::string Material::getPhysicalValueString(const std::string& name) const
{
    return getValueString(_physical, name);
}

QVariant Material::getAppearanceValue(const std::string& name) const
{
    return getValue(_appearance, name);
}

Base::Quantity Material::getAppearanceQuantity(const std::string& name) const
{
    return getValue(_appearance, name).value<Base::Quantity>();
}

std::string Material::getAppearanceValueString(const std::string& name) const
{
    return getValueString(_appearance, name);
}

bool Material::hasPhysicalProperty(const std::string& name) const
{
    return _physical.find(name) != _physical.end();
}

bool Material::hasAppearanceProperty(const std::string& name) const
{
    return _appearance.find(name) != _appearance.end();
}

bool Material::hasNonLegacyProperty(const std::string& name) const
{
    if (hasPhysicalProperty(name) || hasAppearanceProperty(name)) {
        return true;
    }
    return false;
}

bool Material::hasLegacyProperties() const
{
    return !_legacy.empty();
}

bool Material::hasPhysicalProperties() const
{
    return !_physicalUuids.empty();
}

bool Material::hasAppearanceProperties() const
{
    return !_appearanceUuids.empty();
}

bool Material::isInherited(const std::string& uuid) const
{
    if (_physicalUuids.contains(uuid)) {
        return false;
    }
    if (_appearanceUuids.contains(uuid)) {
        return false;
    }

    return _allUuids.contains(uuid);
}

bool Material::hasModel(const std::string& uuid) const
{
    return _allUuids.contains(uuid);
}

bool Material::hasPhysicalModel(const std::string& uuid) const
{
    if (!hasModel(uuid)) {
        return false;
    }

    auto& manager = ModelManager::getManager();

    try {
        auto model = manager.getModel(uuid);
        if (model->getType() == Model::ModelType_Physical) {
            return true;
        }
    }
    catch (ModelNotFound const&) {
    }

    return false;
}

bool Material::hasAppearanceModel(const std::string& uuid) const
{
    if (!hasModel(uuid)) {
        return false;
    }

    auto& manager = ModelManager::getManager();

    try {
        auto model = manager.getModel(uuid);
        if (model->getType() == Model::ModelType_Appearance) {
            return true;
        }
    }
    catch (ModelNotFound const&) {
    }

    return false;
}

bool Material::isPhysicalModelComplete(const std::string& uuid) const
{
    if (!hasPhysicalModel(uuid)) {
        return false;
    }

    auto& manager = ModelManager::getManager();

    try {
        auto model = manager.getModel(uuid);
        for (auto& it : *model) {
            std::string propertyName = it.first;
            auto property = getPhysicalProperty(propertyName);

            if (property->isNull()) {
                return false;
            }
        }
    }
    catch (ModelNotFound const&) {
        return false;
    }

    return true;
}

bool Material::isAppearanceModelComplete(const std::string& uuid) const
{
    if (!hasAppearanceModel(uuid)) {
        return false;
    }

    auto& manager = ModelManager::getManager();

    try {
        auto model = manager.getModel(uuid);
        for (auto& it : *model) {
            std::string propertyName = it.first;
            auto property = getAppearanceProperty(propertyName);

            if (property->isNull()) {
                return false;
            }
        }
    }
    catch (ModelNotFound const&) {
        return false;
    }

    return true;
}

void Material::saveGeneral(Base::ofstream& stream) const
{
    stream << "General:\n";
    stream << "  UUID: \"" << _uuid << "\"\n";
    stream << "  Name: \"" << MaterialValue::escapeString(_name) << "\"\n";
    if (!_author.empty()) {
        stream << "  Author: \"" << MaterialValue::escapeString(_author) << "\"\n";
    }
    if (!_license.empty()) {
        stream << "  License: \"" << MaterialValue::escapeString(_license) << "\"\n";
    }
    if (!_description.empty()) {
        stream << "  Description: \"" << MaterialValue::escapeString(_description) << "\"\n";
    }
    if (!_url.empty()) {
        stream << "  SourceURL: \"" << MaterialValue::escapeString(_url) << "\"\n";
    }
    if (!_reference.empty()) {
        stream << "  ReferenceSource: \"" << MaterialValue::escapeString(_reference) << "\"\n";
    }
    if (!_tags.empty()) {
        stream << "  Tags:\n";
        for (auto tag : _tags) {
            stream << "    - \"" << tag << "\"\n";
        }
    }
}

void Material::saveInherits(Base::ofstream& stream) const
{
    if (!_parentUuid.empty()) {
        try {
            auto material = MaterialManager::getManager().getMaterial(_parentUuid);

            stream << "Inherits:\n";
            stream << "  " << material->getName() << ":\n";
            stream << "    UUID: \"" << _parentUuid << "\"\n";
        }
        catch (const MaterialNotFound&) {
        }
    }
}

bool Material::modelChanged(const Material& parent,
                            const Model& model) const
{
    for (auto& it : model) {
        std::string propertyName = it.first;
        auto property = getPhysicalProperty(propertyName);
        try {
            auto parentProperty = parent.getPhysicalProperty(propertyName);

            if (*property != *parentProperty) {
                return true;
            }
        }
        catch (const PropertyNotFound&) {
            return true;
        }
    }

    return false;
}

bool Material::modelAppearanceChanged(const Material& parent,
                                      const Model& model) const
{
    for (auto& it : model) {
        std::string propertyName = it.first;
        auto property = getAppearanceProperty(propertyName);
        try {
            auto parentProperty = parent.getAppearanceProperty(propertyName);

            if (*property != *parentProperty) {
                return true;
            }
        }
        catch (const PropertyNotFound&) {
            return true;
        }
    }

    return false;
}

void Material::saveModels(Base::ofstream& stream, bool saveInherited) const
{
    if (_physical.empty()) {
        return;
    }

    auto& modelManager = ModelManager::getManager();
    auto& materialManager = MaterialManager::getManager();

    bool inherited = saveInherited && (_parentUuid.size() > 0);
    std::shared_ptr<Material> parent;
    if (inherited) {
        try {
            parent = materialManager.getMaterial(_parentUuid);
        }
        catch (const MaterialNotFound&) {
            inherited = false;
        }
    }

    bool headerPrinted = false;
    for (auto& itm : _physicalUuids) {
        auto model = modelManager.getModel(itm);
        if (!inherited || modelChanged(*parent, *model)) {
            if (!headerPrinted) {
                stream << "Models:\n";
                headerPrinted = true;
            }
            stream << "  " << MaterialValue::escapeString(model->getName()) << ":\n";
            stream << "    UUID: \"" << model->getUUID() << "\"\n";
            for (const auto& it : *model) {
                std::string propertyName = it.first;
                std::shared_ptr<MaterialProperty> property = getPhysicalProperty(propertyName);
                std::shared_ptr<MaterialProperty> parentProperty;
                try {
                    if (inherited) {
                        parentProperty = parent->getPhysicalProperty(propertyName);
                    }
                }
                catch (const PropertyNotFound&) {
                    Base::Console().log("Material::saveModels Property not found '%s'\n",
                                        propertyName.c_str());
                }

                if (!inherited || !parentProperty || (*property != *parentProperty)) {
                    if (!property->isNull()) {
                        stream << "    ";
                        stream << *property << "\n";
                    }
                }
            }
        }
    }
}

void Material::saveAppearanceModels(Base::ofstream& stream, bool saveInherited) const
{
    if (_appearance.empty()) {
        return;
    }

    auto& modelManager = ModelManager::getManager();
    auto& materialManager = MaterialManager::getManager();

    bool inherited = saveInherited && (_parentUuid.size() > 0);
    std::shared_ptr<Material> parent;
    if (inherited) {
        try {
            parent = materialManager.getMaterial(_parentUuid);
        }
        catch (const MaterialNotFound&) {
            inherited = false;
        }
    }

    bool headerPrinted = false;
    for (auto& itm : _appearanceUuids) {
        auto model = modelManager.getModel(itm);
        if (!inherited || modelAppearanceChanged(*parent, *model)) {
            if (!headerPrinted) {
                stream << "AppearanceModels:\n";
                headerPrinted = true;
            }
            stream << "  " << MaterialValue::escapeString(model->getName()) << ":\n";
            stream << "    UUID: \"" << model->getUUID() << "\"\n";
            for (const auto& it : *model) {
                std::string propertyName = it.first;
                std::shared_ptr<MaterialProperty> property = getAppearanceProperty(propertyName);
                std::shared_ptr<MaterialProperty> parentProperty;
                try {
                    if (inherited) {
                        parentProperty = parent->getAppearanceProperty(propertyName);
                    }
                }
                catch (const PropertyNotFound&) {
                }

                if (!inherited || !parentProperty || (*property != *parentProperty)) {
                    if (!property->isNull()) {
                        stream << "    ";
                        stream << *property << "\n";
                    }
                }
            }
        }
    }
}

void Material::newUuid()
{
    _uuid = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
}

std::string Material::getModelByName(const std::string& name) const
{
    auto& manager = ModelManager::getManager();

    for (auto& it : _allUuids) {
        try {
            auto model = manager.getModel(it);
            if (model->getName() == name) {
                return it;
            }
        }
        catch (ModelNotFound const&) {
        }
    }

    return {};
}

void Material::save(Base::ofstream& stream, bool overwrite, bool saveAsCopy, bool saveInherited)
{
    if (saveInherited && !saveAsCopy) {
        // Check to see if we're an original or if we're already in the list of
        // models
        if (MaterialManager::getManager().exists(_uuid) && !overwrite) {
            // Make a new version based on the current
            setParentUUID(_uuid);
        }
    }

    // Prevent self inheritance
    if (_parentUuid == _uuid) {
        _parentUuid = std::string();
    }

    if (saveAsCopy) {
        // Save it in the same format as the parent
        if (_parentUuid.empty()) {
            saveInherited = false;
        }
        else {
            saveInherited = true;
        }
    }
    else {
        if (!overwrite) {
            // Creating a new derived model when overwriting sets itself as a
            // parent, that will no longer exist because it's been overwritten
            newUuid();
        }
    }

    stream << "---\n";
    stream << "# File created by " << App::Application::Config()["ExeName"]
           << " " << App::Application::Config()["ExeVersion"]
           << " Revision: " << App::Application::Config()["BuildRevision"]
           << "\n";
    saveGeneral(stream);
    if (saveInherited) {
        saveInherits(stream);
    }
    saveModels(stream, saveInherited);
    saveAppearanceModels(stream, saveInherited);

    setOldFormat(false);
}

Material& Material::operator=(const Material& other)
{
    if (this == &other) {
        return *this;
    }

    _library = other._library;
    _directory = other._directory;
    _uuid = other._uuid;
    _name = other._name;
    _author = other._author;
    _license = other._license;
    _parentUuid = other._parentUuid;
    _description = other._description;
    _url = other._url;
    _reference = other._reference;
    _dereferenced = other._dereferenced;
    _oldFormat = other._oldFormat;
    _editState = other._editState;

    _tags.clear();
    for (auto& it : other._tags) {
        _tags.insert(it);
    }
    _physicalUuids.clear();
    for (auto& it : other._physicalUuids) {
        _physicalUuids.insert(it);
    }
    _appearanceUuids.clear();
    for (auto& it : other._appearanceUuids) {
        _appearanceUuids.insert(it);
    }
    _allUuids.clear();
    for (auto& it : other._allUuids) {
        _allUuids.insert(it);
    }

    // Create copies of the properties rather than modify the originals
    _physical.clear();
    for (auto& it : other._physical) {
        MaterialProperty prop(it.second);
        _physical[it.first] = std::make_shared<MaterialProperty>(prop);
    }
    _appearance.clear();
    for (auto& it : other._appearance) {
        MaterialProperty prop(it.second);
        _appearance[it.first] = std::make_shared<MaterialProperty>(prop);
    }
    _legacy.clear();
    for (auto& it : other._legacy) {
        _legacy[it.first] = it.second;
    }

    return *this;
}

Material& Material::operator=(const App::Material& other)
{
    if (!hasAppearanceModel(ModelUUIDs::ModelUUID_Rendering_Basic)) {
        addAppearance(ModelUUIDs::ModelUUID_Rendering_Basic);
    }

    getAppearanceProperty("AmbientColor")->setColor(other.ambientColor);
    getAppearanceProperty("DiffuseColor")->setColor(other.diffuseColor);
    getAppearanceProperty("SpecularColor")->setColor(other.specularColor);
    getAppearanceProperty("EmissiveColor")->setColor(other.emissiveColor);
    getAppearanceProperty("Shininess")->setFloat(other.shininess);
    getAppearanceProperty("Transparency")->setFloat(other.transparency);

    if (!other.image.empty() || !other.imagePath.empty()) {
        if (!hasAppearanceModel(ModelUUIDs::ModelUUID_Rendering_Texture)) {
            addAppearance(ModelUUIDs::ModelUUID_Rendering_Texture);
        }

        getAppearanceProperty("TextureImage")->setString(other.image);
        getAppearanceProperty("TexturePath")->setString(other.imagePath);
    }

    return *this;
}

/*
 * Normalize models by removing any inherited models
 */
std::vector<std::string> Material::normalizeModels(const std::vector<std::string>& models)
{
    std::vector<std::string> normalized;

    auto& manager = ModelManager::getManager();

    for (auto& uuid : models) {
        auto model = manager.getModel(uuid);

        bool found = false;
        for (auto& childUuid : models) {
            if (uuid != childUuid) {
                auto childModel = manager.getModel(childUuid);
                if (childModel->inherits(childUuid)) {
                    // We're an inherited model
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            normalized.push_back(uuid);
        }
    }

    return normalized;
}

/*
 * Set or change the base material for the current material, updating the
 * properties as required.
 */
void Material::updateInheritance([[maybe_unused]] const std::string& parent)
{}

/*
 * Return a list of models that are defined in the parent material but not in
 * this one
 */
std::vector<std::string> Material::inheritedMissingModels(const Material& parent) const
{
    std::vector<std::string> missing;
    for (auto& uuid : parent._allUuids) {
        if (!hasModel(uuid)) {
            missing.push_back(uuid);
        }
    }

    return normalizeModels(missing);
}

/*
 * Return a list of models that are defined in this model but not the parent
 */
std::vector<std::string> Material::inheritedAddedModels(const Material& parent) const
{
    std::vector<std::string> added;
    for (auto& uuid : _allUuids) {
        if (!parent.hasModel(uuid)) {
            added.push_back(uuid);
        }
    }

    return normalizeModels(added);
}

/*
 * Return a list of properties that have different values from the parent
 * material
 */
void Material::inheritedPropertyDiff([[maybe_unused]] const std::string& parent)
{}

/*
 * Return an App::Material object describing the materials appearance, or DEFAULT if
 * undefined.
 */
App::Material Material::getMaterialAppearance() const
{
    App::Material material(App::Material::DEFAULT);

    bool custom = false;
    if (hasAppearanceProperty("AmbientColor")) {
        material.ambientColor = getAppearanceProperty("AmbientColor")->getColor();
        custom = true;
    }
    if (hasAppearanceProperty("DiffuseColor")) {
        material.diffuseColor = getAppearanceProperty("DiffuseColor")->getColor();
        custom = true;
    }
    if (hasAppearanceProperty("SpecularColor")) {
        material.specularColor = getAppearanceProperty("SpecularColor")->getColor();
        custom = true;
    }
    if (hasAppearanceProperty("EmissiveColor")) {
        material.emissiveColor = getAppearanceProperty("EmissiveColor")->getColor();
        custom = true;
    }
    if (hasAppearanceProperty("Shininess")) {
        material.shininess = getAppearanceProperty("Shininess")->getFloat();
        custom = true;
    }
    if (hasAppearanceProperty("Transparency")) {
        material.transparency = getAppearanceProperty("Transparency")->getFloat();
        custom = true;
    }
    if (hasAppearanceProperty("TextureImage")) {
        auto property = getAppearanceProperty("TextureImage");
        if (!property->isNull()) {
            material.image = property->getString();
        }

        custom = true;
    }
    else if (hasAppearanceProperty("TexturePath")) {
        auto property = getAppearanceProperty("TexturePath");
        if (!property->isNull()) {
            material.imagePath = property->getString();
        }

        custom = true;
    }

    if (custom) {
        material.setType(App::Material::USER_DEFINED);
        material.uuid = getUUID();
    }

    return material;
}

void Material::validate(Material& other) const
{
    try {
        _library->validate(*other._library);
    }
    catch (const InvalidLibrary& e) {
        throw InvalidMaterial(e.what());
    }

    if (_directory != other._directory) {
        throw InvalidMaterial("Model directories don't match");
    }
    if (_uuid != other._uuid) {
        throw InvalidMaterial("Model UUIDs don't match");
    }
    if (_name != other._name) {
        throw InvalidMaterial("Model names don't match");
    }
    if (_author != other._author) {
        throw InvalidMaterial("Model authors don't match");
    }
    if (_license != other._license) {
        throw InvalidMaterial("Model licenses don't match");
    }
    if (_parentUuid != other._parentUuid) {
        throw InvalidMaterial("Model parents don't match");
    }
    if (_description != other._description) {
        throw InvalidMaterial("Model descriptions don't match");
    }
    if (_url != other._url) {
        throw InvalidMaterial("Model URLs don't match");
    }
    if (_reference != other._reference) {
        throw InvalidMaterial("Model references don't match");
    }

    if (_tags.size() != other._tags.size()) {
        Base::Console().log("Local tags count %d\n", _tags.size());
        Base::Console().log("Remote tags count %d\n", other._tags.size());
        throw InvalidMaterial("Material tags counts don't match");
    }
    if (!other._tags.contains(_tags)) {
        throw InvalidMaterial("Material tags don't match");
    }

    if (_physicalUuids.size() != other._physicalUuids.size()) {
        Base::Console().log("Local physical model count %d\n", _physicalUuids.size());
        Base::Console().log("Remote physical model count %d\n", other._physicalUuids.size());
        throw InvalidMaterial("Material physical model counts don't match");
    }
    if (!other._physicalUuids.contains(_physicalUuids)) {
        throw InvalidMaterial("Material physical models don't match");
    }

    if (_physicalUuids.size() != other._physicalUuids.size()) {
        Base::Console().log("Local appearance model count %d\n", _physicalUuids.size());
        Base::Console().log("Remote appearance model count %d\n", other._physicalUuids.size());
        throw InvalidMaterial("Material appearance model counts don't match");
    }
    if (!other._physicalUuids.contains(_physicalUuids)) {
        throw InvalidMaterial("Material appearance models don't match");
    }

    if (_allUuids.size() != other._allUuids.size()) {
        Base::Console().log("Local model count %d\n", _allUuids.size());
        Base::Console().log("Remote model count %d\n", other._allUuids.size());
        throw InvalidMaterial("Material model counts don't match");
    }
    if (!other._allUuids.contains(_allUuids)) {
        throw InvalidMaterial("Material models don't match");
    }

    // Need to compare properties
    if (_physical.size() != other._physical.size()) {
        throw InvalidMaterial("Material physical property counts don't match");
    }
    for (auto& property : _physical) {
        auto& remote = other._physical[property.first];
        property.second->validate(*remote);
    }

    if (_appearance.size() != other._appearance.size()) {
        throw InvalidMaterial("Material appearance property counts don't match");
    }
    for (auto& property : _appearance) {
        auto& remote = other._appearance[property.first];
        property.second->validate(*remote);
    }
}
