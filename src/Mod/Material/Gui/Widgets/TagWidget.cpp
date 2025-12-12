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

#include <QStyleHints>

#include <Base/Console.h>

#include "TagWidget.h"

using namespace MatGui;

/* TRANSLATOR MatGui::MaterialsEditor */

// Initialize static variables
QMargins TagWidget::pill_thickness = {7, 7, 8, 7};
int TagWidget::pills_h_spacing = 7;
int TagWidget::tag_v_spacing = 2;
qreal TagWidget::tag_cross_size = 8;
int TagWidget::tag_cross_spacing = 3;
qreal TagWidget::rounding_x_radius = 5;
qreal TagWidget::rounding_y_radius = 5;
bool TagWidget::unique = true;
QColor TagWidget::color {255, 164, 100, 100};

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

    setupCompleter();
    setCursorVisible(hasFocus(), this);
    updateDisplayText();

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

void TagWidget::setupCompleter()
{
    completer->setWidget(this);
    QObject::connect(
        completer.get(),
        qOverload<QString const&>(&QCompleter::activated),
        [this](QString const& text) { setEditorText(text); }
    );
}

void TagWidget::resizeEvent(QResizeEvent* event)
{
    QAbstractScrollArea::resizeEvent(event);
    calcRectsUpdateScrollRanges();
}

void TagWidget::focusInEvent(QFocusEvent* event)
{
    QAbstractScrollArea::focusInEvent(event);
    focused_at = std::chrono::steady_clock::now();
    setCursorVisible(true, this);
    updateDisplayText();
    calcRectsUpdateScrollRanges();
    if (event->reason() != Qt::FocusReason::MouseFocusReason
        || restore_cursor_position_on_focus_click) {
        ensureCursorIsVisibleH();
        ensureCursorIsVisibleV();
    }
    viewport()->update();
}

void TagWidget::focusOutEvent(QFocusEvent* event)
{
    QAbstractScrollArea::focusOutEvent(event);
    setCursorVisible(false, this);
    updateDisplayText();
    calcRectsUpdateScrollRanges();
    viewport()->update();
}

void TagWidget::paintEvent(QPaintEvent* e)
{
    QAbstractScrollArea::paintEvent(e);

    QPainter p(viewport());

    p.setClipRect(contentsRect());

    auto const middle = tags.cbegin() + static_cast<ptrdiff_t>(editing_index);

    // tags
    drawTags(p, std::ranges::subrange(tags.cbegin(), middle));

    if (cursorVisible()) {
        drawEditor(p, palette(), offset());
    }
    else if (!editorText().isEmpty()) {
        drawTags(p, std::ranges::subrange(middle, middle + 1));
    }

    // tags
    drawTags(p, std::ranges::subrange(middle + 1, tags.cend()));
}

void TagWidget::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == blink_timer) {
        blink_status = !blink_status;
        viewport()->update();
    }
}

void TagWidget::mousePressEvent(QMouseEvent* event)
{
    // we don't want to change cursor position if this event is part of focusIn
    using namespace std::chrono_literals;
    if (restore_cursor_position_on_focus_click && elapsed(focused_at) < 1ms) {
        return;
    }

    bool keep_cursor_visible = true;
    // EVERLOAD_TAGS_SCOPE_EXIT
    // {
    //     update1(keep_cursor_visible);
    // };

    // remove or edit a tag
    for (size_t i = 0; i < tags.size(); ++i) {
        if (!tags[i].rect.translated(-offset()).contains(event->pos())) {
            continue;
        }

        if (inCrossArea(i, event->pos(), offset())) {
            removeTag(i);
            keep_cursor_visible = false;
        }
        else if (editing_index == i) {
            moveCursor(
                text_layout.lineAt(0).xToCursor(
                    (event->pos()
                     - (editorRect() - pill_thickness).translated(-offset()).topLeft())
                        .x()
                ),
                false
            );
        }
        else {
            editTag(i);
        }

        update1(keep_cursor_visible);
        return;
    }

    // add new tag closest to the cursor
    for (auto it = begin(tags); it != end(tags); ++it) {
        // find the row
        if (it->rect.translated(-offset()).bottom() < event->pos().y()) {
            continue;
        }

        // find the closest spot
        auto const row = it->rect.translated(-offset()).top();
        while (it != end(tags) && it->rect.translated(-offset()).top() == row
               && event->pos().x() > it->rect.translated(-offset()).left()) {
            ++it;
        }

        editNewTag(static_cast<size_t>(std::distance(begin(tags), it)));
        update1(keep_cursor_visible);
        return;
    }

    // append a new nag
    editNewTag(tags.size());
    update1(keep_cursor_visible);
}

void TagWidget::keyPressEvent(QKeyEvent* event)
{
    if (read_only) {
        return;
    }

    if (event == QKeySequence::SelectAll) {
        selectAll();
    }
    else if (event == QKeySequence::SelectPreviousChar) {
        moveCursor(text_layout.previousCursorPosition(cursor), true);
    }
    else if (event == QKeySequence::SelectNextChar) {
        moveCursor(text_layout.nextCursorPosition(cursor), true);
    }
    else {
        switch (event->key()) {
            case Qt::Key_Left:
                if (cursor == 0) {
                    editPreviousTag();
                }
                else {
                    moveCursor(text_layout.previousCursorPosition(cursor), false);
                }
                break;
            case Qt::Key_Right:
                if (cursor == editorText().size()) {
                    editNextTag();
                }
                else {
                    moveCursor(text_layout.nextCursorPosition(cursor), false);
                }
                break;
            case Qt::Key_Home:
                if (cursor == 0) {
                    editTag(0);
                }
                else {
                    moveCursor(0, false);
                }
                break;
            case Qt::Key_End:
                if (cursor == editorText().size()) {
                    editTag(tags.size() - 1);
                }
                else {
                    moveCursor(editorText().length(), false);
                }
                break;
            case Qt::Key_Backspace:
                if (!editorText().isEmpty()) {
                    removeBackwardOne();
                }
                else if (editing_index > 0) {
                    editPreviousTag();
                }
                break;
            case Qt::Key_Space:
                if (!editorText().isEmpty()) {
                    editNewTag(editing_index + 1);
                }
                break;
            default:
                if (isAcceptableInput(*event)) {
                    if (hasSelection()) {
                        removeSelection();
                    }
                    editorText().insert(cursor, event->text());
                    cursor = cursor + event->text().length();
                    break;
                }
                else {
                    event->setAccepted(false);
                    return;
                }
        }
    }

    update1();

    completer->setCompletionPrefix(editorText());
    completer->complete();

    Q_EMIT tagsEdited();
}

void TagWidget::mouseMoveEvent(QMouseEvent* event)
{
    for (size_t i = 0; i < tags.size(); ++i) {
        if (inCrossArea(i, event->pos(), offset())) {
            viewport()->setCursor(Qt::ArrowCursor);
            return;
        }
    }
    if (contentsRect().contains(event->pos())) {
        viewport()->setCursor(Qt::IBeamCursor);
    }
    else {
        QAbstractScrollArea::mouseMoveEvent(event);
    }
}

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

void TagWidget::calcRectsUpdateScrollRanges()
{
    calcRects();
    updateVScrollRange();
    updateHScrollRange();
}

void TagWidget::updateVScrollRange()
{
    if (tags.size() == 1 && tags.front().text.isEmpty()) {
        verticalScrollBar()->setRange(0, 0);
        return;
    }

    auto const fm = fontMetrics();
    auto const row_h = pillHeight(fm.height()) + tag_v_spacing;
    verticalScrollBar()->setPageStep(row_h);
    assert(!tags.empty());  // Invariant-1

    int top = tags.front().rect.top();
    int bottom = tags.back().rect.bottom();

    if (editing_index == 0 && !(cursorVisible() || !editorText().isEmpty())) {
        top = tags[1].rect.top();
    }
    else if (editing_index == tags.size() - 1 && !(cursorVisible() || !editorText().isEmpty())) {
        bottom = tags[tags.size() - 2].rect.bottom();
    }

    auto const h = bottom - top + 1;
    auto const contents_rect = contentsRect();

    if (contents_rect.height() < h) {
        verticalScrollBar()->setRange(0, h - contents_rect.height());
    }
    else {
        verticalScrollBar()->setRange(0, 0);
    }
}

void TagWidget::updateHScrollRange()
{
    assert(!tags.empty());  // Invariant-1
    auto const width = std::max_element(begin(tags), end(tags), [](auto const& x, auto const& y) {
                           return x.rect.width() < y.rect.width();
                       })->rect.width();

    auto const contents_rect_width = contentsRect().width();

    if (contents_rect_width < width) {
        horizontalScrollBar()->setRange(0, width - contents_rect_width);
    }
    else {
        horizontalScrollBar()->setRange(0, 0);
    }
}

void TagWidget::updateDisplayText()
{
    text_layout.clearLayout();
    text_layout.setText(editorText());
    text_layout.beginLayout();
    text_layout.createLine();
    text_layout.endLayout();
}

void TagWidget::setCursorVisible(bool visible, QObject* ifce)
{
    if (blink_timer) {
        killTimer(blink_timer);
        blink_timer = 0;
    }

    if (visible) {
        blink_status = true;
        int flashTime = QGuiApplication::styleHints()->cursorFlashTime();
        if (flashTime >= 2) {
            blink_timer = startTimer(flashTime / 2);
        }
    }
    else {
        blink_status = false;
    }
}

void TagWidget::ensureCursorIsVisibleV()
{
    if (!cursorVisible()) {
        return;
    }
    auto const fm = fontMetrics();
    auto const row_h = pillHeight(fm.height());
    auto const vscroll = verticalScrollBar()->value();
    auto const cursor_top = editorRect().topLeft() + QPoint(qRound(cursorToX()), 0);
    auto const cursor_bottom = cursor_top + QPoint(0, row_h - 1);
    auto const contents_rect = contentsRect().translated(0, vscroll);
    if (contents_rect.bottom() < cursor_bottom.y()) {
        verticalScrollBar()->setValue(cursor_bottom.y() - row_h);
    }
    else if (cursor_top.y() < contents_rect.top()) {
        verticalScrollBar()->setValue(cursor_top.y() - 1);
    }
}

void TagWidget::ensureCursorIsVisibleH()
{
    if (!cursorVisible()) {
        return;
    }
    auto const contents_rect = contentsRect().translated(horizontalScrollBar()->value(), 0);
    auto const cursor_x = (editorRect() - pill_thickness).left() + qRound(cursorToX());
    if (contents_rect.right() < cursor_x) {
        horizontalScrollBar()->setValue(cursor_x - contents_rect.width());
    }
    else if (cursor_x < contents_rect.left()) {
        horizontalScrollBar()->setValue(cursor_x - 1);
    }
}

void TagWidget::update1(bool keep_cursor_visible)
{
    updateDisplayText();
    calcRectsUpdateScrollRanges();
    if (keep_cursor_visible) {
        ensureCursorIsVisibleV();
        ensureCursorIsVisibleH();
    }
    updateCursorBlinking(this);
    viewport()->update();
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

void TagWidget::setTags(std::vector<QString> const& tags)
{
    Base::Console().log("setTags():\n");
    for (auto tag : tags) {
        Base::Console().log("\t%s\n", tag.toStdString().c_str());
    }
    _setTags(tags);
    update1();
}

std::vector<QString> TagWidget::getTags() const
{
    std::vector<QString> ret(tags.size());
    std::transform(tags.begin(), tags.end(), ret.begin(), [](Tag const& tag) {
        return tag.text;
    });
    assert(!ret.empty());  // Invariant-1
    if (ret[editing_index].isEmpty() || (unique && isCurrentTagADuplicate())) {
        ret.erase(ret.begin() + static_cast<std::ptrdiff_t>(editing_index));
    }
    return ret;
}

void TagWidget::clear()
{
    // Set tags to an empty list
    std::vector<QString> tags;
    _setTags(tags);
}

void TagWidget::completion(std::vector<QString> const& completions)
{
    completer = std::make_unique<QCompleter>([&] {
        QStringList ret;
        std::copy(completions.begin(), completions.end(), std::back_inserter(ret));
        return ret;
    }());
    setupCompleter();
}

void TagWidget::_setTags(std::vector<QString> const& tags)
{
    std::unordered_set<QString> unique_tags;
    std::vector<Tag> t;
    for (auto const& x : tags) {
        if (/* Invariant-1 */ !x.isEmpty()
            && /* Invariant-2 */ (!unique || unique_tags.insert(x).second)) {
            t.emplace_back(x, QRect {});
        }
    }
    this->tags = std::move(t);
    this->tags.push_back(Tag {});
    editing_index = this->tags.size() - 1;
    moveCursor(0, false);
}

bool TagWidget::isCurrentTagADuplicate() const
{
    assert(editing_index < tags.size());
    auto const mid = tags.begin() + static_cast<std::ptrdiff_t>(editing_index);
    auto const text_eq = [this](const Tag& x) {
        return x.text == editorText();
    };
    return std::find_if(tags.begin(), mid, text_eq) != mid
        || std::find_if(mid + 1, tags.end(), text_eq) != tags.end();
}

qreal TagWidget::cursorToX()
{
    return text_layout.lineAt(0).cursorToX(cursor);
}

void TagWidget::moveCursor(int pos, bool mark)
{
    if (mark) {
        auto e = select_start + select_size;
        int anchor = select_size > 0 && cursor == select_start ? e
            : select_size > 0 && cursor == e                   ? select_start
                                                               : cursor;
        select_start = qMin(anchor, pos);
        select_size = qMax(anchor, pos) - select_start;
    }
    else {
        deselectAll();
    }
    cursor = pos;
}

void TagWidget::deselectAll()
{
    select_start = 0;
    select_size = 0;
}

bool TagWidget::hasSelection() const noexcept
{
    return select_size > 0;
}

void TagWidget::selectAll()
{
    select_start = 0;
    select_size = editorText().size();
}

void TagWidget::removeSelection()
{
    assert(select_start + select_size <= editorText().size());
    cursor = select_start;
    editorText().remove(cursor, select_size);
    deselectAll();
}

void TagWidget::drawEditor(QPainter& p, QPalette const& palette, QPoint const& offset) const
{
    auto const& r = editorRect();
    auto const& txt_p = r.topLeft() + QPointF(pill_thickness.left(), pill_thickness.top());
    auto const f = formatting(palette);
    text_layout.draw(&p, txt_p - offset, f);
    if (blink_status) {
        text_layout.drawCursor(&p, txt_p - offset, cursor);
    }
}

QVector<QTextLayout::FormatRange> TagWidget::formatting(QPalette const& palette) const
{
    if (select_size == 0) {
        return {};
    }

    QTextLayout::FormatRange selection;
    selection.start = select_start;
    selection.length = select_size;
    selection.format.setBackground(palette.brush(QPalette::Highlight));
    selection.format.setForeground(palette.brush(QPalette::HighlightedText));
    return {selection};
}

/// Makes the tag at `i` currently editing, and ensures Invariant-1 and Invariant-2`.
void TagWidget::setEditorIndex(size_t i)
{
    assert(i < tags.size());
    if (editorText().isEmpty() || (unique && isCurrentTagADuplicate())) {
        tags.erase(std::next(begin(tags), static_cast<std::ptrdiff_t>(editing_index)));
        if (editing_index <= i) {  // Did we shift `i`?
            --i;
        }
    }
    editing_index = i;
}

// Inserts a new tag at `i`, makes the tag currently editing, and ensures Invariant-1.
void TagWidget::editNewTag(size_t i)
{
    assert(i <= tags.size());
    tags.insert(begin(tags) + static_cast<std::ptrdiff_t>(i), Tag {});
    if (i <= editing_index) {  // Did we shift `editing_index`?
        ++editing_index;
    }
    setEditorIndex(i);
    moveCursor(0, false);
}

void TagWidget::editPreviousTag()
{
    if (editing_index > 0) {
        setEditorIndex(editing_index - 1);
        moveCursor(editorText().size(), false);
    }
}

void TagWidget::editNextTag()
{
    if (editing_index < tags.size() - 1) {
        setEditorIndex(editing_index + 1);
        moveCursor(0, false);
    }
}

void TagWidget::editTag(size_t i)
{
    assert(i < tags.size());
    setEditorIndex(i);
    moveCursor(editorText().size(), false);
}

void TagWidget::removeTag(size_t i)
{
    tags.erase(tags.begin() + static_cast<ptrdiff_t>(i));
    if (i <= editing_index) {
        --editing_index;
    }
}

void TagWidget::removeBackwardOne()
{
    if (hasSelection()) {
        removeSelection();
    }
    else {
        editorText().remove(--cursor, 1);
    }
}

void TagWidget::removeDuplicates()
{
    removeDuplicates(tags);
    auto const it = std::find_if(tags.begin(), tags.end(), [](auto const& x) {
        return x.text.isEmpty();  // Thanks to Invariant-1 we can track back the editing_index.
    });
    assert(it != tags.end());
    editing_index = static_cast<size_t>(std::distance(tags.begin(), it));
}

void TagWidget::removeDuplicates(std::vector<Tag>& tags)
{
    std::unordered_map<QString, size_t> unique;
    for (auto const i : std::views::iota(size_t {0}, tags.size())) {
        unique.emplace(tags[i].text, i);
    }

    for (auto b = tags.rbegin(), it = b, e = tags.rend(); it != e;) {
        if (auto const i = static_cast<size_t>(std::distance(it, e) - 1); unique.at(it->text) != i) {
            tags.erase(it++.base() - 1);
        }
        else {
            ++it;
        }
    }
}

bool TagWidget::isAcceptableInput(QKeyEvent const& event)
{
    auto const text = event.text();
    if (text.isEmpty()) {
        return false;
    }

    auto const c = text.at(0);

    if (c.category() == QChar::Other_Format) {
        return true;
    }

    if (event.modifiers() == Qt::ControlModifier
        || event.modifiers() == (Qt::ShiftModifier | Qt::ControlModifier)) {
        return false;
    }

    if (c.isPrint()) {
        return true;
    }

    if (c.category() == QChar::Other_PrivateUse) {
        return true;
    }

    return false;
}

void TagWidget::setEditorText(QString const& text)
{
    editorText() = text;
    moveCursor(editorText().length(), false);
    update1();
}

#include "moc_TagWidget.cpp"
