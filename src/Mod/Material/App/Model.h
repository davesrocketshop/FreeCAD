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

#include <Base/BaseClass.h>
#include <Base/Quantity.h>
#include <Base/Stream.h>

#include <Mod/Material/MaterialGlobal.h>

#include "FolderTree.h"
#include "MaterialValue.h"
// #include "ModelLibrary.h"
#include "StringUtility.h"

namespace Materials
{

class ModelLibrary;

enum ModelFilter
{
    ModelFilter_None,
    ModelFilter_Physical,
    ModelFilter_Appearance
};

class MaterialsExport ModelProperty: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    ModelProperty();
    ModelProperty(const std::string& name,
                  const std::string& header,
                  const std::string& type,
                  const std::string& units,
                  const std::string& url,
                  const std::string& description);
    ModelProperty(const ModelProperty& other);
    ~ModelProperty() override = default;

    const std::string getName() const
    {
        return _name;
    }
    const std::string getDisplayName() const;
    const std::string getPropertyType() const
    {
        return _propertyType;
    }
    const std::string getUnits() const
    {
        return _units;
    }
    const std::string getURL() const
    {
        return _url;
    }
    const std::string getDescription() const
    {
        return _description;
    }
    const std::string getInheritance() const
    {
        return _inheritance;
    }
    bool isInherited() const
    {
        return (_inheritance.length() > 0);
    }

    void setName(const std::string& name)
    {
        _name = name;
    }
    void setDisplayName(const std::string& header)
    {
        _displayName = header;
    }
    virtual void setPropertyType(const std::string& type)
    {
        _propertyType = type;
    }
    void setUnits(const std::string& units)
    {
        _units = units;
    }
    void setURL(const std::string& url)
    {
        _url = url;
    }
    void setDescription(const std::string& description)
    {
        _description = trim_copy(description);
    }
    void setInheritance(const std::string& uuid)
    {
        _inheritance = uuid;
    }

    void addColumn(ModelProperty& column)
    {
        _columns.push_back(column);
    }
    const std::vector<ModelProperty>& getColumns() const
    {
        return _columns;
    }
    int columns() const
    {
        return _columns.size();
    }

    ModelProperty& operator=(const ModelProperty& other);
    bool operator==(const ModelProperty& other) const;
    bool operator!=(const ModelProperty& other) const
    {
        return !operator==(other);
    }

    void validate(const ModelProperty& other) const;

private:
    std::string _name;
    std::string _displayName;
    std::string _propertyType;
    std::string _units;
    std::string _url;
    std::string _description;
    std::string _inheritance;
    std::vector<ModelProperty> _columns;
};

class MaterialsExport Model: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    enum ModelType
    {
        ModelType_Physical,
        ModelType_Appearance
    };

    Model();
    Model(std::shared_ptr<ModelLibrary> library,
          ModelType type,
          const std::string& name,
          const std::string& directory,
          const std::string& uuid,
          const std::string& description,
          const std::string& url,
          const std::string& doi);
    ~Model() override = default;

    std::shared_ptr<ModelLibrary> getLibrary() const
    {
        return _library;
    }
    bool isDisabled() const;
    std::string getBase() const
    {
        return (_type == ModelType_Physical) ? "Model"
                                             : "AppearanceModel";
    }
    std::string getName() const
    {
        return _name;
    }
    ModelType getType() const
    {
        return _type;
    }
    std::string getDirectory() const;
    std::string getFilename() const;
    std::string getFilePath() const;
    std::string getUUID() const
    {
        return _uuid;
    }
    std::string getDescription() const
    {
        return _description;
    }
    std::string getURL() const
    {
        return _url;
    }
    std::string getDOI() const
    {
        return _doi;
    }

    void setLibrary(std::shared_ptr<ModelLibrary> library)
    {
        _library = library;
    }
    void setType(ModelType type)
    {
        _type = type;
    }
    void setName(const std::string& name)
    {
        _name = name;
    }
    void setDirectory(const std::string& directory);
    void setFilename(const std::string& filename);
    void setUUID(const std::string& uuid)
    {
        _uuid = uuid;
    }
    void setDescription(const std::string& description)
    {
        _description = description;
    }
    void setURL(const std::string& url)
    {
        _url = url;
    }
    void setDOI(const std::string& doi)
    {
        _doi = doi;
    }

    void addInheritance(const std::string& uuid)
    {
        _inheritedUuids.push_back(uuid);
    }
    const std::vector<std::string>& getInheritance() const
    {
        return _inheritedUuids;
    }
    bool inherits(const std::string& uuid) const
    {
        auto it = std::find(_inheritedUuids.cbegin(), _inheritedUuids.cend(), uuid);
        if (it != _inheritedUuids.cend()) {
            return true;
        }
        return false;
    }

    bool operator==(const Model& m) const
    {
        return _uuid == m._uuid;
    }
    bool operator!=(const Model& m) const
    {
        return !operator==(m);
    }

    bool hasProperty(const std::string& name) const;
    ModelProperty& operator[](const std::string& key);
    void addProperty(ModelProperty& property)
    {
        _properties[property.getName()] = property;
    }

    using iterator = typename std::map<std::string, ModelProperty>::iterator;
    using const_iterator = typename std::map<std::string, ModelProperty>::const_iterator;
    iterator begin()
    {
        return _properties.begin();
    }
    const_iterator begin() const noexcept
    {
        return _properties.begin();
    }
    iterator end() noexcept
    {
        return _properties.end();
    }
    const_iterator end() const noexcept
    {
        return _properties.end();
    }
    const_iterator cbegin() const noexcept
    {
        return _properties.cbegin();
    }
    const_iterator cend() const noexcept
    {
        return _properties.cend();
    }

    void validate(Model& other) const;
    void save(Base::TextOutputStream& stream);

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

    bool isDereferencing() const
    {
        return _dereferencing;
    }
    void markDereferencing()
    {
        _dereferencing = true;
    }
    void clearDereferencing()
    {
        _dereferencing = false;
    }

private:
    std::shared_ptr<ModelLibrary> _library;
    ModelType _type;
    std::string _name;
    std::string _directory;
    std::string _filename;
    std::string _uuid;
    std::string _description;
    std::string _url;
    std::string _doi;
    std::vector<std::string> _inheritedUuids;
    std::map<std::string, ModelProperty> _properties;

    bool _dereferenced;
    bool _dereferencing;

    void saveGeneral(Base::TextOutputStream& stream) const;
    void saveInherits(Base::TextOutputStream& stream) const;
    void saveProperties(Base::TextOutputStream& stream) const;
};

typedef FolderTreeNode<Model> ModelTreeNode;

}  // namespace Materials

Q_DECLARE_METATYPE(Materials::ModelProperty)
Q_DECLARE_METATYPE(Materials::Model*)
Q_DECLARE_METATYPE(std::shared_ptr<Materials::Model>)
