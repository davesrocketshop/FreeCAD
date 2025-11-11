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

#ifndef MATGUI_LIBRARYTYPE_H
#define MATGUI_LIBRARYTYPE_H

#include <QDialog>

namespace MatGui
{

class Ui_LibraryType;

class LibraryType: public QDialog
{
    Q_OBJECT

public:
    explicit LibraryType(QWidget* parent = nullptr);
    ~LibraryType() override;

    void onOk(bool checked);
    void onCancel(bool checked);

private:
    std::unique_ptr<Ui_LibraryType> ui;

};

}  // namespace MatGui

#endif  // MATGUI_LIBRARYTYPE_H
