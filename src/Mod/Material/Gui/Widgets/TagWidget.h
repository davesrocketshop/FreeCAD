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

    /// Set tags
    void setTags(std::vector<QString> const& tags);

    /// Get tags
    std::vector<QString> getTags() const;

    /// Clear tags
    void clear();

protected:
    // QWidget
    void paintEvent(QPaintEvent* event) override;
    void timerEvent(QTimerEvent* event) override;
    // void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    // void keyPressEvent(QKeyEvent* event) override;
    // void mouseMoveEvent(QMouseEvent* event) override;

private:
    std::vector<Tag> tags {Tag {}};
    size_t editing_index {0};
    int blink_timer {0};
    bool blink_status {true};
    int cursor {0};
    int select_start {0};
    int select_size {0};
    QTextLayout text_layout;
    std::chrono::steady_clock::time_point focused_at {};

    // Behaviour config
    bool restore_cursor_position_on_focus_click = false;
    bool read_only = false;

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

    /// Rounding of the pill
    static qreal rounding_x_radius;

    /// Rounding of the pill
    static qreal rounding_y_radius;

    /// Maintain only unique tags
    static bool unique;

    static QColor color;

    /// Calculate the width that a tag would have with the given text width
    static int pillWidth(int text_width, bool has_cross);

    /// Calculate the height that a tag would have with the given text height
    static int pillHeight(int text_height);

    void _setTags(std::vector<QString> const& tags);
    bool isCurrentTagADuplicate() const;

    qreal cursorToX();
    void moveCursor(int pos, bool mark);

    void deselectAll();
    bool hasSelection() const noexcept;
    void selectAll();
    void removeSelection();

    void calcRects(QRect r, QPoint& lt, QFontMetrics const& fm);
    QRect calcRects(QRect r);
    QRect calcRects();
    void calcRectsUpdateScrollRanges();
    void updateVScrollRange();
    void updateHScrollRange();
    void updateDisplayText();
    void setCursorVisible(bool visible, QObject* ifce);
    void ensureCursorIsVisibleV();
    void ensureCursorIsVisibleH();
    void update1(bool keep_cursor_visible = true);
    void drawEditor(QPainter& p, QPalette const& palette, QPoint const& offset) const;
    QVector<QTextLayout::FormatRange> formatting(QPalette const& palette) const;

    void updateCursorBlinking(QObject* ifce)
    {
        setCursorVisible(blink_timer, ifce);
    }

    QPoint offset() const
    {
        return QPoint {horizontalScrollBar()->value(), verticalScrollBar()->value()};
    }

    QRect const& editorRect() const
    {
        return tags[editing_index].rect;
    }

    QString editorText()
    {
        return tags[editing_index].text;
    }

    QString editorText() const
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

    template<std::ranges::input_range Range>
    static void drawTags(
        QPainter& p,
        Range&& tags,
        QFontMetrics const& fm,
        QPoint const& offset,
        bool has_cross
    )
    {
        for (auto const& tag : tags) {
            QRect const& i_r = tag.rect.translated(offset);
            auto const text_pos = i_r.topLeft()
                + QPointF(pill_thickness.left(),
                          fm.ascent() + ((i_r.height() - fm.height()) / 2));

            // draw tag rect
            QPainterPath path;
            path.addRoundedRect(i_r, rounding_x_radius, rounding_y_radius);
            p.fillPath(path, color);

            // draw text
            p.drawText(text_pos, tag.text);

            if (has_cross) {
                auto const i_cross_r = crossRect(i_r, tag_cross_size);

                QPen pen = p.pen();
                pen.setWidth(2);

                p.save();
                p.setPen(pen);
                p.setRenderHint(QPainter::Antialiasing);
                p.drawLine(QLineF(i_cross_r.topLeft(), i_cross_r.bottomRight()));
                p.drawLine(QLineF(i_cross_r.bottomLeft(), i_cross_r.topRight()));
                p.restore();
            }
        }
    }

    template<std::ranges::input_range Range>
    void drawTags(QPainter& p, Range range) const
    {
        drawTags(p, range, fontMetrics(), -offset(), !read_only);
    }

    static QRectF crossRect(QRectF const& r, qreal cross_size)
    {
        QRectF cross(QPointF {0, 0}, QSizeF {cross_size, cross_size});
        cross.moveCenter(QPointF(r.right() - cross_size, r.center().y()));
        return cross;
    }

    QRectF crossRect(QRectF const& r) const
    {
        return crossRect(r, tag_cross_size);
    }
};

}  // namespace MatGui

#endif  // MATGUI_TAGWIDGET_H
