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

#ifndef NOTEEDITOR_H
#define NOTEEDITOR_H

#include "Utils.h"
#include "ui/Config.h"
#include <QWidget>
#include <QUrl>
class ISynth;

class qtauEvent_NoteAddition;
class qtauEvent_NoteMove;
class qtauEvent_NoteResize;
class qtauEvent_NoteText;
class qtauEvent_NoteEffect;

class QPixmap;
class QLineEdit;

class qtauEvent;
class qtauEdController;

class qtauController;


class qtauNoteEditor : public QWidget
{
    Q_OBJECT
    friend class qtauEdController;

public:
    qtauNoteEditor(QWidget *parent = 0);
    ~qtauNoteEditor();

    void configure(const SNoteSetup &newSetup);
    void deleteSelected();

    void   setVOffset(int voff);
    void   setHOffset(int hoff);
    QPoint scrollTo  (const QRect &r);

    void setRMBScrollEnabled(bool e) { _state.rmbScrollEnabled  = e; }
    void setEditingEnabled  (bool e) { _state.editingEnabled    = e; }
    void setGridSnapEnabled (bool e) { _state.gridSnapEnabled   = e; }
    void doPhonemeTransformation();
    void reset();

signals:
    void editorEvent(qtauEvent *e);

    void hscrolled(int delta);
    void vscrolled(int delta);
    void zoomed   (int delta);

    void heightChanged(int newHeight);
    void widthChanged (int newWidth );

    void rmbScrolled(QPoint posDelta, QPoint origOffset);
    void requestsOffset(QPoint offset);

    // what happens in editor and is sent to session
    void noteAdd   (qtauEvent_NoteAddition *event);
    void noteResize(qtauEvent_NoteResize   *event);
    void noteMove  (qtauEvent_NoteMove     *event);
    void noteText  (qtauEvent_NoteText     *event);
    void noteEffect(qtauEvent_NoteEffect   *event);

    void urisDropped(QList<QUrl> uris);


public slots:
    void onEvent(qtauEvent *e);
    void lazyUpdate(); // use this instead of update() - Qt is drawing much faster than display updates
    void setPlaybackPosition(int pos);


protected:
    void paintEvent           (QPaintEvent  *event);
    void resizeEvent          (QResizeEvent *event);

    void mouseDoubleClickEvent(QMouseEvent  *event);
    void mouseMoveEvent       (QMouseEvent  *event);
    void mousePressEvent      (QMouseEvent  *event);
    void mouseReleaseEvent    (QMouseEvent  *event);
    void wheelEvent           (QWheelEvent  *event);

    SNoteSetup       _setup;
    qne::editorState _state;
    qne::editorNotes _notes;

    QPixmap *_labelCache;
    QPixmap *_bgCache;

    void recalcNoteRects();
    void updateBGCache();

    int  _lastUpdate;
    bool _delayingUpdate;
    bool _updateCalled;

    int _posPulses;
    int _playLine;

    qtauEdController *_ctrl;
    qtauEdController *_lastCtrl; // they keep getting input after changing somehow :/

    void changeController(qtauEdController *c);
    void rmbScrollHappened(const QPoint &delta, const QPoint &origOff);
    void eventHappened(qtauEvent *e);





};


#endif // NOTEEDITOR_H
