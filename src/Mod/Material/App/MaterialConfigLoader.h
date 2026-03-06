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
#include <map>

#include <QSettings>
#include <QVariant>

#include "Materials.h"

namespace Materials
{

class ManagedLibrary;
class MaterialLibraryLocal;

class MaterialConfigLoader
{
public:
    MaterialConfigLoader() = default;
    virtual ~MaterialConfigLoader() = default;


    static bool isConfigStyle(const std::string& path);
    static std::shared_ptr<Material> getMaterialFromPath(
        ManagedLibrary& library,
        const std::string& path
    );
    static std::shared_ptr<Material> getMaterialFromPath(
        const std::shared_ptr<MaterialLibraryLocal>& library,
        const std::string& path
    );
    static std::shared_ptr<Material> getMaterialFromPath(
        const std::string& path
    );

private:
    static std::string value(
        std::map<std::string, std::string>& fcmat,
        const std::string& name,
        const std::string& defaultValue
    )
    {
        try {
            return fcmat[name];
        }
        catch (const std::out_of_range&) {
        }

        return defaultValue;
    }

    static void setPhysicalValue(const std::shared_ptr<Material>& finalModel,
                                 const std::string& name,
                                 const std::string& value)
    {
        if (!value.empty()) {
            finalModel->setPhysicalValue(name, value);
        }
    }
    static void setAppearanceValue(const std::shared_ptr<Material>& finalModel,
                                   const std::string& name,
                                   const std::string& value)
    {
        if (!value.empty()) {
            finalModel->setAppearanceValue(name, value);
        }
    }
    static void setAppearanceValue(const std::shared_ptr<Material>& finalModel,
                                   const std::string& name,
                                   const std::shared_ptr<std::vector<QVariant>>& value)
    {
        if (!value->empty()) {
            finalModel->setAppearanceValue(name, value);
        }
    }
    static void setLegacyValue(const std::shared_ptr<Material>& finalModel,
                                   const std::string& name,
                                   const std::string& value)
    {
        if (!value.empty()) {
            finalModel->setLegacyValue(name, value);
        }
    }

    static bool isTexture(const std::string& value)
    {
        return value.find("Texture") != std::string::npos;
    }

    static bool readFile(const std::string& path, std::map<std::string, std::string>& map);
    static void splitTexture(const std::string& value, std::string* texture, std::string* remain);
    static void
    splitTextureObject(const std::string& value, std::string* texture, std::string* remain, std::string* object);

    static std::string getAuthorAndLicense(const std::string& path);
    static void addMechanical(std::map<std::string, std::string>& fcmat,
                              const std::shared_ptr<Material>& finalModel);
    static void addFluid(std::map<std::string, std::string>& fcmat,
                         const std::shared_ptr<Material>& finalModel);
    static void addThermal(std::map<std::string, std::string>& fcmat,
                           const std::shared_ptr<Material>& finalModel);
    static void addElectromagnetic(std::map<std::string, std::string>& fcmat,
                                   const std::shared_ptr<Material>& finalModel);
    static void addArchitectural(std::map<std::string, std::string>& fcmat,
                                 const std::shared_ptr<Material>& finalModel);
    static void addCosts(std::map<std::string, std::string>& fcmat,
                         const std::shared_ptr<Material>& finalModel);
    static void addRendering(std::map<std::string, std::string>& fcmat,
                             const std::shared_ptr<Material>& finalModel);
    static void addVectorRendering(std::map<std::string, std::string>& fcmat,
                                   const std::shared_ptr<Material>& finalModel);

    static std::string multiLineKey(std::map<std::string, std::string>& fcmat, const std::string& prefix);
    static void addRenderAppleseed(std::map<std::string, std::string>& fcmat,
                                   const std::shared_ptr<Material>& finalModel);
    static void addRenderCarpaint(std::map<std::string, std::string>& fcmat,
                                  const std::shared_ptr<Material>& finalModel);
    static void addRenderCycles(std::map<std::string, std::string>& fcmat,
                                const std::shared_ptr<Material>& finalModel);
    static void addRenderDiffuse(std::map<std::string, std::string>& fcmat,
                                 const std::shared_ptr<Material>& finalModel);
    static void addRenderDisney(std::map<std::string, std::string>& fcmat,
                                const std::shared_ptr<Material>& finalModel);
    static void addRenderEmission(std::map<std::string, std::string>& fcmat,
                                  const std::shared_ptr<Material>& finalModel);
    static void addRenderGlass(std::map<std::string, std::string>& fcmat,
                               const std::shared_ptr<Material>& finalModel);
    static void addRenderLuxcore(std::map<std::string, std::string>& fcmat,
                                 const std::shared_ptr<Material>& finalModel);
    static void addRenderLuxrender(std::map<std::string, std::string>& fcmat,
                                   const std::shared_ptr<Material>& finalModel);
    static void addRenderMixed(std::map<std::string, std::string>& fcmat,
                               const std::shared_ptr<Material>& finalModel);
    static void addRenderOspray(std::map<std::string, std::string>& fcmat,
                                const std::shared_ptr<Material>& finalModel);
    static void addRenderPbrt(std::map<std::string, std::string>& fcmat,
                              const std::shared_ptr<Material>& finalModel);
    static void addRenderPovray(std::map<std::string, std::string>& fcmat,
                                const std::shared_ptr<Material>& finalModel);
    static void addRenderSubstancePBR(std::map<std::string, std::string>& fcmat,
                                      const std::shared_ptr<Material>& finalModel);
    static void addRenderTexture(std::map<std::string, std::string>& fcmat,
                                 const std::shared_ptr<Material>& finalModel);
    static void addRenderWB(std::map<std::string, std::string>& fcmat,
                            const std::shared_ptr<Material>& finalModel);
    static void addLegacy(std::map<std::string, std::string>& fcmat,
                            const std::shared_ptr<Material>& finalModel);
};

}  // namespace Materials