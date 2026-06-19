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

#include <QColorDialog>
#include <QDesktopServices>
#include <QIODevice>
#include <QItemSelectionModel>
#include <QMenu>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QVariant>
#include <limits>


#include <App/Application.h>
#include <App/License.h>
#include <Base/Interpreter.h>
#include <Base/Quantity.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/InputField.h>
#include <Gui/PrefWidgets.h>
#include <Gui/SpinBox.h>
#include <Gui/WaitCursor.h>

#include <Mod/Material/App/Exceptions.h>
#include <Mod/Material/App/MaterialLibrary.h>
#include <Mod/Material/App/ModelManager.h>
#include <Mod/Material/App/ModelUuids.h>

#include "Delegates/MaterialDelegate.h"
#include "Models/MaterialTreeModel.h"
#include "Widgets/MaterialPropertiesWidget.h"
#include "Widgets/PropertiesWidget.h"

#include "MaterialsEditor.h"
#include "ModelSelect.h"
#include "NewLibrary.h"
#include "ui_MaterialsEditor.h"


using namespace MatGui;

/* TRANSLATOR MatGui::MaterialsEditor */

MaterialsEditor::MaterialsEditor(Materials::MaterialFilter filter, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui_MaterialsEditor)
    , _material(std::make_shared<Materials::Material>())
    , _recentMax(0)
    , _filter(filter)
{
    _material->setEditStateNew();
    setup();
}

MaterialsEditor::MaterialsEditor(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui_MaterialsEditor)
    , _material(std::make_shared<Materials::Material>())
    , _recentMax(0)
{
    _material->setEditStateNew();
    setup();
}

void MaterialsEditor::setup()
{
    Gui::WaitCursor wc;
    ui->setupUi(this);
    setupStackedWidgets();

    _warningIcon = QIcon(QStringLiteral(":/icons/Warning.svg"));

    createActions();
    setupData();

    // Reset to previous state
    restoreState();
    setupButtonIcons();
    setupButtonConnections();
    setupEditorCallbacks();
    setupSelectionCallbacks();
    setupContextMenus();
    setupModelCallbacks();
}

void MaterialsEditor::setupStackedWidgets()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void MaterialsEditor::setupData()
{
    getFavorites();
    getRecents();

    createMaterialTree();
    setMaterialDefaults();
}

void MaterialsEditor::restoreState()
{
    // Reset to previous size
    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Editor");
    auto width = param->GetInt("EditorWidth", 835);
    auto height = param->GetInt("EditorHeight", 542);

    resize(width, height);

    auto advanced = param->GetBool("AdvancedSearch", false);
    auto name = param->GetBool("AdvancedSearchName", true);
    auto model = param->GetBool("AdvancedSearchModel", false);
    auto property = param->GetBool("AdvancedSearchProperty", false);
    auto value = param->GetBool("AdvancedSearchValue", false);
    auto tag = param->GetBool("AdvancedSearchTag", false);

    ui->checkAdvancedSearch->setChecked(advanced);
    ui->checkSearchName->setChecked(name);
    ui->checkSearchModel->setChecked(model);
    ui->checkSearchProperty->setChecked(property);
    ui->checkSearchValue->setChecked(value);
    ui->checkSearchTag->setChecked(tag);

    setAdvancedSearchState();
}

void MaterialsEditor::saveState()
{
    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Editor");
    param->SetInt("EditorWidth", width());
    param->SetInt("EditorHeight", height());

    param->SetBool("AdvancedSearch", ui->checkAdvancedSearch->isChecked());
    param->SetBool("AdvancedSearchName", ui->checkSearchName->isChecked());
    param->SetBool("AdvancedSearchModel", ui->checkSearchModel->isChecked());
    param->SetBool("AdvancedSearchProperty", ui->checkSearchProperty->isChecked());
    param->SetBool("AdvancedSearchValue", ui->checkSearchValue->isChecked());
    param->SetBool("AdvancedSearchTag", ui->checkSearchTag->isChecked());

    saveMaterialTree(param);
}

void MaterialsEditor::setupButtonIcons()
{
    // ui->buttonURL->setIcon(QIcon(QStringLiteral(":/icons/internet-web-browser.svg")));

}

void MaterialsEditor::setupButtonConnections()
{
    connect(ui->standardButtons->button(QDialogButtonBox::Ok),
            &QPushButton::clicked,
            this,
            &MaterialsEditor::onOk);
    // connect(ui->standardButtons->button(QDialogButtonBox::Cancel),
    //         &QPushButton::clicked,
    //         this,
    //         &MaterialsEditor::onCancel);
    connect(ui->standardButtons->button(QDialogButtonBox::Save),
            &QPushButton::clicked,
            this,
            &MaterialsEditor::onSave);
}

void MaterialsEditor::setupEditorCallbacks()
{
    connect(
        ui->materialPropertiesWidget,
        &MaterialPropertiesWidget::setName,
        this,
        &MaterialsEditor::onName
    );
    connect(
        ui->materialPropertiesWidget,
        &MaterialPropertiesWidget::setAuthor,
        this,
        &MaterialsEditor::onAuthor
    );
    connect(
        ui->materialPropertiesWidget,
        &MaterialPropertiesWidget::setLicense,
        this,
        &MaterialsEditor::onLicense
    );
    connect(
        ui->materialPropertiesWidget,
        &MaterialPropertiesWidget::setSourceURL,
        this,
        &MaterialsEditor::onSourceURL
    );
    connect(
        ui->materialPropertiesWidget,
        &MaterialPropertiesWidget::setSourceReference,
        this,
        &MaterialsEditor::onSourceReference
    );
    connect(
        ui->materialPropertiesWidget,
        &MaterialPropertiesWidget::setDescription,
        this,
        &MaterialsEditor::onDescription
    );

    connect(
        ui->materialPropertiesWidget,
        &MaterialPropertiesWidget::addPhysicalProperty,
        this,
        &MaterialsEditor::onPhysicalAdd
    );
    connect(
        ui->materialPropertiesWidget,
        &MaterialPropertiesWidget::removePhysicalProperty,
        this,
        &MaterialsEditor::onPhysicalRemove
    );
    connect(
        ui->materialPropertiesWidget,
        &MaterialPropertiesWidget::addAppearanceProperty,
        this,
        &MaterialsEditor::onAppearanceAdd
    );
    connect(
        ui->materialPropertiesWidget,
        &MaterialPropertiesWidget::removeAppearanceProperty,
        this,
        &MaterialsEditor::onAppearanceRemove
    );

    connect(ui->checkAdvancedSearch,
            &QCheckBox::toggled,
            this,
            &MaterialsEditor::onAdvancedSearch);
}

void MaterialsEditor::setupSelectionCallbacks()
{
    QItemSelectionModel* selectionModel = ui->treeMaterials->selectionModel();
    connect(selectionModel,
            &QItemSelectionModel::selectionChanged,
            this,
            &MaterialsEditor::onSelectMaterial);
}

void MaterialsEditor::setupModelCallbacks()
{
    auto tree = ui->treeMaterials;
    auto model = tree->model();
    model->invisibleRootItem()->setFlags(Qt::NoItemFlags);
    connect(model, &MaterialTreeModel::itemChanged, this, &MaterialsEditor::onTreeItemChanged);
    connect(model, &MaterialTreeModel::itemDropped, this, &MaterialsEditor::onTreeItemDropped);
}

void MaterialsEditor::setupContextMenus()
{
    // Context menus
    ui->treeMaterials->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeMaterials,
            &QWidget::customContextMenuRequested,
            this,
            &MaterialsEditor::onContextMenu);
}

void MaterialsEditor::createActions()
{
    _actionNewLibrary.setText(tr("New library"));
    _actionNewLibraryIcon = QIcon(QStringLiteral(":/icons/Material_Library.svg"));
    _actionNewLibrary.setIcon(_actionNewLibraryIcon);
    _actionNewLibrary.setToolTip(tr("New library"));

    _actionDelete.setText(tr("Delete"));
    _actionDeleteIcon = QIcon(QStringLiteral(":/icons/edit-delete.svg"));
    _actionDelete.setIcon(_actionDeleteIcon);
    _actionDelete.setToolTip(tr("Delete the selected item"));
    _actionDelete.setShortcut(Qt::Key_Delete);

    _actionNewFolder.setText(tr("New folder"));
    _actionNewFolderIcon = QIcon(QStringLiteral(":/icons/Group.svg"));
    _actionNewFolder.setIcon(_actionNewFolderIcon);
    _actionNewFolder.setToolTip(tr("New folder"));
    _actionNewFolder.setShortcut(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_N);

    _actionNewMaterial.setText(tr("New material"));
    _actionNewMaterialIcon = QIcon(QStringLiteral(":/icons/Material_Edit.svg"));
    _actionNewMaterial.setIcon(_actionNewMaterialIcon);
    _actionNewMaterial.setToolTip(tr("New material"));
    _actionNewMaterial.setShortcut(Qt::ControlModifier | Qt::Key_N);

    _actionInheritMaterial.setText(tr("New material from selected"));
    _actionInheritMaterialIcon = QIcon(QStringLiteral(":/icons/Material_Inherit.svg"));
    _actionInheritMaterial.setIcon(_actionInheritMaterialIcon);
    _actionInheritMaterial.setToolTip(
        tr("Create a new material based on the currently selected material"));

    _actionFavorite.setText(tr("Add to bookmarks"));
    _actionFavoriteIcon = QIcon(QStringLiteral(":/icons/Material_Favorite.svg"));
    _actionFavorite.setIcon(_actionFavoriteIcon);
    _actionFavorite.setToolTip(tr("Add or remove material from bookmarks list"));

    _actionChangeIcon.setText(tr("Change icon"));

    _actionCut.setText(tr("Cut"));
    _actionCutIcon = QIcon(QStringLiteral(":/icons/edit-cut.svg"));
    _actionCut.setIcon(_actionCutIcon);
    _actionCut.setToolTip(tr("Cut"));
    _actionCut.setShortcut(Qt::ControlModifier | Qt::Key_X);

    _actionCopy.setText(tr("Copy"));
    _actionCopyIcon = QIcon(QStringLiteral(":/icons/edit-copy.svg"));
    _actionCopy.setIcon(_actionCopyIcon);
    _actionCopy.setToolTip(tr("Copy"));
    _actionCopy.setShortcut(Qt::ControlModifier | Qt::Key_C);

    _actionPaste.setText(tr("Paste"));
    _actionPasteIcon = QIcon(QStringLiteral(":/icons/edit-paste.svg"));
    _actionPaste.setIcon(_actionPasteIcon);
    _actionPaste.setToolTip(tr("Paste"));
    _actionPaste.setShortcut(Qt::ControlModifier | Qt::Key_P);

    _actionRename.setText(tr("Rename"));

    _actionEnableDisable.setText(tr("Disable"));
    _actionEnableDisable.setToolTip(tr("Enable or disable a library"));

    _actionLibraryProperties.setText(tr("Properties..."));

    _actionViewFavorites.setText(tr("Bookmarks"));
    _actionViewFavorites.setCheckable(true);
    _actionViewFavorites.setChecked(includeFavorites());
    _actionViewFavorites.setToolTip(tr("Show bookmarked materials"));

    _actionViewRecent.setText(tr("Recent"));
    _actionViewRecent.setCheckable(true);
    _actionViewRecent.setChecked(includeRecent());
    _actionViewRecent.setToolTip(tr("Show recently used materials"));

    _actionViewFolders.setText(tr("Empty folders"));
    _actionViewFolders.setCheckable(true);
    _actionViewFolders.setChecked(includeEmptyFolders());
    _actionViewFolders.setToolTip(tr("Show empty folders"));

    _actionViewLibraries.setText(tr("Empty libraries"));
    _actionViewLibraries.setCheckable(true);
    _actionViewLibraries.setChecked(includeEmptyLibraries());
    _actionViewLibraries.setToolTip(tr("Show empty libraries"));

    _actionViewLegacy.setText(tr("Legacy files"));
    _actionViewLegacy.setCheckable(true);
    _actionViewLegacy.setChecked(includeLegacy());
    _actionViewLegacy.setToolTip(tr("Show materials in the pre-1.0 format"));

    _actionViewDisabled.setText(tr("Disabled libraries"));
    _actionViewDisabled.setCheckable(true);
    _actionViewDisabled.setChecked(includeDisabled());
    _actionViewDisabled.setToolTip(tr("Show disabled libraries"));

    _actionViewMasked.setText(tr("Masked libraries"));
    _actionViewMasked.setCheckable(true);
    _actionViewMasked.setChecked(includeMasked());
    _actionViewMasked.setToolTip(tr("Show local libraries libraries that have been masked by a remote library with the same name"));

    connect(&_actionNewLibrary, &QAction::triggered, this, &MaterialsEditor::onMenuNewLibrary);
    connect(&_actionEnableDisable, &QAction::triggered, this, &MaterialsEditor::onMenuEnableDisable);
    connect(&_actionDelete, &QAction::triggered, this, &MaterialsEditor::onMenuDelete);
    connect(&_actionNewFolder, &QAction::triggered, this, &MaterialsEditor::onMenuNewFolder);
    connect(&_actionNewMaterial, &QAction::triggered, this, &MaterialsEditor::onMenuNewMaterial);
    connect(&_actionInheritMaterial, &QAction::triggered, this, &MaterialsEditor::onMenuInheritMaterial);
    connect(&_actionFavorite, &QAction::triggered, this, &MaterialsEditor::onFavourite);
    connect(&_actionChangeIcon, &QAction::triggered, this, &MaterialsEditor::onMenuChangeIcon);

    connect(&_actionViewFavorites, &QAction::toggled, this, &MaterialsEditor::onMenuViewFavorites);
    connect(&_actionViewRecent, &QAction::toggled, this, &MaterialsEditor::onMenuViewRecent);
    connect(&_actionViewFolders, &QAction::toggled, this, &MaterialsEditor::onMenuViewFolders);
    connect(&_actionViewLibraries, &QAction::toggled, this, &MaterialsEditor::onMenuViewLibraries);
    connect(&_actionViewLegacy, &QAction::toggled, this, &MaterialsEditor::onMenuViewLegacy);
    connect(&_actionViewDisabled, &QAction::toggled, this, &MaterialsEditor::onMenuViewDisabled);
    connect(&_actionViewMasked, &QAction::toggled, this, &MaterialsEditor::onMenuViewMasked);
}

void MaterialsEditor::updateMaterial()
{
    ui->materialPropertiesWidget->updateMaterial(_material);
}

void MaterialsEditor::setMaterialSelected(bool selected)
{
    ui->materialPropertiesWidget->setMaterialSelected(selected);
}

void MaterialsEditor::getFavorites()
{
    _favorites.clear();

    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Favorites");
    int count = param->GetInt("Favorites", 0);
    for (int i = 0; static_cast<long>(i) < count; i++) {
        QString key = QStringLiteral("FAV%1").arg(i);
        QString uuid = QString::fromStdString(param->GetASCII(key.toStdString().c_str(), ""));
        if (_filter.modelIncluded(uuid.toStdString())) {
            _favorites.push_back(uuid);
        }
    }
}

void MaterialsEditor::saveFavorites()
{
    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Favorites");

    // Clear out the existing favorites
    int count = param->GetInt("Favorites", 0);
    for (int i = 0; static_cast<long>(i) < count; i++) {
        QString key = QStringLiteral("FAV%1").arg(i);
        param->RemoveASCII(key.toStdString().c_str());
    }

    // Add the current values
    param->SetInt("Favorites", _favorites.size());
    int j = 0;
    for (auto& favorite : _favorites) {
        QString key = QStringLiteral("FAV%1").arg(j);
        param->SetASCII(key.toStdString().c_str(), favorite.toStdString());

        j++;
    }
}

void MaterialsEditor::addFavorite(const QString& uuid)
{
    // Ensure it is a material. New, unsaved materials will not be
    try {
        auto material = getMaterialManager().getMaterial(uuid.toStdString());
        Q_UNUSED(material)
    }
    catch (const Materials::MaterialNotFound&) {
        return;
    }

    if (!isFavorite(uuid)) {
        _favorites.push_back(uuid);
        saveFavorites();
        refreshMaterialTree();
    }
}

void MaterialsEditor::removeFavorite(const QString& uuid)
{
    if (isFavorite(uuid)) {
        _favorites.remove(uuid);
        saveFavorites();
        refreshMaterialTree();
    }
}

bool MaterialsEditor::isFavorite(const QString& uuid) const
{
    for (auto& it : _favorites) {
        if (it == uuid) {
            return true;
        }
    }
    return false;
}


void MaterialsEditor::getRecents()
{
    _recents.clear();

    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Recent");
    _recentMax = param->GetInt("RecentMax", 5);
    int count = param->GetInt("Recent", 0);
    for (int i = 0; static_cast<long>(i) < count; i++) {
        QString key = QStringLiteral("MRU%1").arg(i);
        QString uuid = QString::fromStdString(param->GetASCII(key.toStdString().c_str(), ""));
        if (_filter.modelIncluded(uuid.toStdString())) {
            _recents.push_back(uuid);
        }
    }
}

void MaterialsEditor::saveRecents()
{
    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Recent");

    // Clear out the existing recents
    int count = param->GetInt("Recent", 0);
    for (int i = 0; static_cast<long>(i) < count; i++) {
        QString key = QStringLiteral("MRU%1").arg(i);
        param->RemoveASCII(key.toStdString().c_str());
    }

    // Add the current values
    int size = _recents.size();
    if (size > _recentMax) {
        size = _recentMax;
    }
    param->SetInt("Recent", size);
    int j = 0;
    for (auto& recent : _recents) {
        QString key = QStringLiteral("MRU%1").arg(j);
        param->SetASCII(key.toStdString().c_str(), recent.toStdString());

        j++;
        if (j >= size) {
            break;
        }
    }
}

void MaterialsEditor::addRecent(const QString& uuid)
{
    // Ensure it is a material. New, unsaved materials will not be
    try {
        auto material = getMaterialManager().getMaterial(uuid.toStdString());
        Q_UNUSED(material)
    }
    catch (const Materials::MaterialNotFound&) {
        return;
    }

    // Ensure no duplicates
    if (isRecent(uuid)) {
        _recents.remove(uuid);
    }

    _recents.push_front(uuid);
    while (_recents.size() > static_cast<std::size_t>(_recentMax)) {
        _recents.pop_back();
    }

    saveRecents();
}

bool MaterialsEditor::isRecent(const QString& uuid) const
{
    for (auto& it : _recents) {
        if (it == uuid) {
            return true;
        }
    }
    return false;
}

void MaterialsEditor::onTreeItemChanged(QStandardItem* item)
{
    auto materialItem = static_cast<MaterialTreeItem*>(item);
    switch (materialItem->getItemFunction()) {
        case TreeFunctionLibrary:
            renameLibrary(materialItem);
            break;

        case TreeFunctionFolder:
            renameFolder(materialItem);
            break;

        case TreeFunctionMaterial:
            renameMaterial(materialItem);
            break;

        default:
            break;
    }
}

void MaterialsEditor::onTreeItemDropped(
    Qt::DropAction action,
    QStandardItem* source,
    QStandardItem* destination
)
{
    Base::Console().log("onTreeItemDropped(%s, %s)\n", source->text().toStdString().c_str(), destination->text().toStdString().c_str());
    auto destinationItem = static_cast<MaterialTreeItem*>(destination);
    auto sourceItem = static_cast<MaterialTreeItem*>(source);

    if (sourceItem->getItemFunction() == TreeFunctionFolder) {
        folderDropped(action, sourceItem, destinationItem);
    }
    else if (sourceItem->getItemFunction() == TreeFunctionMaterial) {
        materialDropped(action, sourceItem, destinationItem);
    }
}

void MaterialsEditor::folderDropped(
    Qt::DropAction action,
    MaterialTreeItem* source,
    MaterialTreeItem* destination
)
{
    auto sourceLibraryName = source->libraryName();
    auto sourceLibrary = getMaterialManager().getLibrary(sourceLibraryName.toStdString());
    auto sourceFolder = source->path(); //getDirectoryForItem(source);

    auto destinationItem = destination;
    if (destinationItem->getItemFunction() == TreeFunctionMaterial) {
        destinationItem = destinationItem->parent();
    }
    auto destinationLibrary = getLibraryForItem(destinationItem);
    auto destinationFolder = getDirectoryForItem(destinationItem);

    Base::Console().log(
        "Source: [%s]%s\n",
        sourceLibraryName.toStdString().c_str(),
        sourceFolder.toStdString().c_str()
    );
    Base::Console().log(
        "Destination: [%s]%s\n",
        destinationLibrary->getName().c_str(),
        destinationFolder.toStdString().c_str()
    );

    getMaterialManager().moveFolder(
        sourceLibrary,
        sourceFolder.toStdString(),
        destinationLibrary,
        destinationFolder.toStdString()
    );
    addExpanded(ui->treeMaterials, destination, source);
    source->setPath();
}

void MaterialsEditor::materialDropped(
    Qt::DropAction action,
    MaterialTreeItem* source,
    MaterialTreeItem* destination
)
{
    auto material = getItemAsMaterial(source);
    auto library = material->getLibrary();
    auto destinationItem = destination;
    if (destinationItem->getItemFunction() == TreeFunctionMaterial) {
        destinationItem = destinationItem->parent();
    }
    auto destinationLibrary = getLibraryForItem(destinationItem);
    auto destinationFolder = getDirectoryForItem(destinationItem);

    Base::Console().log(
        "Library '%s' -> '%s'\n",
        library->getName().c_str(),
        destinationLibrary->getName().c_str()
    );
    Base::Console().log(
        "Path '%s' -> '%s'\n",
        material->getDirectory().c_str(),
        destinationFolder.toStdString().c_str()
    );
    Base::Console().log("Drop Action\n");
    if (action & Qt::CopyAction) {
        Base::Console().log("\t Copy\n");
    }
    if (action & Qt::MoveAction) {
        Base::Console().log("\t Move\n");
    }
    if (action & Qt::LinkAction) {
        Base::Console().log("\t Link\n");
    }
    if (action == Qt::IgnoreAction) {
        Base::Console().log("\t Ignore\n");
    }
    if (action == Qt::TargetMoveAction) {
        Base::Console().log("\t Target move\n");
    }

    if ((action & Qt::MoveAction) || (*library == *destinationLibrary)) {
        Base::Console().log("Move\n");
        getMaterialManager()
            .move(destinationLibrary, destinationFolder.toStdString(), material);
        refreshMaterialTree();
    }
}

void MaterialsEditor::onName(const QString& name)
{
    Base::Console().log("onName(%s)\n", name.toStdString().c_str());
    updateMaterialTreeName(name);
    updateFavoritesName();
    updateRecentsName();
}

void MaterialsEditor::onAuthor(const QString& author)
{
    _material->setAuthor(author.toStdString());
}

void MaterialsEditor::onLicense(const QString& license)
{
    _material->setLicense(license.toStdString());
}

void MaterialsEditor::onSourceURL(const QString& url)
{
    _material->setURL(url.toStdString());
}

void MaterialsEditor::onSourceReference(const QString& reference)
{
    _material->setReference(reference.toStdString());
}

void MaterialsEditor::onDescription(const QString& description)
{
    _material->setDescription(description.toStdString());
}

void MaterialsEditor::onPhysicalAdd()
{
    ModelSelect dialog(this, Materials::ModelFilter_Physical);
    dialog.setModal(true);
    if (dialog.exec() == QDialog::Accepted) {
        QString selected = dialog.selectedModel();
        _material->addPhysical(selected.toStdString());
        updateMaterial();
    }
    else {
        Base::Console().log("No model selected\n");
    }
}

void MaterialsEditor::onPhysicalRemove(const QString& propertyName)
{
    auto uuid = _material->getModelByName(propertyName.toStdString());
    _material->removePhysical(uuid);
    updateMaterial();
}

void MaterialsEditor::onAppearanceAdd()
{
    ModelSelect dialog(this, Materials::ModelFilter_Appearance);
    dialog.setModal(true);
    if (dialog.exec() == QDialog::Accepted) {
        QString selected = dialog.selectedModel();
        _material->addAppearance(selected.toStdString());
        auto model = getModelManager().getModel(selected.toStdString());
        if (selected == Materials::ModelUUIDs::ModelUUID_Rendering_Basic
            || model->inherits(Materials::ModelUUIDs::ModelUUID_Rendering_Basic)) {
            // Add default appearance properties
            *_material = *(getMaterialManager().defaultAppearance());
        }

        updateMaterial();
    }
    else {
        Base::Console().log("No model selected\n");
    }
}

void MaterialsEditor::onAppearanceRemove(const QString& propertyName)
{
    auto uuid = _material->getModelByName(propertyName.toStdString());
    _material->removeAppearance(uuid);
    updateMaterial();
}

void MaterialsEditor::onFavourite(bool checked)
{
    Q_UNUSED(checked)

    auto selected = QString::fromStdString(_material->getUUID());
    if (isFavorite(selected)) {
        removeFavorite(selected);
    }
    else {
        addFavorite(selected);
    }
}

void MaterialsEditor::onAdvancedSearch(bool checked)
{
    setAdvancedSearchState(checked);
}

void MaterialsEditor::setAdvancedSearchState(bool checked)
{
    ui->groupAdvancedSearch->setVisible(checked);
}

void MaterialsEditor::setAdvancedSearchState()
{
    setAdvancedSearchState(ui->checkAdvancedSearch->isChecked());
}

void MaterialsEditor::setLibraryPropertyState()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void MaterialsEditor::setFolderPropertyState()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void MaterialsEditor::setMaterialPropertyState()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void MaterialsEditor::setMaterialDefaults()
{
    _material->setName(tr("Unnamed").toStdString());
    std::string Author = App::GetApplication()
                             .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document")
                             ->GetASCII("prefAuthor", "");
    _material->setAuthor(Author);

    // license stuff
    auto paramGrp {App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Document")};
    auto index = static_cast<int>(paramGrp->GetInt("prefLicenseType", 0));
    const char* name = App::licenseItems.at(index).at(App::posnOfFullName);
    // const char* url = App::licenseItems.at(index).at(App::posnOfUrl);
    // std::string licenseUrl = (paramGrp->GetASCII("prefLicenseUrl", url));
    _material->setLicense(name);

    // Empty materials will have no parent
    getMaterialManager().dereference(_material);

    updateMaterial();
    _material->resetEditState();
}

void MaterialsEditor::onOk(bool checked)
{
    Q_UNUSED(checked)

    // Ensure data is saved (or discarded) before exiting
    if (_material->getEditState() != Materials::Material::MaterialEdit_None) {
        // Prompt the user to save or discard changes
        int res = confirmSave(this);
        if (res == QMessageBox::Cancel) {
            return;
        }
        else if (res == QMessageBox::Discard) {
            discardIfNew();
        }
    }

    accept();
}

// void MaterialsEditor::onCancel(bool checked)
// {
//     Q_UNUSED(checked)

//     reject();
// }

void MaterialsEditor::onSave(bool checked)
{
    Q_UNUSED(checked)

    saveMaterial();
}

MaterialSaveResult MaterialsEditor::overwriteOrCopy()
{
    QMessageBox box(this);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(tr("Confirm Overwrite"));

    QString prompt = tr("Overwrite material");
    box.setText(prompt);

    box.setInformativeText(
        tr("Material changes can break existing documents. "
           "It is recommended to save as a new material.")
    );

    QCheckBox* setAsParent = new QCheckBox(tr("Set as parent"));
    setAsParent->setChecked(false);
    box.setCheckBox(setAsParent);

    QPushButton* newButton = box.addButton(tr("Save As New"), QMessageBox::AcceptRole);
    QPushButton* overwiteButton = box.addButton(tr("Save"), QMessageBox::ActionRole);
    QPushButton* cancelButton = box.addButton(QMessageBox::Cancel);

    box.setDefaultButton(cancelButton);
    box.setEscapeButton(cancelButton);

    box.adjustSize();  // Silence warnings from Qt on Windows
    box.exec();

    MaterialSaveResult res = MaterialSave_Cancel;
    if (box.clickedButton() == overwiteButton) {
        res = MaterialSave_Overwrite;
    }
    else if (box.clickedButton() == newButton) {
        if (box.checkBox()->isChecked()) {
            res = MaterialSave_Inherit;
        }
        else {
            res = MaterialSave_New;
        }
    }

    return res;
}

void MaterialsEditor::saveMaterial()
{
    Base::Console().log("Material path %s\n", _material->getDirectory().c_str());
    bool overwrite = true;
    bool saveAsCopy = false;
    bool saveInherited = true;
    if (_material->getEditState() != Materials::Material::MaterialEdit_None) {
        if (_material->getEditState() == Materials::Material::MaterialEdit_InvariantChanged) {
            MaterialSaveResult ret = overwriteOrCopy();
            if (ret == MaterialSave_Cancel) {
                return;
            }
            else if (ret == MaterialSave_New || ret == MaterialSave_Inherit) {
                // Save as new material
                auto item = getItemFromMaterial(*_material);
                if (!item) {
                    Base::Console().log("Material not found in tree\n");
                    return;
                }

                auto uniqueName = item->parent()->getUniqueName(
                    QString::fromStdString(_material->getName()),
                    TreeFunctionMaterial
                );
                if (ret == MaterialSave_New) {
                    _material = getMaterialManager().copyNew(*_material, uniqueName.toStdString());
                }
                else {
                    _material
                        = getMaterialManager().copyInherited(*_material, uniqueName.toStdString());
                }
                updateMaterial();

                // We then fall through and save with the new name
                overwrite = false;
            }
        }

        auto library = _material->getLibrary();
        QFileInfo filepath(
            QString::fromStdString(_material->getDirectory()) + QStringLiteral("/")
            + QString::fromStdString(_material->getName()) + QStringLiteral(".FCMat")
        );
        if (!library || library->isReadOnly()) {
            Base::Console().log("No library assigned\n");
            library = getMaterialManager().getLibrary("User");
            filepath = QFileInfo(QString::fromStdString(_material->getName()) + QStringLiteral(".FCMat"));
        }
        Base::Console().log("Using library '%s'\n", library->getName().c_str());
        Base::Console().log("\tPath '%s'\n", filepath.filePath().toStdString().c_str());
        getMaterialManager().saveMaterial(
            _material->getLibrary(),
            _material,
            filepath.filePath().toStdString(),
            overwrite,
            saveAsCopy,
            saveInherited
        );
    }
    else {
        Base::Console().log("Nothing to save\n");
    }
    // refreshMaterialTree();
}

void MaterialsEditor::accept()
{
    if (_material->isOldFormat()) {
        Base::Console().log("*** Old format file ***\n");
        oldFormatError();

        return;
    }
    addRecent(QString::fromStdString(_material->getUUID()));
    saveState();
    QDialog::accept();
}

void MaterialsEditor::oldFormatError()
{
    QMessageBox box(this);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(tr("Old Format Material"));

    box.setText(tr("This file is in the old material card format."));
    box.setInformativeText(QObject::tr("Save the material before using it."));
    box.adjustSize();  // Silence warnings from Qt on Windows
    box.exec();
}

void MaterialsEditor::reject()
{
    saveState();
    QDialog::reject();
}

void MaterialsEditor::saveMaterialTreeChildren(const Base::Reference<ParameterGrp>& param,
                                               MaterialTreeView* tree,
                                               MaterialTreeModel* model,
                                               MaterialTreeItem* item)
{
    if (item->hasChildren()) {
        auto text = item->originalName();
        if (text.isEmpty()) {
            text = item->text();
        }
        param->SetBool(text.toStdString().c_str(), tree->isExpanded(item->index()));

        auto treeParam = param->GetGroup(text.toStdString().c_str());
        for (int i = 0; i < item->rowCount(); i++) {
            auto child = item->child(i);

            saveMaterialTreeChildren(treeParam, tree, model, child);
        }
    }
}

void MaterialsEditor::saveMaterialTree(const Base::Reference<ParameterGrp>& param)
{
    auto treeParam = param->GetGroup("MaterialTree");
    treeParam->Clear();

    auto tree = ui->treeMaterials;
    auto model = tree->model();

    auto root = model->invisibleRootItem();
    for (int i = 0; i < root->rowCount(); i++) {
        auto child = static_cast<MaterialTreeItem*>(root->child(i));

        saveMaterialTreeChildren(treeParam, tree, model, child);
    }
}

void MaterialsEditor::addMaterials(
    MaterialTreeItem& parent,
    const std::shared_ptr<std::map<std::string, std::shared_ptr<Materials::MaterialTreeNode>>>
        materialTree,
    const QIcon& folderIcon,
    const QIcon& icon,
    const Base::Reference<ParameterGrp>& param)
{
    auto childParam = param->GetGroup(parent.text().toStdString().c_str());
    auto tree = ui->treeMaterials;
    for (auto& mat : *materialTree) {
        std::shared_ptr<Materials::MaterialTreeNode> nodePtr = mat.second;
        Qt::ItemFlags flags = (Qt::ItemIsEnabled);
        if (!nodePtr->isReadOnly()) {
            flags |= (Qt::ItemIsEditable | Qt::ItemIsDropEnabled);
        }
        if (nodePtr->getType() == Materials::MaterialTreeNode::NodeType::DataNode) {
            QString uuid = QString::fromStdString(nodePtr->getUUID());

            QIcon matIcon = icon;
            if (nodePtr->isOldFormat()) {
                matIcon = _warningIcon;
            }
            auto card = new MaterialTreeMaterialItem(matIcon, QString::fromStdString(mat.first), uuid);
            card->setFlags(flags | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
            if (nodePtr->isOldFormat()) {
                card->setToolTip(tr("This card uses the old format and must be saved before use"));
            }
            card->setLibraryName(parent.libraryName());
            card->setPath(parent.path() + QStringLiteral("/") + QString::fromStdString(mat.first));

            addExpanded(tree, &parent, card);
        }
        else {
            auto node = new MaterialTreeFolderItem(folderIcon, QString::fromStdString(mat.first));
            node->setFlags(flags | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
            node->setLibraryName(parent.libraryName());
            node->setPath(parent.path() + QStringLiteral("/") + QString::fromStdString(mat.first));
            auto treeMap = nodePtr->getFolder();

            addExpanded(tree, &parent, node, childParam);
            addMaterials(*node, treeMap, folderIcon, icon, childParam);
        }
    }
}

void MaterialsEditor::addExpanded(MaterialTreeView* tree, MaterialTreeItem* parent, MaterialTreeItem* child)
{
    parent->appendRow(child);
    tree->setExpanded(child->index(), true);
}

void MaterialsEditor::addExpanded(MaterialTreeView* tree,
                                  MaterialTreeItem* parent,
                                  MaterialTreeItem* child,
                                  const Base::Reference<ParameterGrp>& param)
{
    parent->appendRow(child);

    // Restore to any previous expansion state
    auto text = child->originalName();
    auto expand = param->GetBool(text.toStdString().c_str(), true);
    tree->setExpanded(child->index(), expand);
}

void MaterialsEditor::addExpanded(MaterialTreeView* tree, MaterialTreeModel* parent, MaterialTreeItem* child)
{
    parent->appendRow(child);
    tree->setExpanded(child->index(), true);
}

void MaterialsEditor::addExpanded(MaterialTreeView* tree,
                                  MaterialTreeModel* parent,
                                  MaterialTreeItem* child,
                                  const Base::Reference<ParameterGrp>& param)
{
    parent->appendRow(child);

    // Restore to any previous expansion state
    auto text = child->originalName();
    auto expand = param->GetBool(text.toStdString().c_str(), true);
    tree->setExpanded(child->index(), expand);
}

void MaterialsEditor::removeItem(MaterialTreeItem* parent, MaterialTreeItem* child)
{
    for (int row = 0; row < parent->rowCount(); row++) {
        if (*parent->child(row) == *child) {
            parent->removeRow(row);
            return;
        }
    }
}

QIcon MaterialsEditor::getIcon(const std::shared_ptr<Materials::Library>& library)
{
    // Load from the QByteArray if available
    QIcon icon;
    if (library->hasIcon()) {
        QImage image;
        if (!image.loadFromData(library->getIcon())) {
            Base::Console().log("Unable to load icon image for library '%s'\n",
                                library->getName().c_str());
            return QIcon();
        }
        icon = QIcon(QPixmap::fromImage(image));
    }

    return icon;
}

QIcon MaterialsEditor::getIcon(const std::shared_ptr<Materials::ModelLibrary>& library)
{
    return getIcon(std::static_pointer_cast<Materials::Library>(library));
}

QIcon MaterialsEditor::getIcon(const std::shared_ptr<Materials::MaterialLibrary>& library)
{
    return getIcon(std::static_pointer_cast<Materials::Library>(library));
}

void MaterialsEditor::addRecents(MaterialTreeItem* parent)
{
    auto tree = ui->treeMaterials;
    for (auto& uuid : _recents) {
        try {
            auto material = getMaterialManager().getMaterial(uuid.toStdString());
            QIcon icon = getIcon(material->getLibrary());
            auto card = new MaterialTreeFavoriteItem(
                icon,
                QString::fromStdString(material->getLibraryPath()),
                uuid
            );

            addExpanded(tree, parent, card);
        }
        catch (const Materials::MaterialNotFound&) {
        }
    }
}

void MaterialsEditor::addFavorites(MaterialTreeItem* parent)
{
    auto tree = ui->treeMaterials;
    for (auto& uuid : _favorites) {
        try {
            auto material = getMaterialManager().getMaterial(uuid.toStdString());
            QIcon icon = getIcon(material->getLibrary());
            auto card = new MaterialTreeFavoriteItem(
                icon,
                QString::fromStdString(material->getLibraryPath()),
                uuid
            );

            addExpanded(tree, parent, card);
        }
        catch (const Materials::MaterialNotFound&) {
        }
    }
}

void MaterialsEditor::fillMaterialTree()
{
    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Editor/MaterialTree");

    auto tree = ui->treeMaterials;
    auto model = tree->model();

    if (_filterOptions.includeFavorites()) {
        auto lib = new MaterialTreeFavoriteItem(tr("Bookmarks"));
        lib->setFlags(Qt::ItemIsEnabled);
        addExpanded(tree, model, lib, param);
        addFavorites(lib);
    }

    if (_filterOptions.includeRecent()) {
        auto lib = new MaterialTreeRecentItem(tr("Recent"));
        lib->setFlags(Qt::ItemIsEnabled);
        addExpanded(tree, model, lib, param);
        addRecents(lib);
    }

    auto libraries = getMaterialManager().getLibraries(includeDisabled(), includeMasked());
    for (const auto& library : *libraries) {
        auto materialTree = getMaterialManager().getMaterialTree(*library);

        bool showLibraries = _filterOptions.includeEmptyLibraries();
        if (!_filterOptions.includeEmptyLibraries() && materialTree->size() > 0) {
            showLibraries = true;
        }

        if (showLibraries) {
            QString title = QString::fromStdString(library->getName());
            auto lib = new MaterialTreeLibraryItem(title);
            if (!library->isLocal()) {
                QIcon icon(QStringLiteral(":/icons/Material_Remote.svg"));
                lib->setIcon(icon);
            }
            if (library->isDisabled()) {
                auto font = lib->font();
                font.setStrikeOut(true);
                lib->setFont(font);
            }
            if (library->isReadOnly()) {
                lib->setFlags(Qt::ItemIsEnabled);
            }
            else {
                lib->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);
            }
            lib->setLibrary(library);
            lib->setLibraryName(library->getName());
            addExpanded(tree, model, lib, param);

            QIcon icon = getIcon(library);
            QIcon folderIcon(QStringLiteral(":/icons/folder.svg"));

            if (!library->isDisabled()) {
                addMaterials(*lib, materialTree, folderIcon, icon, param);
            }
        }
    }
}

void MaterialsEditor::createMaterialTree()
{
    auto tree = ui->treeMaterials;
    auto model = new MaterialTreeModel();
    tree->setModel(model);

    tree->setHeaderHidden(true);
    auto toolbar = ui->treeToolBar;
    toolbar->addAction(&_actionNewMaterial);
    toolbar->addAction(&_actionInheritMaterial);
    toolbar->addAction(&_actionNewLibrary);
    toolbar->addAction(&_actionNewFolder);
    toolbar->addSeparator();
    toolbar->addAction(&_actionCut);
    toolbar->addAction(&_actionCopy);
    toolbar->addAction(&_actionPaste);
    toolbar->addSeparator();
    toolbar->addAction(&_actionFavorite);
    ui->frameLayout->insertWidget(0, toolbar);
    fillMaterialTree();
}

void MaterialsEditor::refreshMaterialTree()
{
    Gui::WaitCursor wc;
    auto tree = ui->treeMaterials;
    auto model = tree->model();
    model->clear();

    fillMaterialTree();
}

void MaterialsEditor::onSelectMaterial(const QItemSelection& selected,
                                       const QItemSelection& deselected)
{
    Q_UNUSED(deselected);

    // Get the UUID before changing the underlying data model
    QString uuid;
    auto model = ui->treeMaterials->model();
    QModelIndexList indexes = selected.indexes();
    for (auto it = indexes.begin(); it != indexes.end(); it++) {
        MaterialTreeItem* item = static_cast<MaterialTreeItem*>(model->itemFromIndex(*it));

        if (item) {
            auto fun = item->getItemFunction();
            switch (fun) {
                case TreeFunctionLibrary:
                    setLibraryPropertyState();
                    return;

                case TreeFunctionFolder:
                    setFolderPropertyState();
                    return;

                case TreeFunctionFavorites:
                case TreeFunctionRecents:
                case TreeFunctionMaterial:
                    {
                        auto materialItem = static_cast<MaterialTreeMaterialItem*>(item);
                        uuid = materialItem->getUUID();
                    }
                    break;
            }
            break;
        }
    }
    setMaterialPropertyState();

    if (uuid.isEmpty() || uuid != _material->getUUID()) {
        // Ensure data is saved (or discarded) before changing materials
        if (_material->getEditState() != Materials::Material::MaterialEdit_None) {
            // Prompt the user to save or discard changes
            int res = confirmSave(this);
            if (res == QMessageBox::Cancel) {
                return;
            }
            else if (res == QMessageBox::Discard) {
                discardIfNew();
            }
        }
    }

    if (uuid.isEmpty()) {
        // Clear selection
        setMaterialSelected(false);
        updateMaterial();
        _material->resetEditState();
        return;
    }

    // Get the selected material
    try {
        if (!_material || _material->getUUID() != uuid) {
            _material = std::make_shared<Materials::Material>(
                *getMaterialManager().getMaterial(uuid.toStdString())
            );
            setMaterialSelected(true);
            updateMaterial();
            _material->resetEditState();
        }
        // else don't reset edit state
    }
    catch (Materials::ModelNotFound const&) {
        Base::Console().log("*** Unable to load material '%s'\n", uuid.toStdString().c_str());
        _material = std::make_shared<Materials::Material>();
        // _material->setEditStateNew();
        setMaterialSelected(true);
        updateMaterial();
        _material->resetEditState();
    }
}

bool MaterialsEditor::actionHasContext() const
{
    if (!_actionIndex.isValid()) {
        Base::Console().log("Not in a context menu!!\n");
    }
    return _actionIndex.isValid();
}

const MaterialTreeModel* MaterialsEditor::getActionModel() const
{
    return qobject_cast<const MaterialTreeModel*>(_actionIndex.model());
}

MaterialTreeItem* MaterialsEditor::getActionItem() const
{
    auto model = ui->treeMaterials->model();
    if (model) {
        return static_cast<MaterialTreeItem*>(model->itemFromIndex(_actionIndex));
    }
    return nullptr;
}

TreeFunctionType MaterialsEditor::getActionFunction() const
{
    auto item = getActionItem();
    if (item) {
        return item->getItemFunction();
    }
    throw ActionError();
}

std::shared_ptr<Materials::MaterialLibrary> MaterialsEditor::getItemAsLibrary(
    const MaterialTreeItem* item
) const
{
    if (item && item->getItemFunction() == TreeFunctionLibrary) {
        auto libraryItem = static_cast<const MaterialTreeLibraryItem*>(item);
        return libraryItem->getLibrary();
    }
    throw ActionError();
}

std::shared_ptr<Materials::MaterialLibrary> MaterialsEditor::getActionLibrary() const
{
    return getItemAsLibrary(getActionItem());
}

std::shared_ptr<Materials::Material> MaterialsEditor::getItemAsMaterial(const MaterialTreeItem* item) const
{
    if (item && item->getItemFunction() == TreeFunctionMaterial) {
        auto material = static_cast<const MaterialTreeMaterialItem*>(item);
        auto uuid = material->getUUID();
        if (uuid == _material->getUUID()) {
            // A new material won't be managed by the MaterialManager yet
            return _material;
        }
        return getMaterialManager().getMaterial(uuid.toStdString());
    }
    throw ActionError();
}

std::shared_ptr<Materials::Material> MaterialsEditor::getActionMaterial() const
{
    return getItemAsMaterial(getActionItem());
}

MaterialTreeItem* MaterialsEditor::getItemFromRoot(TreeFunctionType function) const
{
    auto model = ui->treeMaterials->model();
    if (model) {
        auto root = model->invisibleRootItem();
        for (auto row = 0; row < root->rowCount(); row++) {
            auto item = static_cast<MaterialTreeItem*>(root->child(row));
            if (item) {
                if (item->getItemFunction() == function) {
                    return item;
                }
            }
        }
    }
    return nullptr;
}

MaterialTreeItem* MaterialsEditor::getFavoritesItem() const
{
    return getItemFromRoot(TreeFunctionFavorites);
}

MaterialTreeItem* MaterialsEditor::getRecentsItem() const
{
    return getItemFromRoot(TreeFunctionRecents);
}

MaterialTreeItem* MaterialsEditor::getItemFromLibrary(const Materials::Library& library) const
{
    auto model = ui->treeMaterials->model();
    if (model) {
        auto root = model->invisibleRootItem();
        for (auto row = 0; row < root->rowCount(); row++) {
            auto item = static_cast<MaterialTreeItem*>(root->child(row));
            if (item->getItemFunction() == TreeFunctionLibrary) {
                auto treeLibrary = static_cast<MaterialTreeLibraryItem*>(item)->getLibrary();
                if (*treeLibrary == library) {
                    Base::Console().log(
                        "Found library '%s'\n",
                        treeLibrary->getName().c_str()
                    );
                    return item;
                }
            }
        }
    }
    return nullptr;
}

MaterialTreeItem* MaterialsEditor::getItemFromMaterial(const Materials::Material& material) const
{
    if (!material.getLibrary()) {
        // A new dummy material won't have a library
        return nullptr;
    }
    auto libraryItem = getItemFromLibrary(*material.getLibrary());
    if (libraryItem) {
        MaterialTreeItem* folderItem = libraryItem;
        auto directory = QString::fromStdString(material.getDirectory());
        if (!(directory.isEmpty() || directory == QStringLiteral("/"))) {
            Base::Console().log("Checking directory '%s'\n", directory.toStdString().c_str());
            auto path = directory.split(QStringLiteral("/"));
            for (auto folder : path)
            {
                int row = 0;
                auto item = folderItem->child(row);
                while (item) {
                    if (item->getItemFunction() == TreeFunctionFolder) {
                        auto folderName = item->originalName();
                        if (folderName == folder.toStdString()) {
                            Base::Console().log(
                                "Folder '%s'\n",
                                folderName.toStdString().c_str()
                            );
                            folderItem = item;
                            break;
                        }
                    }
                    row++;
                    item = folderItem->child(row);
                }
            }
        }
        Base::Console().log(
            "Folder '%s'\n",
            folderItem->originalName().toStdString().c_str()
        );
        int row = 0;
        auto item = folderItem->child(row);
        while (item) {
            if (item->getItemFunction() == TreeFunctionMaterial) {
                auto materialItem = static_cast<MaterialTreeMaterialItem*>(item);
                auto uuid = materialItem->getUUID().toStdString();
                if (uuid == material.getUUID()) {
                    Base::Console().log(
                        "Material '%s'\n",
                        material.getName().c_str()
                    );
                    return materialItem;
                }
            }
            row++;
            item = folderItem->child(row);
        }
    }
    return nullptr;
}

std::shared_ptr<Materials::MaterialLibrary> MaterialsEditor::getLibraryForItem(
    const MaterialTreeItem* item
) const
{
    if (item->getItemFunction() == TreeFunctionLibrary) {
        return getItemAsLibrary(item);
    }

    auto parent = item->parent();
    while (parent) {
        if (parent->getItemFunction() == TreeFunctionLibrary) {
            return getItemAsLibrary(parent);
        }
        parent = parent->parent();
    }

    // Not found
    return nullptr;
}

QString MaterialsEditor::getDirectoryForItem(
    const MaterialTreeItem* item
) const
{
    if (item) {
        auto function = item->getItemFunction();
        if (function == TreeFunctionMaterial) {
            return getDirectoryForItem(item->parent());
        }
        if (function == TreeFunctionFolder) {
            return getDirectoryForItem(item->parent()) + QStringLiteral("/") + item->text();
        }
    }
    return QString();
}

bool MaterialsEditor::isAncestor(const MaterialTreeItem* item, const Materials::Material& material) const
{
    auto materialItem = getItemFromMaterial(material);
    if (materialItem) {
        return isAncestor(item, materialItem);
    }
    return false;
}

bool MaterialsEditor::isAncestor(const MaterialTreeItem* item, const MaterialTreeItem* child) const
{
    if (*getLibraryForItem(item) == *getLibraryForItem(child)) {
        auto itemPath = getDirectoryForItem(item);
        auto childPath = getDirectoryForItem(child);

        return childPath.startsWith(itemPath);
    }
    return false;
}

void MaterialsEditor::onContextMenu(const QPoint& pos)
{
    _actionIndex = ui->treeMaterials->indexAt(pos);

    QMenu contextMenu(tr("Context menu"), this);

    // The menu gets customized depending on where in the tree the mouse action is performed
    try {
        auto function = getActionFunction();
        switch (function) {
            case TreeFunctionFavorites:
                favoriteContextMenu(contextMenu);
                break;

            case TreeFunctionRecents:
                recentContextMenu(contextMenu);
                break;

            case TreeFunctionLibrary:
                libraryContextMenu(contextMenu);
                break;

            case TreeFunctionFolder:
                folderContextMenu(contextMenu);
                break;

            case TreeFunctionMaterial:
                materialContextMenu(contextMenu);
                break;
        }
    }
    catch (ActionError e) {
        defaultContextMenu(contextMenu);
    }

    contextMenu.exec(ui->treeMaterials->mapToGlobal(pos));

    // The action is complete, or the menu was deselected
    resetActionContext();
}

void MaterialsEditor::resetActionContext()
{
    _actionIndex = QModelIndex(); // An invalid index
    _actionNewMaterial.setEnabled(true);
    _actionChangeIcon.setEnabled(true);
    _actionNewFolder.setEnabled(true);
    _actionCut.setEnabled(true);
    _actionPaste.setEnabled(true);
    _actionRename.setEnabled(true);
    _actionDelete.setEnabled(true);
}

void MaterialsEditor::favoriteActionAdd()
{
    _actionFavorite.setText(tr("Add to bookmarks"));
}

void MaterialsEditor::favoriteActionRemove()
{
    _actionFavorite.setText(tr("Remove from bookmarks"));
}

void MaterialsEditor::favoriteContextMenu(QMenu& contextMenu)
{
    Q_UNUSED(contextMenu);

    contextMenu.addAction(&_actionInheritMaterial);
    contextMenu.addAction(&_actionNewLibrary);
    contextMenu.addSeparator();

    auto item = getActionItem();
    if (item->text() != tr("Bookmarks")) {
        favoriteActionRemove();
        contextMenu.addAction(&_actionFavorite);
    }

    addViewMenu(contextMenu);
}

void MaterialsEditor::recentContextMenu(QMenu& contextMenu)
{
    contextMenu.addAction(&_actionInheritMaterial);
    contextMenu.addAction(&_actionNewLibrary);
    contextMenu.addSeparator();
    auto item = getActionItem();
    if (item->text() != tr("Recent")) {
        auto selected = QString::fromStdString(_material->getUUID());
        if (isFavorite(selected)) {
            favoriteActionRemove();
        }
        else {
            favoriteActionAdd();
        }
        contextMenu.addAction(&_actionFavorite);
    }

    addViewMenu(contextMenu);
}

void MaterialsEditor::libraryContextMenu(QMenu& contextMenu)
{
    auto library = getActionLibrary();
    bool enabled = !library->isReadOnly();

    _actionNewMaterial.setEnabled(enabled);
    _actionChangeIcon.setEnabled(enabled);
    _actionNewFolder.setEnabled(enabled);
    _actionCut.setEnabled(enabled);
    _actionPaste.setEnabled(enabled);
    if (library->isLocal() && library->getName() == QStringLiteral("User")) {
        // We can't delete the user library
        _actionDelete.setEnabled(false);
    }
    else {
        _actionDelete.setEnabled(enabled);
    }

    contextMenu.addAction(&_actionNewMaterial);
    contextMenu.addAction(&_actionNewLibrary);
    contextMenu.addAction(&_actionChangeIcon);
    contextMenu.addSeparator();
    contextMenu.addAction(&_actionNewFolder);
    contextMenu.addSeparator();
    if (library->isDisabled()) {
        _actionEnableDisable.setText(tr("Enable"));
    }
    else {
        _actionEnableDisable.setText(tr("Disable"));
    }
    contextMenu.addAction(&_actionEnableDisable);
    contextMenu.addAction(&_actionDelete);
    contextMenu.addAction(&_actionLibraryProperties);

    addViewMenu(contextMenu);
}

void MaterialsEditor::folderContextMenu(QMenu& contextMenu)
{
    auto item = getActionItem();
    auto path = getPath(item, QString());
    auto libraryName = getLibraryName(item);
    auto library = getMaterialManager().getLibrary(libraryName.toStdString());
    bool enabled = !library->isReadOnly();

    _actionNewMaterial.setEnabled(enabled);
    _actionNewFolder.setEnabled(enabled);
    _actionCut.setEnabled(enabled);
    _actionPaste.setEnabled(enabled);
    _actionRename.setEnabled(enabled);
    _actionDelete.setEnabled(enabled);

    contextMenu.addAction(&_actionNewMaterial);
    contextMenu.addAction(&_actionNewLibrary);
    contextMenu.addSeparator();
    contextMenu.addAction(&_actionNewFolder);
    contextMenu.addSeparator();
    contextMenu.addAction(&_actionCut);
    contextMenu.addAction(&_actionCopy);
    contextMenu.addAction(&_actionPaste);
    contextMenu.addSeparator();
    contextMenu.addAction(&_actionRename);
    contextMenu.addAction(&_actionDelete);

    addViewMenu(contextMenu);
}

void MaterialsEditor::materialContextMenu(QMenu& contextMenu)
{
    auto item = getActionItem();
    auto libraryName = getLibraryName(item);
    auto library = getMaterialManager().getLibrary(libraryName.toStdString());
    bool enabled = !library->isReadOnly();

    _actionNewMaterial.setEnabled(enabled);
    _actionNewFolder.setEnabled(enabled);
    _actionCut.setEnabled(enabled);
    _actionPaste.setEnabled(enabled);
    _actionRename.setEnabled(enabled);
    _actionDelete.setEnabled(enabled);

    contextMenu.addAction(&_actionNewMaterial);
    contextMenu.addAction(&_actionInheritMaterial);
    contextMenu.addAction(&_actionNewLibrary);
    contextMenu.addSeparator();
    contextMenu.addAction(&_actionNewFolder);
    contextMenu.addSeparator();
    auto selected = QString::fromStdString(_material->getUUID());
    if (isFavorite(selected)) {
        favoriteActionRemove();
    }
    else {
        favoriteActionAdd();
    }
    contextMenu.addAction(&_actionFavorite);
    contextMenu.addSeparator();
    contextMenu.addAction(&_actionCut);
    contextMenu.addAction(&_actionCopy);
    contextMenu.addAction(&_actionPaste);
    contextMenu.addSeparator();
    contextMenu.addAction(&_actionRename);
    contextMenu.addAction(&_actionDelete);

    addViewMenu(contextMenu);
}

void MaterialsEditor::defaultContextMenu(QMenu& contextMenu)
{
    contextMenu.addAction(&_actionNewLibrary);

    addViewMenu(contextMenu);
}

void MaterialsEditor::addViewMenu(QMenu& contextMenu)
{
    auto viewMenu = new QMenu(tr("View"), this);
    viewMenu->addAction(&_actionViewFavorites);
    viewMenu->addAction(&_actionViewRecent);
    viewMenu->addAction(&_actionViewFolders);
    viewMenu->addAction(&_actionViewLibraries);
    viewMenu->addAction(&_actionViewLegacy);
    viewMenu->addAction(&_actionViewDisabled);
    viewMenu->addAction(&_actionViewMasked);

    contextMenu.addMenu(viewMenu);
}

QString MaterialsEditor::getPath(const MaterialTreeItem* item, const QString& path) const
{
    auto function = item->getItemFunction();
    QString newPath;
    if (function == TreeFunctionLibrary) {
        return QStringLiteral("/") + path;
    }
    else if (function == TreeFunctionFolder) {
        newPath = item->text() + QStringLiteral("/") + path;
    }
    // Files use the empty path

    auto parent = item->parent();
    if (parent) {
        return getPath(parent, newPath);
    }

    return QStringLiteral("/") + newPath;
}

QString MaterialsEditor::getParentPath(const MaterialTreeItem* item) const
{
    auto parent = item->parent();
    if (parent) {
        return getPath(parent, QString());
    }

    return QStringLiteral("/");
}

QString MaterialsEditor::getLibraryName(const MaterialTreeItem* item) const
{
    auto function = item->getItemFunction();
    if (function == TreeFunctionLibrary) {
        return item->text();
    }

    auto parent = item->parent();
    if (parent) {
        return getLibraryName(parent);
    }

    throw Materials::LibraryNotFound();
}

void MaterialsEditor::onMenuNewLibrary(bool checked)
{
    Q_UNUSED(checked)

    auto newLibraryDialog = new NewLibrary(this);

    // Must be a blocking call to ensure the tree refresh has the new library
    newLibraryDialog->exec();

    refreshMaterialTree();
}

void MaterialsEditor::onMenuEnableDisable(bool checked)
{
    Q_UNUSED(checked)

    auto item = getActionItem();
    if (item) {
        auto library = getActionLibrary();

        Gui::WaitCursor wc;
        getMaterialManager().setDisabled(*library, !library->isDisabled());
        getMaterialManager().refresh();
        refreshMaterialTree();
    }
}

void MaterialsEditor::onMenuDelete(bool checked)
{
    Q_UNUSED(checked)

    MaterialTreeItem* item = nullptr;
    if (actionHasContext()) {
        item = getActionItem();
        if (!item) {
            return;
        }
    }
    else {
        if (_material) {
            item = getItemFromMaterial(*_material);
        }
        else {
            return;
        }
    }
    switch (item->getItemFunction()) {
        case TreeFunctionLibrary:
            deleteLibrary(item);
            return;

        case TreeFunctionFolder:
            deleteFolder(item);
            return;

        case TreeFunctionMaterial:
            deleteMaterial(item);
            return;
    }
}

void MaterialsEditor::deleteLibrary(MaterialTreeItem* item)
{
    auto library = getActionLibrary();
    int ret = QMessageBox::warning(
        this,
        tr("Delete Library"),
        tr("Deleting the library is immediate and permanent.\n"
            "Are you sure?"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    if (ret == QMessageBox::Yes) {
        Gui::WaitCursor wc;

        getMaterialManager().removeLibrary(library->getName());
        getMaterialManager().refresh();
        refreshMaterialTree();
    }
}

void MaterialsEditor::deleteFolder(MaterialTreeItem* item)
{
    auto path = getPath(item, QString());
    auto libraryName = getLibraryName(item);

    Base::Console().log(
        "Delete folder '%s' from '%s'\n",
        path.toStdString().c_str(),
        libraryName.toStdString().c_str()
    );
    auto library = getMaterialManager().getLibrary(libraryName.toStdString());

    if (item->hasChildren()) {
        int ret = QMessageBox::warning(
            this,
            tr("Delete Folder"),
            tr("Deleting the folder will also delete its contents. This is immediate and "
                "permanent.\n"
                "Are you sure?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        if (ret != QMessageBox::Yes) {
            return;
        }
    }

    Gui::WaitCursor wc;
    // Need to handle the case where the current material is a child
    if (_material && isAncestor(item, *_material)) {
        _material->resetEditState(); // We've already confirmed deletion

        _material = std::make_shared<Materials::Material>();
        _material->resetEditState();
        updateMaterial();
    }
    getMaterialManager().deleteRecursive(library, path.toStdString());
    removeItem(item->parent(), item);
}

void MaterialsEditor::deleteMaterial(MaterialTreeItem* item)
{
    auto original = getItemAsMaterial(item);
    auto uuid = original->getUUID();

    _material->resetEditState();
    getMaterialManager().remove(uuid);
    removeItem(item->parent(), item);
}

void MaterialsEditor::onMenuNewFolder(bool checked)
{
    Q_UNUSED(checked)

    MaterialTreeItem* item = nullptr;
    if (actionHasContext()) {
        item = getActionItem();
    }
    else {
        try {
            item = getItemFromLibrary(*getMaterialManager().getDefaultLibrary());
            if (!item) {
                return;
            }
        }
        catch (...) {
            return;
        }
    }

    // Find the library and path where we are
    auto path = getPath(item, QString());
    auto libraryName = getLibraryName(item);
    auto library = getMaterialManager().getLibrary(libraryName.toStdString());
    auto name = item->getUniqueName(tr("New Folder"), TreeFunctionFolder);

    Base::Console().log("path(%s)\n", path.toStdString().c_str());
    Base::Console().log("library(%s)\n", libraryName.toStdString().c_str());

    QIcon folderIcon(QStringLiteral(":/icons/folder.svg"));

    getMaterialManager().createFolder(library, (path + name).toStdString());

    Qt::ItemFlags flags
        = (Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    auto node = new MaterialTreeFolderItem(folderIcon, name);
    node->setFlags(flags);
    node->setLibraryName(libraryName);
    node->setPath(path + QStringLiteral("/") + name);

    addExpanded(ui->treeMaterials, item, node);

    // Start editing
    ui->treeMaterials->edit(node->index());
}

void MaterialsEditor::onMenuNewMaterial(bool checked)
{
    Q_UNUSED(checked)

    if (!actionHasContext()) {
        return;
    }

    // Ensure data is saved (or discarded) before changing materials
    if (_material->getEditState() != Materials::Material::MaterialEdit_None) {
        // Prompt the user to save or discard changes
        int res = confirmSave(this);
        if (res == QMessageBox::Cancel) {
            return;
        }
        else if (res == QMessageBox::Discard) {
            discardIfNew();
        }
        _material->resetEditState();
    }

    // Find the library and path where we are
    auto item = getActionItem();
    auto path = getPath(item, QString());
    auto libraryName = getLibraryName(item);
    auto library = getMaterialManager().getLibrary(libraryName.toStdString());

    Base::Console().log("path(%s)\n", path.toStdString().c_str());
    Base::Console().log("library(%s)\n", libraryName.toStdString().c_str());

    // Create a new material
    auto uniqueName = item->getUniqueName(tr("New Material"), TreeFunctionMaterial);
    _material = std::make_shared<Materials::Material>();
    _material->setEditStateNew();
    // _material->setEditStateInvariantChanged();
    setMaterialDefaults();
    _material->setLibrary(library);
    _material->setName(uniqueName.toStdString());
    _material->setDirectory(path.toStdString());
    Base::Console().log("uuid(%s)\n", _material->getUUID().c_str());

    QIcon matIcon = getIcon(library);
    auto card = new MaterialTreeMaterialItem(
        matIcon,
        QString::fromStdString(_material->getName()),
        QString::fromStdString(_material->getUUID())
    );
    card->setLibraryName(libraryName);
    card->setPath(item->path() + QStringLiteral("/") + QString::fromStdString(_material->getName()));

    addExpanded(ui->treeMaterials, item, card);

    setMaterialSelected(true);
    updateMaterial();

    // Now select the material in the tree
    auto index = card->index();
    if (index.isValid()) {
        QItemSelectionModel* selectionModel = ui->treeMaterials->selectionModel();
        selectionModel->select(index, QItemSelectionModel::ClearAndSelect);
        ui->treeMaterials->scrollTo(index);

        // Start editing
        ui->treeMaterials->edit(index);
    }
}

void MaterialsEditor::onMenuInheritMaterial(bool checked)
{
    Q_UNUSED(checked)

    if (!actionHasContext()) {
        return;
    }

    // Ensure data is saved (or discarded) before changing materials
    if (_material->getEditState() != Materials::Material::MaterialEdit_None) {
        // Prompt the user to save or discard changes
        int res = confirmSave(this);
        if (res == QMessageBox::Cancel) {
            return;
        }
        else if (res == QMessageBox::Discard) {
            discardIfNew();
        }
        _material->resetEditState();
    }

    // Find the library and path where we are
    auto item = getActionItem();
    auto parent = item->parent();
    auto original = getItemAsMaterial(item);

    // Create a new material
    auto uniqueName
        = parent->getUniqueName(QString::fromStdString(original->getName()), TreeFunctionMaterial);
    _material = getMaterialManager().copyInherited(*original, uniqueName.toStdString());
    _material->setEditStateNew();

    if (_material->getLibrary()->isReadOnly()) {
        auto library = getMaterialManager().getLibrary("User");
        _material->setLibrary(library);
        _material->setDirectory("");
        parent = getItemFromLibrary(*library);
    }

    QIcon matIcon = getIcon(_material->getLibrary());
    auto card = new MaterialTreeMaterialItem(
        matIcon,
        QString::fromStdString(_material->getName()),
        QString::fromStdString(_material->getUUID())
    );
    card->setLibraryName(_material->getLibrary()->getName());
    card->setPath(parent->path() + QStringLiteral("/") + QString::fromStdString(_material->getName()));

    addExpanded(ui->treeMaterials, parent, card);

    setMaterialSelected(true);
    updateMaterial();

    // Now select the material in the tree
    auto index = card->index();
    if (index.isValid()) {
        QItemSelectionModel* selectionModel = ui->treeMaterials->selectionModel();
        selectionModel->select(index, QItemSelectionModel::ClearAndSelect);
        ui->treeMaterials->scrollTo(index);

        // Start editing
        ui->treeMaterials->edit(index);
    }
}

void MaterialsEditor::onMenuChangeIcon(bool checked)
{
    Q_UNUSED(checked)
}

void MaterialsEditor::onInherit(bool checked)
{
    Q_UNUSED(checked)
}

void MaterialsEditor::onInheritNew(bool checked)
{
    Q_UNUSED(checked)
}

void MaterialsEditor::onMenuViewFavorites(bool checked)
{
    setIncludeFavorites(checked);
    refreshMaterialTree();
}

void MaterialsEditor::onMenuViewRecent(bool checked)
{
    setIncludeRecent(checked);
    refreshMaterialTree();
}

void MaterialsEditor::onMenuViewFolders(bool checked)
{
    setIncludeEmptyFolders(checked);
    refreshMaterialTree();
}

void MaterialsEditor::onMenuViewLibraries(bool checked)
{
    setIncludeEmptyLibraries(checked);
    refreshMaterialTree();
}

void MaterialsEditor::onMenuViewLegacy(bool checked)
{
    setIncludeLegacy(checked);
    refreshMaterialTree();
}

void MaterialsEditor::onMenuViewDisabled(bool checked)
{
    setIncludeDisabled(checked);
    refreshMaterialTree();
}

void MaterialsEditor::onMenuViewMasked(bool checked)
{
    setIncludeMasked(checked);
    refreshMaterialTree();
}

void MaterialsEditor::discardIfNew()
{
    // If the material we're discarding is a new one, remove it from the tree
    auto item = getItemFromMaterial(*_material);
    if (_material->getEditState() != Materials::Material::MaterialEdit_New) {
        _material->resetEditState();
        removeItem(item->parent(), item);
    }
}

int MaterialsEditor::confirmSave(QWidget* parent)
{
    QMessageBox box(parent ? parent : this);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(QObject::tr("Unsaved Material"));
    box.setText(QObject::tr("Save changes to the material before closing?"));
    box.setInformativeText(QObject::tr("Otherwise, all changes will be lost."));
    box.setStandardButtons(QMessageBox::Discard | QMessageBox::Cancel | QMessageBox::Save);
    box.setDefaultButton(QMessageBox::Save);
    box.setEscapeButton(QMessageBox::Cancel);

    // add shortcuts
    QAbstractButton* saveBtn = box.button(QMessageBox::Save);
    if (saveBtn->shortcut().isEmpty()) {
        QString text = saveBtn->text();
        text.prepend(QLatin1Char('&'));
        saveBtn->setShortcut(QKeySequence::mnemonic(text));
    }

    QAbstractButton* discardBtn = box.button(QMessageBox::Discard);
    if (discardBtn->shortcut().isEmpty()) {
        QString text = discardBtn->text();
        text.prepend(QLatin1Char('&'));
        discardBtn->setShortcut(QKeySequence::mnemonic(text));
    }

    int res = QMessageBox::Cancel;
    box.adjustSize();  // Silence warnings from Qt on Windows
    switch (box.exec()) {
        case QMessageBox::Save:
            saveMaterial();
            res = QMessageBox::Save;
            break;
        case QMessageBox::Discard:
            res = QMessageBox::Discard;
            break;
    }

    return res;
}

void MaterialsEditor::renameLibrary(MaterialTreeItem* item)
{
    auto originalName = item->originalName();
    auto newName = item->text();

    if (originalName != newName) {
        Base::Console().log("Library edited '%s'->'%s'\n",
                            originalName.toStdString().c_str(),
                            newName.toStdString().c_str());
        getMaterialManager().renameLibrary(originalName.toStdString(), newName.toStdString());
        item->setOriginalName(newName);
    }
}

void MaterialsEditor::renameFolder(MaterialTreeItem* item)
{
    auto originalName = item->originalName();
    auto newName = item->text();
    auto path = getParentPath(item);
    auto oldPath = path + originalName;
    auto newPath = path + newName;

    auto libraryName = getLibraryName(item);
    auto library = getMaterialManager().getLibrary(libraryName.toStdString());

    if (originalName != newName) {
        Base::Console().log("Folder edited '%s'->'%s'\n",
                            originalName.toStdString().c_str(),
                            newName.toStdString().c_str());
        Base::Console().log("\t path '%s'->'%s'\n",
                            oldPath.toStdString().c_str(),
                            newPath.toStdString().c_str());
        getMaterialManager().renameFolder(library, oldPath.toStdString(), newPath.toStdString());
        item->setOriginalName(newName);

        // Update the current material
        QString currentPath = QString::fromStdString(_material->getDirectory());
        oldPath = stripLeadingSeparator(oldPath);
        newPath = stripLeadingSeparator(newPath);
        if (currentPath.startsWith(oldPath)) {
            currentPath = newPath + currentPath.remove(0, oldPath.size());
            _material->setDirectory(currentPath.toStdString());
        }
    }
}

QString MaterialsEditor::stripLeadingSeparator(const QString& filePath) const
{
    auto path = filePath;
    if (path.startsWith(QStringLiteral("/"))) {
        path.remove(0, 1);
    }
    return path;
}


void MaterialsEditor::renameMaterial(MaterialTreeItem* item)
{
    auto originalName = item->originalName();
    auto newName = item->text();
    auto path = getParentPath(item);

    if (originalName != newName) {
        auto oldPath = path + originalName;
        auto newPath = path + newName;
        _material->setName(newName.toStdString());
        item->setOriginalName(newName);
        updateMaterial();

        updateFavoritesName();
        updateRecentsName();
    }
}

void MaterialsEditor::updateMaterialTreeName(const QString& name)
{
    auto item = getItemFromMaterial(*_material);
    if (item) {
        if (_material->getName() != name) {
            _material->setName(name.toStdString());
        }
        if (item->text() != name) {
            item->setText(name);
            item->setOriginalName(name);
        }
    }
}

void MaterialsEditor::updateFavoritesRecentsName(MaterialTreeItem* parent, const QString& uuid, const QString& name)
{
    if (parent) {
        int row = 0;
        auto item = static_cast<MaterialTreeMaterialItem*>(parent->child(row));
        while (item && item->getItemFunction() == TreeFunctionMaterial) {
            auto modelUuid = item->getUUID();
            if (modelUuid == uuid) {
                if (item->text() != name) {
                    item->setText(name);
                    item->setOriginalName(name);
                }
                return;
            }
            row++;
            item = static_cast<MaterialTreeMaterialItem*>(parent->child(row));
        }
    }
}

void MaterialsEditor::updateRecentsName(const QString& uuid, const QString& name)
{
    updateFavoritesRecentsName(getRecentsItem(), uuid, name);
}

void MaterialsEditor::updateRecentsName()
{
    updateRecentsName(
        QString::fromStdString(_material->getUUID()),
        QString::fromStdString(_material->getLibraryPath())
    );
}

void MaterialsEditor::updateFavoritesName(const QString& uuid, const QString& name)
{
    updateFavoritesRecentsName(getFavoritesItem(), uuid, name);
}

void MaterialsEditor::updateFavoritesName()
{
    updateFavoritesName(
        QString::fromStdString(_material->getUUID()),
        QString::fromStdString(_material->getLibraryPath())
    );
}

#include "moc_MaterialsEditor.cpp"
