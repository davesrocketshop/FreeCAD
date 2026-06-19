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

#ifndef MATGUI_CLIPBOARDTEXT_H
#define MATGUI_CLIPBOARDTEXT_H

#include <QStandardItem>

namespace MatGui
{

class ClipboardText
{
public:
    explicit ClipboardText();
    ~ClipboardText() = default;

    void appendClip(QString text);
    QStandardItem* clipItem(QString text);
    void indent();
    void unindent();
    void resetClipboard();
    void resetIndent();

    QString getClipboardText() const;
    void copyToClipboard() const;

private:
    QString clipboardText;
    int clipboardIndent;
};


}  // namespace MatGui

#endif  // MATGUI_CLIPBOARDTEXT_H
