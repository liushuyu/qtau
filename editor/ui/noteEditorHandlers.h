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

#ifndef NOTEEDITORHANDLERS_H
#define NOTEEDITORHANDLERS_H

#include "Utils.h"
#include "ui/Config.h"


class qtauEvent;
class qtauNoteEditor;

class qtauEvent_NoteAddition;
class qtauEvent_NoteMove;
class qtauEvent_NoteResize;
class qtauEvent_NoteText;
class qtauEvent_NoteEffect;

class QLineEdit;
class QMouseEvent;


/// default controller for note editor
class qtauEdController : public QObject
{
    Q_OBJECT
    friend class qtauNoteEditor;

public:
    qtauEdController(qtauNoteEditor &ne, SNoteSetup &ns, qne::editorNotes &nts, qne::editorState &st) :
        _owner(&ne), _setup(&ns), _notes(&nts), _state(&st), _edit(0), _pointedNote(0), _absFirstClickPos(-1,-1),
        _rmbDragging(false), _idOffset(0) {}

    qtauEdController(qtauEdController *c) :
        _owner(c->_owner), _setup(c->_setup), _notes(c->_notes), _state(c->_state), _edit(c->_edit),
        _pointedNote(c->_pointedNote), _absFirstClickPos(c->_absFirstClickPos), _rmbDragging(false),
        _idOffset(c->_idOffset) {}

    virtual ~qtauEdController() {}

    void onEvent(qtauEvent *e);
    void deleteSelected();

protected:
    qtauNoteEditor   *_owner;
    SNoteSetup       *_setup;
    qne::editorNotes *_notes;
    qne::editorState *_state;

    QLineEdit        *_edit; // should be alive after deleting text controller - may receive buffered event
    qne::editorNote  *_pointedNote;

    QPoint            _absFirstClickPos;

    qne::editorNote  *noteInPoint(const QPoint &p);

    virtual void init()    {}
    virtual void reset()   {} // called when event happens - current editing activity should be halted
    virtual void cleanup() {} // called when changing controllers - should check if stopped its activity

    inline void addToGrid(int noteCoord, quint64 id)
    {
        int bar = noteCoord / _setup->barWidth;

        if (bar >= _notes->grid.size())
            _notes->grid.resize(bar + 10);

        if (_notes->grid[bar].indexOf(id) == -1)
            _notes->grid[bar].append(id);
    }

    inline void removeFromGrid(int noteCoord, quint64 id)
    {
        int bar = noteCoord / _setup->barWidth;

        if (bar < _notes->grid.size())
        {
            int i = _notes->grid[bar].indexOf(id);

            if (i >= 0)
                _notes->grid[bar].remove(i);
        }
    }

    inline void unselectAll()
    {
        foreach (const quint64 &id, _notes->selected)
            _notes->idMap[id].selected = false;

        _notes->selected.clear();
    }

    inline void updateModelData(qne::editorNote &n)
    {
        float pixelToPulses = 480.f / _setup->note.width();
        n.pulseLength = n.r.width() * pixelToPulses + 0.001;
        n.pulseOffset = n.r.left()  * pixelToPulses + 0.001;

        int totalKeys = (_setup->baseOctave + _setup->numOctaves - 1) * 12;
        int screenKeyInd = n.r.y() / _setup->note.height();
        n.keyNumber = totalKeys - screenKeyInd-1;
    }

    // just because friends aren't inherited ---------
    void changeController(qtauEdController *c);
    void eventHappened   (qtauEvent        *e);

    void recalcNoteRects();
    void lazyUpdate();
    //------------------------------------------------

    virtual void mouseDoubleClickEvent(QMouseEvent*);
    virtual void mouseMoveEvent       (QMouseEvent*);
    virtual void mousePressEvent      (QMouseEvent*);
    virtual void mouseReleaseEvent    (QMouseEvent*);

    // what happens in session and needs to be reflected in editor
    void onNoteAdd   (qtauEvent_NoteAddition *event);
    void onNoteResize(qtauEvent_NoteResize   *event);
    void onNoteMove  (qtauEvent_NoteMove     *event);
    void onNoteText  (qtauEvent_NoteText     *event);
    void onNoteEffect(qtauEvent_NoteEffect   *event);

    QPoint _rmbScrollStartPos;
    QPoint _rmbStartVpOffset;
    bool   _rmbDragging;

    quint64 _idOffset;

};


/// mouse handler for changing phoneme/lyrics in note(s)
class qtauEd_TextInput : public qtauEdController
{
    Q_OBJECT

public:
    qtauEd_TextInput(qtauNoteEditor &ne, SNoteSetup &ns, qne::editorNotes &nts, qne::editorState &st);
    qtauEd_TextInput(qtauEdController *c);
    ~qtauEd_TextInput();

    bool getEditingNote() const;
    void setEditingNote(bool value);

protected:
    void reset()    override;
    void cleanup()  override;
    void init()     override;

    bool _managedOnEdited; // if onEdited() is called manually and need a different controller next
    bool _editingNote;
    qne::editorNote *_editedNote;

protected slots:
    void onEdited();
    void unfocus();
    bool eventFilter(QObject *obj, QEvent *event) override;

};


/// mouse handler for note selection with rectangle
class qtauEd_SelectRect : public qtauEdController
{
    Q_OBJECT

public:
    qtauEd_SelectRect(qtauNoteEditor &ne, SNoteSetup &ns, qne::editorNotes &nts, qne::editorState &st);
    qtauEd_SelectRect(qtauEdController *c);
    ~qtauEd_SelectRect();

protected:
    void mouseMoveEvent   (QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

    void mouseDoubleClickEvent(QMouseEvent*) override { changeController(new qtauEdController(this)); }
    void mousePressEvent      (QMouseEvent*) override { changeController(new qtauEdController(this)); }

    void reset() override;

};


/// mouse handler for dragging note(s) when pressing lmb
class qtauEd_DragNotes : public qtauEdController
{
    Q_OBJECT

public:
    qtauEd_DragNotes(qtauNoteEditor &ne, SNoteSetup &ns, qne::editorNotes &nts, qne::editorState &st);
    qtauEd_DragNotes(qtauEdController *c);
    ~qtauEd_DragNotes();

protected:
    void mouseMoveEvent   (QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

    void mouseDoubleClickEvent(QMouseEvent*) override { changeController(new qtauEdController(this)); }
    void mousePressEvent      (QMouseEvent*) override { changeController(new qtauEdController(this)); }

    void init()  override;
    void reset() override;
    qne::editorNote *_mainMovedNote;

    QVector<QRect> _selRects;
    QRect _selBounds;

};


/// mouse handler for resizing notes by dragging their left/right borders
class qtauEd_ResizeNote : public qtauEdController
{
    Q_OBJECT

public:
    qtauEd_ResizeNote(qtauNoteEditor &ne, SNoteSetup &ns, qne::editorNotes &nts, qne::editorState &st, bool left);
    qtauEd_ResizeNote(qtauEdController *c, bool left);
    ~qtauEd_ResizeNote();

protected:
    void mouseMoveEvent   (QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

    void mouseDoubleClickEvent(QMouseEvent*) override { changeController(new qtauEdController(this)); }
    void mousePressEvent      (QMouseEvent*) override { changeController(new qtauEdController(this)); }

    void init()  override;
    void reset() override;

    qne::editorNote *_editedNote;
    bool  _toLeft; // if should resize to left direction (move beginning) or to right (move end)
    int   _minNoteWidth;
    QRect _originalRect;
};


/// mouse handler for adding notes with mouse drag
class qtauEd_AddNote : public qtauEdController
{
    Q_OBJECT

public:
    qtauEd_AddNote(qtauNoteEditor &ne, SNoteSetup &ns, qne::editorNotes &nts, qne::editorState &st);
    qtauEd_AddNote(qtauEdController *c);
    ~qtauEd_AddNote();

protected:
    void mouseMoveEvent   (QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

    void mouseDoubleClickEvent(QMouseEvent*) override { changeController(new qtauEdController(this)); }
    void mousePressEvent      (QMouseEvent*) override { changeController(new qtauEdController(this)); }

    void init()  override;
    void reset() override;

    qne::editorNote *_editedNote;
    int _minOffset;

};

#endif // NOTEEDITORHANDLERS_H
