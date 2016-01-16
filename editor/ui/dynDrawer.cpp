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

#include "ui/dynDrawer.h"
#include "ui/Config.h"

//FIXME dynamics not supported yet, need synth/storeage support

#include <QPainter>
#include <qevent.h>
#include <QPixmap>


qtauDynLabel::qtauDynLabel(const QString &txt, QWidget *parent) : QLabel(txt, parent), _state(off) {}
qtauDynLabel::~qtauDynLabel() {}

void qtauDynLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        emit leftClicked();
    else if (event->button() == Qt::RightButton)
        emit rightClicked();
}

qtauDynLabel::EState qtauDynLabel::state() { return _state; }
void qtauDynLabel::setState(EState s)      { _state = s; }

//===========================================================================


qtauDynDrawer::qtauDynDrawer(QWidget *parent) :
    QWidget(parent), _offset(0), _bgCache(nullptr)
{
    //
}

qtauDynDrawer::~qtauDynDrawer()
{
    //
}

void qtauDynDrawer::setOffset(int off)
{
    if (off != _offset)
    {
        _offset = off;
        update();
    }
}

void qtauDynDrawer::configure(const SNoteSetup &newSetup)
{
    _ns = newSetup;
    updateCache();
    update();
}

//------------------------------------

void qtauDynDrawer::paintEvent(QPaintEvent *event)
{
    // draw bg
    int hSt         = event->rect().x() + _offset;
    int barWidth    = _ns.note.width() * _ns.notesInBar;
    int firstBar    = hSt / barWidth;
    int cacheOffset = hSt - firstBar * barWidth;

    QRect screenRect(event->rect());
    QRect cacheRect(screenRect);
    cacheRect.moveLeft(cacheRect.x() + cacheOffset);

    QPainter p(this);
    p.drawPixmap(screenRect, *_bgCache, cacheRect);
}

void qtauDynDrawer::resizeEvent(QResizeEvent*)
{
    updateCache();
}

void qtauDynDrawer::mouseDoubleClickEvent(QMouseEvent*)
{
    //
}

void qtauDynDrawer::mouseMoveEvent(QMouseEvent*)
{
    //
}

void qtauDynDrawer::mousePressEvent(QMouseEvent*)
{
    //
}

void qtauDynDrawer::mouseReleaseEvent(QMouseEvent*)
{
    //
}

void qtauDynDrawer::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier)
        emit zoomed(event->delta());
    else
        emit scrolled(event->delta());
}

void qtauDynDrawer::updateCache()
{
    if (geometry().height() == 0)
    {
        vsLog::e("Zero height of dynDrawer in updateCache()... This definitely shouldn't happen.");
        return;
    }

    // bars fully fit in + 2 for partially visible bars
    int barWidth = _ns.note.width() * _ns.notesInBar;
    int requiredCacheWidth = (geometry().width() / barWidth + 2) * barWidth;

    if (!_bgCache || (_bgCache->width() < requiredCacheWidth || _bgCache->height() < geometry().height()))
    {
        if (_bgCache)
            delete _bgCache;

        _bgCache = new QPixmap(requiredCacheWidth, geometry().height());
    }

    // prepare painting data
    int vSt  = 0;
    int vEnd = geometry().height();
    int mSt  = vEnd / 2 - 10;
    int mEnd = mSt + 20;

    int barCounter = 0;
    int pxOff      = 0;

    QVector<QPoint> noteLines;
    QVector<QPoint> barLines;

    while (true)
    {
        if (pxOff >= requiredCacheWidth)
            break;

        if (barCounter == _ns.notesInBar)
            barCounter = 0;

        if (barCounter == 0) // bar line
        {
            barLines.append(QPoint(pxOff, vSt ));
            barLines.append(QPoint(pxOff, vEnd));
        }
        else
        {
            noteLines.append(QPoint(pxOff, mSt ));
            noteLines.append(QPoint(pxOff, mEnd));
        }

        barCounter++;
        pxOff += _ns.note.width();

    }

    // paint 'em!
    _bgCache->fill(Qt::white);
    QPainter p(_bgCache);

    p.setPen(QColor(cdef_color_inner_line));
    p.drawLines(noteLines);

    p.setPen(QColor(cdef_color_outer_line));
    p.drawLines(barLines);
}
