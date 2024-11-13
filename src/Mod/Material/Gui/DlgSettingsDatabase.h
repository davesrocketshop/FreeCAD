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

#ifndef MATGUI_DLGSETTINGSDATABASE_H
#define MATGUI_DLGSETTINGSDATABASE_H

#include <Gui/PropertyPage.h>
#include <memory>


namespace MatGui
{
class Ui_DlgSettingsDatabase;

class DlgSettingsDatabase: public Gui::Dialog::PreferencePage
{
    Q_OBJECT

public:
    explicit DlgSettingsDatabase(QWidget* parent = nullptr);
    ~DlgSettingsDatabase() override;

protected:
    void saveSettings() override;
    void loadSettings() override;
    void changeEvent(QEvent* e) override;

private Q_SLOTS:
    void testConnection(bool);

private:
    QString toPerCent(double value) const;
    
    std::unique_ptr<Ui_DlgSettingsDatabase> ui;
};

}  // namespace MatGui

#endif  // MATGUI_DLGSETTINGSDATABASE_H
