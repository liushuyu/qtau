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

#include "ui/noteEditor.h"
#include "ui/noteEditorHandlers.h"
#include "NoteEvents.h"

#include <qevent.h>
#include <QTime>
#include <QTimer>
#include <qpainter.h>
#include <QPainterPath>
#include <QMimeData>

#include <QLineEdit>
#include <QDebug>

//qtauEvent_Plugin

#include "../Controller.h"
#include "../PluginInterfaces.h"

const int cdef_cache_labels_num  = 1000;
const int cdef_cache_line_height = 12;
const int cdef_cache_line_width  = 100;
const int cdef_lbl_draw_minwidth = 20; // minimal note width on screen in pixels to draw phoneme in it

const int cdef_scroll_margin     = 20; // px of space between edge of widget and target rect

const double F_ROUNDER = 0.1; // because float is floor'ed to int by default, so 3.9999999 becomes 3

//QTime t;


qtauNoteEditor::qtauNoteEditor(QWidget *parent) :
    QWidget(parent), _bgCache(0), _delayingUpdate(false), _updateCalled(false), _lastCtrl(0)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);

    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    _labelCache = new QPixmap(cdef_cache_line_width, cdef_cache_line_height * cdef_cache_labels_num);
    _labelCache->fill(Qt::transparent);

    _ctrl = new qtauEdController(*this, _setup, _notes, _state);





    //t.start();
}

qtauNoteEditor::~qtauNoteEditor()
{
    if (_labelCache)
        delete _labelCache;

    if (_bgCache)
        delete _bgCache;

    if (_lastCtrl)
        delete _lastCtrl;

    if (_ctrl)
        delete _ctrl;
}

void qtauNoteEditor::configure(const SNoteSetup &newSetup)
{
    _ctrl->reset();
    bool gridChanged = newSetup.note != _setup.note || newSetup.notesInBar != _setup.notesInBar;
    _setup = newSetup;

    if (gridChanged)
    {
        recalcNoteRects();
        updateBGCache();
        lazyUpdate();
    }
}

void qtauNoteEditor::deleteSelected()
{
    if (!_notes.selected.isEmpty())
    {
        _ctrl->reset();
        _ctrl->deleteSelected();
    }
}

void qtauNoteEditor::onEvent(qtauEvent *e)
{
    _ctrl->onEvent(e);
}

void qtauNoteEditor::lazyUpdate()
{
    update();
}

//--------------------------------------------------

void qtauNoteEditor::recalcNoteRects()
{
    _notes.grid.clear();

    _setup.barWidth  = _setup.note.width() * _setup.notesInBar;
    _setup.octHeight = _setup.note.height() * 12;

    double pulsesToPixels = (double)_setup.note.width() / c_midi_ppq;
    int startBar = 0, endBar = 0;

    _playLine = pulsesToPixels*_posPulses;

    foreach (quint64 key, _notes.idMap.keys())
    {
        qne::editorNote &n = _notes.idMap[key];

        int nn1 = n.keyNumber+1;
        //TODO -- validate nn1
        //FIXME NN
        n.r.setRect((double)n.pulseOffset * pulsesToPixels + F_ROUNDER,
                   ((_setup.baseOctave + _setup.numOctaves - 1) * 12 - nn1) * _setup.note.height(),
                   (double)n.pulseLength * pulsesToPixels + F_ROUNDER, _setup.note.height());

        // determine bar(s) that have that note fully or partially
        startBar = n.r.left()  / _setup.barWidth;
        endBar   = n.r.right() / _setup.barWidth;

        if (endBar >= _notes.grid.size())
            _notes.grid.resize(endBar + 10);



        _notes.grid[startBar].append(n.id);


        if (endBar != startBar)
            _notes.grid[endBar].append(n.id);
    }
}

void qtauNoteEditor::updateBGCache()
{
    _setup.barWidth  = _setup.note.width()  * _setup.notesInBar;
    _setup.octHeight = _setup.note.height() * 12;

    int requiredCacheWidth  = (geometry().width()  / _setup.barWidth  + 2) * _setup.barWidth;
    int requiredCacheHeight = (geometry().height() / _setup.octHeight + 2) * _setup.octHeight;

    if (!_bgCache || (_bgCache->width() < requiredCacheWidth || _bgCache->height() < requiredCacheHeight))
    {
        if (_bgCache)
            delete _bgCache;

        _bgCache = new QPixmap(requiredCacheWidth, requiredCacheHeight);
    }

    // prepare bg/grid data =========================================
    QPainterPath blacks;   // rects for black keys
    QPainterPath outerLines;
    QPainterPath innerLines;

    // calculating indexes of visible notes of grid ----------------------
    int hSt  = 0;
    int vSt  = 0;
    int hEnd = requiredCacheWidth  - 1;
    int vEnd = requiredCacheHeight - 1;

    int pxHOff = 0;
    int pxVOff = 0;

    int barCounter = 0;
    int octCounter = 0;

    // horizontal pass to calc note & bar vertical delimiter lines
    do {
        if (barCounter == _setup.notesInBar)
            barCounter = 0;

        if (barCounter == 0)
        {
            outerLines.moveTo(QPoint(pxHOff, vSt ));
            outerLines.lineTo(QPoint(pxHOff, vEnd));
        }
        else {
            innerLines.moveTo(QPoint(pxHOff, vSt ));
            innerLines.lineTo(QPoint(pxHOff, vEnd));
        }

        barCounter++;
        pxHOff += _setup.note.width();

    } while (pxHOff <= hEnd);

    QRect noteBG(hSt, 0, hEnd - hSt, _setup.note.height());

    // vertical pass to calc note backgrounds, note & octave delimiter lines
    do {
        if (octCounter == 12)
            octCounter = 0;

        if (octCounter == 0)
        {
            outerLines.moveTo(QPoint(hSt,  pxVOff));
            outerLines.lineTo(QPoint(hEnd, pxVOff));
        }
        else {
            innerLines.moveTo(QPoint(hSt,  pxVOff));
            innerLines.lineTo(QPoint(hEnd, pxVOff));
        }

        //--- note bg's --------------
        noteBG.moveTop(pxVOff);

        if (octCounter == 1 || octCounter == 3 || octCounter == 5 || octCounter == 8 || octCounter == 10)
            blacks.addRect(noteBG);
        //----------------------------

        octCounter++;
        pxVOff += _setup.note.height();

    } while (pxVOff <= vEnd);

    // paint 'em! ======================
    _bgCache->fill (Qt::white);
    QPainter p    (_bgCache);
    QBrush   brush(p.brush());

    p.setPen(Qt::NoPen);

    // background -------------
    if (!blacks.isEmpty())
    {
        brush.setStyle(Qt::Dense6Pattern);
        brush.setColor(cdef_color_black_noteline_bg);
        p.setBrush(brush);

        p.drawPath(blacks);
    }

    p.setPen(Qt::SolidLine);

    // lines ------------------
    if (!innerLines.isEmpty())
    {
        p.setPen(QColor(cdef_color_inner_line));
        p.drawPath(innerLines);
    }

    if (!outerLines.isEmpty())
    {
        p.setPen(QColor(cdef_color_outer_line));
        p.drawPath(outerLines);
    }
}

void qtauNoteEditor::setVOffset(int voff)
{
    if (voff != _state.viewport.y())
    {
        _ctrl->reset();
        _state.viewport.moveTop(voff);
        lazyUpdate();
    }
}

void qtauNoteEditor::setHOffset(int hoff)
{
    if (hoff != _state.viewport.x())
    {
        _ctrl->reset();
        _state.viewport.moveLeft(hoff);
        lazyUpdate();
    }
}

QPoint qtauNoteEditor::scrollTo(const QRect &r)
{
    QPoint result = _state.viewport.topLeft();

    if (r.x() < _state.viewport.x())
        result.setX(r.x() - cdef_scroll_margin);
    else
        if (r.x() > _state.viewport.x() + geometry().width() - cdef_cache_line_width - cdef_scroll_margin)
            result.setX(r.x() - geometry().width() / 2);

    if (r.y() < _state.viewport.y())
        result.setY(r.y() - cdef_scroll_margin);
    else
        if (r.y() > _state.viewport.y() + geometry().height() - cdef_scroll_margin)
            result.setY(r.y() - geometry().height() / 2);

    if (result != _state.viewport.topLeft())
        emit requestsOffset(result);

    return result;
}

//----------------------------------------------------

void qtauNoteEditor::paintEvent(QPaintEvent *event)
{
    //lastUpdate = t.elapsed();

    // draw bg
    QRect r(event->rect());

    int hSt          = r.x() + _state.viewport.x();
    int firstBar     = hSt / _setup.barWidth;
    int cacheHOffset = hSt - firstBar * _setup.barWidth;

    int vSt          = r.y() + _state.viewport.y();
    int firstOct     = vSt / _setup.octHeight;
    int cacheVOffset = vSt - firstOct * _setup.octHeight;

    QRect cacheRect(r);
    cacheRect.moveTo(cacheRect.x() + cacheHOffset, cacheRect.y() + cacheVOffset);

    QPainter p(this);
    p.drawPixmap(r, *_bgCache, cacheRect);

    // singing notes with phoneme labels -------
    int barSt    = firstBar;
    int barEnd   = (hSt + r.width()) / _setup.barWidth;

    QPainterPath noteRects;
    QPainterPath selNoteRects;

    QMap<quint64, bool>               processedIDMap;
    QVector<QPainter::PixmapFragment> cachedLabels;

    QPainter cacheP(_labelCache);
    cacheP.setBrush(Qt::white); // to clear pixmap completely

    p.translate(-_state.viewport.topLeft());

    if (barSt < _notes.grid.size())
    {
        if (barEnd >= _notes.grid.size())
            barEnd = _notes.grid.size() - 1;

        bool hasVisibleNotes = false;
        r.moveTo(_state.viewport.topLeft());

        for (int i = barSt; i < barEnd + 1; ++i)
            for (int k = 0; k < _notes.grid[i].size(); ++k)
            {
                qne::editorNote &n = _notes.idMap[_notes.grid[i][k]];

                if (!processedIDMap.contains(n.id) && r.intersects(n.r))
                {
                    hasVisibleNotes = true;

                    if (n.selected) selNoteRects.addRect(n.r);
                    else            noteRects   .addRect(n.r);

                    if (n.r.width() > cdef_lbl_draw_minwidth) // don't draw labels for too narrow rects
                    {
                        QRectF fR(0, n.id * cdef_cache_line_height,
                                  cdef_cache_line_width, cdef_cache_line_height);

                        if (!n.cached)
                        {
                            // clearing cache line
                            cacheP.setCompositionMode(QPainter::CompositionMode_Clear);
                            cacheP.drawRect(fR);
                            cacheP.setCompositionMode(QPainter::CompositionMode_SourceOver);

                            cacheP.drawText(fR, Qt::AlignLeft | Qt::AlignVCenter, n.txt);
                            n.cached = true;
                        }

                        cachedLabels.append(QPainter::PixmapFragment::create(
                                                QPointF(n.r.x() + 55, n.r.y() + 7), fR)); // wtf is with pos?..
                    }
                }

                processedIDMap[n.id] = true; // to avoid processing notes that go through 2 or more bars
            }

        if (hasVisibleNotes)
        {
            p.setPen  (QColor(cdef_color_note_border));
            p.setBrush(QColor(cdef_color_note_bg));

            p.drawPath(noteRects);
            p.setBrush(QColor(cdef_color_note_sel_bg));
            p.setPen(QColor(cdef_color_note_sel));
            p.drawPath(selNoteRects);

            p.drawPixmapFragments(cachedLabels.data(), cachedLabels.size(), *_labelCache);
        }
    }

    if (_state.selectionRect.x() > -1 && _state.selectionRect.y() > -1)
    {
        QPen pen = p.pen();
        pen.setWidth(1);
        pen.setColor(QColor(100,100,100,80));
        p.setPen(pen);
        p.setBrush(QBrush(QColor(128,128,128,50))); // TODO: unhardcode colors?
        p.drawRect(_state.selectionRect);
    }

    if (_state.snapLine > -1)
    {
        QPen pen = p.pen();
        pen.setWidth(1);
        pen.setColor(QColor(cdef_color_snap_line));
        p.setPen(pen);
        p.drawLine(_state.snapLine, vSt, _state.snapLine, vSt + _state.viewport.height());
    }


    if (_playLine > -1)
    {
        QPen pen = p.pen();
        pen.setWidth(1);
        pen.setColor(QColor(0x00FF0000));
        p.setPen(pen);
        p.drawLine(_playLine, vSt, _playLine, vSt + _state.viewport.height());
    }

    _updateCalled = false;
}

void qtauNoteEditor::resizeEvent(QResizeEvent *event)
{
    _state.viewport.setSize(event->size());
    updateBGCache();
    emit heightChanged(_state.viewport.height());
    emit widthChanged (_state.viewport.width ());
}

void qtauNoteEditor::mouseDoubleClickEvent(QMouseEvent *event) { _ctrl->mouseDoubleClickEvent(event); }
void qtauNoteEditor::mouseMoveEvent       (QMouseEvent *event) { _ctrl->mouseMoveEvent(event);        }
void qtauNoteEditor::mousePressEvent      (QMouseEvent *event) { _ctrl->mousePressEvent(event);       }
void qtauNoteEditor::mouseReleaseEvent    (QMouseEvent *event) { _ctrl->mouseReleaseEvent(event);     }

void qtauNoteEditor::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ShiftModifier)
        emit hscrolled(event->delta());
    else if (event->modifiers() & Qt::ControlModifier)
        emit zoomed(event->delta());
    else
        emit vscrolled(event->delta());
}

void qtauNoteEditor::changeController(qtauEdController *c)
{
    if (c)
    {
        if (_lastCtrl)
            delete _lastCtrl;

        if (_ctrl)
            _ctrl->cleanup(); // since we're not deleting last one (it's dangerous), need to stop it if it isn't

        _lastCtrl = _ctrl;
        _ctrl = c;
        _ctrl->init();
    }
}

void qtauNoteEditor::rmbScrollHappened(const QPoint &delta, const QPoint &offset)
{
    emit rmbScrolled(delta, offset);
}

void qtauNoteEditor::eventHappened(qtauEvent *e)
{
    emit editorEvent(e);
}

void qtauNoteEditor::setPlaybackPosition(int pos)
{
    _posPulses = pos;
    double pulsesToPixels = (double)_setup.note.width() / c_midi_ppq;
    _playLine = pulsesToPixels*_posPulses;
    lazyUpdate();
}

inline bool editorNotesComparison(const qne::editorNote *n1, const qne::editorNote *n2)
{
    return n1->pulseOffset < n2->pulseOffset;
}


void qtauNoteEditor::doPhonemeTransformation()
{
    QVector<qne::editorNote*> ednotes;

    foreach (quint64 key, _notes.selected)
    {
        qne::editorNote &n = _notes.idMap[key];

        if(!ednotes.contains(&n)) ednotes.insert(0,&n);
    }

    if(ednotes.length()==0) foreach (quint64 key, _notes.idMap.keys())
    {
        qne::editorNote &n = _notes.idMap[key];
        if(!ednotes.contains(&n)) ednotes.insert(0,&n);
    }

    qStableSort(ednotes.begin(), ednotes.end(), editorNotesComparison);


    QStringList lyrics;

    foreach(qne::editorNote* n,ednotes)
    {
       //qDebug() << n->txt;
       lyrics.push_back(n->txt);
    }

    ISynth* s = qtauController::instance()->getSynth();
    s->doPhonemeTransformation(lyrics);

    qtauEvent_NoteText::noteTextVector v;
    qtauEvent_NoteText::noteTextData d;

    int i=0;
    foreach(qne::editorNote* n,ednotes)
    {
       d.id      = n->id;
       d.txt     = lyrics[i];
       n->cached = false;
       d.prevTxt = n->txt;
       n->txt = d.txt;
       v.append(d);
       i++;
    }

    qtauEvent_NoteText *e = new qtauEvent_NoteText(v);
    this->eventHappened(e);

}


void qtauNoteEditor::reset()
{
    _notes.grid.clear();
    _notes.idMap.clear();
}
