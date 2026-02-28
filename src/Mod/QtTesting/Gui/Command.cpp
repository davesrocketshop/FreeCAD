/***************************************************************************
 *   Copyright (c) 2026 David Carter <dcarter@david.carter.ca>             *
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

#include <QApplication>
#include <QDesktopServices>
#include <QFileDialog>
#include <QUrl>

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>

#include "QtTestUtility.h"

// #include "Action.h"
// #include "Application.h"
// #include "BitmapFactory.h"
// #include "Dialogs/DlgMacroExecuteImp.h"
// #include "Dialogs/DlgMacroRecordImp.h"
// #include "Macro.h"
// #include "MainWindow.h"
// #include "PythonDebugger.h"


// using namespace QtTestingGui;


//===========================================================================
// Std_QtTestRecord
//===========================================================================
DEF_STD_CMD_A(StdCmdQtTestRecord)

StdCmdQtTestRecord::StdCmdQtTestRecord()
    : Command("Std_QtTestRecord")
{
    sGroup = "QtTest";
    sMenuText = QT_TR_NOOP("Record Qt Test");

    sToolTipText = QT_TR_NOOP("Opens a dialog to record Qt GUI interactions");
    sWhatsThis = "Std_QtTestRecord";
    sStatusTip = sToolTipText;
    sPixmap = "media-record";
    eType = 0;
}

void StdCmdQtTestRecord::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    auto mainWindow = Gui::getMainWindow();
    QString filename = QFileDialog::getSaveFileName(
        mainWindow,
        QStringLiteral("Test File Name"),
        QString(),
        QStringLiteral("XML Files (*.xml)")
    );
    if (!filename.isEmpty()) {
        Base::Console().log("Recording to file '%s'\n", filename.toStdString().c_str());
        QApplication::setActiveWindow(mainWindow);
        auto& testUtility = QtTestingGui::QtTestUtility::getTestUtility();
        // if (testUtility) {
            testUtility.recordTests(filename);
        // }
    }
}

bool StdCmdQtTestRecord::isActive()
{
    return true;
}

//===========================================================================
// Std_QtTestPlayback
//===========================================================================
DEF_STD_CMD_A(StdCmdQtTestPlayback)

StdCmdQtTestPlayback::StdCmdQtTestPlayback()
    : Command("Std_QtTestPlayback")
{
    sGroup = "QtTest";
    sMenuText = QT_TR_NOOP("Play back Qt Test");

    sToolTipText = QT_TR_NOOP("Opens a dialog to play back previously recorded Qt GUI interactions");
    sWhatsThis = "Std_QtTestPlayback";
    sStatusTip = sToolTipText;
    sPixmap = "media-playback-start";
    eType = 0;
}

void StdCmdQtTestPlayback::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    auto mainWindow = Gui::getMainWindow();
    QApplication::setActiveWindow(mainWindow);
    auto& testUtility = QtTestingGui::QtTestUtility::getTestUtility();
    // if (testUtility) {
        testUtility.openPlayerDialog();
    // }
}

bool StdCmdQtTestPlayback::isActive()
{
    return true;
}

// namespace QTTesting
// {

void CreateQtTestingCommands()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new StdCmdQtTestRecord());
    rcCmdMgr.addCommand(new StdCmdQtTestPlayback());
}

// }  // namespace QTTesting
