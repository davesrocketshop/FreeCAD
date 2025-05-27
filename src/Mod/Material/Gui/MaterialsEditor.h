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

#include <QDialog>
#include <QDir>
#include <QIcon>
#include <QPoint>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QSvgWidget>
#include <QTreeView>

#include <Base/Handle.h>
#include <Base/Parameter.h>

#include <Mod/Material/App/MaterialFilter.h>
#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/Materials.h>
#include <Mod/Material/App/ModelManager.h>

#include "AppearancePreview.h"

namespace MatGui
{

class Ui_MaterialsEditor;

const int TreeDataRole = Qt::UserRole;
const int TreeFunctionRole = Qt::UserRole + 1;
const int TreeNameRole = Qt::UserRole + 2;

typedef enum
{
    TreeFunctionFavorites,
    TreeFunctionRecents,
    TreeFunctionLibrary,
    TreeFunctionFolder,
    TreeFunctionMaterial
} TreeFunctionType;

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
    explicit MaterialsEditor(std::shared_ptr<Materials::MaterialFilter> filter,
                             QWidget* parent = nullptr);
    explicit MaterialsEditor(QWidget* parent = nullptr);
    ~MaterialsEditor() override = default;

    static QIcon getIcon(const std::shared_ptr<Materials::MaterialLibrary>& library);
    static QIcon getIcon(const std::shared_ptr<Materials::ModelLibrary>& library);
    static QIcon getIcon(const std::shared_ptr<Materials::Library>& library);

    void onTreeItemChanged(QStandardItem* item);

    void onName(const QString& text);
    void onAuthor(const QString& text);
    void onLicense(const QString& text);
    void onSourceURL(const QString& text);
    void onSourceReference(const QString& text);
    void onDescription();

    void propertyChange(const QString& property, const QVariant& value);
    void onInheritNewMaterial(bool checked);
    void onNewMaterial(bool checked);
    void onFavourite(bool checked);
    void onURL(bool checked);
    void onPhysicalAdd(bool checked);
    void onPhysicalRemove(bool checked);
    void onAppearanceAdd(bool checked);
    void onAppearanceRemove(bool checked);
    void onOk(bool checked);
    void onCancel(bool checked);
    void onSave(bool checked);
    void accept() override;
    void reject() override;

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

    void updateMaterialAppearance();
    void updateMaterialProperties();
    void updateMaterialGeneral();
    void updateMaterial();
    void onSelectMaterial(const QItemSelection& selected, const QItemSelection& deselected);
    void onContextMenu(const QPoint& pos);

    bool isMaterialSelected() const
    {
        return _materialSelected;
    }
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
    AppearancePreview* _rendered;
    bool _materialSelected;
    std::list<QString> _favorites;
    std::list<QString> _recents;
    int _recentMax;
    QIcon _warningIcon;
    std::shared_ptr<Materials::MaterialFilter> _filter;
    Materials::MaterialFilterOptions _filterOptions;
    QStandardItem* _newItem;

    // Actions
    QModelIndex _actionIndex;
#if defined(BUILD_MATERIAL_EXTERNAL)
    QAction _actionNewRemoteLibrary;
    QIcon _actionNewRemoteLibraryIcon;
#endif
    QAction _actionNewLocalLibrary;
    QAction _actionNewFolder;
    QIcon _actionNewFolderIcon;
    QAction _actionNewMaterial;
    QIcon _actionNewMaterialIcon;
    QAction _actionFavorite;
    QAction _actionChangeIcon;

    void setup();
    void setupData();
    void setupDialogSize();
    void setupButtonIcons();
    void setupButtonConnections();
    void setupEditorCallbacks();
    void setupSelectionCallbacks();
    void setupContextMenus();
    void setupModelCallbacks();

    void saveWindow();
    void saveMaterialTreeChildren(const Base::Reference<ParameterGrp>& param,
                                  QTreeView* tree,
                                  QStandardItemModel* model,
                                  QStandardItem* item);
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

    const QStandardItemModel* getActionModel() const;
    QStandardItem* getActionItem();
    TreeFunctionType getActionFunction(const QStandardItem* item) const;
    TreeFunctionType getActionFunction();

    void favoriteContextMenu(QMenu& contextMenu);
    void recentContextMenu(QMenu& contextMenu);
    void libraryContextMenu(QMenu& contextMenu);
    void folderContextMenu(QMenu& contextMenu);
    void materialContextMenu(QMenu& contextMenu);

    QString getPath(const QStandardItem* item, const QString& path) const;
    QString getParentPath(const QStandardItem* item) const;
    QString getLibraryName(const QStandardItem* item) const;

    void onMenuNewLibrary(bool checked);
    void onMenuNewFolder(bool checked);
    void onMenuNewMaterial(bool checked);
    void onMenuChangeIcon(bool checked);
    void onInherit(bool checked);
    void onInheritNew(bool checked);

    void setMaterialDefaults();
    bool updateTexturePreview() const;
    bool updateMaterialPreview() const;
    void updatePreview() const;
    static QString getColorHash(const QString& colorString, int colorRange = 255);

    static void addExpanded(QTreeView* tree, QStandardItem* parent, QStandardItem* child);
    static void addExpanded(QTreeView* tree,
                            QStandardItem* parent,
                            QStandardItem* child,
                            const Base::Reference<ParameterGrp>& param);
    static void addExpanded(QTreeView* tree, QStandardItemModel* parent, QStandardItem* child);
    static void addExpanded(QTreeView* tree,
                            QStandardItemModel* parent,
                            QStandardItem* child,
                            const Base::Reference<ParameterGrp>& param);
    void addRecents(QStandardItem* parent);
    void addFavorites(QStandardItem* parent);
    void createPreviews();
    void createAppearanceTree();
    void createPhysicalTree();
    void createMaterialTree();
    void fillMaterialTree();
    void refreshMaterialTree();
    void addMaterials(
        QStandardItem& parent,
        const std::shared_ptr<std::map<QString, std::shared_ptr<Materials::MaterialTreeNode>>>
            modelTree,
        const QIcon& folderIcon,
        const QIcon& icon,
        const Base::Reference<ParameterGrp>& param);

    void renameLibrary(QStandardItem* item);
    void renameFolder(QStandardItem* item);
    void renameMaterial(QStandardItem* item);
    void discardIfNew();


    /* Indicates if we should show favourite materials
     */
    bool includeFavorites() const
    {
        return _filterOptions.includeFavorites();
    }
    void setIncludeFavorites(bool value)
    {
        _filterOptions.setIncludeFavorites(value);
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
    }

    /* Indicates if we should include empty libraries
     */
    bool includeEmptyLibraries() const
    {
        return _filterOptions.includeEmptyLibraries();
    }
    void setIncludeEmptyLibraries(bool value)
    {
        Base::Console().log("setIncludeEmptyLibraries(%s)\n", (value ? "true" : "false"));
        _filterOptions.setIncludeEmptyLibraries(value);
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
    }
};

}  // namespace MatGui

Q_DECLARE_METATYPE(MatGui::TreeFunctionType)

#endif  // MATGUI_MATERIALSEDITOR_H
