// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 David Carter <dcarter@david.carter.ca>             *
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
#include <ranges>
#include <vector>
#include <string_view>

#include "StringUtility.h"

using namespace Materials;

std::vector<std::string> Materials::split(std::string_view str, char delimiter)
{
    auto tokens_view = std::views::split(str, delimiter);
    std::vector<std::string> parts;

    for (const auto& token_range : tokens_view) {
        // Convert range to string or string_view for output/storage
        std::string_view token(token_range.begin(), token_range.end());
        parts.emplace_back(token);  // Efficiently adds string views to the vector
    }
    return parts;
}
