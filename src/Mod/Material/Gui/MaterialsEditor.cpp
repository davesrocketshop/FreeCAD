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

#include "LibraryType.h"
#include "MaterialDelegate.h"
#include "MaterialSave.h"
#include "MaterialsEditor.h"
#include "ModelSelect.h"
#include "ui_MaterialsEditor.h"

#include "Widgets/MaterialPropertiesWidget.h"
#include "Widgets/PropertiesWidget.h"


using namespace MatGui;

/* TRANSLATOR MatGui::MaterialsEditor */

MaterialsEditor::MaterialsEditor(Materials::MaterialFilter filter, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui_MaterialsEditor)
    , _material(std::make_shared<Materials::Material>())
    , _recentMax(0)
    , _filter(filter)
{
    setup();
}

MaterialsEditor::MaterialsEditor(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui_MaterialsEditor)
    , _material(std::make_shared<Materials::Material>())
    , _recentMax(0)
{
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
    connect(ui->standardButtons->button(QDialogButtonBox::Cancel),
            &QPushButton::clicked,
            this,
            &MaterialsEditor::onCancel);
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
    auto model = qobject_cast<QStandardItemModel*>(tree->model());
    connect(model, &QStandardItemModel::itemChanged, this, &MaterialsEditor::onTreeItemChanged);
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
    connect(&_actionNewLibrary, &QAction::triggered, this, &MaterialsEditor::onMenuNewLibrary);
#if defined(BUILD_MATERIAL_EXTERNAL)
    connect(&_actionNewRemoteLibrary, &QAction::triggered, this, &MaterialsEditor::onMenuNewLibrary);
#endif
    connect(&_actionNewLocalLibrary, &QAction::triggered, this, &MaterialsEditor::onMenuNewLibrary);
    connect(&_actionNewFolder, &QAction::triggered, this, &MaterialsEditor::onMenuNewFolder);
    connect(&_actionNewMaterial, &QAction::triggered, this, &MaterialsEditor::onMenuNewMaterial);
    connect(&_actionFavorite, &QAction::triggered, this, &MaterialsEditor::onFavourite);
    connect(&_actionChangeIcon, &QAction::triggered, this, &MaterialsEditor::onMenuChangeIcon);

    // TODO: Add tooltips
    _actionNewLibrary.setText(tr("New library"));
    _actionNewLocalLibraryIcon = QIcon(QStringLiteral(":/icons/Material_Library.svg"));
    _actionNewLibrary.setIcon(_actionNewLocalLibraryIcon);
    _actionNewLibrary.setToolTip(tr("New library"));

    #if defined(BUILD_MATERIAL_EXTERNAL)
    _actionNewRemoteLibrary.setText(tr("New remote library"));
    _actionNewRemoteLibraryIcon = QIcon(QStringLiteral(":/icons/Material_Library.svg"));
    _actionNewRemoteLibrary.setIcon(_actionNewRemoteLibraryIcon);
    _actionNewRemoteLibrary.setToolTip(tr("New remote library"));
#endif
    _actionNewLocalLibrary.setText(tr("New local library"));
    // _actionNewLocalLibraryIcon = QIcon(QStringLiteral(":/icons/Material_Library.svg"));
    _actionNewLocalLibrary.setIcon(_actionNewLocalLibraryIcon);
    _actionNewLocalLibrary.setToolTip(tr("New Local library"));

    _actionNewFolder.setText(tr("New folder"));
    _actionNewFolderIcon = QIcon(QStringLiteral(":/icons/Group.svg"));
    _actionNewFolder.setIcon(_actionNewFolderIcon);
    _actionNewFolder.setToolTip(tr("New folder"));

    _actionNewMaterial.setText(tr("New material"));
    _actionNewMaterialIcon = QIcon(QStringLiteral(":/icons/Material_Edit.svg"));
    _actionNewMaterial.setIcon(_actionNewMaterialIcon);
    _actionNewMaterial.setToolTip(tr("New material"));

    _actionInheritMaterial.setText(tr("New material from selected"));
    _actionInheritMaterialIcon = QIcon(QStringLiteral(":/icons/Material_Inherit.svg"));
    _actionInheritMaterial.setIcon(_actionInheritMaterialIcon);
    _actionInheritMaterial.setToolTip(
        tr("Create a new material based on the currently selected material"));

    _actionFavorite.setText(tr("Add to favorites"));
    _actionFavoriteIcon = QIcon(QStringLiteral(":/icons/Material_Favorite.svg"));
    _actionFavorite.setIcon(_actionFavoriteIcon);
    _actionFavorite.setToolTip(tr("Add or remove material from favorites list"));

    _actionChangeIcon.setText(tr("Change icon"));

    _actionCut.setText(tr("Cut"));
    _actionCutIcon = QIcon(QStringLiteral(":/icons/edit-cut.svg"));
    _actionCut.setIcon(_actionCutIcon);
    _actionCut.setToolTip(tr("Cut"));

    _actionCopy.setText(tr("Copy"));
    _actionCopyIcon = QIcon(QStringLiteral(":/icons/edit-copy.svg"));
    _actionCopy.setIcon(_actionCopyIcon);
    _actionCopy.setToolTip(tr("Copy"));

    _actionPaste.setText(tr("Paste"));
    _actionPasteIcon = QIcon(QStringLiteral(":/icons/edit-paste.svg"));
    _actionPaste.setIcon(_actionPasteIcon);
    _actionPaste.setToolTip(tr("Paste"));

    _actionRename.setText(tr("Rename"));
    _actionDelete.setText(tr("Delete"));
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
        if (_filter.modelIncluded(uuid)) {
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
        auto material = getMaterialManager().getMaterial(uuid);
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
        if (_filter.modelIncluded(uuid)) {
            _recents.push_back(uuid);
        }
    }
}

void MaterialsEditor::saveRecents()
{
    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Recent");

    // Clear out the existing favorites
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
        auto material = getMaterialManager().getMaterial(uuid);
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
    auto typeVariant = item->data(TreeFunctionRole);
    auto function = typeVariant.value<TreeFunctionType>();

    switch (function) {
        case TreeFunctionLibrary:
            renameLibrary(item);
            break;

        case TreeFunctionFolder:
            renameFolder(item);
            break;

        case TreeFunctionMaterial:
            renameMaterial(item);
            break;
    }
}

void MaterialsEditor::onName(const QString& name)
{
    _material->setName(name);
}

void MaterialsEditor::onAuthor(const QString& author)
{
    _material->setAuthor(author);
}

void MaterialsEditor::onLicense(const QString& license)
{
    _material->setLicense(license);
}

void MaterialsEditor::onSourceURL(const QString& url)
{
    _material->setURL(url);
}

void MaterialsEditor::onSourceReference(const QString& reference)
{
    _material->setReference(reference);
}

void MaterialsEditor::onDescription(const QString& description)
{
    _material->setDescription(description);
}

void MaterialsEditor::onPhysicalAdd()
{
    ModelSelect dialog(this, Materials::ModelFilter_Physical);
    dialog.setModal(true);
    if (dialog.exec() == QDialog::Accepted) {
        QString selected = dialog.selectedModel();
        _material->addPhysical(selected);
        updateMaterial();
    }
    else {
        Base::Console().log("No model selected\n");
    }
}

void MaterialsEditor::onPhysicalRemove(const QString& propertyName)
{
    // QItemSelectionModel* selectionModel = ui->treePhysicalProperties->selectionModel();
    // if (selectionModel->hasSelection()) {
    //     auto index = selectionModel->currentIndex().siblingAtColumn(0);

    //     auto treeModel = dynamic_cast<const QStandardItemModel*>(index.model());

    //     // Check we're the material model root.
    //     auto item = treeModel->itemFromIndex(index);
    //     auto group = item->parent();
    //     if (!group) {
    //         QString propertyName = index.data().toString();

    QString uuid = _material->getModelByName(propertyName);
    _material->removePhysical(uuid);
    updateMaterial();
    //     }
    // }
}

void MaterialsEditor::onAppearanceAdd()
{
    ModelSelect dialog(this, Materials::ModelFilter_Appearance);
    dialog.setModal(true);
    if (dialog.exec() == QDialog::Accepted) {
        QString selected = dialog.selectedModel();
        _material->addAppearance(selected);
        auto model = Materials::ModelManager::getManager().getModel(selected);
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
    // QItemSelectionModel* selectionModel = ui->treeAppearance->selectionModel();
    // if (selectionModel->hasSelection()) {
    //     auto index = selectionModel->currentIndex().siblingAtColumn(0);

    //     auto treeModel = dynamic_cast<const QStandardItemModel*>(index.model());

    //     // Check we're the material model root.
    //     auto item = treeModel->itemFromIndex(index);
    //     auto group = item->parent();
    //     if (!group) {
    //         QString propertyName = index.data().toString();

    QString uuid = _material->getModelByName(propertyName);
    _material->removeAppearance(uuid);
    updateMaterial();
    //     }
    // }
}

void MaterialsEditor::onFavourite(bool checked)
{
    Q_UNUSED(checked)

    auto selected = _material->getUUID();
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
    _material->setName(tr("Unnamed"));
    std::string Author = App::GetApplication()
                             .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document")
                             ->GetASCII("prefAuthor", "");
    _material->setAuthor(QString::fromStdString(Author));

    // license stuff
    auto paramGrp {App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Document")};
    auto index = static_cast<int>(paramGrp->GetInt("prefLicenseType", 0));
    const char* name = App::licenseItems.at(index).at(App::posnOfFullName);
    // const char* url = App::licenseItems.at(index).at(App::posnOfUrl);
    // std::string licenseUrl = (paramGrp->GetASCII("prefLicenseUrl", url));
    _material->setLicense(QLatin1String(name));

    // Empty materials will have no parent
    getMaterialManager().dereference(_material);

    updateMaterial();
    _material->resetEditState();
}

void MaterialsEditor::onNewMaterial(bool checked)
{
    Q_UNUSED(checked)

    // Ensure data is saved (or discarded) before changing materials
    if (_material->getEditState() != Materials::Material::ModelEdit_None) {
        // Prompt the user to save or discard changes
        int res = confirmSave(this);
        if (res == QMessageBox::Cancel) {
            return;
        }
        else if (res == QMessageBox::Discard) {
            discardIfNew();
        }
    }

    // Create a new material
    _material = std::make_shared<Materials::Material>();
    setMaterialDefaults();
    setMaterialSelected(false);
}

void MaterialsEditor::onInheritNewMaterial(bool checked)
{
    Q_UNUSED(checked)

    // Save the current UUID to use as out parent
    auto parent = _material->getUUID();

    // Ensure data is saved (or discarded) before changing materials
    if (_material->getEditState() != Materials::Material::ModelEdit_None) {
        // Prompt the user to save or discard changes
        int res = confirmSave(this);
        if (res == QMessageBox::Cancel) {
            return;
        }
        else if (res == QMessageBox::Discard) {
            discardIfNew();
        }
    }

    // Create a new material
    _material = std::make_shared<Materials::Material>();
    _material->setParentUUID(parent);
    setMaterialDefaults();
}

void MaterialsEditor::onOk(bool checked)
{
    Q_UNUSED(checked)

    // Ensure data is saved (or discarded) before exiting
    if (_material->getEditState() != Materials::Material::ModelEdit_None) {
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

void MaterialsEditor::onCancel(bool checked)
{
    Q_UNUSED(checked)

    reject();
}

void MaterialsEditor::onSave(bool checked)
{
    Q_UNUSED(checked)

    saveMaterial();
}

void MaterialsEditor::saveMaterial()
{
    MaterialSave dialog(_material, this);
    dialog.setModal(true);
    if (dialog.exec() == QDialog::Accepted) {
        updateMaterial();
        _material->resetEditState();
        refreshMaterialTree();
        setMaterialSelected(true);
    }
}

void MaterialsEditor::accept()
{
    if (_material->isOldFormat()) {
        Base::Console().log("*** Old format file ***\n");
        oldFormatError();

        return;
    }
    addRecent(_material->getUUID());
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
                                               QTreeView* tree,
                                               QStandardItemModel* model,
                                               QStandardItem* item)
{
    if (item->hasChildren()) {
        param->SetBool(item->text().toStdString().c_str(), tree->isExpanded(item->index()));

        auto treeParam = param->GetGroup(item->text().toStdString().c_str());
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
    auto model = qobject_cast<QStandardItemModel*>(tree->model());

    auto root = model->invisibleRootItem();
    for (int i = 0; i < root->rowCount(); i++) {
        auto child = root->child(i);
        // treeParam->SetBool(child->text().toStdString().c_str(),
        // tree->isExpanded(child->index()));

        saveMaterialTreeChildren(treeParam, tree, model, child);
    }
}

void MaterialsEditor::addMaterials(
    QStandardItem& parent,
    const std::shared_ptr<std::map<QString, std::shared_ptr<Materials::MaterialTreeNode>>>
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
            flags |= (Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        }
        if (nodePtr->getType() == Materials::MaterialTreeNode::NodeType::DataNode) {
            QString uuid = nodePtr->getUUID();

            QIcon matIcon = icon;
            if (nodePtr->isOldFormat()) {
                matIcon = _warningIcon;
            }
            auto card = new QStandardItem(matIcon, mat.first);
            card->setFlags(flags | Qt::ItemIsSelectable);
            card->setData(QVariant(uuid), TreeDataRole);
            card->setData(QVariant(TreeFunctionType::TreeFunctionMaterial), TreeFunctionRole);
            card->setData(QVariant(mat.first), TreeNameRole);
            if (nodePtr->isOldFormat()) {
                card->setToolTip(tr("This card uses the old format and must be saved before use"));
            }

            addExpanded(tree, &parent, card);
        }
        else {
            auto node = new QStandardItem(folderIcon, mat.first);
            node->setFlags(flags);
            node->setData(QVariant(TreeFunctionType::TreeFunctionFolder), TreeFunctionRole);
            node->setData(QVariant(mat.first), TreeNameRole);
            auto treeMap = nodePtr->getFolder();

            addExpanded(tree, &parent, node, childParam);
            addMaterials(*node, treeMap, folderIcon, icon, childParam);
        }
    }
}

void MaterialsEditor::addExpanded(QTreeView* tree, QStandardItem* parent, QStandardItem* child)
{
    parent->appendRow(child);
    tree->setExpanded(child->index(), true);
}

void MaterialsEditor::addExpanded(QTreeView* tree,
                                  QStandardItem* parent,
                                  QStandardItem* child,
                                  const Base::Reference<ParameterGrp>& param)
{
    parent->appendRow(child);

    // Restore to any previous expansion state
    auto expand = param->GetBool(child->text().toStdString().c_str(), true);
    tree->setExpanded(child->index(), expand);
}

void MaterialsEditor::addExpanded(QTreeView* tree, QStandardItemModel* parent, QStandardItem* child)
{
    parent->appendRow(child);
    tree->setExpanded(child->index(), true);
}

void MaterialsEditor::addExpanded(QTreeView* tree,
                                  QStandardItemModel* parent,
                                  QStandardItem* child,
                                  const Base::Reference<ParameterGrp>& param)
{
    parent->appendRow(child);

    // Restore to any previous expansion state
    auto expand = param->GetBool(child->text().toStdString().c_str(), true);
    tree->setExpanded(child->index(), expand);
}

QIcon MaterialsEditor::getIcon(const std::shared_ptr<Materials::Library>& library)
{
    // Load from the QByteArray if available
    QIcon icon;
    if (library->hasIcon()) {
        QImage image;
        if (!image.loadFromData(library->getIcon())) {
            Base::Console().log("Unable to load icon image for library '%s'\n",
                                library->getName().toStdString().c_str());
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

void MaterialsEditor::addRecents(QStandardItem* parent)
{
    auto tree = ui->treeMaterials;
    for (auto& uuid : _recents) {
        try {
            auto material = getMaterialManager().getMaterial(uuid);
            QIcon icon = getIcon(material->getLibrary());
            auto card = new QStandardItem(icon, libraryPath(material));
            card->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
                            | Qt::ItemIsDropEnabled);
            card->setData(QVariant(uuid), TreeDataRole);
            card->setData(QVariant(TreeFunctionType::TreeFunctionRecents), TreeFunctionRole);

            addExpanded(tree, parent, card);
        }
        catch (const Materials::MaterialNotFound&) {
        }
    }
}

void MaterialsEditor::addFavorites(QStandardItem* parent)
{
    auto tree = ui->treeMaterials;
    for (auto& uuid : _favorites) {
        try {
            auto material = getMaterialManager().getMaterial(uuid);
            QIcon icon = getIcon(material->getLibrary());
            auto card = new QStandardItem(icon, libraryPath(material));
            card->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
                            | Qt::ItemIsDropEnabled);
            card->setData(QVariant(uuid), TreeDataRole);
            card->setData(QVariant(TreeFunctionType::TreeFunctionFavorites), TreeFunctionRole);

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
    auto model = qobject_cast<QStandardItemModel*>(tree->model());

    if (_filterOptions.includeFavorites()) {
        auto lib = new QStandardItem(tr("Favorites"));
        lib->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        lib->setData(QVariant(TreeFunctionType::TreeFunctionFavorites), TreeFunctionRole);
        addExpanded(tree, model, lib, param);
        addFavorites(lib);
    }

    if (_filterOptions.includeRecent()) {
        auto lib = new QStandardItem(tr("Recent"));
        lib->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        lib->setData(QVariant(TreeFunctionType::TreeFunctionRecents), TreeFunctionRole);
        addExpanded(tree, model, lib, param);
        addRecents(lib);
    }

    auto libraries = getMaterialManager().getLibraries();
    for (const auto& library : *libraries) {
        auto materialTree = getMaterialManager().getMaterialTree(*library);

        bool showLibraries = _filterOptions.includeEmptyLibraries();
        if (!_filterOptions.includeEmptyLibraries() && materialTree->size() > 0) {
            showLibraries = true;
        }

        if (showLibraries) {
            auto lib = new QStandardItem(library->getName());
            if (library->isReadOnly()) {
                lib->setFlags(Qt::ItemIsEnabled);
            }
            else {
                lib->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled
                              | Qt::ItemIsDropEnabled);
            }
            lib->setData(QVariant(TreeFunctionType::TreeFunctionLibrary), TreeFunctionRole);
            lib->setData(QVariant(library->getName()), TreeNameRole);
            addExpanded(tree, model, lib, param);

            QIcon icon = getIcon(library);
            QIcon folderIcon(QStringLiteral(":/icons/folder.svg"));

            addMaterials(*lib, materialTree, folderIcon, icon, param);
        }
    }
}

void MaterialsEditor::createMaterialTree()
{
    auto tree = ui->treeMaterials;
    auto model = new QStandardItemModel();
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
    auto tree = ui->treeMaterials;
    auto model = qobject_cast<QStandardItemModel*>(tree->model());
    model->clear();

    fillMaterialTree();
}

QString MaterialsEditor::getColorHash(const QString& colorString, int colorRange)
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

QString MaterialsEditor::libraryPath(const std::shared_ptr<Materials::Material>& material)
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

void MaterialsEditor::onSelectMaterial(const QItemSelection& selected,
                                       const QItemSelection& deselected)
{
    Q_UNUSED(deselected);

    // Get the UUID before changing the underlying data model
    QString uuid;
    auto model = qobject_cast<QStandardItemModel*>(ui->treeMaterials->model());
    QModelIndexList indexes = selected.indexes();
    for (auto it = indexes.begin(); it != indexes.end(); it++) {
        QStandardItem* item = model->itemFromIndex(*it);

        if (item) {
            auto fun = getActionFunction(item);
            if (fun == TreeFunctionLibrary) {
                setLibraryPropertyState();
                return;
            }
            else if (fun == TreeFunctionLibrary) {
                setFolderPropertyState();
                return;
            }

            setMaterialPropertyState();
            uuid = item->data(TreeDataRole).toString();
            break;
        }
    }

    if (uuid.isEmpty() || uuid != _material->getUUID()) {
        // Ensure data is saved (or discarded) before changing materials
        if (_material->getEditState() != Materials::Material::ModelEdit_None) {
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
            _material = std::make_shared<Materials::Material>(*getMaterialManager().getMaterial(uuid));
            setMaterialSelected(true);
            updateMaterial();
            _material->resetEditState();
        }
        // else don't reset edit state
    }
    catch (Materials::ModelNotFound const&) {
        Base::Console().log("*** Unable to load material '%s'\n", uuid.toStdString().c_str());
        _material = std::make_shared<Materials::Material>();
        setMaterialSelected(true);
        updateMaterial();
        _material->resetEditState();
    }
}

const QStandardItemModel* MaterialsEditor::getActionModel() const
{
    return qobject_cast<const QStandardItemModel*>(_actionIndex.model());
}

QStandardItem* MaterialsEditor::getActionItem()
{
    // auto model = const_cast<QStandardItemModel*>(getActionModel());
    auto model = qobject_cast<QStandardItemModel*>(ui->treeMaterials->model());
    if (model) {
        return model->itemFromIndex(_actionIndex);
    }
    return nullptr;
}

TreeFunctionType MaterialsEditor::getActionFunction(const QStandardItem* item) const
{
    if (item) {
        auto typeVariant = item->data(TreeFunctionRole);
        return typeVariant.value<TreeFunctionType>();
    }
    throw ActionError();
}

TreeFunctionType MaterialsEditor::getActionFunction()
{
    return getActionFunction(getActionItem());
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
        Base::Console().log("MaterialsEditor::onContextMenu(): No action to handle.\n");
    }

    // QAction actionInheritFrom(tr("Inherit from"), this);
    // connect(&actionInheritFrom, &QAction::triggered, this, &MaterialsEditor::onInherit);
    // contextMenu.addAction(&actionInheritFrom);

    // QAction actionInheritNew(tr("Inherit new material"), this);
    // connect(&actionInheritNew, &QAction::triggered, this, &MaterialsEditor::onInheritNew);
    // contextMenu.addAction(&actionInheritNew);

    contextMenu.exec(ui->treeMaterials->mapToGlobal(pos));
}

void MaterialsEditor::favoriteActionAdd()
{
    _actionFavorite.setText(tr("Add to favorites"));
}

void MaterialsEditor::favoriteActionRemove()
{
    _actionFavorite.setText(tr("Remove from favorites"));
}

void MaterialsEditor::favoriteContextMenu(QMenu& contextMenu)
{
    Q_UNUSED(contextMenu);

    contextMenu.addAction(&_actionInheritMaterial);
#if defined(BUILD_MATERIAL_EXTERNAL)
    if (useExternal()) {
        contextMenu.addAction(&_actionNewRemoteLibrary);
    }
#endif
    contextMenu.addAction(&_actionNewLocalLibrary);
    contextMenu.addSeparator();

    auto item = getActionItem();
    if (item->text() != tr("Favorites")) {
        favoriteActionRemove();
        contextMenu.addAction(&_actionFavorite);
    }
}

void MaterialsEditor::recentContextMenu(QMenu& contextMenu)
{
    contextMenu.addAction(&_actionInheritMaterial);
#if defined(BUILD_MATERIAL_EXTERNAL)
    if (useExternal()) {
        contextMenu.addAction(&_actionNewRemoteLibrary);
    }
#endif
    contextMenu.addAction(&_actionNewLocalLibrary);
    contextMenu.addSeparator();
    auto item = getActionItem();
    if (item->text() != tr("Recent")) {
        auto selected = _material->getUUID();
        if (isFavorite(selected)) {
            favoriteActionRemove();
        }
        else {
            favoriteActionAdd();
        }
        contextMenu.addAction(&_actionFavorite);
    }
}

void MaterialsEditor::libraryContextMenu(QMenu& contextMenu)
{
    contextMenu.addAction(&_actionNewMaterial);
#if defined(BUILD_MATERIAL_EXTERNAL)
    if (useExternal()) {
        contextMenu.addAction(&_actionNewRemoteLibrary);
    }
#endif
    contextMenu.addAction(&_actionNewLocalLibrary);
    contextMenu.addAction(&_actionChangeIcon);
    contextMenu.addSeparator();
    contextMenu.addAction(&_actionNewFolder);
}

void MaterialsEditor::folderContextMenu(QMenu& contextMenu)
{
    contextMenu.addAction(&_actionNewMaterial);
#if defined(BUILD_MATERIAL_EXTERNAL)
    if (useExternal()) {
        contextMenu.addAction(&_actionNewRemoteLibrary);
    }
#endif
    contextMenu.addAction(&_actionNewLocalLibrary);
    contextMenu.addSeparator();
    contextMenu.addAction(&_actionNewFolder);
    contextMenu.addSeparator();
    contextMenu.addAction(&_actionCut);
    contextMenu.addAction(&_actionCopy);
    contextMenu.addAction(&_actionPaste);
    contextMenu.addSeparator();
    contextMenu.addAction(&_actionRename);
    contextMenu.addAction(&_actionDelete);
}

void MaterialsEditor::materialContextMenu(QMenu& contextMenu)
{
    contextMenu.addAction(&_actionNewMaterial);
    contextMenu.addAction(&_actionInheritMaterial);
#if defined(BUILD_MATERIAL_EXTERNAL)
    if (useExternal()) {
        contextMenu.addAction(&_actionNewRemoteLibrary);
    }
#endif
    contextMenu.addAction(&_actionNewLocalLibrary);
    contextMenu.addSeparator();
    contextMenu.addAction(&_actionNewFolder);
    contextMenu.addSeparator();
    auto selected = _material->getUUID();
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
}

QString MaterialsEditor::getPath(const QStandardItem* item, const QString& path) const
{
    auto function = getActionFunction(item);
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

QString MaterialsEditor::getParentPath(const QStandardItem* item) const
{
    auto parent = item->parent();
    if (parent) {
        return getPath(parent, QString());
    }

    return QStringLiteral("/");
}

QString MaterialsEditor::getLibraryName(const QStandardItem* item) const
{
    auto function = getActionFunction(item);
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

    Base::Console().log("onMenuNewLibrary()\n");

    LibraryType dialog(this);
    dialog.setModal(true);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QString name(QStringLiteral("New Library"));
    try {
        auto library = getMaterialManager().getLibrary(name);
        if (library) {
            Base::Console().log("Unable to create library '%s': already exists\n", name.toStdString().c_str());
            return;
        }
    }
    catch (const Materials::LibraryNotFound &) {}

    try {
        getMaterialManager().createLibrary(name, QStringLiteral(":/icons/freecad.svg"), false);
    }
    catch (const Materials::CreationError& e) {
        Base::Console().log("Unable to create library '%s': %s\n",
                            name.toStdString().c_str(), e.what());
    }
    refreshMaterialTree();

    // Check if local or remote library

    // Check if library exists

    // If local get path
}

void MaterialsEditor::onMenuNewFolder(bool checked)
{
    Q_UNUSED(checked)

    // Find the library and path where we are
    auto item = getActionItem();
    auto path = getPath(item, QString());
    auto libraryName = getLibraryName(item);
    auto library = getMaterialManager().getLibrary(libraryName);
    auto name = tr("New Folder");

    Base::Console().log("path(%s)\n", path.toStdString().c_str());
    Base::Console().log("library(%s)\n", libraryName.toStdString().c_str());

    QIcon folderIcon(QStringLiteral(":/icons/folder.svg"));

    getMaterialManager().createFolder(library, path + name);

    Qt::ItemFlags flags =
        (Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    auto node = new QStandardItem(folderIcon, name);
    node->setFlags(flags);
    node->setData(QVariant(TreeFunctionType::TreeFunctionFolder), TreeFunctionRole);
    node->setData(QVariant(name), TreeNameRole);

    addExpanded(ui->treeMaterials, item, node);
}

void MaterialsEditor::onMenuNewMaterial(bool checked)
{
    Q_UNUSED(checked)

    // Find the library and path where we are
    auto item = getActionItem();
    auto path = getPath(item, QString());
    auto libraryName = getLibraryName(item);
    auto library = getMaterialManager().getLibrary(libraryName);

    Base::Console().log("path(%s)\n", path.toStdString().c_str());
    Base::Console().log("library(%s)\n", libraryName.toStdString().c_str());

    // Create a new material
    _material = std::make_shared<Materials::Material>();
    _material->setEditStateAlter();
    setMaterialDefaults();
    _material->setLibrary(library);
    _material->setName(tr("New Material"));
    _material->setDirectory(path);
    Base::Console().log("uuid(%s)\n", _material->getUUID().toStdString().c_str());

    Qt::ItemFlags flags = (Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled
                           | Qt::ItemIsDropEnabled | Qt::ItemIsSelectable);
    QIcon matIcon = getIcon(library);
    auto card = new QStandardItem(matIcon, _material->getName());
    card->setFlags(flags);
    card->setData(QVariant(_material->getUUID()), TreeDataRole);
    card->setData(QVariant(TreeFunctionType::TreeFunctionMaterial), TreeFunctionRole);
    card->setData(QVariant(_material->getName()), TreeNameRole);

    addExpanded(ui->treeMaterials, item, card);

    setMaterialSelected(true);
    _newItem = card;
    updateMaterial();

    // Now select the material in the tree
    auto index = card->index();
    if (index.isValid()) {
        QItemSelectionModel* selectionModel = ui->treeMaterials->selectionModel();
        selectionModel->select(index, QItemSelectionModel::SelectCurrent);
        ui->treeMaterials->scrollTo(index);
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

void MaterialsEditor::discardIfNew()
{
    // If the material we're discarding is a new one, remove it from the tree
    if (_newItem) {
        auto parent = _newItem->parent();
        if (parent) {
            parent->removeRow(_newItem->row());
        }
        _newItem = nullptr;
    }

    // If it's an existing one, restore the original name
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

void MaterialsEditor::renameLibrary(QStandardItem* item)
{
    auto originalName = item->data(TreeNameRole).toString();
    auto newName = item->text();

    if (originalName != newName) {
        Base::Console().log("Library edited '%s'->'%s'\n",
                            originalName.toStdString().c_str(),
                            newName.toStdString().c_str());
        getMaterialManager().renameLibrary(originalName, newName);
        item->setData(QVariant(newName), TreeNameRole);
    }
}

void MaterialsEditor::renameFolder(QStandardItem* item)
{
    auto originalName = item->data(TreeNameRole).toString();
    auto newName = item->text();
    auto path = getParentPath(item);
    auto oldPath = path + originalName;
    auto newPath = path + newName;

    auto libraryName = getLibraryName(item);
    auto library = getMaterialManager().getLibrary(libraryName);

    if (originalName != newName) {
        Base::Console().log("Folder edited '%s'->'%s'\n",
                            originalName.toStdString().c_str(),
                            newName.toStdString().c_str());
        Base::Console().log("\t path '%s'->'%s'\n",
                            oldPath.toStdString().c_str(),
                            newPath.toStdString().c_str());
        getMaterialManager().renameFolder(library, oldPath, newPath);
        item->setData(QVariant(newName), TreeNameRole);
    }
}

void MaterialsEditor::renameMaterial(QStandardItem* item)
{
    auto originalName = item->data(TreeNameRole).toString();
    auto newName = item->text();
    auto path = getParentPath(item);
    auto oldPath = path + originalName;
    auto newPath = path + newName;

    if (originalName != newName) {
        Base::Console().log("Material edited '%s'->'%s'\n",
                            originalName.toStdString().c_str(),
                            newName.toStdString().c_str());
        Base::Console().log("\t path '%s'->'%s'\n",
                            oldPath.toStdString().c_str(),
                            newPath.toStdString().c_str());
        _material->setName(newName);
        item->setData(QVariant(newName), TreeNameRole);
    }
}

#include "moc_MaterialsEditor.cpp"
