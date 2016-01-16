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

#ifndef DYNDRAWER_H
#define DYNDRAWER_H

#include "Utils.h"

#include <QLabel>

class QPixmap;


class qtauDynLabel : public QLabel
{
    Q_OBJECT

public:
    explicit qtauDynLabel(const QString& txt = "", QWidget *parent = 0);
    ~qtauDynLabel();

    typedef enum {
        off = 0,
        front,
        back
    } EState;

    EState state();
    void setState(EState s);

signals:
    void leftClicked();
    void rightClicked();

protected:
    void mousePressEvent(QMouseEvent * event);

    EState _state;

};

//----------------------------------------------

class qtauDynDrawer : public QWidget
{
    Q_OBJECT

public:
    qtauDynDrawer(QWidget *parent = 0);
    ~qtauDynDrawer();

    void setOffset(int off);
    void configure(const SNoteSetup &newSetup);



signals:
    void scrolled(int delta);
    void zoomed  (int delta);

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

    int _offset;

    SNoteSetup _ns;

    QPixmap *_bgCache;
    void updateCache();

};

#endif // DYNDRAWER_H
