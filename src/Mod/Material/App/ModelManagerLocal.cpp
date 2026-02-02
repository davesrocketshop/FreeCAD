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

#include <QDirIterator>
#include <QMutexLocker>

#include <Base/Console.h>

#include "Model.h"
#include "ModelLoader.h"
#include "ModelManager.h"
#include "ModelManagerLocal.h"

using namespace Materials;

std::shared_ptr<std::list<std::shared_ptr<ModelLibraryLocal>>> ModelManagerLocal::_libraryList = nullptr;
std::shared_ptr<std::multimap<std::string, std::shared_ptr<Model>>> ModelManagerLocal::_modelMap = nullptr;
QMutex ModelManagerLocal::_mutex;


TYPESYSTEM_SOURCE(Materials::ModelManagerLocal, Base::BaseClass)

ModelManagerLocal::ModelManagerLocal()
{
    initLibraries();
}

void ModelManagerLocal::initLibraries()
{
    QMutexLocker locker(&_mutex);

    if (_modelMap == nullptr) {
        _modelMap = std::make_shared<std::multimap<std::string, std::shared_ptr<Model>>>();
        if (_libraryList == nullptr) {
            _libraryList = std::make_shared<std::list<std::shared_ptr<ModelLibraryLocal>>>();
        }

        // Load the libraries
        ModelLoader loader(_modelMap, _libraryList);
    }
}

bool ModelManagerLocal::isModel(const std::string& file)
{
    // if (!fs::is_regular_file(p))
    //     return false;
    // check file extension
    if (file.ends_with(".yml")) {
        return true;
    }
    return false;
}

void ModelManagerLocal::cleanup()
{
    if (_libraryList) {
        _libraryList->clear();
    }

    if (_modelMap) {
        for (auto& it : *_modelMap) {
            // This is needed to resolve cyclic dependencies
            it.second->setLibrary(nullptr);
        }
        _modelMap->clear();
    }
}

void ModelManagerLocal::refresh()
{
    for (auto& it : *_modelMap) {
        // This is needed to resolve cyclic dependencies
        it.second->setLibrary(nullptr);
    }
    _modelMap->clear();
    _libraryList->clear();  // This list will be regenerated

    // Load the libraries
    ModelLoader loader(_modelMap, _libraryList);
}

// std::shared_ptr<std::list<std::shared_ptr<ModelLibrary>>> ModelManagerLocal::getLibraries()
// {
//     return reinterpret_cast<std::shared_ptr<std::list<std::shared_ptr<ModelLibrary>>>&>(_libraryList);
// }

// void ModelManagerLocal::renameLibrary(const std::string& libraryName, const std::string& newName)
// {
//     for (auto& library : *_libraryList) {
//         if (library->isName(libraryName.toStdString())) {
//             library->setName(newName.toStdString());
//             return;
//         }
//     }

//     throw LibraryNotFound();
// }

// void ModelManagerLocal::changeIcon(const std::string& libraryName, const std::string& icon)
// {
//     for (auto& library : *_libraryList) {
//         if (library->isName(libraryName.toStdString())) {
//             library->setIcon(icon.toStdString());
//             return;
//         }
//     }

//     throw LibraryNotFound();
// }

// void ModelManagerLocal::removeLibrary(const std::string& libraryName)
// {
//     for (auto& library : *_libraryList) {
//         if (library->isName(libraryName.toStdString())) {
//             _libraryList->remove(library);

//             // At this point we should rebuild the model map
//             return;
//         }
//     }

//     throw LibraryNotFound();
// }

std::shared_ptr<std::vector<LibraryObject>> ModelManagerLocal::libraryModels(const std::string& libraryName)
{
    auto models = std::make_shared<std::vector<LibraryObject>>();

    for (auto& it : *_modelMap) {
        // This is needed to resolve cyclic dependencies
        if (it.second->getLibrary()->isName(libraryName)) {
            models->push_back(LibraryObject(
                it.first,
                it.second->getDirectory().toStdString(),
                it.second->getName().toStdString()
            ));
        }
    }

    return models;
}

std::shared_ptr<std::map<std::string, std::shared_ptr<Model>>> ModelManagerLocal::getModels()
{
    auto localModels = std::make_shared<std::map<std::string, std::shared_ptr<Model>>>();
    for (auto& [name, model_ptr] : *_modelMap) {
        if (!model_ptr->isDisabled()) {
            localModels->try_emplace(name, model_ptr);
        }
    }
    return localModels;
}

// void ModelManagerLocal::setDisabled(Library& library, bool disabled)
// {
//     for (auto& modelLibrary : *_libraryList) {
//         if (modelLibrary->isLocal() && modelLibrary->isName(library.getName())) {
//             modelLibrary->setDisabled(disabled);
//             return;
//         }
//     }
//     // throw LibraryNotFound(); - There may be no models in this library, hence no model library object
// }

std::shared_ptr<Model> ModelManagerLocal::getModel(const std::string& uuid) const
{
    try {
        if (_modelMap == nullptr) {
            throw Uninitialized();
        }

        auto range = _modelMap->equal_range(uuid);
        for (auto it = range.first; it != range.second; it++) {
            auto& model = it->second;
            if (!model->isDisabled()) {
                ModelManager::dereference(model);
                return model;
            }
        }
        throw ModelNotFound();
    }
    catch (std::out_of_range const&) {
        throw ModelNotFound();
    }
}

std::shared_ptr<Model> ModelManagerLocal::getModelByPath(const std::string& path) const
{
    QString cleanPath = QDir::cleanPath(QString::fromStdString(path));

    for (auto& library : *_libraryList) {
        if (library->isLocal()) {
            auto localLibrary = std::static_pointer_cast<Materials::ModelLibraryLocal>(library);
            if (cleanPath.startsWith(QString::fromStdString(localLibrary->getDirectory()))) {
                auto model = localLibrary->getModelByPath(cleanPath.toStdString());
                ModelManager::dereference(model);
                return model;
            }
        }
    }

    throw ModelNotFound();
}

std::shared_ptr<Model> ModelManagerLocal::getModelByPath(const std::string& path, const std::string& lib) const
{
    auto library = getLibrary(lib);  // May throw LibraryNotFound
    if (library->isLocal()) {
        auto localLibrary = std::static_pointer_cast<Materials::ModelLibraryLocal>(library);
        auto model = localLibrary->getModelByPath(path);  // May throw ModelNotFound
        ModelManager::dereference(model);
        return model;
    }

    throw ModelNotFound();
}

std::shared_ptr<ModelLibrary> ModelManagerLocal::getLibrary(const std::string& name) const
{
    for (auto& library : *_libraryList) {
        if (library->isName(name)) {
            return library;
        }
    }

    throw LibraryNotFound();
}
