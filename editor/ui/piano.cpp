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

#include "ui/piano.h"
#include <qevent.h>
#include <qpainter.h>
#include <QDebug>

#include "ui/Config.h"

const int CONST_PIANO_LABELHEIGHT = 14;
const int CONST_PIANO_LABELWIDTH  = 50;

qtauPiano::qtauPiano(QWidget *parent) :
    QWidget(parent), _offset(0,0), _labelCache(nullptr), _pressedKey(-1), _hoveredKey(-1)
{
    // setup widget
    setFocusPolicy(Qt::NoFocus);
    setMouseTracking(true);
    installEventFilter(this);
}

bool qtauPiano::eventFilter(QObject *object, QEvent *event)
{
    if(object==this && (event->type()==QEvent::Enter || event->type()==QEvent::Leave))
        onHover(event->type()==QEvent::Enter);

    return false;
}

//----------------------------------------------------------------
inline void cacheLbl(QPainter &p, QRect &r, const QString &txt, int &vOff, const QColor &offClr, const QColor &onClr)
{
    r.moveTopLeft(QPoint(0, vOff));
    p.setPen(offClr);
    p.drawText(r, txt);

    r.moveLeft(CONST_PIANO_LABELWIDTH);
    p.setPen(onClr);
    p.drawText(r, txt);

    vOff += CONST_PIANO_LABELHEIGHT;
}

inline void whiteLbl(QPainter &p, QRect &r, const QString &txt, int &vOff)
{
    cacheLbl(p, r, txt, vOff, QColor(cdef_color_piano_lbl_wh), QColor(cdef_color_piano_lbl_wh_on));
}

inline void blackLbl(QPainter &p, QRect &r, const QString &txt, int &vOff)
{
    cacheLbl(p, r, txt, vOff, QColor(cdef_color_piano_lbl_bl), QColor(cdef_color_piano_lbl_bl_on));
}
//----------------------------------------------------------------


qtauPiano::~qtauPiano()
{
}

void qtauPiano::setOffset(int voff)
{
    if (_offset.y() != voff)
    {
        _offset.setY(voff);
        update();
    }
}

void qtauPiano::configure(const SNoteSetup &newSetup)
{
    _ns = newSetup;
    update();
}

//move to midi_support.c
bool isBlack(int num)
{
    num = num % 12;
    if(num==1 || num==3 || num==6 || num==8 || num==10) return true;
    return false;
}

//--------------------------------------------

void qtauPiano::paintEvent(QPaintEvent *event)
{
    //FIXME: all notes are redrawn -- TODO @ add transpose support
    //do not hardcode minimum and maximum notes

    QVector<QRect> whites;
    QVector<QRect> blacks;

    double pixels_per_semitone = _ns.octHeight/12.0;

    int basenote = (_ns.baseOctave+_ns.numOctaves)*12-1;//basenote is a B in a flipped geometry


    for(int i=_ns.baseOctave*12;i<=basenote;i++)
    {
        int w = this->width();
        QRect r(0,(basenote-i)*pixels_per_semitone,w,pixels_per_semitone);

        //gray for thirds ?? (as in rosegarden)

        if(isBlack(i)) blacks.append(r);
        else whites.append(r);
        //TODO only visible keys
    }

    QPainter p(this);
    p.translate(-_offset);

    p.setBrush(Qt::white);
    p.drawRects(whites);

    p.setBrush(QColor(cdef_color_black_key_bg));
    p.drawRects(blacks);

    for(int i=12;i<96;i++)
    {
        int w = this->width();
        QRect r(0,(95-i)*pixels_per_semitone,w,pixels_per_semitone);
        if(i%12==0) //draw hover notename (transposed)
        {
               int oct = i/12-2;
               p.drawText(r,"C"+QVariant(oct).toString());//TODO -- allow transpose
        }

    }
}

void qtauPiano::resizeEvent(QResizeEvent * event)
{

    emit heightChanged(event->size().height());
}


//------------ input handling ---------------

void qtauPiano::mouseDoubleClickEvent(QMouseEvent *event)
{
    //IGNORE -> set Transpose ???, does nothing
}



void qtauPiano::mouseMoveEvent(QMouseEvent *event)
{
    //hovered event (show midi note number and notename, position in selected key)
}

void qtauPiano::mousePressEvent(QMouseEvent *event)
{
    //int nn(QMouseEvent *event)
    double y = event->y()+_offset.y();
    double pixels_per_semitone = _ns.octHeight/12.0;
    int basenote = (_ns.baseOctave+_ns.numOctaves)*12-1;//basenote is a B in a flipped geometry
    int ngeom = y/pixels_per_semitone;
    int nn = basenote-ngeom;
    //nn_func_end
    //qDebug() << nn;//midi note numbers are corrent
    emit keyPressed(nn);
    _pressedKey = nn;

}



void qtauPiano::mouseReleaseEvent(QMouseEvent *event)
{
    emit keyReleased(_pressedKey);
    _pressedKey=-1;//invalid
}

void qtauPiano::wheelEvent(QWheelEvent *event)
{
    emit scrolled(event->delta());
}


void qtauPiano::onHover(bool inside)
{
       //NO hover in new design ?
}


