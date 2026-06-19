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

#pragma once

#include <memory>

#include <yaml-cpp/yaml.h>

#include "Model.h"
#include "ModelLibrary.h"

namespace Materials
{

class ModelEntry
{
public:
    ModelEntry(const std::shared_ptr<ModelLibraryLocal>& library,
               const std::string& baseName,
               const std::string& modelName,
               const std::string& dir,
               const std::string& modelUuid,
               const YAML::Node& modelData);
    virtual ~ModelEntry() = default;

    std::shared_ptr<ModelLibraryLocal> getLibrary() const
    {
        return _library;
    }
    const std::string getBase() const
    {
        return _base;
    }
    const std::string getName() const
    {
        return _name;
    }
    const std::string getDirectory() const
    {
        return _directory;
    }
    const std::string getUUID() const
    {
        return _uuid;
    }
    const YAML::Node& getModel() const
    {
        return _model;
    }
    YAML::Node* getModelPtr()
    {
        return &_model;
    }

private:
    ModelEntry();

    std::shared_ptr<ModelLibraryLocal> _library;
    std::string _base;
    std::string _name;
    std::string _directory;
    std::string _uuid;
    YAML::Node _model;
};

class ModelLoader
{
public:
    ModelLoader(std::shared_ptr<std::multimap<std::string, std::shared_ptr<Model>>> modelMap);
    virtual ~ModelLoader() = default;

    static const std::string getUUIDFromPath(const std::string& path);

private:
    ModelLoader();

    std::string
    yamlValue(const YAML::Node& node, const std::string& key, const std::string& defaultValue);
    void addToTree(std::shared_ptr<ModelEntry> model);
    void showYaml(const YAML::Node& yaml) const;
    std::shared_ptr<ModelEntry> getModelFromPath(std::shared_ptr<ModelLibrary> library,
                                                 const std::string& path) const;
    void loadLibrary(std::shared_ptr<ModelLibraryLocal> library);
    void loadLibraries();

    static std::unique_ptr<std::map<std::string, std::shared_ptr<ModelEntry>>> _modelEntryMap;
    std::shared_ptr<std::multimap<std::string, std::shared_ptr<Model>>> _modelMap;
};

}  // namespace Materials