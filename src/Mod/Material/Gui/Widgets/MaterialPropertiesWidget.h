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

#ifndef MATGUI_MATERIALPROPERTIESWIDGET_H
#define MATGUI_MATERIALPROPERTIESWIDGET_H

#include <QWidget>

#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/Materials.h>

#include <Mod/Material/Gui/AppearancePreview.h>

namespace MatGui
{

class Ui_MaterialPropertiesWidget;

class MaterialPropertiesWidget: public QWidget
{
    Q_OBJECT

public:
    explicit MaterialPropertiesWidget(QWidget* parent = nullptr);
    ~MaterialPropertiesWidget() override;

    void updateMaterial(const std::shared_ptr<Materials::Material>& material);
    void setMaterialSelected(bool selected) {
        _materialSelected = selected;
    }

Q_SIGNALS:
    void addPhysicalProperty();
    void removePhysicalProperty(const QString& propertyName);
    void addAppearanceProperty();
    void removeAppearanceProperty(const QString& propertyName);
    void setName(const QString& name);
    void setAuthor(const QString& author);
    void setLicense(const QString& license);
    void setSourceURL(const QString& url);
    void setSourceReference(const QString& reference);
    void setDescription(const QString& description);

private:
    std::unique_ptr<Ui_MaterialPropertiesWidget> ui;
    std::shared_ptr<Materials::Material> _material;
    AppearancePreview* _rendered;
    bool _materialSelected;

    Materials::MaterialManager& getMaterialManager()
    {
        return Materials::MaterialManager::getManager();
    }
    Materials::MaterialManager& getMaterialManager() const
    {
        return Materials::MaterialManager::getManager();
    }

    static QString libraryPath(const std::shared_ptr<Materials::Material>& material);

    static QString getColorHash(const QString& colorString, int colorRange = 255);

    static void addExpanded(QTreeView* tree, QStandardItemModel* parent, QStandardItem* child);

    void setup();
    void setupButtons();

    void createPhysicalTree();
    void createPreviews();
    void createAppearanceTree();

    void updateMaterial();
    void updateMaterialGeneral();
    void updateMaterialAppearance();
    void updateMaterialProperties();
    void updatePreview() const;
    bool updateTexturePreview() const;
    bool updateMaterialPreview() const;

    void onPropertyChange(const QString& property, const QVariant& value);
    void onPhysicalAdd();
    void onPhysicalRemove();
    void onAppearanceAdd();
    void onAppearanceRemove();

    void onName(const QString& text);
    void onAuthor(const QString& text);
    void onLicense(const QString& text);
    void onSourceURL(const QString& text);
    void onSourceReference(const QString& text);
    void onDescription();

    void onURL(bool checked);
};

}  // namespace MatGui

#endif  // MATGUI_MATERIALPROPERTIESWIDGET_H
