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

#ifndef PIANO_H
#define PIANO_H

#include "Utils.h"
#include <QWidget>

#include <QDebug>

class QPixmap;

//write new PianoRoll that works using affine transformations and modulo 12

class qtauPiano : public QWidget
{
    Q_OBJECT

public:
    qtauPiano(QWidget *parent = 0);
    ~qtauPiano();

    void setOffset(int voff);
    void configure(const SNoteSetup &newSetup);

signals:
    void keyPressed (int keyNum);
    void keyReleased(int keyNum);

    void scrolled(int delta);
    void heightChanged(int newHeight);

public slots:
    //

protected:
    void paintEvent           (QPaintEvent  *event);
    void resizeEvent          (QResizeEvent *event);

    void mouseDoubleClickEvent(QMouseEvent  *event);
    void mouseMoveEvent       (QMouseEvent  *event);
    void mousePressEvent      (QMouseEvent  *event);
    void mouseReleaseEvent    (QMouseEvent  *event);
    void wheelEvent           (QWheelEvent  *event);

    void onHover(bool inside);
    bool eventFilter(QObject *object, QEvent *event);

    void initPiano(int baseOctave, int octavesNum);

    QPoint   _offset; // graphical offset of keys, used for virtual scrolling
    QPixmap *_labelCache;

public:
    //-------------------------------------------------------------
    typedef enum {
        Passive = 0,
        Pressed,
        Highlighted
    } pianoKeyState;



protected:
    int _pressedKey;
    int _hoveredKey;
    SNoteSetup _ns;

};

#endif // PIANO_H
