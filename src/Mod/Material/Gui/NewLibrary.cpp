// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 David Carter <dcarter@david.carter.ca>             *
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

#include <QClipboard>
#include <QMessageBox>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTreeView>


#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>
#include <Gui/Selection/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/WaitCursor.h>

#include <Mod/Material/App/MaterialLibrary.h>
#include <Mod/Material/App/PropertyMaterial.h>

#include "NewLibrary.h"
#include "ui_NewLibrary.h"


using namespace MatGui;

/* TRANSLATOR MatGui::NewLibrary */

NewLibrary::NewLibrary(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui_NewLibrary)
{
    ui->setupUi(this);

    ui->fileLocal->setMode(Gui::FileChooser::Mode::Directory);
    ui->labelImage->resize(64, 64);
    // This is to support multiple remote instances, so hide for now
    ui->comboRemote->setVisible(false);
#if !defined(BUILD_MATERIAL_EXTERNAL)
    ui->radioRemote->setVisible(false);
#endif

    setDefaults();

    connect(ui->radioLocal, &QPushButton::clicked, this, &NewLibrary::onLocal);
    connect(ui->fileLocal, &Gui::FileChooser::fileNameSelected, this, &NewLibrary::onLocalFolder);
#if defined(BUILD_MATERIAL_EXTERNAL)
    connect(ui->radioRemote, &QPushButton::clicked, this, &NewLibrary::onRemote);
#endif
    connect(ui->editName, &QLineEdit::textEdited, this, &NewLibrary::onNameEdited);
    // connect(ui->checkReadOnly, &QCheckBox::checkStateChanged, this, &NewLibrary::onReadOnly);
    connect(ui->buttonChangeIcon, &QPushButton::clicked, this, &NewLibrary::onChangeIcon);

    validateOk();
}

NewLibrary::~NewLibrary()
{
    // no need to delete child widgets, Qt does it all for us
    // but we can't use default as Ui_NewLibrary is undefined
}

bool NewLibrary::isLocal() const
{
    return ui->radioLocal->isChecked();
}

bool NewLibrary::isReadOnly() const
{
    return ui->checkReadOnly->isChecked();
}

void NewLibrary::setDefaults()
{
    setIcon(QStringLiteral(":/icons/freecad.svg"));
    setLocalList();

    ui->editName->setText(tr("New Library"));
}

void NewLibrary::setIcon(const QString& file)
{
    if (_icon != file) {
        _icon = file;

        QPixmap pixmap(file);
        ui->labelImage->setPixmap(pixmap);
    }
}

void NewLibrary::setLocalList()
{
    auto libraries = getMaterialManager().getLocalLibraries();
    setLibraryList(libraries);
}

#if defined(BUILD_MATERIAL_EXTERNAL)
void NewLibrary::setRemoteList()
{
    auto libraries = getMaterialManager().getRemoteLibraries();
    setLibraryList(libraries);
}
#endif

void NewLibrary::setLibraryList(
    const std::shared_ptr<std::list<std::shared_ptr<Materials::MaterialLibrary>>>& libraries
)
{
    ui->listLibraries->clear();
    for (const auto& library : *libraries) {
        // The constructor automatically adds the item to the list widget
        auto name = library->getName();
        if (library->isDisabled()) {
            name += tr(" - disabled");
        }
        auto item = new QListWidgetItem(name, ui->listLibraries);
        item->setFlags(Qt::NoItemFlags);
    }
}

void NewLibrary::onLocal(bool checked)
{
    Q_UNUSED(checked)

    setLocalList();

    validateOk();
}

#if defined(BUILD_MATERIAL_EXTERNAL)
void NewLibrary::onRemote(bool checked)
{
    Q_UNUSED(checked)

    setRemoteList();

    validateOk();
}
#endif

void NewLibrary::onLocalFolder(const QString& filename)
{
    Q_UNUSED(filename)

    validateOk();
}

void NewLibrary::onNameEdited(const QString& text)
{
    Q_UNUSED(text)

    validateOk();
}

void NewLibrary::onReadOnly(Qt::CheckState state)
{
    Q_UNUSED(state)
}

void NewLibrary::onChangeIcon(bool checked)
{
    Q_UNUSED(checked)
    
    QString prechosenDirectory = Gui::FileDialog::getWorkingDirectory();

    QFileDialog::Options dlgOpt;
    if (Gui::DialogOptions::dontUseNativeFileDialog()) {
        dlgOpt = QFileDialog::DontUseNativeDialog;
    }

    QString fn;
    QString filter;
    fn = QFileDialog::getOpenFileName(this, tr("Select a File"), prechosenDirectory, filter, nullptr, dlgOpt);

    if (!fn.isEmpty()) {
        fn = QDir::fromNativeSeparators(fn);
        setIcon(fn);
        Gui::FileDialog::setWorkingDirectory(fn);
        // Q_EMIT fileNameSelected(fn);
    }
}

bool NewLibrary::checkLocalName(const QString& name) const
{
    const auto& libraries = getMaterialManager().getLocalLibraries();
    return checkLibraryName(name, libraries, tr("A local library with that name already exists."));
}

#if defined(BUILD_MATERIAL_EXTERNAL)
bool NewLibrary::checkRemoteName(const QString& name) const
{
    const auto& libraries = getMaterialManager().getRemoteLibraries();
    return checkLibraryName(name, libraries, tr("A remote library with that name already exists."));
}
#endif

bool NewLibrary::checkLibraryName(
    const QString& name,
    const std::shared_ptr<std::list<std::shared_ptr<Materials::MaterialLibrary>>>& libraries,
    const QString& error
) const
{
    for (const auto& library : *libraries) {
        if (name == library->getName()) {
            QMessageBox msgBox;
            msgBox.setText(error);
            msgBox.exec();
            return false;
        }
    }
    return true;
}

bool NewLibrary::checkLibraryName(const QString& name) const
{

#if defined(BUILD_MATERIAL_EXTERNAL)
    if (!isLocal()) {
        return checkRemoteName(name);
    }
#endif
    return checkLocalName(name);
}

bool NewLibrary::createLibrary(const QString& name)
{
    try {
        if (isLocal()) {
            return createLocalLibrary(name);
        }
        return createRemoteLibrary(name);
    }
    catch (...) {
        QMessageBox msgBox;
        msgBox.setText(tr("An unknown exception occurred."));
        msgBox.exec();
        return false;
    }
    return false;
}

bool NewLibrary::createLocalLibrary(const QString& name)
{
    try {
        Gui::WaitCursor wc;
        auto directory = ui->fileLocal->fileName();
        getMaterialManager().createLocalLibrary(name, directory, _icon, isReadOnly());
        getMaterialManager().refresh();
    }
    catch (const Materials::CreationError& e) {
        QMessageBox msgBox;
        msgBox.setText(tr("Unable to create local library."));
        msgBox.exec();
        return false;
    }
    return true;
}

bool NewLibrary::createRemoteLibrary(const QString& name)
{
    try {
        Gui::WaitCursor wc;
        getMaterialManager().createLibrary(name, _icon, isReadOnly());
        getMaterialManager().refresh();
    }
    catch (const Materials::CreationError& e) {
        QMessageBox msgBox;
        msgBox.setText(tr("Unable to create remote library."));
        msgBox.exec();
        return false;
    }
    catch (const Materials::ConnectionError& e) {
        QMessageBox msgBox;
        msgBox.setText(tr("Unable to connect."));
        msgBox.exec();
        return false;
    }
    return true;
}

void NewLibrary::accept()
{
    // Validate the library name
    auto name = ui->editName->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox msgBox;
        msgBox.setText(tr("The library must have a name."));
        msgBox.exec();
        return;
    }

    if (!checkLibraryName(name)) {
        return;
    }

    // Validate the local directory

    // Create the library
    createLibrary(name);

    QDialog::accept();
}

void NewLibrary::setOkEnabled(bool enabled)
{
    auto ok = ui->standardButtons->button(QDialogButtonBox::Ok);
    ok->setEnabled(enabled);
}

void NewLibrary::enableOk()
{
    setOkEnabled(true);

}

void NewLibrary::disableOk()
{
    setOkEnabled(false);
}

void NewLibrary::validateOk()
{
    // Ensure our name is unique
    Base::Console().log("validateOk()\n");
    auto name = ui->editName->text();
    if (name.isEmpty()) {
        Base::Console().log("name is empty\n");
        disableOk();
        return;
    }

    if (isLocal()) {
        auto directory = ui->fileLocal->fileName();
        if (directory.isEmpty()) {
            Base::Console().log("directory is empty\n");
            disableOk();
            return;
        }
        QDir dir(directory);
        if (!dir.exists()) {
            Base::Console().log("directory doesn't exist\n");
            disableOk();
            return;
        }

        auto libraries = getMaterialManager().getLocalLibraries();
        for (auto library : *libraries) {
            if (library->getName() == name) {
                Base::Console().log("library name already exists\n");
                disableOk();
                return;
            }
        }
    }
#if defined(BUILD_MATERIAL_EXTERNAL)
    else {
        auto libraries = getMaterialManager().getRemoteLibraries();
        for (auto library : *libraries) {
            if (library->getName() == name) {
                Base::Console().log("remote library name already exists\n");
                disableOk();
                return;
            }
        }
    }
#endif

    Base::Console().log("Enable\n");
    enableOk();
}

#include "moc_NewLibrary.cpp"
