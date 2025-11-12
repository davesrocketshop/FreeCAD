/***************************************************************************
 *   Copyright (c) 2025 David Carter <dcarter@david.carter.ca>             *
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

#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <Gui/WaitCursor.h>

#include <Mod/Material/App/Exceptions.h>
#include <Mod/Material/App/ModelManager.h>
#include <Mod/Material/App/ModelUuids.h>

#include <Mod/Material/Gui/MaterialDelegate.h>

#include "MaterialPropertiesWidget.h"
#include "ui_MaterialPropertiesWidget.h"

using namespace MatGui;

/* TRANSLATOR MatGui::MaterialsEditor */

MaterialPropertiesWidget::MaterialPropertiesWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui_MaterialPropertiesWidget)
    , _material(std::make_shared<Materials::Material>())
    , _rendered(nullptr)
    , _materialSelected(false)
{
    setup();
}

/*
 *  Destroys the object and frees any allocated resources
 */
MaterialPropertiesWidget::~MaterialPropertiesWidget()
{
    // no need to delete child widgets, Qt does it all for us
    // but we can't use default as Ui_MaterialPropertiesWidget is undefined
}

void MaterialPropertiesWidget::setup()
{
    Gui::WaitCursor wc;
    ui->setupUi(this);

    setupButtons();

    createPhysicalTree();
    createAppearanceTree();
    createPreviews();
}

void MaterialPropertiesWidget::setupButtons()
{
    connect(ui->buttonPhysicalAdd, &QPushButton::clicked, this, &MaterialPropertiesWidget::onPhysicalAdd);
    connect(ui->buttonPhysicalRemove, &QPushButton::clicked, this, &MaterialPropertiesWidget::onPhysicalRemove);
    connect(ui->buttonAppearanceAdd, &QPushButton::clicked, this, &MaterialPropertiesWidget::onAppearanceAdd);
    connect(ui->buttonAppearanceRemove, &QPushButton::clicked, this, &MaterialPropertiesWidget::onAppearanceRemove);
}

void MaterialPropertiesWidget::onPhysicalAdd()
{
    Q_EMIT addPhysicalProperty();
}

void MaterialPropertiesWidget::onPhysicalRemove()
{
    QItemSelectionModel* selectionModel = ui->treePhysicalProperties->selectionModel();
    if (selectionModel->hasSelection()) {
        auto index = selectionModel->currentIndex().siblingAtColumn(0);

        auto treeModel = dynamic_cast<const QStandardItemModel*>(index.model());

        // Check we're the material model root.
        auto item = treeModel->itemFromIndex(index);
        auto group = item->parent();
        if (!group) {
            QString propertyName = index.data().toString();

            Q_EMIT removePhysicalProperty(propertyName);
        }
    }
}

void MaterialPropertiesWidget::onAppearanceAdd()
{
    Q_EMIT addAppearanceProperty();
}

void MaterialPropertiesWidget::onAppearanceRemove()
{
    QItemSelectionModel* selectionModel = ui->treeAppearance->selectionModel();
    if (selectionModel->hasSelection()) {
        auto index = selectionModel->currentIndex().siblingAtColumn(0);

        auto treeModel = dynamic_cast<const QStandardItemModel*>(index.model());

        // Check we're the material model root.
        auto item = treeModel->itemFromIndex(index);
        auto group = item->parent();
        if (!group) {
            QString propertyName = index.data().toString();

            Q_EMIT removeAppearanceProperty(propertyName);
        }
    }
}


QString MaterialPropertiesWidget::libraryPath(const std::shared_ptr<Materials::Material>& material)
{
    QString path;
    auto library = material->getLibrary();
    if (library) {
        path = QStringLiteral("/%1/%2/%3")
                   .arg(library->getName())
                   .arg(material->getDirectory())
                   .arg(material->getName());
        return path;
    }

    path = QStringLiteral("%1/%2").arg(material->getDirectory()).arg(material->getName());
    return path;
}

QString MaterialPropertiesWidget::getColorHash(const QString& colorString, int colorRange)
{
    /*
        returns a '#000000' string from a '(0.1,0.2,0.3)' string. Optionally the string
        has a fourth value for alpha (transparency)
    */
    std::stringstream stream(colorString.toStdString());

    char c;
    stream >> c;  // read "("
    double red;
    stream >> red;
    stream >> c;  // ","
    double green;
    stream >> green;
    stream >> c;  // ","
    double blue;
    stream >> blue;
    stream >> c;  // ","
    double alpha = 1.0;
    if (c == ',') {
        stream >> alpha;
    }

    QColor color(static_cast<int>(red * colorRange),
                 static_cast<int>(green * colorRange),
                 static_cast<int>(blue * colorRange),
                 static_cast<int>(alpha * colorRange));
    return color.name();
}

void MaterialPropertiesWidget::addExpanded(QTreeView* tree,
                                           QStandardItemModel* parent,
                                           QStandardItem* child)
{
    parent->appendRow(child);
    tree->setExpanded(child->index(), true);
}

void MaterialPropertiesWidget::createPhysicalTree()
{
    auto tree = ui->treePhysicalProperties;
    auto model = new QStandardItemModel();
    tree->setModel(model);

    QStringList headers;
    headers.append(tr("Property"));
    headers.append(tr("Value"));
    headers.append(tr("Type"));
    model->setHorizontalHeaderLabels(headers);

    tree->setColumnWidth(0, 250);
    tree->setColumnWidth(1, 250);
    tree->setColumnHidden(2, true);

    tree->setHeaderHidden(false);
    tree->setUniformRowHeights(false);
    auto delegate = new MaterialDelegate(this);
    tree->setItemDelegateForColumn(1, delegate);

    connect(delegate,
            &MaterialDelegate::propertyChange,
            this,
            &MaterialPropertiesWidget::onPropertyChange);
}

void MaterialPropertiesWidget::createPreviews()
{
    _rendered = new AppearancePreview();
    ui->layoutAppearance->addWidget(_rendered);

    updatePreview();
}

void MaterialPropertiesWidget::createAppearanceTree()
{
    auto tree = ui->treeAppearance;
    auto model = new QStandardItemModel();
    tree->setModel(model);

    QStringList headers;
    headers.append(tr("Property"));
    headers.append(tr("Value"));
    headers.append(tr("Type"));
    model->setHorizontalHeaderLabels(headers);

    tree->setColumnWidth(0, 250);
    tree->setColumnWidth(1, 250);
    tree->setColumnHidden(2, true);

    tree->setHeaderHidden(false);
    tree->setUniformRowHeights(false);
    auto delegate = new MaterialDelegate(this);
    tree->setItemDelegateForColumn(1, delegate);

    connect(delegate,
            &MaterialDelegate::propertyChange,
            this,
            &MaterialPropertiesWidget::onPropertyChange);
}

void MaterialPropertiesWidget::updateMaterial(const std::shared_ptr<Materials::Material>& material)
{
    _material = material;
    updateMaterial();
}

void MaterialPropertiesWidget::updateMaterial()
{
    updateMaterialGeneral();
    updateMaterialProperties();
    updateMaterialAppearance();

    updatePreview();
}

void MaterialPropertiesWidget::updateMaterialGeneral()
{
    if (_materialSelected) {
        QString parentString;
        try {
            auto parent = getMaterialManager().getParent(_material);
            parentString = libraryPath(parent);
        }
        catch (const Materials::MaterialNotFound&) {
        }

        // Update the general information
        ui->editName->setText(_material->getName());
        ui->editAuthor->setText(_material->getAuthor());
        ui->editLicense->setText(_material->getLicense());
        ui->editParent->setText(parentString);
        ui->editParent->setReadOnly(true);
        ui->editSourceURL->setText(_material->getURL());
        ui->editSourceReference->setText(_material->getReference());
        // ui->editTags->setText(_material->getName());
        ui->editDescription->setText(_material->getDescription());
    }
    else {
        ui->editName->clear();
        ui->editAuthor->clear();
        ui->editLicense->clear();
        ui->editParent->clear();
        ui->editParent->clear();
        ui->editSourceURL->clear();
        ui->editSourceReference->clear();
        ui->editTags->clear();
        ui->editDescription->clear();
    }

    ui->editName->setEnabled(_materialSelected);
    ui->editAuthor->setEnabled(_materialSelected);
    ui->editLicense->setEnabled(_materialSelected);
    ui->editParent->setEnabled(_materialSelected);
    ui->editParent->setEnabled(_materialSelected);
    ui->editSourceURL->setEnabled(_materialSelected);
    ui->editSourceReference->setEnabled(_materialSelected);
    ui->editTags->setEnabled(_materialSelected);
    ui->editDescription->setEnabled(_materialSelected);
}

void MaterialPropertiesWidget::updateMaterialAppearance()
{
    QTreeView* tree = ui->treeAppearance;
    auto treeModel = qobject_cast<QStandardItemModel*>(tree->model());
    treeModel->clear();

    QStringList headers;
    headers.append(tr("Property"));
    headers.append(tr("Value"));
    headers.append(tr("Type"));
    treeModel->setHorizontalHeaderLabels(headers);

    tree->setColumnWidth(0, 250);
    tree->setColumnWidth(1, 250);
    tree->setColumnHidden(2, true);

    ui->buttonAppearanceAdd->setEnabled(_materialSelected);
    ui->buttonAppearanceRemove->setEnabled(_materialSelected);

    if (_materialSelected) {
        auto models = _material->getAppearanceModels();
        if (models) {
            for (auto it = models->begin(); it != models->end(); it++) {
                QString uuid = *it;
                try {
                    auto model = Materials::ModelManager::getManager().getModel(uuid);
                    QString name = model->getName();

                    auto modelRoot = new QStandardItem(name);
                    modelRoot->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled |
                    Qt::ItemIsDragEnabled
                                        | Qt::ItemIsDropEnabled);
                    addExpanded(tree, treeModel, modelRoot);
                    for (auto itp = model->begin(); itp != model->end(); itp++) {
                        QList<QStandardItem*> items;

                        QString key = itp->first;
                        // auto propertyItem = new QStandardItem(key);
                        auto propertyItem = new QStandardItem(itp->second.getDisplayName());
                        propertyItem->setData(key);
                        propertyItem->setToolTip(itp->second.getDescription());
                        items.append(propertyItem);

                        auto valueItem = new
                        QStandardItem(_material->getAppearanceValueString(key));
                        valueItem->setToolTip(itp->second.getDescription());
                        QVariant variant;
                        // variant.setValue(_material->getAppearanceValueString(key));
                        variant.setValue(_material);
                        valueItem->setData(variant);
                        items.append(valueItem);

                        auto typeItem = new QStandardItem(itp->second.getPropertyType());
                        items.append(typeItem);

                        auto unitsItem = new QStandardItem(itp->second.getUnits());
                        items.append(unitsItem);

                        modelRoot->appendRow(items);
                        tree->setExpanded(modelRoot->index(), true);
                    }
                }
                catch (Materials::ModelNotFound const&) {
                }
            }
        }
    }
}

void MaterialPropertiesWidget::updateMaterialProperties()
{
    QTreeView* tree = ui->treePhysicalProperties;
    auto treeModel = qobject_cast<QStandardItemModel*>(tree->model());
    treeModel->clear();

    QStringList headers;
    headers.append(tr("Property"));
    headers.append(tr("Value"));
    headers.append(tr("Type"));
    headers.append(tr("Units"));
    treeModel->setHorizontalHeaderLabels(headers);

    tree->setColumnWidth(0, 250);
    tree->setColumnWidth(1, 250);
    tree->setColumnHidden(2, true);
    tree->setColumnHidden(3, true);

    ui->buttonPhysicalAdd->setEnabled(_materialSelected);
    ui->buttonPhysicalRemove->setEnabled(_materialSelected);

    if (_materialSelected) {
        auto models = _material->getPhysicalModels();
        if (models) {
            for (auto it = models->begin(); it != models->end(); it++) {
                QString uuid = *it;
                try {
                    auto model = Materials::ModelManager::getManager().getModel(uuid);
                    QString name = model->getName();

                    auto modelRoot = new QStandardItem(name);
                    modelRoot->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
                                        | Qt::ItemIsDropEnabled);
                    addExpanded(tree, treeModel, modelRoot);
                    for (auto itp = model->begin(); itp != model->end(); itp++) {
                        QList<QStandardItem*> items;

                        QString key = itp->first;
                        Materials::ModelProperty modelProperty =
                            static_cast<Materials::ModelProperty>(itp->second);
                        // auto propertyItem = new QStandardItem(key);
                        auto propertyItem = new QStandardItem(modelProperty.getDisplayName());
                        propertyItem->setData(key);
                        propertyItem->setToolTip(modelProperty.getDescription());
                        items.append(propertyItem);

                        auto valueItem = new
                        QStandardItem(_material->getPhysicalValueString(key));
                        valueItem->setToolTip(modelProperty.getDescription());
                        QVariant variant;
                        variant.setValue(_material);
                        valueItem->setData(variant);
                        items.append(valueItem);

                        auto typeItem = new QStandardItem(modelProperty.getPropertyType());
                        items.append(typeItem);

                        auto unitsItem = new QStandardItem(modelProperty.getUnits());
                        items.append(unitsItem);

                        // addExpanded(tree, modelRoot, propertyItem);
                        modelRoot->appendRow(items);
                        tree->setExpanded(modelRoot->index(), true);
                    }
                }
                catch (Materials::ModelNotFound const&) {
                }
            }
        }
    }
}

void MaterialPropertiesWidget::updatePreview() const
{
    if (updateTexturePreview()) {
        return;
    }
    updateMaterialPreview();
}


bool MaterialPropertiesWidget::updateTexturePreview() const
{
    bool hasImage = false;
    QImage image;
    // double scaling = 99.0;
    if (_material->hasModel(Materials::ModelUUIDs::ModelUUID_Rendering_Texture)) {
        // First try loading an embedded image
        try {
            auto property = _material->getAppearanceProperty(QStringLiteral("TextureImage"));
            if (!property->isNull()) {
                auto propertyValue = property->getString();
                if (!propertyValue.isEmpty()) {
                    QByteArray by = QByteArray::fromBase64(propertyValue.toUtf8());
                    image = QImage::fromData(by);
                    hasImage = !image.isNull();
                }
            }
        }
        catch (const Materials::PropertyNotFound&) {
        }

        // If no embedded image, load from a path
        if (!hasImage) {
            try {
                auto property = _material->getAppearanceProperty(QStringLiteral("TexturePath"));
                if (!property->isNull()) {
                    // Base::Console().log("Has 'TexturePath'\n");
                    auto filePath = property->getString();
                    if (!image.load(filePath)) {
                        Base::Console().log("Unable to load image '%s'\n",
                                            filePath.toStdString().c_str());
                        hasImage = false;
                    }
                    else {
                        hasImage = !image.isNull();
                    }
                }
            }
            catch (const Materials::PropertyNotFound&) {
            }
        }

        // Apply any scaling
        try {
            auto property = _material->getAppearanceProperty(QStringLiteral("TextureScaling"));
            if (!property->isNull()) {
                // scaling = property->getFloat();
                //  Base::Console().log("Has 'TextureScaling' = %g\n", scaling);
            }
        }
        catch (const Materials::PropertyNotFound&) {
        }

        if (hasImage) {
            _rendered->setTexture(image);
        }
    }

    return hasImage;
}

bool MaterialPropertiesWidget::updateMaterialPreview() const
{
    if (_material->hasAppearanceProperty(QStringLiteral("AmbientColor"))) {
        QString color = _material->getAppearanceValueString(QStringLiteral("AmbientColor"));
        _rendered->setAmbientColor(getColorHash(color, 255));
    }
    else {
        _rendered->resetAmbientColor();
    }
    if (_material->hasAppearanceProperty(QStringLiteral("DiffuseColor"))) {
        QString color = _material->getAppearanceValueString(QStringLiteral("DiffuseColor"));
        _rendered->setDiffuseColor(getColorHash(color, 255));
    }
    else {
        _rendered->resetDiffuseColor();
    }
    if (_material->hasAppearanceProperty(QStringLiteral("SpecularColor"))) {
        QString color = _material->getAppearanceValueString(QStringLiteral("SpecularColor"));
        _rendered->setSpecularColor(getColorHash(color, 255));
    }
    else {
        _rendered->resetSpecularColor();
    }
    if (_material->hasAppearanceProperty(QStringLiteral("EmissiveColor"))) {
        QString color = _material->getAppearanceValueString(QStringLiteral("EmissiveColor"));
        _rendered->setEmissiveColor(getColorHash(color, 255));
    }
    else {
        _rendered->resetEmissiveColor();
    }
    if (_material->hasAppearanceProperty(QStringLiteral("Shininess"))) {
        double value = _material->getAppearanceValue(QStringLiteral("Shininess")).toDouble();
        _rendered->setShininess(value);
    }
    else {
        _rendered->resetShininess();
    }
    if (_material->hasAppearanceProperty(QStringLiteral("Transparency"))) {
        double value = _material->getAppearanceValue(QStringLiteral("Transparency")).toDouble();
        _rendered->setTransparency(value);
    }
    else {
        _rendered->resetTransparency();
    }

    return true;
}

void MaterialPropertiesWidget::onPropertyChange(const QString& property, const QVariant& value)
{
    if (_material->hasPhysicalProperty(property)) {
        _material->setPhysicalValue(property, value);
    }
    else if (_material->hasAppearanceProperty(property)) {
        _material->setAppearanceValue(property, value);
        updatePreview();
    }
    update();
}

#include "moc_MaterialPropertiesWidget.cpp"
