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

#ifndef MATERIAL_MATERIALPROPERTY_H
#define MATERIAL_MATERIALPROPERTY_H

#include <memory>

// #include <QDir>
// #include <QSet>

// #include <App/Application.h>
#include <Base/Color.h>
// #include <App/Material.h>
// #include <Base/BaseClass.h>
#include <Base/Stream.h>

#include <Mod/Material/MaterialGlobal.h>

#include "MaterialValue.h"
#include "Model.h"

namespace Materials
{

class MaterialsExport MaterialProperty: public ModelProperty
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    MaterialProperty();
    MaterialProperty(const MaterialProperty& other);
    explicit MaterialProperty(const ModelProperty& other, std::string modelUUID);
    explicit MaterialProperty(const std::shared_ptr<MaterialProperty>& other);
    ~MaterialProperty() override = default;

    MaterialValue::ValueType getType() const
    {
        return _valuePtr->getType();
    }

    const std::string getModelUUID() const
    {
        return _modelUUID;
    }

    QVariant getValue();
    QVariant getValue() const;
    std::vector<QVariant> getList()
    {
        return _valuePtr->getList();
    }
    std::vector<QVariant> getList() const
    {
        return _valuePtr->getList();
    }
    bool isNull() const
    {
        return _valuePtr->isNull();
    }
    bool isEmpty() const
    {
        return _valuePtr->isEmpty();
    }
    std::shared_ptr<MaterialValue> getMaterialValue();
    std::shared_ptr<MaterialValue> getMaterialValue() const;
    std::string getString() const;
    std::string getYAMLString() const;
    std::string getDictionaryString() const;  // Non-localized string
    bool getBoolean() const
    {
        return getValue().toBool();
    }
    int getInt() const
    {
        return getValue().toInt();
    }
    double getFloat() const
    {
        return getValue().toFloat();
    }
    const Base::Quantity& getQuantity() const;
    std::string getURL() const
    {
        return getValue().toString().toStdString();
    }
    Base::Color getColor() const;

    MaterialProperty& getColumn(int column);
    const MaterialProperty& getColumn(int column) const;
    MaterialValue::ValueType getColumnType(int column) const;
    std::string getColumnUnits(int column) const;
    QVariant getColumnNull(int column) const;
    const std::vector<MaterialProperty>& getColumns() const
    {
        return _columns;
    }

    void setModelUUID(const std::string& uuid);
    void setPropertyType(const std::string& type) override;
    void setValue(const QVariant& value);
    void setValue(const std::string& value);
    void setValue(const std::shared_ptr<MaterialValue>& value);
    void setString(const std::string& value);
    void setBoolean(bool value);
    void setBoolean(int value);
    void setBoolean(const std::string& value);
    void setInt(int value);
    void setInt(const std::string& value);
    void setFloat(double value);
    void setFloat(const std::string& value);
    void setQuantity(const Base::Quantity& value);
    void setQuantity(double value, const std::string& units);
    void setQuantity(const std::string& value);
    void setList(const std::vector<QVariant>& value);
    void setURL(const std::string& value);
    void setColor(const Base::Color& value);

    MaterialProperty& operator=(const MaterialProperty& other);
    friend Base::ofstream& operator<<(Base::ofstream& output, const MaterialProperty& property);

    bool operator==(const MaterialProperty& other) const;
    bool operator!=(const MaterialProperty& other) const
    {
        return !operator==(other);
    }

    void validate(const MaterialProperty& other) const;

    // Define precision for displaying floating point values
    static int const PRECISION;

protected:
    void setType(const std::string& type);
    // void setType(MaterialValue::ValueType type) { _valueType = type; }
    void copyValuePtr(const std::shared_ptr<MaterialValue>& value);

    void addColumn(MaterialProperty& column)
    {
        _columns.push_back(column);
    }

private:
    std::string _modelUUID;
    std::shared_ptr<MaterialValue> _valuePtr;
    std::vector<MaterialProperty> _columns;
};

inline Base::ofstream& operator<<(Base::ofstream& output, const MaterialProperty& property)
{
    output << MaterialValue::escapeString(property.getName()) + ":" + property.getYAMLString();
    return output;
}

}  // namespace Materials

#endif  // MATERIAL_MATERIALPROPERTY_H
