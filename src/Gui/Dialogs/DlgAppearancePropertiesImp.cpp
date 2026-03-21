// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include <App/PropertyStandard.h>

#include "Dialogs/DlgAppearancePropertiesImp.h"
#include "ui_DlgAppearanceProperties.h"
#include "ViewProvider.h"

#include <Base/Tools.h>


using namespace Gui::Dialog;


/* TRANSLATOR Gui::Dialog::DlgAppearancePropertiesImp */

DlgAppearancePropertiesImp::DlgAppearancePropertiesImp(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
    , ui(new Ui_DlgAppearanceProperties)
{
    ui->setupUi(this);
    setupConnections();

    // Not implemented yet
    ui->faceLabel->setVisible(false);
    ui->faceCombo->setVisible(false);

    ui->ambientColor->setAutoChangeColor(true);
    ui->diffuseColor->setAutoChangeColor(true);
    ui->emissiveColor->setAutoChangeColor(true);
    ui->specularColor->setAutoChangeColor(true);
    ui->blendColor->setAutoChangeColor(true);

    // Set the texture mode combo box
    ui->textureModeCombo->addItem(tr("None"), static_cast<int>(App::Material::TextureMode::NONE));
    ui->textureModeCombo->addItem(tr("Decal"), static_cast<int>(App::Material::TextureMode::DECAL));
    ui->textureModeCombo->addItem(tr("Modulate"), static_cast<int>(App::Material::TextureMode::MODULATE));
    ui->textureModeCombo->addItem(tr("Blend"), static_cast<int>(App::Material::TextureMode::BLEND));
    // ui->textureModeCombo->addItem(tr("Replace"), static_cast<int>(App::Material::TextureMode::REPLACE));

    if (customMaterial.textureMode == App::Material::TextureMode::BLEND) {
        ui->blendColor->setEnabled(true);
    }
    else {
        ui->blendColor->setEnabled(false);
    }
}

DlgAppearancePropertiesImp::~DlgAppearancePropertiesImp() = default;

void DlgAppearancePropertiesImp::setupConnections()
{
    // clang-format off
    connect(ui->ambientColor, &ColorButton::changed,
            this, &DlgAppearancePropertiesImp::onAmbientColorChanged);
    connect(ui->diffuseColor, &ColorButton::changed,
            this, &DlgAppearancePropertiesImp::onDiffuseColorChanged);
    connect(ui->emissiveColor, &ColorButton::clicked,
            this, &DlgAppearancePropertiesImp::onEmissiveColorChanged);
    connect(ui->specularColor, &ColorButton::clicked,
            this, &DlgAppearancePropertiesImp::onSpecularColorChanged);
    connect(ui->shininess, qOverload<int>(&QSpinBox::valueChanged),
            this, &DlgAppearancePropertiesImp::onShininessValueChanged);
    connect(ui->transparency, qOverload<int>(&QSpinBox::valueChanged),
            this, &DlgAppearancePropertiesImp::onTransparencyValueChanged);
    connect(ui->buttonReset, &QPushButton::clicked,
            this, &DlgAppearancePropertiesImp::onButtonReset);
    connect(ui->buttonDefault, &QPushButton::clicked,
            this, &DlgAppearancePropertiesImp::onButtonDefault);

    connect(ui->textureModeCombo, &QComboBox::currentIndexChanged,
            this, &DlgAppearancePropertiesImp::onTextureModeChanged);
    connect(ui->textureFileChooser, &FileChooser::fileNameSelected,
            this, &DlgAppearancePropertiesImp::onTextureFileSelected);
    connect(ui->blendColor, &ColorButton::clicked,
            this, &DlgAppearancePropertiesImp::onBlendColorChanged);
    connect(ui->sWrapCombo, &QComboBox::currentIndexChanged,
            this, &DlgAppearancePropertiesImp::onSWrapChanged);
    connect(ui->tWrapCombo, &QComboBox::currentIndexChanged,
            this, &DlgAppearancePropertiesImp::onTWrapChanged);
    // clang-format on
}

void DlgAppearancePropertiesImp::setCustomMaterial(const App::Material& mat)
{
    customMaterial = mat;
    setButtonColors(customMaterial);
    setTextureValues(customMaterial);
}

App::Material DlgAppearancePropertiesImp::getCustomMaterial() const
{
    return customMaterial;
}

void DlgAppearancePropertiesImp::setDefaultMaterial(const App::Material& mat)
{
    defaultMaterial = mat;
}

App::Material DlgAppearancePropertiesImp::getDefaultMaterial() const
{
    return defaultMaterial;
}

/**
 * Sets the ambient color.
 */
void DlgAppearancePropertiesImp::onAmbientColorChanged()
{
    customMaterial.ambientColor.setValue(ui->ambientColor->color());
}

/**
 * Sets the diffuse color.
 */
void DlgAppearancePropertiesImp::onDiffuseColorChanged()
{
    customMaterial.diffuseColor.setValue(ui->diffuseColor->color());
}

/**
 * Sets the emissive color.
 */
void DlgAppearancePropertiesImp::onEmissiveColorChanged()
{
    customMaterial.emissiveColor.setValue(ui->emissiveColor->color());
}

/**
 * Sets the specular color.
 */
void DlgAppearancePropertiesImp::onSpecularColorChanged()
{
    customMaterial.specularColor.setValue(ui->specularColor->color());
}

/**
 * Sets the current shininess.
 */
void DlgAppearancePropertiesImp::onShininessValueChanged(int sh)
{
    customMaterial.shininess = Base::fromPercent(sh);
}

/**
 * Sets the current transparency.
 */
void DlgAppearancePropertiesImp::onTransparencyValueChanged(int sh)
{
    customMaterial.transparency = Base::fromPercent(sh);
}

/**
 * Sets the current texture mode.
 */
void DlgAppearancePropertiesImp::onTextureModeChanged(int index)
{
    customMaterial.textureMode = static_cast<App::Material::TextureMode>(ui->textureModeCombo->currentData().toInt());
    if (customMaterial.textureMode == App::Material::TextureMode::BLEND) {
        ui->blendColor->setEnabled(true);
    } else {
        ui->blendColor->setEnabled(false);
    }
}

/**
 * Sets the current texture file.
 */
void DlgAppearancePropertiesImp::onTextureFileSelected(const QString& file)
{
    customMaterial.imagePath = file.toStdString();
    if (!customMaterial.imagePath.empty()) {
        auto pixmap = QPixmap(QString::fromStdString(customMaterial.imagePath));
        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        pixmap.save(&buffer, "PNG");
        QByteArray base64 = buffer.data().toBase64();
        customMaterial.image = base64.toStdString();
        if (customMaterial.textureMode == App::Material::TextureMode::NONE) {
            customMaterial.textureMode = App::Material::TextureMode::DECAL;
            ui->textureModeCombo->setCurrentIndex(
                ui->textureModeCombo->findData(static_cast<int>(App::Material::TextureMode::DECAL))
            );
        }
    }
    else {
        customMaterial.image.clear();
        customMaterial.textureMode = App::Material::TextureMode::NONE;
        ui->textureModeCombo->setCurrentIndex(
            ui->textureModeCombo->findData(static_cast<int>(App::Material::TextureMode::NONE))
        );
    }
}

/**
 * Sets the blend color.
 */
void DlgAppearancePropertiesImp::onBlendColorChanged()
{
    customMaterial.blendColor.setValue(ui->blendColor->color());
}

/**
 * Sets the S wrap mode.
 */
void DlgAppearancePropertiesImp::onSWrapChanged(int index)
{
    customMaterial.textureSWrap = static_cast<App::Material::TextureWrapMode>(ui->sWrapCombo->currentIndex());
}

/**
 * Sets the T wrap mode.
 */
void DlgAppearancePropertiesImp::onTWrapChanged(int index)
{
    customMaterial.textureTWrap = static_cast<App::Material::TextureWrapMode>(ui->tWrapCombo->currentIndex());
}

/**
 * Reset the colors to the Coin3D defaults
 */
void DlgAppearancePropertiesImp::onButtonReset()
{
    setCustomMaterial(getDefaultMaterial());
}

/**
 * Reset the colors to the current default
 */
void DlgAppearancePropertiesImp::onButtonDefault()
{
    App::Material mat = App::Material::getDefaultAppearance();
    setCustomMaterial(mat);
}

/**
 * Sets the button colors to match the current material settings.
 */
void DlgAppearancePropertiesImp::setButtonColors(const App::Material& mat)
{
    ui->ambientColor->setColor(mat.ambientColor.asValue<QColor>());
    ui->diffuseColor->setColor(mat.diffuseColor.asValue<QColor>());
    ui->emissiveColor->setColor(mat.emissiveColor.asValue<QColor>());
    ui->specularColor->setColor(mat.specularColor.asValue<QColor>());
    ui->shininess->blockSignals(true);
    ui->shininess->setValue((int)(100.0F * (mat.shininess + 0.001F)));
    ui->shininess->blockSignals(false);
    ui->transparency->blockSignals(true);
    ui->transparency->setValue((int)(100.0F * (mat.transparency + 0.001F)));
    ui->transparency->blockSignals(false);
}

/**
 * Sets the texture values to match the current material settings.
 */
void DlgAppearancePropertiesImp::setTextureValues(const App::Material& mat)
{
    ui->textureModeCombo->setCurrentIndex(
        ui->textureModeCombo->findData(static_cast<int>(mat.textureMode))
    );
    ui->textureFileChooser->setFileName(QString::fromStdString(mat.imagePath));
    ui->sWrapCombo->setCurrentIndex(static_cast<int>(mat.textureSWrap));
    ui->tWrapCombo->setCurrentIndex(static_cast<int>(mat.textureTWrap));
    ui->blendColor->setColor(mat.blendColor.asValue<QColor>());
    ui->bumpFileChooser->setFileName(QString::fromStdString(mat.bumpImagePath));
}

#include "moc_DlgAppearancePropertiesImp.cpp"
