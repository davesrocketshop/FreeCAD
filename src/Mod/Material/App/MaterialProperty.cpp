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

#include "MaterialProperty.h"
#include "ModelManager.h"


using namespace Materials;

/* TRANSLATOR Material::MaterialProperty */

TYPESYSTEM_SOURCE(Materials::MaterialProperty, Materials::ModelProperty)

MaterialProperty::MaterialProperty()
{
    _valuePtr = std::make_shared<MaterialValue>(MaterialValue::None);
}

MaterialProperty::MaterialProperty(const ModelProperty& other, std::string modelUUID)
    : ModelProperty(other)
    , _modelUUID(modelUUID)
    , _valuePtr(nullptr)
{
    setType(getPropertyType());
    auto columns = other.getColumns();
    for (auto& it : columns) {
        MaterialProperty prop(it, modelUUID);
        addColumn(prop);
    }
}

void MaterialProperty::copyValuePtr(const std::shared_ptr<MaterialValue>& value)
{
    if (value->getType() == MaterialValue::Array2D) {
        _valuePtr =
            std::make_shared<Array2D>(*(std::static_pointer_cast<Array2D>(value)));
    }
    else if (value->getType() == MaterialValue::Array3D) {
        _valuePtr =
            std::make_shared<Array3D>(*(std::static_pointer_cast<Array3D>(value)));
    }
    else {
        _valuePtr = std::make_shared<MaterialValue>(*value);
    }
}

MaterialProperty::MaterialProperty(const MaterialProperty& other)
    : ModelProperty(other)
    , _modelUUID(other._modelUUID)
    , _columns(other._columns)
{
    copyValuePtr(other._valuePtr);
}

MaterialProperty::MaterialProperty(const std::shared_ptr<MaterialProperty>& other)
    : MaterialProperty(*other)
{}

void MaterialProperty::setModelUUID(const std::string& uuid)
{
    _modelUUID = uuid;
}

QVariant MaterialProperty::getValue()
{
    return _valuePtr->getValue();
}

QVariant MaterialProperty::getValue() const
{
    return _valuePtr->getValue();
}

std::shared_ptr<MaterialValue> MaterialProperty::getMaterialValue()
{
    return _valuePtr;
}

std::shared_ptr<MaterialValue> MaterialProperty::getMaterialValue() const
{
    return _valuePtr;
}

std::string MaterialProperty::getString() const
{
    // This method produces a localized string. For a non-localized string use
    // getDictionaryString()
    if (isNull()) {
        return {};
    }
    if (getType() == MaterialValue::Quantity) {
        auto quantity = getValue().value<Base::Quantity>();
        return quantity.getUserString();
    }
    if (getType() == MaterialValue::Float) {
        auto value = getValue();
        if (value.isNull()) {
            return {};
        }
        return QString(QStringLiteral("%L1")).arg(value.toFloat(), 0, 'g', MaterialValue::PRECISION).toStdString();
    }
    return getValue().toString().toStdString();
}

std::string MaterialProperty::getYAMLString() const
{
    return _valuePtr->getYAMLString();
}

Base::Color MaterialProperty::getColor() const
{
    auto colorString = getValue().toString().toStdString();
    std::stringstream stream(colorString);

    char c;
    stream >> c;  // read "("
    float red;
    stream >> red;
    stream >> c;  // ","
    float green;
    stream >> green;
    stream >> c;  // ","
    float blue;
    stream >> blue;
    stream >> c;  // ","
    float alpha = 1.0;
    if (c == ',') {
        stream >> alpha;
    }

    Base::Color color(red, green, blue, alpha);
    return color;
}


std::string MaterialProperty::getDictionaryString() const
{
    // This method produces a non-localized string. For a localized string use
    // getString()
    if (isNull()) {
        return {};
    }
    if (getType() == MaterialValue::Quantity) {
        auto quantity = getValue().value<Base::Quantity>();
        auto string = QString(QStringLiteral("%1 %2"))
                          .arg(quantity.getValue(), 0, 'g', MaterialValue::PRECISION)
                          .arg(QString::fromStdString(quantity.getUnit().getString()));
        return string.toStdString();
    }
    if (getType() == MaterialValue::Float) {
        auto value = getValue();
        if (value.isNull()) {
            return {};
        }
        return QString(QStringLiteral("%1"))
            .arg(value.toFloat(), 0, 'g', MaterialValue::PRECISION)
            .toStdString();
    }
    return getValue().toString().toStdString();
}

void MaterialProperty::setPropertyType(const std::string& type)
{
    ModelProperty::setPropertyType(type);
    setType(type);
}

void MaterialProperty::setType(const std::string& type)
{
    auto mappedType = MaterialValue::mapType(type);
    if (mappedType == MaterialValue::None) {
        throw UnknownValueType();
    }
    if (mappedType == MaterialValue::Array2D) {
        auto arrayPtr = std::make_shared<Array2D>();
        arrayPtr->setColumns(columns());
        _valuePtr = arrayPtr;
    }
    else if (mappedType == MaterialValue::Array3D) {
        auto arrayPtr = std::make_shared<Array3D>();
        // First column is third dimension
        arrayPtr->setColumns(columns() - 1);
        _valuePtr = arrayPtr;
    }
    else {
        _valuePtr = std::make_shared<MaterialValue>(mappedType);
    }
}

MaterialProperty& MaterialProperty::getColumn(int column)
{
    try {
        return _columns.at(column);
    }
    catch (std::out_of_range const&) {
        throw InvalidIndex();
    }
}

const MaterialProperty& MaterialProperty::getColumn(int column) const
{
    try {
        return _columns.at(column);
    }
    catch (std::out_of_range const&) {
        throw InvalidIndex();
    }
}

MaterialValue::ValueType MaterialProperty::getColumnType(int column) const
{
    try {
        return _columns.at(column).getType();
    }
    catch (std::out_of_range const&) {
        throw InvalidIndex();
    }
}

std::string MaterialProperty::getColumnUnits(int column) const
{
    try {
        return _columns.at(column).getUnits();
    }
    catch (std::out_of_range const&) {
        throw InvalidIndex();
    }
}

QVariant MaterialProperty::getColumnNull(int column) const
{
    MaterialValue::ValueType valueType = getColumnType(column);

    switch (valueType) {
        case MaterialValue::Quantity: {
            Base::Quantity quant = Base::Quantity(0, getColumnUnits(column));
            return QVariant::fromValue(quant);
        }

        case MaterialValue::Float:
        case MaterialValue::Integer:
            return 0;

        default:
            break;
    }

    return QString();
}

void MaterialProperty::setValue(const QVariant& value)
{
    if (_valuePtr->getType() == MaterialValue::Quantity && value.canConvert<Base::Quantity>()) {
        // Ensure the units are set correctly
        auto quantity = value.value<Base::Quantity>();
        if (quantity.isValid()) {
            setQuantity(quantity);
        }
        else {
            // Set a default value with default units
            setValue(std::string("0"));
        }
    }
    else {
        _valuePtr->setValue(value);
    }
}

void MaterialProperty::setValue(const std::string& value)
{
    if (_valuePtr->getType() == MaterialValue::Boolean) {
        setBoolean(value);
    }
    else if (_valuePtr->getType() == MaterialValue::Integer) {
        setInt(value);
    }
    else if (_valuePtr->getType() == MaterialValue::Float) {
        setFloat(value);
    }
    else if (_valuePtr->getType() == MaterialValue::URL) {
        setURL(value);
    }
    else if (_valuePtr->getType() == MaterialValue::Array2D
             || _valuePtr->getType() == MaterialValue::Array3D) {
        // This value can't be directly assigned
    }
    else if (_valuePtr->getType() == MaterialValue::Quantity) {
        try {
            setQuantity(Base::Quantity::parse(value));
        }
        catch (const Base::ParserError& e) {
            Base::Console().log("MaterialProperty::setValue Error '%s' - '%s'\n",
                                e.what(),
                                value.c_str());
            // Save as a string
            setString(value);
        }
    }
    else {
        setString(value);
    }
}

void MaterialProperty::setValue(const std::shared_ptr<MaterialValue>& value)
{
    _valuePtr = value;
}

void MaterialProperty::setString(const std::string& value)
{
    _valuePtr->setValue(QString::fromStdString(value));
}

// void MaterialProperty::setString(const std::string& value)
// {
//     _valuePtr->setValue(QString::fromStdString(value));
// }

void MaterialProperty::setBoolean(bool value)
{
    _valuePtr->setValue(QVariant(value));
}

void MaterialProperty::setBoolean(int value)
{
    _valuePtr->setValue(QVariant(value != 0));
}

void MaterialProperty::setBoolean(const std::string& value)
{
    bool boolean = false;
    std::string val = value;
    if ((val == "true") || (val == "True")) {
        boolean = true;
    }
    else if ((val == "false") || (val == "False")) {
        boolean = false;
    }
    else {
        boolean = (std::stoi(val) != 0);
    }

    setBoolean(boolean);
}

void MaterialProperty::setInt(int value)
{
    _valuePtr->setValue(QVariant(value));
}

void MaterialProperty::setInt(const std::string& value)
{
    _valuePtr->setValue(QString::fromStdString(value).toInt());
}

void MaterialProperty::setFloat(double value)
{
    _valuePtr->setValue(QVariant(value));
}

void MaterialProperty::setFloat(const std::string& value)
{
    _valuePtr->setValue(QString::fromStdString(value).toFloat());
}

void MaterialProperty::setQuantity(const Base::Quantity& value)
{
    auto quantity = value;
    if (quantity.isDimensionless()) {
        // Assign the default units when none are provided.
        //
        // This needs to be parsed rather than just setting units. Otherwise we get mm->m conversion
        // errors, etc
        quantity = Base::Quantity::parse(quantity.getUserString() + getUnits());
    }
    else {
        auto propertyUnit = Base::Quantity::parse(getUnits()).getUnit();
        auto units = quantity.getUnit();
        if (propertyUnit != units) {
            throw Base::ValueError("Incompatible material units");
        }
    }
    quantity.setFormat(MaterialValue::getQuantityFormat());
    _valuePtr->setValue(QVariant(QVariant::fromValue(quantity)));
}

void MaterialProperty::setQuantity(double value, const std::string& units)
{
    setQuantity(Base::Quantity(value, units));
}

void MaterialProperty::setQuantity(const std::string& value)
{
    setQuantity(Base::Quantity::parse(value));
}

void MaterialProperty::setList(const std::vector<QVariant>& value)
{
    _valuePtr->setList(value);
}

void MaterialProperty::setURL(const std::string& value)
{
    _valuePtr->setValue(QString::fromStdString(value));
}

void MaterialProperty::setColor(const Base::Color& value)
{
    std::stringstream ss;
    ss << "(" << value.r << ", " << value.g << ", " << value.b << ", " << value.a << ")";
    _valuePtr->setValue(QString::fromStdString(ss.str()));
}

MaterialProperty& MaterialProperty::operator=(const MaterialProperty& other)
{
    if (this == &other) {
        return *this;
    }

    ModelProperty::operator=(other);

    _modelUUID = other._modelUUID;
    _columns = other._columns;
    copyValuePtr(other._valuePtr);

    return *this;
}

bool MaterialProperty::operator==(const MaterialProperty& other) const
{
    if (this == &other) {
        return true;
    }

    if (ModelProperty::operator==(other)) {
        return (*_valuePtr == *other._valuePtr);
    }
    return false;
}

void MaterialProperty::validate(const MaterialProperty& other) const {
    _valuePtr->validate(*other._valuePtr);

    if (_columns.size() != other._columns.size()) {
        throw InvalidProperty("Model property column counts don't match");
    }
    for (size_t i = 0; i < _columns.size(); i++) {
        _columns[i].validate(other._columns[i]);
    }
}
