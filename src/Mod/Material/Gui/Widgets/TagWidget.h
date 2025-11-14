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

#ifndef MATGUI_TAGWIDGET_H
#define MATGUI_TAGWIDGET_H

#include <QWidget>

namespace MatGui
{

struct Tag
{
    QString text;
    QRect rect;

    bool operator==(Tag const& rhs) const
    {
        return text == rhs.text && rect == rhs.rect;
    }
};
// typedef std::ranges::output_range<Tag> TagRange;

class TagWidget: public QAbstractScrollArea
{
    Q_OBJECT

public:
    explicit TagWidget(QWidget* parent = nullptr);
    ~TagWidget() override;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    int heightForWidth(int w) const override;

private:
    std::vector<Tag> tags {Tag {}};
    size_t editing_index {0};
    int blink_timer {0};

    /// Padding from the text to the the pill border
    static QMargins pill_thickness;

    /// Space between pills
    static int pills_h_spacing;

    /// Space between rows of pills (for multi line tags)
    static int tag_v_spacing;

    /// Size of cross side
    static qreal tag_cross_size;

    /// Distance between text and the cross
    static int tag_cross_spacing;

    /// Calculate the width that a tag would have with the given text width
    static int pillWidth(int text_width, bool has_cross);

    /// Calculate the height that a tag would have with the given text height
    static int pillHeight(int text_height);

    void calcRects(QRect r, QPoint& lt, QFontMetrics const& fm);
    QRect calcRects(QRect r);
    QRect calcRects();

    QString& editorText()
    {
        return tags[editing_index].text;
    }

    bool cursorVisible() const
    {
        return blink_timer;
    }

    template<std::ranges::output_range<Tag> Range>
    static void calcRects(
        QPoint& lt,
        Range&& tags,
        QFontMetrics const& fm,
        std::optional<QRect> const& fit,
        bool has_cross
    )
    {
        for (auto& tag : tags) {
            auto const text_width = fm.horizontalAdvance(tag.text);
            QRect rect(lt, QSize(pillWidth(text_width, has_cross), pillHeight(fm.height())));

            if (fit) {
                if (
                    fit->right() < rect.right() &&  // doesn't fit in current line
                    rect.left() != fit->left()      // doesn't occupy entire line already
                ) {
                    rect.moveTo(fit->left(), rect.bottom() + tag_v_spacing);
                    lt = rect.topLeft();
                }
            }

            tag.rect = rect;
            lt.setX(rect.right() + pills_h_spacing);
        }
    }

    template<std::ranges::output_range<Tag> Range>
    void calcRects(
        QPoint& lt,
        Range&& tags,
        QFontMetrics const& fm,
        std::optional<QRect> const& fit = std::nullopt
    ) const
    {
        calcRects(lt, tags, fm, fit, true);
    }
};

}  // namespace MatGui

#endif  // MATGUI_TAGWIDGET_H
