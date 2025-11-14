/***************************************************************************
 *   Copyright (c) 2025 David Carter <dcarter@david.carter.ca>             *
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

#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include "TagWidget.h"

using namespace MatGui;

/* TRANSLATOR MatGui::MaterialsEditor */

// Initialize static variables
QMargins TagWidget::pill_thickness = {7, 7, 8, 7};
int TagWidget::pills_h_spacing = 7;
int TagWidget::tag_v_spacing = 2;
qreal TagWidget::tag_cross_size = 8;
int TagWidget::tag_cross_spacing = 3;

TagWidget::TagWidget(QWidget* parent)
    : QAbstractScrollArea(parent)
{
    // ui->setupUi(this);
    QSizePolicy size_policy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    size_policy.setHeightForWidth(true);
    setSizePolicy(size_policy);

    setFocusPolicy(Qt::StrongFocus);
    viewport()->setCursor(Qt::IBeamCursor);
    setAttribute(Qt::WA_InputMethodEnabled, true);
    setMouseTracking(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // impl->setupCompleter();
    // impl->setCursorVisible(hasFocus(), this);
    // impl->updateDisplayText();

    viewport()->setContentsMargins(1, 1, 1, 1);
}

/*
 *  Destroys the object and frees any allocated resources
 */
TagWidget::~TagWidget() = default;
// {
//     // no need to delete child widgets, Qt does it all for us
//     // but we can't use default as Ui_PropertiesWidget is undefined
// }

/// Calculate the width that a tag would have with the given text width
int TagWidget::pillWidth(int text_width, bool has_cross)
{
    return text_width + pill_thickness.left()
        + (has_cross ? (tag_cross_spacing + tag_cross_size) : 0) + pill_thickness.right();
}

/// Calculate the height that a tag would have with the given text height
int TagWidget::pillHeight(int text_height)
{
    return text_height + pill_thickness.top() + pill_thickness.bottom();
}

void TagWidget::calcRects(QRect r, QPoint& lt, QFontMetrics const& fm)
{
    auto const middle = tags.begin() + static_cast<ptrdiff_t>(editing_index);

    calcRects(lt, std::ranges::subrange(tags.begin(), middle), fm, r);

    if (cursorVisible() || !editorText().isEmpty()) {
        calcRects(lt, std::ranges::subrange(middle, middle + 1), fm, r);
    }

    calcRects(lt, std::ranges::subrange(middle + 1, tags.end()), fm, r);
}

QRect TagWidget::calcRects(QRect r)
{
    auto lt = r.topLeft();
    QFontMetrics fm = fontMetrics();
    calcRects(r, lt, fm);
    r.setBottom(lt.y() + pillHeight(fm.height()) - 1);
    return r;
}

QRect TagWidget::calcRects()
{
    return calcRects(contentsRect());
}

QSize TagWidget::sizeHint() const
{
    return minimumSizeHint();
}

QSize TagWidget::minimumSizeHint() const
{
    ensurePolished();
    QFontMetrics fm = fontMetrics();
    QRect rect(0, 0, pillWidth(fm.maxWidth(), true), pillHeight(fm.height()));
    rect += contentsMargins() + viewport()->contentsMargins() + viewportMargins();
    return rect.size();
}

int TagWidget::heightForWidth(int w) const
{
    auto const content_width = w;
    QRect contents_rect(0, 0, content_width, 100);
    contents_rect -= contentsMargins() + viewport()->contentsMargins() + viewportMargins();
    // auto tags = impl->tags;
    contents_rect = const_cast<TagWidget*>(this)->calcRects(contents_rect);
    contents_rect += contentsMargins() + viewport()->contentsMargins() + viewportMargins();
    return contents_rect.height();
}

#include "moc_TagWidget.cpp"
