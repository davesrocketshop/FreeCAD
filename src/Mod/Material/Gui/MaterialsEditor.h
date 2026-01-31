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

#ifndef MATGUI_MATERIALSEDITOR_H
#define MATGUI_MATERIALSEDITOR_H

#include <memory>

#include <QAction>
#include <QDialog>
#include <QDir>
#include <QIcon>
#include <QPoint>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QSvgWidget>
#include <QTreeView>
#include <QWidget>

#include <Base/Handle.h>
#include <Base/Parameter.h>

#include <Mod/Material/App/MaterialFilter.h>
#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/Materials.h>
#include <Mod/Material/App/ModelManager.h>
#include <Mod/Material/Gui/Models/MaterialTreeModel.h>
#include <Mod/Material/Gui/Widgets/MaterialTreeItem.h>
#include <Mod/Material/Gui/Widgets/MaterialTreeView.h>

#include "AppearancePreview.h"

namespace MatGui
{

class Ui_MaterialsEditor;
class MaterialPropertiesWidget;
class PropertiesWidget;

typedef enum
{
    MaterialSave_Cancel,
    MaterialSave_Overwrite,
    MaterialSave_New,
    MaterialSave_Inherit
} MaterialSaveResult;

class ActionError: public Base::Exception
{
public:
    ActionError()
        : Base::Exception("Invalid action")
    {}
    explicit ActionError(const char* msg)
        : Base::Exception(msg)
    {}
    explicit ActionError(const QString& msg)
        : Base::Exception(msg.toStdString().c_str())
    {}
    ~ActionError() noexcept override = default;
};

class MaterialsEditor: public QDialog
{
    Q_OBJECT

public:
    explicit MaterialsEditor(Materials::MaterialFilter filter,
                             QWidget* parent = nullptr);
    explicit MaterialsEditor(QWidget* parent = nullptr);
    ~MaterialsEditor() override = default;

    static QIcon getIcon(const std::shared_ptr<Materials::MaterialLibrary>& library);
    static QIcon getIcon(const std::shared_ptr<Materials::ModelLibrary>& library);
    static QIcon getIcon(const std::shared_ptr<Materials::Library>& library);

    void onTreeItemChanged(QStandardItem* item);
    void onTreeItemDropped(Qt::DropAction action, QStandardItem* source, QStandardItem* destination);

    void onName(const QString& name);
    void onAuthor(const QString& author);
    void onLicense(const QString& license);
    void onSourceURL(const QString& url);
    void onSourceReference(const QString& reference);
    void onDescription(const QString& description);

    void onInheritNewMaterial(bool checked);
    void onNewMaterial(bool checked);
    void onFavourite(bool checked);
    void onAdvancedSearch(bool checked);
    void onPhysicalAdd();
    void onPhysicalRemove(const QString& propertyName);
    void onAppearanceAdd();
    void onAppearanceRemove(const QString& propertyName);
    void onOk(bool checked);
    void onCancel(bool checked);
    void onSave(bool checked);
    void accept() override;
    void reject() override;

    void setAdvancedSearchState(bool checked);
    void setAdvancedSearchState();

    Materials::MaterialManager& getMaterialManager()
    {
        return Materials::MaterialManager::getManager();
    }
    Materials::MaterialManager& getMaterialManager() const
    {
        return Materials::MaterialManager::getManager();
    }
    bool useExternal() const
    {
        return getMaterialManager().useExternal();
    }

    static QString libraryPath(const std::shared_ptr<Materials::Material>& material);

    void onSelectMaterial(const QItemSelection& selected, const QItemSelection& deselected);
    void onContextMenu(const QPoint& pos);

    std::shared_ptr<Materials::Material> getMaterial()
    {
        return _material;
    }

protected:
    int confirmSave(QWidget* parent);
    void saveMaterial();

private:
    std::unique_ptr<Ui_MaterialsEditor> ui;
    std::shared_ptr<Materials::Material> _material;
    std::list<QString> _favorites;
    std::list<QString> _recents;
    int _recentMax;
    QIcon _warningIcon;
    Materials::MaterialFilter _filter;
    Materials::MaterialFilterOptions _filterOptions;
    MaterialTreeItem* _newItem;

    // Actions
    QModelIndex _actionIndex;
    QAction _actionNewLibrary;
    QIcon _actionNewLibraryIcon;
    QAction _actionNewFolder;
    QIcon _actionNewFolderIcon;
    QAction _actionNewMaterial;
    QIcon _actionNewMaterialIcon;
    QAction _actionInheritMaterial;
    QIcon _actionInheritMaterialIcon;
    QAction _actionFavorite;
    QIcon _actionFavoriteIcon;
    QAction _actionChangeIcon;
    QAction _actionCut;
    QIcon _actionCutIcon;
    QAction _actionCopy;
    QIcon _actionCopyIcon;
    QAction _actionPaste;
    QIcon _actionPasteIcon;
    QAction _actionRename;
    QAction _actionDeleteMaterial;
    QAction _actionDeleteFolder;
    QAction _actionDeleteLibrary;
    QAction _actionEnableDisable;
    QAction _actionLibraryProperties;

    QAction _actionViewFavorites;
    QAction _actionViewRecent;
    QAction _actionViewFolders;
    QAction _actionViewLibraries;
    QAction _actionViewLegacy;
    QAction _actionViewDisabled;
    QAction _actionViewMasked;

    void setup();
    void setupStackedWidgets();
    void setupData();
    void restoreState();
    void setupButtonIcons();
    void setupButtonConnections();
    void setupEditorCallbacks();
    void setupSelectionCallbacks();
    void setupContextMenus();
    void createActions();
    void setupModelCallbacks();

    void setLibraryPropertyState();
    void setFolderPropertyState();
    void setMaterialPropertyState();

    void saveState();
    void saveMaterialTreeChildren(const Base::Reference<ParameterGrp>& param,
                                  MaterialTreeView* tree,
                                  MaterialTreeModel* model,
                                  MaterialTreeItem* item);
    void saveMaterialTree(const Base::Reference<ParameterGrp>& param);

    void oldFormatError();

    void getFavorites();
    void saveFavorites();
    void addFavorite(const QString& uuid);
    void removeFavorite(const QString& uuid);
    bool isFavorite(const QString& uuid) const;
    void favoriteActionAdd();
    void favoriteActionRemove();

    void getRecents();
    void saveRecents();
    void addRecent(const QString& uuid);
    bool isRecent(const QString& uuid) const;

    const MaterialTreeModel* getActionModel() const;
    MaterialTreeItem* getActionItem() const;
    TreeFunctionType getActionFunction() const;
    std::shared_ptr<Materials::MaterialLibrary> getItemAsLibrary(const MaterialTreeItem* item) const;
    std::shared_ptr<Materials::MaterialLibrary> getActionLibrary() const;
    std::shared_ptr<Materials::Material> getItemAsMaterial(const MaterialTreeItem* item) const;
    std::shared_ptr<Materials::Material> getActionMaterial() const;
    MaterialTreeItem* getItemFromRoot(TreeFunctionType function) const;
    MaterialTreeItem* getFavoritesItem() const;
    MaterialTreeItem* getRecentsItem() const;
    MaterialTreeItem* getItemFromLibrary(const Materials::Library& library) const;
    MaterialTreeItem* getItemFromMaterial(const Materials::Material& material) const;
    std::shared_ptr<Materials::MaterialLibrary> getLibraryForItem(const MaterialTreeItem* item) const;
    QString getDirectoryForItem(const MaterialTreeItem* item) const;

    void favoriteContextMenu(QMenu& contextMenu);
    void recentContextMenu(QMenu& contextMenu);
    void libraryContextMenu(QMenu& contextMenu);
    void folderContextMenu(QMenu& contextMenu);
    void materialContextMenu(QMenu& contextMenu);
    void defaultContextMenu(QMenu& contextMenu);
    void addViewMenu(QMenu& contextMenu);

    QString getPath(const MaterialTreeItem* item, const QString& path) const;
    QString getParentPath(const MaterialTreeItem* item) const;
    QString getLibraryName(const MaterialTreeItem* item) const;
    QString stripLeadingSeparator(const QString& filePath) const;

    void onMenuNewLibrary(bool checked);
    void onMenuEnableDisable(bool checked);
    void onMenuDeleteLibrary(bool checked);
    void onMenuNewFolder(bool checked);
    void onMenuDeleteFolder(bool checked);
    void onMenuNewMaterial(bool checked);
    void onMenuInheritMaterial(bool checked);
    void onMenuDeleteMaterial(bool checked);
    void onMenuChangeIcon(bool checked);
    void onInherit(bool checked);
    void onInheritNew(bool checked);

    void onMenuViewFavorites(bool checked);
    void onMenuViewRecent(bool checked);
    void onMenuViewFolders(bool checked);
    void onMenuViewLibraries(bool checked);
    void onMenuViewLegacy(bool checked);
    void onMenuViewDisabled(bool checked);
    void onMenuViewMasked(bool checked);

    void setMaterialDefaults();
    static QString getColorHash(const QString& colorString, int colorRange = 255);

    static void addExpanded(MaterialTreeView* tree, MaterialTreeItem* parent, MaterialTreeItem* child);
    static void addExpanded(MaterialTreeView* tree,
                            MaterialTreeItem* parent,
                            MaterialTreeItem* child,
                            const Base::Reference<ParameterGrp>& param);
    static void addExpanded(MaterialTreeView* tree, MaterialTreeModel* parent, MaterialTreeItem* child);
    static void addExpanded(MaterialTreeView* tree,
                            MaterialTreeModel* parent,
                            MaterialTreeItem* child,
                            const Base::Reference<ParameterGrp>& param);
    void addRecents(MaterialTreeItem* parent);
    void addFavorites(MaterialTreeItem* parent);
    void createMaterialTree();
    void fillMaterialTree();
    void refreshMaterialTree();
    void addMaterials(
        MaterialTreeItem& parent,
        const std::shared_ptr<std::map<QString, std::shared_ptr<Materials::MaterialTreeNode>>>
            modelTree,
        const QIcon& folderIcon,
        const QIcon& icon,
        const Base::Reference<ParameterGrp>& param);

    void renameLibrary(MaterialTreeItem* item);
    void renameFolder(MaterialTreeItem* item);
    void renameMaterial(MaterialTreeItem* item);
    void updateMaterialTreeName(const QString& name);
    void updateFavoritesRecentsName(MaterialTreeItem* parent, const QString& uuid, const QString& name);
    void updateRecentsName(const QString& uuid, const QString& name);
    void updateRecentsName();
    void updateFavoritesName(const QString& uuid, const QString& name);
    void updateFavoritesName();
    void discardIfNew();

    MaterialSaveResult overwriteOrCopy();


    /* Indicates if we should show favourite materials
     */
    bool includeFavorites() const
    {
        return _filterOptions.includeFavorites();
    }
    void setIncludeFavorites(bool value)
    {
        _filterOptions.setIncludeFavorites(value);
        _filterOptions.save();
    }

    /* Indicates if we should show recent materials
     */
    bool includeRecent() const
    {
        return _filterOptions.includeRecent();
    }
    void setIncludeRecent(bool value)
    {
        _filterOptions.setIncludeRecent(value);
        _filterOptions.save();
    }

    /* Indicates if we should include empty folders
     */
    bool includeEmptyFolders() const
    {
        return _filterOptions.includeEmptyFolders();
    }
    void setIncludeEmptyFolders(bool value)
    {
        _filterOptions.setIncludeEmptyFolders(value);
        _filterOptions.save();
    }

    /* Indicates if we should include empty libraries
     */
    bool includeEmptyLibraries() const
    {
        return _filterOptions.includeEmptyLibraries();
    }
    void setIncludeEmptyLibraries(bool value)
    {
        _filterOptions.setIncludeEmptyLibraries(value);
        _filterOptions.save();
    }

    /* Indicates if we should include materials in the older format
     */
    bool includeLegacy() const
    {
        return _filterOptions.includeLegacy();
    }
    void setIncludeLegacy(bool legacy)
    {
        _filterOptions.setIncludeLegacy(legacy);
        _filterOptions.save();
    }

    bool includeDisabled() const
    {
        return _filterOptions.includeDisabled();
    }

    void setIncludeDisabled(bool disabled)
    {
        _filterOptions.setIncludeDisabled(disabled);
        _filterOptions.save();
    }
    bool includeMasked() const
    {
        return _filterOptions.includeMasked();
    }
    void setIncludeMasked(bool masked)
    {
        _filterOptions.setIncludeMasked(masked);
        _filterOptions.save();
    }

    void updateMaterial();
    void setMaterialSelected(bool selected);
};

}  // namespace MatGui

Q_DECLARE_METATYPE(MatGui::TreeFunctionType)

#endif  // MATGUI_MATERIALSEDITOR_H
