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

#include <QCompleter>
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
    Q_PROPERTY(std::vector<QString> tags READ getTags WRITE setTags RESET clear)

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

    /// Set list of completions
    void setCompletions(std::vector<QString> const& completions);

Q_SIGNALS:
    void tagsEdited();

protected:
    // QWidget
    void paintEvent(QPaintEvent* event) override;
    void timerEvent(QTimerEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    std::vector<Tag> _tags {Tag {}};
    size_t _editingIndex {0};
    int _blinkTimer {0};
    bool _blinkStatus {true};
    int _cursor {0};
    int _selectStart {0};
    int _selectSize {0};
    QTextLayout _textLayout;
    std::unique_ptr<QCompleter> _completer {new QCompleter {}};
    std::chrono::steady_clock::time_point _focusedAt {};

    // Behaviour config
    bool _restoreCursorPositionOnFocusClick = false;
    bool _readOnly = false;

    /// Padding from the text to the the pill border
    static QMargins _pillThickness;

    /// Space between pills
    static int _pillsHorizontalSpacing;

    /// Space between rows of pills (for multi line tags)
    static int _tagVerticalSpacing;

    /// Size of cross side
    static qreal _tagCrossSize;

    /// Distance between text and the cross
    static int _tagCrossSpacing;

    /// Rounding of the pill
    static qreal _roundingXRadius;

    /// Rounding of the pill
    static qreal _roundingYRadius;

    /// Maintain only unique tags
    static bool _uniqueTagsOnly;

    static QColor _tagColor;

    /// Calculate the width that a tag would have with the given text width
    static int pillWidth(int textWidth, bool hasCross);

    /// Calculate the height that a tag would have with the given text height
    static int pillHeight(int textHeight);

    void _setTags(std::vector<QString> const& tags);
    bool isCurrentTagADuplicate() const;
    void setupCompleter();

    qreal cursorToX();
    void moveCursor(int pos, bool mark);

    void deselectAll();
    bool hasSelection() const noexcept;
    void selectAll();
    void removeSelection();
    void removeBackwardOne();
    void removeDuplicates();
    void removeDuplicates(std::vector<Tag>& tags);

    void calcRects(QRect r, QPoint& leftTop, QFontMetrics const& metrics);
    QRect calcRects(QRect r);
    QRect calcRects();
    void calcRectsUpdateScrollRanges();
    void updateVScrollRange();
    void updateHScrollRange();
    void updateDisplayText();
    void setCursorVisible(bool visible);
    void ensureCursorIsVisibleV();
    void ensureCursorIsVisibleH();
    void updateTagDisplay(bool keep_cursor_visible = true);
    void drawEditor(QPainter& painter, QPalette const& palette, QPoint const& offset) const;
    QVector<QTextLayout::FormatRange> formatting(QPalette const& palette) const;
    void setEditorIndex(size_t i);
    void editNewTag(size_t i);
    void editPreviousTag();
    void editNextTag();
    void editTag(size_t i);
    void removeTag(size_t i);
    bool isAcceptableInput(QKeyEvent const& event);
    void setEditorText(QString const& text);

    void updateCursorBlinking()
    {
        setCursorVisible(_blinkTimer);
    }

    auto elapsed(std::chrono::steady_clock::time_point const& ts)
    {
        return std::chrono::steady_clock::now() - ts;
    }

    QPoint offset() const
    {
        return QPoint {horizontalScrollBar()->value(), verticalScrollBar()->value()};
    }

    bool inCrossArea(size_t tag_index, QPoint const& point, QPoint const& offset) const
    {
        return crossRect(_tags[tag_index].rect).adjusted(-1, -1, 1, 1).translated(-offset).contains(point)
            && (!cursorVisible() || tag_index != _editingIndex);
    }

    QRect const& editorRect() const
    {
        return _tags[_editingIndex].rect;
    }

    QString& editorText()
    {
        return _tags[_editingIndex].text;
    }

    QString const& editorText() const
    {
        return _tags[_editingIndex].text;
    }

    bool cursorVisible() const
    {
        return _blinkTimer;
    }

    template<std::ranges::output_range<Tag> Range>
    static void calcRects(
        QPoint& leftTop,
        Range&& tags,
        QFontMetrics const& metrics,
        std::optional<QRect> const& fit,
        bool hasCross
    )
    {
        for (auto& tag : tags) {
            auto const text_width = metrics.horizontalAdvance(tag.text);
            QRect rect(leftTop, QSize(pillWidth(text_width, hasCross), pillHeight(metrics.height())));

            if (fit) {
                if (
                    fit->right() < rect.right() &&  // doesn't fit in current line
                    rect.left() != fit->left()      // doesn't occupy entire line already
                ) {
                    rect.moveTo(fit->left(), rect.bottom() + _tagVerticalSpacing);
                    leftTop = rect.topLeft();
                }
            }

            tag.rect = rect;
            leftTop.setX(rect.right() + _pillsHorizontalSpacing);
        }
    }

    template<std::ranges::output_range<Tag> Range>
    void calcRects(
        QPoint& leftTop,
        Range&& tags,
        QFontMetrics const& metrics,
        std::optional<QRect> const& fit = std::nullopt
    ) const
    {
        calcRects(leftTop, tags, metrics, fit, true);
    }

    template<std::ranges::input_range Range>
    static void drawTags(
        QPainter& painter,
        Range&& tags,
        QFontMetrics const& metrics,
        QPoint const& offset,
        bool hasCross
    )
    {
        for (auto const& tag : tags) {
            QRect const& i_r = tag.rect.translated(offset);
            auto const text_pos = i_r.topLeft()
                + QPointF(_pillThickness.left(),
                          metrics.ascent() + ((i_r.height() - metrics.height()) / 2));

            // draw tag rect
            QPainterPath path;
            path.addRoundedRect(i_r, _roundingXRadius, _roundingYRadius);
            painter.fillPath(path, _tagColor);

            // draw text
            painter.drawText(text_pos, tag.text);

            if (hasCross) {
                auto const i_cross_r = crossRect(i_r, _tagCrossSize);

                QPen pen = painter.pen();
                pen.setWidth(2);

                painter.save();
                painter.setPen(pen);
                painter.setRenderHint(QPainter::Antialiasing);
                painter.drawLine(QLineF(i_cross_r.topLeft(), i_cross_r.bottomRight()));
                painter.drawLine(QLineF(i_cross_r.bottomLeft(), i_cross_r.topRight()));
                painter.restore();
            }
        }
    }

    template<std::ranges::input_range Range>
    void drawTags(QPainter& painter, Range range) const
    {
        drawTags(painter, range, fontMetrics(), -offset(), !_readOnly);
    }

    static QRectF crossRect(QRectF const& rect, qreal crossSize)
    {
        QRectF cross(QPointF {0, 0}, QSizeF {crossSize, crossSize});
        cross.moveCenter(QPointF(rect.right() - crossSize, rect.center().y()));
        return cross;
    }

    QRectF crossRect(QRectF const& rect) const
    {
        return crossRect(rect, _tagCrossSize);
    }
};

}  // namespace MatGui

#endif  // MATGUI_TAGWIDGET_H
