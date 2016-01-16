/*
    This file is part of QTau
    Copyright (C) 2013-2015  Tobias Platen <tobias@platen-software.de>
    Copyright (C) 2013       digited       <https://github.com/digited>
    Copyright (C) 2013       HAL@ShurabaP  <https://github.com/haruneko>

    QTau is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ui/noteEditorHandlers.h"
#include "ui/noteEditor.h"
#include "NoteEvents.h"

#include <qevent.h>
#include <QLineEdit>
#include <QKeyEvent>

#include "../Controller.h"
#include "../PluginInterfaces.h"

const int CONST_NOTE_RESIZE_CURSOR_MARGIN = 6;


inline int snap(int value, int unit, int baseValue = 0)
{
    int baseOffset = baseValue % unit;
    value -= baseOffset;
    int prev = value / unit;
    float percent = (float)(value % unit) / (float)unit;

    return ((percent >= 0.5) ? (prev+1) * unit : prev * unit) + baseOffset;
}


void qtauEdController::changeController(qtauEdController *c) { _owner->changeController(c); }
void qtauEdController::eventHappened   (qtauEvent *e)        { _owner->eventHappened   (e); }
void qtauEdController::recalcNoteRects()                     { _owner->recalcNoteRects();   }
void qtauEdController::lazyUpdate()                          { _owner->lazyUpdate();        }


void qtauEdController::onEvent(qtauEvent *e)
{
    reset();

    switch(e->type())
    {
    case ENoteEvents::add:
        onNoteAdd(static_cast<qtauEvent_NoteAddition*>(e));
        break;
    case ENoteEvents::text:
        onNoteText(static_cast<qtauEvent_NoteText*>(e));
        break;
    case ENoteEvents::move:
        onNoteMove(static_cast<qtauEvent_NoteMove*>(e));
        break;
    case ENoteEvents::resize:
        onNoteResize(static_cast<qtauEvent_NoteResize*>(e));
        break;
    case ENoteEvents::effect:
        onNoteEffect(static_cast<qtauEvent_NoteEffect*>(e));
        break;
    default:
        vsLog::e(QString("Editor controller got event of unknown type %1").arg(e->type()));
    }
}

void qtauEdController::deleteSelected()
{
    if (!_notes->selected.isEmpty())
    {
        _pointedNote = 0;
        qtauEvent_NoteAddition::noteAddVector v;

        foreach (const quint64 &id, _notes->selected)
        {
            const qne::editorNote &n = _notes->idMap[id];
            qtauEvent_NoteAddition::noteAddData d;
            d.id          = id;
            d.lyrics      = n.txt;
            d.pulseLength = n.pulseLength;
            d.pulseOffset = n.pulseOffset;
            d.keyNumber   = n.keyNumber;
            v.append(d);

            removeFromGrid(n.r.left(),  n.id);
            removeFromGrid(n.r.right(), n.id);
            _notes->idMap.remove(id);
        }

        _notes->selected.clear();
        lazyUpdate();

        qtauEvent_NoteAddition *event = new qtauEvent_NoteAddition(v, true, true);
        eventHappened(event);
    }
}

void qtauEdController::mouseDoubleClickEvent(QMouseEvent *event)
{
    unselectAll();

    if (event->button() == Qt::LeftButton && _pointedNote && _state->editingEnabled)
    {
        _notes->selected.append(_pointedNote->id);
        _pointedNote->selected = true;

        QRect r(_pointedNote->r);
        r.setSize(QSize(100, _setup->note.height())); // TODO: move constants from noteEditor in some common header

        if (!_state->viewport.contains(r))
            _owner->scrollTo(r);

        _owner->lazyUpdate();

        _owner->changeController(new qtauEd_TextInput(this));
    }
}

void qtauEdController::mousePressEvent(QMouseEvent *event)
{
    if (_state->rmbScrollEnabled && event->button() == Qt::RightButton)
    {
        _rmbScrollStartPos = event->pos();
        _rmbStartVpOffset  = _state->viewport.topLeft();
        _rmbDragging = true;
        _owner->setCursor(Qt::ClosedHandCursor);
    }
    else if (event->button() == Qt::LeftButton)
    {
        _absFirstClickPos = event->pos() + _state->viewport.topLeft();

        if (_state->editingEnabled && _owner->cursor().shape() == Qt::SplitHCursor && _pointedNote)
        {
            int msPosInNote = event->pos().x() + _state->viewport.x() - _pointedNote->r.x();
            bool left = (float)msPosInNote / _pointedNote->r.width() < 0.5;
            _owner->changeController(new qtauEd_ResizeNote(this, left));
        }
    }
}

void qtauEdController::mouseMoveEvent(QMouseEvent *event)
{
    if (_rmbDragging)
        _owner->rmbScrollHappened(event->pos() - _rmbScrollStartPos, _rmbStartVpOffset);
    else
    {
        QPoint absPos(event->pos() + _state->viewport.topLeft());
        QPoint delta = absPos - _absFirstClickPos;

        _pointedNote = noteInPoint(absPos);

        if (_pointedNote)
        {
            if (event->buttons() & Qt::LeftButton)
            {
                if (abs(delta.x()) > 3 || abs(delta.y()) > 3) // mousepress exceeded accidental slide click margin
                {
                    if (!_pointedNote->selected)
                    {
                        unselectAll();
                        _pointedNote->selected = true;
                        _notes->selected.append(_pointedNote->id);
                    }

                    _owner->changeController(new qtauEd_DragNotes(this));
                }
            }
            else if (_state->editingEnabled)  // no mousepress, just moving cursor
            {
                // if above edge, show resize cursor
                if (absPos.x() - _pointedNote->r.left()  <= CONST_NOTE_RESIZE_CURSOR_MARGIN ||
                    _pointedNote->r.right() - absPos.x() <= CONST_NOTE_RESIZE_CURSOR_MARGIN)
                    _owner->setCursor(Qt::SplitHCursor);
                else
                    _owner->setCursor(Qt::ArrowCursor);
            }
            else // no button pressed, outside any notes
                if (_owner->cursor().shape() != Qt::ArrowCursor)
                    _owner->setCursor(Qt::ArrowCursor);
        }
        else
        {
            if (_owner->cursor().shape() != Qt::ArrowCursor)
                _owner->setCursor(Qt::ArrowCursor);

            if (!noteInPoint(_absFirstClickPos))
                if (event->buttons() & Qt::LeftButton && (abs(delta.x()) > 3 || abs(delta.y()) > 3))
                {
                    if (_state->editingEnabled) _owner->changeController(new qtauEd_AddNote(this));
                    else                       _owner->changeController(new qtauEd_SelectRect(this));
                }
        }
    }
}

void qtauEdController::mouseReleaseEvent(QMouseEvent *event)
{
    if (_rmbDragging)
    {
        _rmbDragging = false;
        _owner->setCursor(Qt::ArrowCursor);
    }
    else if (_pointedNote)
    {
        if (event->modifiers() & Qt::ControlModifier)   // control toggles selection
        {
            if (_pointedNote->selected) // unselect
                _notes->selected.remove(_notes->selected.indexOf(_pointedNote->id));
            else                       // select
                _notes->selected.prepend(_pointedNote->id);

            _pointedNote->selected = !_pointedNote->selected;
        }
        else if (event->modifiers() & Qt::ShiftModifier) // just add it to selection, if not already
        {
            if (!_pointedNote->selected)
            {
                _pointedNote->selected = true;
                _notes->selected.prepend(_pointedNote->id);
            }
        }
        else    // unselect everything except this note
        {
            unselectAll();
            _notes->selected.append(_pointedNote->id);
            _pointedNote->selected = true;
        }
    }
    else
        unselectAll();

    _absFirstClickPos = QPoint(-1,-1);
    _owner->lazyUpdate();
}

//TODO: scoreLoad()

void qtauEdController::onNoteAdd(qtauEvent_NoteAddition *event)
{
    unselectAll();

    foreach (const qtauEvent_NoteAddition::noteAddData &d, event->getAdded())
    {
        bool reallyForward = (event->isForward() && !event->isDeleteEvent()) ||
                            (!event->isForward() &&  event->isDeleteEvent());

        if (reallyForward)
        {
            fprintf(stderr,"note %i %i %i %i\n",d.id,d.pulseOffset,d.pulseLength,d.keyNumber);
            qne::editorNote n;
            n.id  = d.id;
            n.txt = d.lyrics;
            n.pulseOffset = d.pulseOffset;
            n.pulseLength = d.pulseLength;
            n.keyNumber   = d.keyNumber;

            _idOffset = qMax(_idOffset, d.id);
            _notes->idMap[d.id] = n;
        }
        else
        {
            qne::editorNote &n = _notes->idMap[d.id];
            removeFromGrid(n.r.left(),  n.id);
            removeFromGrid(n.r.right(), n.id);
            _notes->idMap.remove(n.id);
        }
    }

    _owner->recalcNoteRects();
    _owner->lazyUpdate();
}

void qtauEdController::onNoteResize(qtauEvent_NoteResize *event)
{
    foreach (const qtauEvent_NoteResize::noteResizeData &d, event->getResized())
    {
        qne::editorNote &n = _notes->idMap[d.id];
        removeFromGrid(n.r.left(),  n.id);
        removeFromGrid(n.r.right(), n.id);

        if (event->isForward())
        {
            n.pulseOffset = d.offset;
            n.pulseLength = d.length;
        }
        else
        {
            n.pulseOffset = d.prevOffset;
            n.pulseLength = d.prevLength;
        }

        addToGrid(n.r.left(),  n.id);
        addToGrid(n.r.right(), n.id);
    }

    recalcNoteRects();
    _owner->lazyUpdate();
}

// TODO: resize and move generally look the same, maybe merge them?
void qtauEdController::onNoteMove(qtauEvent_NoteMove *event)
{
    foreach (const qtauEvent_NoteMove::noteMoveData &d, event->getMoved())
    {
        qne::editorNote &n = _notes->idMap[d.id];
        removeFromGrid(n.r.left(),  n.id);
        removeFromGrid(n.r.right(), n.id);

        if (event->isForward())
        {
            n.pulseOffset += d.pulseOffDelta;
            n.keyNumber   =  d.keyNumber;
        }
        else
        {
            n.pulseOffset -= d.pulseOffDelta;
            n.keyNumber   =  d.prevKeyNumber;
        }

        addToGrid(n.r.left(),  n.id);
        addToGrid(n.r.right(), n.id);
    }

    recalcNoteRects();
    _owner->lazyUpdate();
}

void qtauEdController::onNoteText(qtauEvent_NoteText *event)
{
    foreach (const qtauEvent_NoteText::noteTextData &d, event->getText())
    {
        qne::editorNote &n = _notes->idMap[d.id];

        if (event->isForward()) n.txt = d.txt;
        else                    n.txt = d.prevTxt;

        n.cached = false;
    }

    _owner->lazyUpdate();
}

void qtauEdController::onNoteEffect(qtauEvent_NoteEffect*)
{
    //FIXME: does nothing -- remove if not needed
}


qne::editorNote* qtauEdController::noteInPoint(const QPoint &p)
{
    qne::editorNote *result = 0;

    int barUnderCursor = p.x() / _setup->barWidth;

    if (barUnderCursor < _notes->grid.size())
        for (int i = 0; i < _notes->grid[barUnderCursor].size(); ++i)
        {
            qne::editorNote &n = _notes->idMap[_notes->grid[barUnderCursor][i]];

            if (n.r.contains(p))
            {
                result = &n;
                break;
            }
        }

    return result;
}

//========================================================================

qtauEd_TextInput::qtauEd_TextInput(qtauNoteEditor &ne, SNoteSetup &ns, qne::editorNotes &nts, qne::editorState &st) :
    qtauEdController(ne, ns, nts, st), _managedOnEdited(false), _editingNote(false) {}

qtauEd_TextInput::qtauEd_TextInput(qtauEdController *c) : qtauEdController(c) {}

qtauEd_TextInput::~qtauEd_TextInput() {}

void qtauEd_TextInput::cleanup()
{
    if (_editingNote)
    {
        _editingNote = false;
        disconnect(_edit, SIGNAL(editingFinished()), this, SLOT(onEdited()));
        disconnect(_edit, SIGNAL(returnPressed  ()), this, SLOT(unfocus()));
        _edit->removeEventFilter(this);
        _owner->setFocus();
        _edit->setVisible(false);
    }
}

void qtauEd_TextInput::init()
{
    _editingNote = !_notes->selected.isEmpty();

    if (_editingNote)
    {
        if (!_edit)
            _edit = new QLineEdit(_owner);

        connect(_edit, SIGNAL(editingFinished()), SLOT(onEdited()));
        connect(_edit, SIGNAL(returnPressed  ()), SLOT(unfocus()));
        _editedNote = _pointedNote; // may change when clicked outside editbox, so need to store

        QRect r(_pointedNote->r);
        r.moveTo(r.topLeft() - _state->viewport.topLeft());
        r.setRight (r.right()  + 1);
        r.setBottom(r.bottom() + 1);

        _edit->setGeometry(r);
        _edit->setVisible(true);
        _edit->setText(_editedNote->txt);
        _edit->setFocus();
        _edit->selectAll();

        _edit->installEventFilter(this);
    }
}
bool qtauEd_TextInput::getEditingNote() const
{
    return _editingNote;
}

void qtauEd_TextInput::setEditingNote(bool value)
{
    _editingNote = value;
}


void qtauEd_TextInput::unfocus() { _owner->setFocus(); }
void qtauEd_TextInput::reset()   { cleanup(); changeController(new qtauEdController(this)); }

void qtauEd_TextInput::onEdited()
{
    if (_editingNote && _editedNote)
    {
        _editingNote = false;
        QString txt = _edit->text();
        disconnect(_edit, SIGNAL(editingFinished()), this, SLOT(onEdited()));
        _edit->setVisible(false);

        if (txt != _editedNote->txt)
        {
            qtauEvent_NoteText::noteTextData d;

            if(txt.endsWith(" "))
            {
              ISynth* s = qtauController::instance()->getSynth();
              txt = s->getTranscription(txt);
            }

            d.id      = _editedNote->id;
            d.txt     = txt;
            d.prevTxt = _editedNote->txt;


            qtauEvent_NoteText::noteTextVector v;
            v.append(d);
            qtauEvent_NoteText *add = new qtauEvent_NoteText(v);

            _editedNote->txt = txt;
            _editedNote->cached = false;
            _editedNote = 0;

            eventHappened(add);
        }

        lazyUpdate();

        if (!_managedOnEdited)
            changeController(new qtauEdController(this));
    }
}

bool qtauEd_TextInput::eventFilter(QObject */*obj*/, QEvent *event)
{
    bool result = false; // we don't handle the event by default, passing it to be handled by qlineedit itself

    if (_editingNote && (event->type() == QEvent::KeyPress || event->type() == QEvent::Shortcut))
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

        if (keyEvent->key() == Qt::Key_Tab || (keyEvent->key() == Qt::Key_Backtab))
        {
            int targetOffset = _editedNote->r.x();
            int stBar  = targetOffset / _setup->barWidth;
            qne::editorNote *nextNote = 0;

            if (stBar < _notes->grid.size())
            {
                if (keyEvent->key() == Qt::Key_Backtab)  // move editing focus to previous note ----------
                {
                    if (targetOffset > 0) // no point in shift-tabbing if current note is first already
                    {
                        for (int bar = stBar; bar >= 0; --bar)
                        {
                            const QVector<quint64> &gridBar = _notes->grid[bar];

                            for (int n = 0; n < gridBar.size(); ++n)
                            {
                                if (gridBar[n] != _editedNote->id)
                                {
                                    qne::editorNote *note = &_notes->idMap[gridBar[n]];

                                    if (note->r.x() < targetOffset)
                                        if (!nextNote || nextNote->r.x() < note->r.x())
                                            nextNote = note;
                                }
                            } // looping though all notes in a bar without break to find closest one, since they're unordered

                            if (nextNote)
                                break;
                        } //               end looping bar grid
                    }
                }
                else // ------------------------------------------ move editing focus to next note --------------
                {
                    int endBar = _notes->grid.size();

                    for (int bar = stBar; bar < endBar; ++bar)
                    {
                        const QVector<quint64> &gridBar = _notes->grid[bar];

                        for (int n = 0; n < gridBar.size(); ++n)
                        {
                            if (gridBar[n] != _editedNote->id)
                            {
                                qne::editorNote *note = &_notes->idMap[gridBar[n]];

                                if (note->r.x() > targetOffset)
                                    if (!nextNote || nextNote->r.x() > note->r.x())
                                        nextNote = note;
                            }
                        } // looping though all notes in a bar without break to find closest one, since they're unordered

                        if (nextNote)
                            break;
                    } //               end looping bar grid
                }
            }
            else vsLog::e("Currently edited note isn't in notes grid. How could this happen?..");

            _managedOnEdited = nextNote != 0;
            onEdited(); // shouldn't delete this if managedOnEdited

            if (nextNote)
            {
                result = true; // flag qlineedit that the event was handled
                _pointedNote = nextNote;
                changeController(new qtauEd_TextInput(this));
            }
        }
    }

    return result;
}

//========================================================================

qtauEd_SelectRect::qtauEd_SelectRect(qtauNoteEditor &ne, SNoteSetup &ns, qne::editorNotes &nts, qne::editorState &st) :
        qtauEdController(ne, ns, nts, st) {}

qtauEd_SelectRect::qtauEd_SelectRect(qtauEdController *c) : qtauEdController(c) {}

qtauEd_SelectRect::~qtauEd_SelectRect() {}

void qtauEd_SelectRect::reset()
{
    // rect resetting should just deselect all notes and remove rect itself
    unselectAll();
    _state->selectionRect.setRect(-1,-1,0,0);
    _pointedNote = 0;
    lazyUpdate();
    changeController(new qtauEdController(this));
}

void qtauEd_SelectRect::mouseMoveEvent(QMouseEvent *event)
{
    QMap<quint64, bool> usedIDs;
    QVector<quint64> selected;

    int X = _setup->barWidth  * 128; // TODO: move numBars in setup
    int Y = _setup->octHeight * _setup->numOctaves;

    QPoint pos = event->pos() + _state->viewport.topLeft();
    QPoint tl(qMax(qMin(pos.x(), _absFirstClickPos.x()), 0), qMax(qMin(pos.y(), _absFirstClickPos.y()), 0));
    QPoint br(qMin(qMax(pos.x(), _absFirstClickPos.x()), X), qMin(qMax(pos.y(), _absFirstClickPos.y()), Y));

    _state->selectionRect.setTopLeft(tl);
    _state->selectionRect.setBottomRight(br);

    foreach (const quint64 &id, _notes->selected)
    {
        usedIDs[id] = true;
        qne::editorNote &n = _notes->idMap[id];
        bool intersects = _state->selectionRect.intersects(n.r);

        if (intersects) selected.append(n.id);
        else            n.selected = false;
    }

    int stBar  = tl.x() / _setup->barWidth;
    int endBar = br.x() / _setup->barWidth;

    // check all notes in bars from first that has left edge of sel rect to last that has right edge
    if (stBar < _notes->grid.size())
    {
        if (endBar >= _notes->grid.size())
            endBar = _notes->grid.size() - 1;

        for (int i = stBar; i <= endBar; ++i)
            for (int k = 0; k < _notes->grid[i].size(); ++k)
            {
                quint64 id = _notes->grid[i][k];

                if (!usedIDs.contains(_notes->grid[i][k]))
                {
                    qne::editorNote &n = _notes->idMap[id];

                    if (_state->selectionRect.intersects(n.r))
                    {
                        selected.append(id);
                        n.selected = true;
                    }
                }
            }
    }

    _notes->selected = selected;
    lazyUpdate();
}

void qtauEd_SelectRect::mouseReleaseEvent(QMouseEvent*)
{
    _state->selectionRect = QRect(-1,-1,0,0); // disable
    lazyUpdate();
    changeController(new qtauEdController(this));
}

//========================================================================

qtauEd_DragNotes::qtauEd_DragNotes(qtauNoteEditor &ne, SNoteSetup &ns, qne::editorNotes &nts, qne::editorState &st) :
        qtauEdController(ne, ns, nts, st) {}

qtauEd_DragNotes::qtauEd_DragNotes(qtauEdController *c) : qtauEdController(c) {}

qtauEd_DragNotes::~qtauEd_DragNotes() {}

void qtauEd_DragNotes::reset()
{
    // drag reset should move all selected notes back to original position
    foreach (const quint64 id, _notes->selected)
    {
        qne::editorNote &n = _notes->idMap[id];

        removeFromGrid(n.r.left(), n.id);
        removeFromGrid(n.r.right(), n.id);

        n.r.moveTo(n.dragSt);

        addToGrid(n.r.left(), n.id);
        addToGrid(n.r.right(), n.id);
    }

    _pointedNote = 0;
    _state->snapLine = -1;
    lazyUpdate();
    changeController(new qtauEdController(this));
}

void qtauEd_DragNotes::init()
{
    if (!_pointedNote)
    {
        vsLog::e("Can't drug notes without note under cursor");
        changeController(new qtauEdController(this));
        return;
    }

    _mainMovedNote = _pointedNote;

    _selBounds = _mainMovedNote->r;
    _selRects.clear(); // just to be sure

    if (!_mainMovedNote->selected)
    {
        _mainMovedNote->selected = true;
        _notes->selected.append(_mainMovedNote->id);
    }

    foreach (const quint64 &id, _notes->selected)
    {
        qne::editorNote &n = _notes->idMap[id];
        n.dragSt = n.r.topLeft();
        _selRects.append(n.r);

        _selBounds.setLeft  (qMin(_selBounds.left  (), n.r.left  ()));
        _selBounds.setTop   (qMin(_selBounds.top   (), n.r.top   ()));
        _selBounds.setRight (qMax(_selBounds.right (), n.r.right ()));
        _selBounds.setBottom(qMax(_selBounds.bottom(), n.r.bottom()));
    }
}

void qtauEd_DragNotes::mouseMoveEvent(QMouseEvent *event)
{
    QPoint dynDelta = (event->pos() + _state->viewport.topLeft()) - _absFirstClickPos;
    QPoint desiredPos = _mainMovedNote->dragSt + dynDelta;

    int minOffset = (_setup->note.width() * 4) / _setup->quantize;

    if (_state->gridSnapEnabled)
        desiredPos.setX(qMax(0, qMin(snap(desiredPos.x(), minOffset), _setup->barWidth * 128)));

    desiredPos.setY(qMax(0, qMin(snap(desiredPos.y(), _setup->note.height()),
                                 _setup->octHeight * _setup->numOctaves))); // always snapping Y to pitch grid

    QPoint snappedDelta = desiredPos - _mainMovedNote->dragSt;

    // collision detection ---------------------------------------------------
    QRect workspaceZone(0, 0, _setup->barWidth * 128, _setup->numOctaves * _setup->octHeight);
    QRect newSelBounds(_selBounds);
    newSelBounds.moveTo(_selBounds.topLeft() + snappedDelta);

    bool noCollision = workspaceZone.contains(newSelBounds);

    if (noCollision)
    {
        QVector<QRect> newSelRects;

        for (int i = 0; i < _selRects.size(); ++i)
        {
            QRect r(_selRects[i]);
            r.moveTo(r.topLeft() + snappedDelta);
            newSelRects.append(r);
        }

        int firstBar = newSelBounds.left()  / _setup->barWidth;
        int lastBar  = newSelBounds.right() / _setup->barWidth;

        if (firstBar < _notes->grid.size())
        {
            if (lastBar >= _notes->grid.size())
                lastBar = _notes->grid.size() - 1;

            for (int i = firstBar; i <= lastBar; ++i)
            {
                for (int k = 0; k < _notes->grid[i].size(); ++k)
                {
                    const qne::editorNote &n = _notes->idMap[_notes->grid[i][k]];

                    if (!n.selected && n.r.intersects(newSelBounds)) // skip selected and those out of grouprect
                    {
                        for (int m = 0; m < newSelRects.size(); ++m)
                            if (n.r.intersects(newSelRects[m]))
                            {
                                noCollision = false;
                                break;
                            }

                        if (!noCollision)
                            break;
                    }
                }

                if (!noCollision)
                    break;
            }
        }
    }
    //------------------------------------------------------------------------

    if (noCollision)
    {
        foreach (const quint64 &id, _notes->selected)
        {
            qne::editorNote &n = _notes->idMap[id];

            removeFromGrid(n.r.left(),  n.id);
            removeFromGrid(n.r.right(), n.id);

            n.r.moveTo(n.dragSt + snappedDelta);
            updateModelData(n);

            addToGrid(n.r.left(),  n.id);
            addToGrid(n.r.right(), n.id);
        }

        _state->snapLine = desiredPos.x();
        lazyUpdate();
    }
}

void qtauEd_DragNotes::mouseReleaseEvent(QMouseEvent*)
{
    _state->snapLine = -1;

    QPoint pSt  = _mainMovedNote->r.topLeft();
    QPoint pEnd = _mainMovedNote->dragSt;

    if (pSt.x() != pEnd.x() || pSt.y() != pEnd.y())
    {
        // some movement was applied, so generate move event
        qtauEvent_NoteMove::noteMoveVector v;
        float pixelsToPulses = 480.0 / _setup->note.width();
        // TODO: as barWidth and octHeight, maybe move it to setup struct and precalc on configure?
        // TODO: move precalc from editor to mainwindow, where zoom change is processed
        // TODO: I now must have, like, a billion of todos here. Do them, damnit
        int totalKeys = (_setup->baseOctave + _setup->numOctaves - 1) * 12;

        foreach (const quint64 &id, _notes->selected)
        {
            qne::editorNote &n = _notes->idMap[id];
            int pxDelta = n.r.x() - n.dragSt.x();
            float rounder = (pxDelta < 0) ? -0.001 : 0.001;

            qtauEvent_NoteMove::noteMoveData d;
            d.id = n.id;
            d.pulseOffDelta = (float)pxDelta * pixelsToPulses + rounder;
            d.keyNumber     = n.keyNumber;
            d.prevKeyNumber = totalKeys - n.dragSt.y() / _setup->note.height();
            v.append(d);
        }

        qtauEvent_NoteMove *event = new qtauEvent_NoteMove(v);
        eventHappened(event);
    }

    lazyUpdate(); // need to remove that snap line
    changeController(new qtauEdController(this));
}

//========================================================================

qtauEd_ResizeNote::qtauEd_ResizeNote(qtauNoteEditor &ne, SNoteSetup &ns, qne::editorNotes &nts, qne::editorState &st, bool left) :
    qtauEdController(ne, ns, nts, st), _toLeft(left) {}

qtauEd_ResizeNote::qtauEd_ResizeNote(qtauEdController *c, bool left) : qtauEdController(c), _toLeft(left) {}

qtauEd_ResizeNote::~qtauEd_ResizeNote() {}

void qtauEd_ResizeNote::reset()
{
    // reset() should just revert resized note to its original size, since resize event may mess with
    //   undo/redo stack if reset is called because of event
    _pointedNote->r = _originalRect;
    _pointedNote = 0;
    _state->snapLine = -1;
    lazyUpdate();
    changeController(new qtauEdController(this));
}

void qtauEd_ResizeNote::init()
{
    if (!_pointedNote)
    {
        vsLog::e("Note resize controller is created without note to resize");
        changeController(new qtauEdController(this));
        return;
    }

    // min note width is 1/64 -> 1 px at max zoom, when 1/4 note width == 16 px
    _minNoteWidth = (_setup->note.width() * 4) / _setup->length;

    _editedNote = _pointedNote;
    _originalRect = _pointedNote->r;
}

void qtauEd_ResizeNote::mouseMoveEvent(QMouseEvent *event)
{
    float pixelsToPulses = 480.0 / (float)_setup->note.width(); // TODO: maybe shouldn't calc it every time?

    QRect newNoteRect(_editedNote->r);

    int cursorHPos = event->pos().x() + _state->viewport.x();

    if (_state->gridSnapEnabled)
        cursorHPos = snap(cursorHPos, _minNoteWidth, (_toLeft ? _editedNote->r.right() + 1 : _editedNote->r.left()));

    if (_toLeft)
        // calc new left coord, with magical +1's, because no code can work properly without a bit of magic
        newNoteRect.setLeft (qMax(0, qMin(cursorHPos, _editedNote->r.right() - _minNoteWidth + 1)));
    else
        newNoteRect.setRight(qMin(_setup->barWidth * 128, // TODO: move that 128 somewhere already!
                                   qMax(cursorHPos - 1, _editedNote->r.left() + _minNoteWidth - 1)));

    int barSt  = newNoteRect.left()  / _setup->barWidth;
    int barEnd = newNoteRect.right() / _setup->barWidth;

    bool noCollision = true;

    if (barSt < _notes->grid.size())
    {
        if (barEnd >= _notes->grid.size())
            barEnd = _notes->grid.size() - 1;

        int i = barSt;

        while (i <= barEnd && noCollision)
        {
            for (int k = 0; k < _notes->grid[i].size(); ++k)
            {
                qne::editorNote &n = _notes->idMap[_notes->grid[i][k]];

                if (n.id != _editedNote->id && n.r.intersects(newNoteRect))
                {
                    noCollision = false;
                    break;
                }
            }

            ++i;
        }
    }
    else
        _notes->grid.resize(barEnd + 10); // that shouldn't really happen, but whatever

    if (noCollision && (_editedNote->r.x() != newNoteRect.x() || _editedNote->r.width() != newNoteRect.width()))
    {
        removeFromGrid(_editedNote->r.left(),  _editedNote->id);
        removeFromGrid(_editedNote->r.right(), _editedNote->id);

        if (_toLeft)
        {
            _state->snapLine = newNoteRect.left();
            _editedNote->r.setLeft(_state->snapLine);
            _editedNote->pulseOffset = _editedNote->r.x() * pixelsToPulses + 0.001;
        }
        else
        {
            _state->snapLine = newNoteRect.right() + 1;
            _editedNote->r.setRight(newNoteRect.right());
        }

        addToGrid(_editedNote->r.left(),  _editedNote->id);
        addToGrid(_editedNote->r.right(), _editedNote->id);

        _editedNote->pulseLength = _editedNote->r.width() * pixelsToPulses + 0.001;
        lazyUpdate();
    }
}

void qtauEd_ResizeNote::mouseReleaseEvent(QMouseEvent*)
{
    _state->snapLine = -1;
    _owner->setCursor(Qt::ArrowCursor);
    float pixelsToPulses = 480.0 / _setup->note.width();

    if (_editedNote->r.width() != _originalRect.width())
    {
        qtauEvent_NoteResize::noteResizeData d;
        d.id         = _editedNote->id;
        d.offset     = _editedNote->r.x()     * pixelsToPulses + 0.001;
        d.length     = _editedNote->r.width() * pixelsToPulses + 0.001;
        d.prevOffset = _originalRect.x()      * pixelsToPulses + 0.001;
        d.prevLength = _originalRect.width()  * pixelsToPulses + 0.001;

        qtauEvent_NoteResize::noteResizeVector v;
        v.append(d);
        qtauEvent_NoteResize *noteResize = new qtauEvent_NoteResize(v);

        eventHappened(noteResize);
    }

    lazyUpdate();
    changeController(new qtauEdController(this));
}

//========================================================================

qtauEd_AddNote::qtauEd_AddNote(qtauNoteEditor &ne, SNoteSetup &ns, qne::editorNotes &nts, qne::editorState &st) :
        qtauEdController(ne, ns, nts, st) {}

qtauEd_AddNote::qtauEd_AddNote(qtauEdController *c) : qtauEdController(c) {}

qtauEd_AddNote::~qtauEd_AddNote() {}

void qtauEd_AddNote::reset()
{
    // resetting adding should remove still dragged new note completely, since its add event on mouseUp
    //   may mess with undo/redo stack if reason for reset is new event
    removeFromGrid(_editedNote->r.left(), _editedNote->id);
    removeFromGrid(_editedNote->r.right(), _editedNote->id);

    if (_editedNote->selected)
    {
        int i = _notes->selected.indexOf(_editedNote->id);

        if (i >= 0)
            _notes->selected.remove(i);
    }

    _notes->idMap.remove(_editedNote->id);
    _pointedNote = 0;
    _state->snapLine = -1;
    lazyUpdate();
    changeController(new qtauEdController(this));
}

// won't overload resizenote's init() because using virtual funcs from constructors in C++ is a bad idea
void qtauEd_AddNote::init()
{
    _minOffset = (_setup->note.width() * 4) / _setup->quantize;  // using quantizing to place note

    int hUnits = _absFirstClickPos.y() / _setup->note.height();
    int vOff = hUnits * _setup->note.height(); // floor to prev note offset

    int   units     = _absFirstClickPos.x() / _minOffset;
    float percent   = (float)(_absFirstClickPos.x() % _minOffset) / (float)_minOffset;

    int hOff = (percent >= 0.5) ? (units + 1) * _minOffset : units * _minOffset;

    _minOffset = (_setup->note.width() * 4) / _setup->length; // using musical note length (1/64..1/4) for size

    qne::editorNote n;
    n.r.setRect(hOff, vOff, _minOffset, _setup->note.height());
    n.id  = ++_idOffset;
    n.txt = "a";

    updateModelData(n);
    addToGrid(n.r.left(), n.id);
    _notes->idMap[n.id] = n;

    _editedNote  = &_notes->idMap[n.id];
    _pointedNote = _editedNote;
}

void qtauEd_AddNote::mouseMoveEvent(QMouseEvent *event)
{
    int cursorHPos = event->pos().x() + _state->viewport.x();

    if (_state->gridSnapEnabled)
        cursorHPos = snap(cursorHPos, _minOffset, _editedNote->r.x());

    int desiredRight = qMin(_setup->barWidth * 128,
                            qMax(cursorHPos - 1, _editedNote->r.left() + _minOffset - 1));

    QRect newNoteRect(_editedNote->r);
    newNoteRect.setRight(desiredRight);

    int barSt  = newNoteRect.left()  / _setup->barWidth;
    int barEnd = newNoteRect.right() / _setup->barWidth;

    bool noCollision = true;

    if (barSt < _notes->grid.size())
    {
        if (barEnd >= _notes->grid.size())
            barEnd = _notes->grid.size() - 1;

        int i = barSt;

        while (i <= barEnd && noCollision)
        {
            for (int k = 0; k < _notes->grid[i].size(); ++k)
            {
                qne::editorNote &n = _notes->idMap[_notes->grid[i][k]];

                if (n.id != _editedNote->id && n.r.intersects(newNoteRect))
                {
                    noCollision = false;
                    break;
                }
            }

            ++i;
        }
    }
    else
        _notes->grid.resize(barEnd + 10); // that shouldn't really happen, but whatever

    if (noCollision && _editedNote->r.width() != newNoteRect.width())
    {
        // remove right coord from grid
        removeFromGrid(_editedNote->r.left(),  _editedNote->id);
        removeFromGrid(_editedNote->r.right(), _editedNote->id);

        // calc new right coord
        _state->snapLine = cursorHPos;
        _editedNote->r.setRight(cursorHPos - 1);
        updateModelData(*_editedNote);

        addToGrid(_editedNote->r.left(),  _editedNote->id);
        addToGrid(_editedNote->r.right(), _editedNote->id);

        lazyUpdate();
    }
}

void qtauEd_AddNote::mouseReleaseEvent(QMouseEvent*)
{
    _state->snapLine = -1;

    qtauEvent_NoteAddition::noteAddData d;
    d.id     = _editedNote->id;
    d.lyrics = _editedNote->txt;

    d.pulseOffset = _editedNote->pulseOffset;
    d.pulseLength = _editedNote->pulseLength;
    d.keyNumber   = _editedNote->keyNumber;

    qtauEvent_NoteAddition::noteAddVector v;
    v.append(d);
    qtauEvent_NoteAddition *noteAdd = new qtauEvent_NoteAddition(v);

    eventHappened(noteAdd);

    lazyUpdate();
    changeController(new qtauEdController(this));
}
