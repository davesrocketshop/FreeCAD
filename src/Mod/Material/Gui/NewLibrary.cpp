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

    setDefaults();

    connect(ui->radioLocal, &QPushButton::clicked, this, &NewLibrary::onLocal);
    connect(ui->fileLocal, &Gui::FileChooser::fileNameSelected, this, &NewLibrary::onLocalFolder);
    connect(ui->radioRemote, &QPushButton::clicked, this, &NewLibrary::onRemote);
    // connect(ui->checkReadOnly, &QCheckBox::checkStateChanged, this, &NewLibrary::onReadOnly);
    connect(ui->buttonChangeIcon, &QPushButton::clicked, this, &NewLibrary::onChangeIcon);
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

void NewLibrary::setRemoteList()
{
    auto libraries = getMaterialManager().getRemoteLibraries();
    setLibraryList(libraries);
}

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
}

void NewLibrary::onRemote(bool checked)
{
    Q_UNUSED(checked)

    setRemoteList();
}

void NewLibrary::onLocalFolder(const QString& filename)
{
    Q_UNUSED(filename)
}

void NewLibrary::onReadOnly(Qt::CheckState state)
{
    Q_UNUSED(state)
}

void NewLibrary::onChangeIcon(bool checked)
{
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

bool NewLibrary::checkRemoteName(const QString& name) const
{
    const auto& libraries = getMaterialManager().getRemoteLibraries();
    return checkLibraryName(name, libraries, tr("A remote library with that name already exists."));
}

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

    if (isLocal()) {
        return checkLocalName(name);
    }
    return checkRemoteName(name);
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
        auto directory = ui->fileLocal->fileName();
        getMaterialManager().createLocalLibrary(name, directory, _icon, isReadOnly());
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
        getMaterialManager().createLibrary(name, _icon, isReadOnly());
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

#include "moc_NewLibrary.cpp"
