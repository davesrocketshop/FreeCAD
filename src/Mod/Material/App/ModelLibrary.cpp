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

#include <string>
#include <QLatin1Char>

#include <QFileInfo>

#include <App/Application.h>

#include "Exceptions.h"
#include "Model.h"
#include "ModelLibrary.h"
#include "ModelManager.h"


using namespace Materials;

TYPESYSTEM_SOURCE(Materials::ModelLibrary, Materials::Library)

ModelLibrary::ModelLibrary()
{}

ModelLibrary::ModelLibrary(const std::shared_ptr<ManagedLibrary>& library)
    : Library(library)
{}

ModelLibrary::ModelLibrary(const Library& other)
    : Library(other)
{}

bool ModelLibrary::isRoot(const std::string& path) const
{
    std::string localPath = getLocalPath(cleanPath(path));
    std::string clean = getLocalPath("");
    return (clean == localPath);
}

std::string ModelLibrary::getLocalPath(const std::string& path) const
{
    return Library::getLocalPath(getModelDirectoryPath(), path);
}

std::shared_ptr<std::map<std::string, std::shared_ptr<ModelTreeNode>>>
ModelLibrary::getModelTree(ModelFilter filter) const
{
    std::shared_ptr<std::map<std::string, std::shared_ptr<ModelTreeNode>>> modelTree =
        std::make_shared<std::map<std::string, std::shared_ptr<ModelTreeNode>>>();

    auto models = ModelManager::getManager().libraryModels(getName());
    for (auto& it : *models) {
        auto uuid = it.getUUID();
        auto path = it.getPath();
        auto filename = it.getName();

        auto model = ModelManager::getManager().getModel(getName(), uuid);
        if (ModelManager::passFilter(filter, model->getType())) {
            QStringList list = QString::fromStdString(path).split(QLatin1Char('/'));

            // Start at the root
            std::shared_ptr<std::map<std::string, std::shared_ptr<ModelTreeNode>>> node = modelTree;
            for (auto& itp : list) {
                // Add the folder only if it's not already there
                if (!node->contains(itp.toStdString())) {
                    auto mapPtr =
                        std::make_shared<std::map<std::string, std::shared_ptr<ModelTreeNode>>>();
                    std::shared_ptr<ModelTreeNode> child = std::make_shared<ModelTreeNode>();
                    child->setFolder(mapPtr);
                    (*node)[itp.toStdString()] = child;
                    node = mapPtr;
                }
                else {
                    node = (*node)[itp.toStdString()]->getFolder();
                }
            }
            std::shared_ptr<ModelTreeNode> child = std::make_shared<ModelTreeNode>();
            child->setUUID(uuid);
            child->setData(model);
            (*node)[filename] = child;
        }
    }

    return modelTree;
}

TYPESYSTEM_SOURCE(Materials::ModelLibraryLocal, Materials::ModelLibrary)

ModelLibraryLocal::ModelLibraryLocal()
{
    setLocal(true);

    _modelPathMap = std::make_shared<std::map<std::string, std::shared_ptr<Model>>>();
}

ModelLibraryLocal::ModelLibraryLocal(const std::shared_ptr<ManagedLibrary>& library)
    : ModelLibrary(library)
{
    setLocal(true);

    _modelPathMap = std::make_shared<std::map<std::string, std::shared_ptr<Model>>>();
}

ModelLibraryLocal::ModelLibraryLocal(const Library& other)
    : ModelLibrary(other)
{
    setLocal(true);

    _modelPathMap = std::make_shared<std::map<std::string, std::shared_ptr<Model>>>();
}

std::shared_ptr<Model> ModelLibraryLocal::getModelByPath(const std::string& path) const
{
    std::string filePath = getRelativePath(path);
    try {
        std::shared_ptr<Model> model = _modelPathMap->at(filePath);
        return model;
    }
    catch (std::out_of_range&) {
        throw ModelNotFound();
    }
}

std::shared_ptr<Model> ModelLibraryLocal::addModel(const Model& model, const std::string& path)
{
    std::string filePath = getRelativePath(path);
    QFileInfo info(QString::fromStdString(filePath));
    std::shared_ptr<Model> newModel = std::make_shared<Model>(model);
    newModel->setLibrary(getptr());
    newModel->setDirectory(QString::fromStdString(getLibraryPath(filePath, info.fileName().toStdString())));
    newModel->setFilename(info.fileName());

    (*_modelPathMap)[filePath] = newModel;

    return newModel;
}
