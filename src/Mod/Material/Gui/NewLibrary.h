// SPDX-License-Identifier: LGPL-2.1-or-later

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

#ifndef MATGUI_NEWLIBRARY_H
#define MATGUI_NEWLIBRARY_H

#include <memory>

#include <QStandardItem>
#include <QStandardItemModel>
#include <QTreeView>
#include <QDialog>

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>

#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/ModelManager.h>

namespace Gui
{
class ViewProvider;
}

namespace MatGui
{
class Ui_NewLibrary;

class NewLibrary: public QDialog
{
    Q_OBJECT

public:
    explicit NewLibrary(QWidget* parent = nullptr);
    ~NewLibrary() override;

    void accept() override;

private:
    std::unique_ptr<Ui_NewLibrary> ui;
    QString _icon;

    void onLocal(bool checked);
#if defined(BUILD_MATERIAL_EXTERNAL)
    void onRemote(bool checked);
#endif
    void onLocalFolder(const QString& filename);
    void onNameEdited(const QString& text);
    void onReadOnly(Qt::CheckState state);
    void onChangeIcon(bool checked);

    bool isLocal() const;
    bool isReadOnly() const;

    void setDefaults();
    void setIcon(const QString& file);
    void setLocalList();
#if defined(BUILD_MATERIAL_EXTERNAL)
    void setRemoteList();
#endif
    void setLibraryList(
        const std::shared_ptr<std::list<std::shared_ptr<Materials::MaterialLibrary>>>& libraries
    );

    bool checkLocalName(const QString& name) const;
#if defined(BUILD_MATERIAL_EXTERNAL)
    bool checkRemoteName(const QString& name) const;
#endif
    bool checkLibraryName(
        const QString& name,
        const std::shared_ptr<std::list<std::shared_ptr<Materials::MaterialLibrary>>>& libraries,
        const QString& error
    ) const;
    bool checkLibraryName(const QString& name) const;

    bool createLibrary(const QString& name);
    bool createLocalLibrary(const QString& name);
    bool createRemoteLibrary(const QString& name);

    void setOkEnabled(bool enabled);
    void enableOk();
    void disableOk();
    void validateOk();

    Materials::MaterialManager& getMaterialManager() const
    {
        return Materials::MaterialManager::getManager();
    }
};

}  // namespace MatGui

#endif  // MATGUI_NEWLIBRARY_H
