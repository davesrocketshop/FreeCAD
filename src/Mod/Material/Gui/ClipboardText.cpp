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
#include <QApplication>

#include "ClipboardText.h"

using namespace MatGui;

/* TRANSLATOR MatGui::ClipboardText */

ClipboardText::ClipboardText()
    : clipboardIndent(0)
{}

void ClipboardText::appendClip(QString text)
{
    // Need to add indent
    QString indent(clipboardIndent * 4, QLatin1Char(' '));
    clipboardText += indent + text + QStringLiteral("\n");
}

QStandardItem* ClipboardText::clipItem(QString text)
{
    appendClip(text);
    auto item = new QStandardItem(text);
    return item;
}

void ClipboardText::indent()
{
    clipboardIndent += 1;
}

void ClipboardText::unindent()
{
    if (clipboardIndent > 0) {
        clipboardIndent -= 1;
    }
}

void ClipboardText::resetClipboard()
{
    clipboardText.clear();
    resetIndent();
}

void ClipboardText::resetIndent()
{
    clipboardIndent = 0;
}

QString ClipboardText::getClipboardText() const
{
    return clipboardText;
}

void ClipboardText::copyToClipboard() const
{
    QApplication::clipboard()->setText(clipboardText);
}
