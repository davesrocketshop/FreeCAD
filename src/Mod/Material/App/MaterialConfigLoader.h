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

#ifndef MATERIAL_MATERIALCONFIGLOADER_H
#define MATERIAL_MATERIALCONFIGLOADER_H

#include <memory>

#include <QDir>
#include <QList>
#include <QMap>
#include <QSettings>
#include <QString>
#include <QVariant>

#include "Materials.h"

namespace Materials
{

class MaterialLibraryLocal;

class MaterialConfigLoader
{
public:
    MaterialConfigLoader() = default;
    virtual ~MaterialConfigLoader() = default;


    static bool isConfigStyle(const std::string& path);
    static std::shared_ptr<Material>
    getMaterialFromPath(const std::shared_ptr<MaterialLibraryLocal>& library, const std::string& path);

private:
    static std::string value(const QMap<std::string, std::string>& fcmat,
                         const std::string& name,
                         const std::string& defaultValue)
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
            finalModel->setPhysicalValue(QString::fromStdString(name), QString::fromStdString(value));
        }
    }
    static void setAppearanceValue(const std::shared_ptr<Material>& finalModel,
                                   const std::string& name,
                                   const std::string& value)
    {
        if (!value.empty()) {
            finalModel->setAppearanceValue(QString::fromStdString(name), QString::fromStdString(value));
        }
    }
    static void setAppearanceValue(const std::shared_ptr<Material>& finalModel,
                                   const std::string& name,
                                   const std::shared_ptr<QList<QVariant>>& value)
    {
        if (!value->empty()) {
            finalModel->setAppearanceValue(QString::fromStdString(name), value);
        }
    }
    static void setLegacyValue(const std::shared_ptr<Material>& finalModel,
                                   const std::string& name,
                                   const std::string& value)
    {
        if (!value.empty()) {
            finalModel->setLegacyValue(QString::fromStdString(name), QString::fromStdString(value));
        }
    }

    static bool isTexture(const std::string& value)
    {
        return value.find("Texture") != std::string::npos;
    }

    static bool readFile(const std::string& path, QMap<std::string, std::string>& map);
    static void splitTexture(const std::string& value, std::string* texture, std::string* remain);
    static void
    splitTextureObject(const std::string& value, std::string* texture, std::string* remain, std::string* object);

    static std::string getAuthorAndLicense(const std::string& path);
    static void addMechanical(const QMap<std::string, std::string>& fcmat,
                              const std::shared_ptr<Material>& finalModel);
    static void addFluid(const QMap<std::string, std::string>& fcmat,
                         const std::shared_ptr<Material>& finalModel);
    static void addThermal(const QMap<std::string, std::string>& fcmat,
                           const std::shared_ptr<Material>& finalModel);
    static void addElectromagnetic(const QMap<std::string, std::string>& fcmat,
                                   const std::shared_ptr<Material>& finalModel);
    static void addArchitectural(const QMap<std::string, std::string>& fcmat,
                                 const std::shared_ptr<Material>& finalModel);
    static void addCosts(const QMap<std::string, std::string>& fcmat,
                         const std::shared_ptr<Material>& finalModel);
    static void addRendering(const QMap<std::string, std::string>& fcmat,
                             const std::shared_ptr<Material>& finalModel);
    static void addVectorRendering(const QMap<std::string, std::string>& fcmat,
                                   const std::shared_ptr<Material>& finalModel);

    static std::string multiLineKey(QMap<std::string, std::string>& fcmat, const std::string& prefix);
    static void addRenderAppleseed(QMap<std::string, std::string>& fcmat,
                                   const std::shared_ptr<Material>& finalModel);
    static void addRenderCarpaint(QMap<std::string, std::string>& fcmat,
                                  const std::shared_ptr<Material>& finalModel);
    static void addRenderCycles(QMap<std::string, std::string>& fcmat,
                                const std::shared_ptr<Material>& finalModel);
    static void addRenderDiffuse(QMap<std::string, std::string>& fcmat,
                                 const std::shared_ptr<Material>& finalModel);
    static void addRenderDisney(QMap<std::string, std::string>& fcmat,
                                const std::shared_ptr<Material>& finalModel);
    static void addRenderEmission(QMap<std::string, std::string>& fcmat,
                                  const std::shared_ptr<Material>& finalModel);
    static void addRenderGlass(QMap<std::string, std::string>& fcmat,
                               const std::shared_ptr<Material>& finalModel);
    static void addRenderLuxcore(QMap<std::string, std::string>& fcmat,
                                 const std::shared_ptr<Material>& finalModel);
    static void addRenderLuxrender(QMap<std::string, std::string>& fcmat,
                                   const std::shared_ptr<Material>& finalModel);
    static void addRenderMixed(QMap<std::string, std::string>& fcmat,
                               const std::shared_ptr<Material>& finalModel);
    static void addRenderOspray(QMap<std::string, std::string>& fcmat,
                                const std::shared_ptr<Material>& finalModel);
    static void addRenderPbrt(QMap<std::string, std::string>& fcmat,
                              const std::shared_ptr<Material>& finalModel);
    static void addRenderPovray(QMap<std::string, std::string>& fcmat,
                                const std::shared_ptr<Material>& finalModel);
    static void addRenderSubstancePBR(QMap<std::string, std::string>& fcmat,
                                      const std::shared_ptr<Material>& finalModel);
    static void addRenderTexture(QMap<std::string, std::string>& fcmat,
                                 const std::shared_ptr<Material>& finalModel);
    static void addRenderWB(QMap<std::string, std::string>& fcmat,
                            const std::shared_ptr<Material>& finalModel);
    static void addLegacy(const QMap<std::string, std::string>& fcmat,
                            const std::shared_ptr<Material>& finalModel);
};

}  // namespace Materials

#endif  // MATERIAL_MATERIALCONFIGLOADER_H
