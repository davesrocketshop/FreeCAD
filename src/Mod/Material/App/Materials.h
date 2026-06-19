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

#pragma once

#include <memory>

#include <QSet>

#include <App/Material.h>
#include <Base/BaseClass.h>

#include <Mod/Material/MaterialGlobal.h>

#include "MaterialProperty.h"
#include "MaterialValue.h"
#include "Model.h"

namespace Materials
{

class MaterialLibrary;

class MaterialsExport Material: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    enum MaterialEdit
    {
        MaterialEdit_None,             // No change
        MaterialEdit_New,              // New unsaved material
        MaterialEdit_InvariantChanged, // Changes that could impact existing documents
        MaterialEdit_Changed           // All other changes
    };

    Material();
    Material(const std::shared_ptr<MaterialLibrary>& library,
             const std::string& directory,
             const std::string& uuid,
             const std::string& name);
    Material(const Material& other);
    ~Material() override = default;

    std::shared_ptr<MaterialLibrary> getLibrary() const
    {
        return _library;
    }
    bool isDisabled() const;
    std::string getDirectory() const;
    std::string getFilePath() const;
    std::string getLibraryPath() const;
    std::string getUUID() const
    {
        return _uuid;
    }
    std::string getName() const
    {
        return _name;
    }
    std::string getAuthorAndLicense() const;
    std::string getAuthor() const
    {
        return _author;
    }
    std::string getLicense() const
    {
        return _license;
    }
    std::string getParentUUID() const
    {
        return _parentUuid;
    }
    std::string getDescription() const
    {
        return _description;
    }
    std::string getURL() const
    {
        return _url;
    }
    std::string getReference() const
    {
        return _reference;
    }
    MaterialEdit getEditState() const
    {
        return _editState;
    }
    const QSet<std::string>& getTags() const
    {
        return _tags;
    }
    const QSet<std::string>* getPhysicalModels() const
    {
        return &_physicalUuids;
    }
    const QSet<std::string>* getAppearanceModels() const
    {
        return &_appearanceUuids;
    }

    App::Material getMaterialAppearance() const;

    void setLibrary(const std::shared_ptr<MaterialLibrary>& library);

    void setDirectory(const std::string& directory);
    void setDirectory(const QString& directory) { setDirectory(directory.toStdString()); }
    void setDirectory(const char* directory) { setDirectory(std::string(directory)); }

    void setUUID(const std::string& uuid)
    {
        _uuid = uuid;
    }

    void setName(const std::string& name);
    void setName(const QString& name) { setName(name.toStdString()); }
    void setName(const char* name) { setName(std::string(name)); }

    void setAuthor(const std::string& author);
    void setAuthor(const QString& author) { setAuthor(author.toStdString()); }
    void setAuthor(const char* author) { setAuthor(std::string(author)); }

    void setLicense(const std::string& license);
    void setLicense(const QString& license) { setLicense(license.toStdString()); }
    void setLicense(const char* license) { setLicense(std::string(license)); }

    void setParentUUID(const std::string& uuid);
    void setParentUUID(const QString& uuid) { setParentUUID(uuid.toStdString()); }
    void setParentUUID(const char* uuid) { setParentUUID(std::string(uuid)); }

    void setDescription(const std::string& description);
    void setDescription(const QString& description) { setDescription(description.toStdString()); }
    void setDescription(const char* description) { setDescription(std::string(description)); }

    void setURL(const std::string& url);
    void setURL(const QString& url) { setURL(url.toStdString()); }
    void setURL(const char* url) { setURL(std::string(url)); }

    void setReference(const std::string& reference);
    void setReference(const QString& reference) { setReference(reference.toStdString()); }
    void setReference(const char* reference) { setReference(std::string(reference)); }

    void setEditState(MaterialEdit newState);
    void setEditStateInvariantChanged()
    {
        setEditState(MaterialEdit_InvariantChanged);
    }
    void setEditStateChanged()
    {
        setEditState(MaterialEdit_Changed);
    }
    void setEditStateNew()
    {
        setEditState(MaterialEdit_New);
    }

    void setPropertyEditState(const std::string& name);
    void setPropertyEditState(const QString& name) { setPropertyEditState(name.toStdString()); }
    void setPropertyEditState(const char* name) { setPropertyEditState(std::string(name)); }

    void setPhysicalEditState(const std::string& name);
    void setPhysicalEditState(const QString& name) { setPhysicalEditState(name.toStdString()); }
    void setPhysicalEditState(const char* name) { setPhysicalEditState(std::string(name)); }

    void setAppearanceEditState(const std::string& name);
    void setAppearanceEditState(const QString& name) { setAppearanceEditState(name.toStdString()); }
    void setAppearanceEditState(const char* name) { setAppearanceEditState(std::string(name)); }

    void resetEditState()
    {
        _editState = MaterialEdit_None;
    }

    void addTag(const std::string& tag);
    void addTag(const QString& tag) { addTag(tag.toStdString()); }
    void addTag(const char* tag) { addTag(std::string(tag)); }

    void removeTag(const std::string& tag);
    void removeTag(const QString& tag) { removeTag(tag.toStdString()); }
    void removeTag(const char* tag) { removeTag(std::string(tag)); }

    bool hasTag(const std::string& tag)
    {
        return _tags.contains(tag);
    }
    bool hasTag(const QString& tag) { return hasTag(tag.toStdString()); }
    bool hasTag(const char* tag) { return hasTag(std::string(tag)); }

    void addPhysical(const std::string& uuid);
    void addPhysical(const QString& uuid) { addPhysical(uuid.toStdString()); }
    void addPhysical(const char* uuid) { addPhysical(std::string(uuid)); }

    void removePhysical(const std::string& uuid);
    void removePhysical(const QString& uuid) { removePhysical(uuid.toStdString()); }
    void removePhysical(const char* uuid) { removePhysical(std::string(uuid)); }

    void addAppearance(const std::string& uuid);
    void addAppearance(const QString& uuid) { addAppearance(uuid.toStdString()); }
    void addAppearance(const char* uuid) { addAppearance(std::string(uuid)); }

    void removeAppearance(const std::string& uuid);
    void removeAppearance(const QString& uuid) { removeAppearance(uuid.toStdString()); }
    void removeAppearance(const char* uuid) { removeAppearance(std::string(uuid)); }

    void clearModels();
    void clearInherited();
    void newUuid();

    void setPhysicalValue(const std::string& name, const std::string& value);
    void setPhysicalValue(const std::string& name, int value);
    void setPhysicalValue(const std::string& name, double value);
    void setPhysicalValue(const std::string& name, const Base::Quantity& value);
    void setPhysicalValue(const std::string& name, const std::shared_ptr<MaterialValue>& value);
    void setPhysicalValue(const std::string& name, const std::shared_ptr<std::vector<QVariant>>& value);
    void setPhysicalValue(const std::string& name, const QVariant& value);

    void setAppearanceValue(const std::string& name, const std::string& value);
    void setAppearanceValue(const std::string& name, const std::shared_ptr<MaterialValue>& value);
    void setAppearanceValue(const std::string& name, const std::shared_ptr<std::vector<QVariant>>& value);
    void setAppearanceValue(const std::string& name, const QVariant& value);

    void setValue(const std::string& name, const std::string& value);
    void setValue(const std::string& name, const QVariant& value);
    void setValue(const std::string& name, const std::shared_ptr<MaterialValue>& value);

    /*
     * Legacy values are thosed contained in old format files that don't fit in the new
     * property format. It should not be used as a catch all for defining a property with
     * no model.
     *
     * These values are transient and will not be saved.
     */
    void setLegacyValue(const std::string& name, const std::string& value);

    std::shared_ptr<MaterialProperty> getPhysicalProperty(const std::string& name) const;
    std::shared_ptr<MaterialProperty> getPhysicalProperty(const QString& name) const { return getPhysicalProperty(name.toStdString()); }
    std::shared_ptr<MaterialProperty> getPhysicalProperty(const char* name) const { return getPhysicalProperty(std::string(name)); }

    std::shared_ptr<MaterialProperty> getAppearanceProperty(const std::string& name) const;
    std::shared_ptr<MaterialProperty> getAppearanceProperty(const QString& name) const { return getAppearanceProperty(name.toStdString()); }
    std::shared_ptr<MaterialProperty> getAppearanceProperty(const char* name) const { return getAppearanceProperty(std::string(name)); }

    std::shared_ptr<MaterialProperty> getProperty(const std::string& name) const;
    std::shared_ptr<MaterialProperty> getProperty(const QString& name) const { return getProperty(name.toStdString()); }
    std::shared_ptr<MaterialProperty> getProperty(const char* name) const { return getProperty(std::string(name)); }

    QVariant getPhysicalValue(const std::string& name) const;
    QVariant getPhysicalValue(const QString& name) const { return getPhysicalValue(name.toStdString()); }
    QVariant getPhysicalValue(const char* name) const { return getPhysicalValue(std::string(name)); }

    Base::Quantity getPhysicalQuantity(const std::string& name) const;
    Base::Quantity getPhysicalQuantity(const QString& name) const { return getPhysicalQuantity(name.toStdString()); }
    Base::Quantity getPhysicalQuantity(const char* name) const { return getPhysicalQuantity(std::string(name)); }

    std::string getPhysicalValueString(const std::string& name) const;
    std::string getPhysicalValueString(const QString& name) const { return getPhysicalValueString(name.toStdString()); }
    std::string getPhysicalValueString(const char* name) const { return getPhysicalValueString(std::string(name)); }

    QVariant getAppearanceValue(const std::string& name) const;
    QVariant getAppearanceValue(const QString& name) const { return getAppearanceValue(name.toStdString()); }
    QVariant getAppearanceValue(const char* name) const { return getAppearanceValue(std::string(name)); }

    Base::Quantity getAppearanceQuantity(const std::string& name) const;
    Base::Quantity getAppearanceQuantity(const QString& name) const { return getAppearanceQuantity(name.toStdString()); }
    Base::Quantity getAppearanceQuantity(const char* name) const { return getAppearanceQuantity(std::string(name)); }

    std::string getAppearanceValueString(const std::string& name) const;
    std::string getAppearanceValueString(const QString& name) const { return getAppearanceValueString(name.toStdString()); }
    std::string getAppearanceValueString(const char* name) const { return getAppearanceValueString(std::string(name)); }

    bool hasPhysicalProperty(const std::string& name) const;
    bool hasPhysicalProperty(const QString& name) const { return hasPhysicalProperty(name.toStdString()); }
    bool hasPhysicalProperty(const char* name) const { return hasPhysicalProperty(std::string(name)); }

    bool hasAppearanceProperty(const std::string& name) const;
    bool hasAppearanceProperty(const QString& name) const { return hasAppearanceProperty(name.toStdString()); }
    bool hasAppearanceProperty(const char* name) const { return hasAppearanceProperty(std::string(name)); }

    bool hasNonLegacyProperty(const std::string& name) const;
    bool hasNonLegacyProperty(const QString& name) const { return hasNonLegacyProperty(name.toStdString()); }
    bool hasNonLegacyProperty(const char* name) const { return hasNonLegacyProperty(std::string(name)); }

    bool hasLegacyProperty(const std::string& name) const;
    bool hasLegacyProperty(const QString& name) const { return hasLegacyProperty(name.toStdString()); }
    bool hasLegacyProperty(const char* name) const { return hasLegacyProperty(std::string(name)); }

    bool hasLegacyProperties() const;
    bool hasPhysicalProperties() const;
    bool hasAppearanceProperties() const;

    // Test if the model is defined, and if values are provided for all properties
    bool hasModel(const std::string& uuid) const;
    bool hasModel(const QString& uuid) const { return hasModel(uuid.toStdString()); }
    bool hasModel(const char* uuid) const { return hasModel(std::string(uuid)); }

    bool hasPhysicalModel(const std::string& uuid) const;
    bool hasPhysicalModel(const QString& uuid) const { return hasPhysicalModel(uuid.toStdString()); }
    bool hasPhysicalModel(const char* uuid) const { return hasPhysicalModel(std::string(uuid)); }

    bool hasAppearanceModel(const std::string& uuid) const;
    bool hasAppearanceModel(const QString& uuid) const { return hasAppearanceModel(uuid.toStdString()); }
    bool hasAppearanceModel(const char* uuid) const { return hasAppearanceModel(std::string(uuid)); }

    bool isInherited(const std::string& uuid) const;
    bool isInherited(const QString& uuid) const { return isInherited(uuid.toStdString()); }
    bool isInherited(const char* uuid) const { return isInherited(std::string(uuid)); }

    bool isModelComplete(const std::string& uuid) const
    {
        return isPhysicalModelComplete(uuid) || isAppearanceModelComplete(uuid);
    }
    bool isModelComplete(const QString& uuid) const { return isModelComplete(uuid.toStdString()); }
    bool isModelComplete(const char* uuid) const { return isModelComplete(std::string(uuid)); }

    bool isPhysicalModelComplete(const std::string& uuid) const;
    bool isPhysicalModelComplete(const QString& uuid) const { return isPhysicalModelComplete(uuid.toStdString()); }
    bool isPhysicalModelComplete(const char* uuid) const { return isPhysicalModelComplete(std::string(uuid)); }
    
    bool isAppearanceModelComplete(const std::string& uuid) const;
    bool isAppearanceModelComplete(const QString& uuid) const { return isAppearanceModelComplete(uuid.toStdString()); }
    bool isAppearanceModelComplete(const char* uuid) const { return isAppearanceModelComplete(std::string(uuid)); }

    std::map<std::string, std::shared_ptr<MaterialProperty>>& getPhysicalProperties()
    {
        return _physical;
    }
    const std::map<std::string, std::shared_ptr<MaterialProperty>>& getPhysicalProperties() const
    {
        return _physical;
    }
    std::map<std::string, std::shared_ptr<MaterialProperty>>& getAppearanceProperties()
    {
        return _appearance;
    }
    const std::map<std::string, std::shared_ptr<MaterialProperty>>& getAppearanceProperties() const
    {
        return _appearance;
    }
    std::map<std::string, std::string>& getLegacyProperties()
    {
        return _legacy;
    }

    std::string getModelByName(const std::string& name) const;
    std::string getModelByName(const QString& name) const { return getModelByName(name.toStdString()); }
    std::string getModelByName(const char* name) const { return getModelByName(std::string(name)); }

    bool isDereferenced() const
    {
        return _dereferenced;
    }
    void markDereferenced()
    {
        _dereferenced = true;
    }
    void clearDereferenced()
    {
        _dereferenced = false;
    }
    bool isOldFormat() const
    {
        return _oldFormat;
    }
    void setOldFormat(bool isOld)
    {
        _oldFormat = isOld;
    }

    /*
     * Normalize models by removing any inherited models
     */
    static std::vector<std::string> normalizeModels(const std::vector<std::string>& models);

    /*
     * Set or change the base material for the current material, updating the properties as
     * required.
     */
    void updateInheritance(const std::string& parent);
    /*
     * Return a list of models that are defined in the parent material but not in this one
     */
    std::vector<std::string> inheritedMissingModels(const Material& parent) const;
    /*
     * Return a list of models that are defined in this model but not the parent
     */
    std::vector<std::string> inheritedAddedModels(const Material& parent) const;
    /*
     * Return a list of properties that have different values from the parent material
     */
    void inheritedPropertyDiff(const std::string& parent);

    void save(Base::ofstream& stream, bool overwrite, bool saveAsCopy, bool saveInherited);

    /*
     * Assignment operator
     */
    Material& operator=(const Material& other);

    /*
     * Set the appearance properties
     */
    Material& operator=(const App::Material& other);

    bool operator==(const Material& other) const
    {
        if (&other == this) {
            return true;
        }
        return getTypeId() == other.getTypeId() && _uuid == other._uuid;
    }

    void validate(Material& other) const;

protected:
    void addModel(const std::string& uuid);
    static void removeUUID(QSet<std::string>& uuidList, const std::string& uuid);

    static QVariant
    getValue(const std::map<std::string, std::shared_ptr<MaterialProperty>>& propertyList,
             const std::string& name);
    static std::string
    getValueString(const std::map<std::string, std::shared_ptr<MaterialProperty>>& propertyList,
                   const std::string& name);

    bool modelChanged(const Material& parent,
                      const Model& model) const;
    bool modelAppearanceChanged(const Material& parent,
                                const Model& model) const;
    void saveGeneral(Base::ofstream& stream) const;
    void saveInherits(Base::ofstream& stream) const;
    void saveModels(Base::ofstream& stream, bool saveInherited) const;
    void saveAppearanceModels(Base::ofstream& stream, bool saveInherited) const;

private:
    std::shared_ptr<MaterialLibrary> _library;
    std::string _directory;
    std::string _uuid;
    std::string _name;
    std::string _author;
    std::string _license;
    std::string _parentUuid;
    std::string _description;
    std::string _url;
    std::string _reference;
    QSet<std::string> _tags;
    QSet<std::string> _physicalUuids;
    QSet<std::string> _appearanceUuids;
    QSet<std::string> _allUuids;  // Includes inherited models
    std::map<std::string, std::shared_ptr<MaterialProperty>> _physical;
    std::map<std::string, std::shared_ptr<MaterialProperty>> _appearance;
    std::map<std::string, std::string> _legacy;
    bool _dereferenced;
    bool _oldFormat;
    MaterialEdit _editState;
};

using MaterialTreeNode = FolderTreeNode<Material>;

}  // namespace Materials

Q_DECLARE_METATYPE(Materials::Material*)
Q_DECLARE_METATYPE(std::shared_ptr<Materials::Material>)