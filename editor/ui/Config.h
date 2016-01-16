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

#ifndef QTAUCONFIG_H
#define QTAUCONFIG_H

#include <QPoint>
#include <QSize>
#include <QRect>
#include <QVector>
#include <QMap>
#include <QString>


// default config for custom ui widgets
const unsigned int cdef_color_black_key_bg      = 0xff666666;
const unsigned int cdef_color_black_noteline_bg = 0xffb8b8b8; // aarrggbb
const unsigned int cdef_color_inner_line        = 0xffbcbcbc;
const unsigned int cdef_color_outer_line        = 0xff0095c6;

const unsigned int cdef_color_note_border       = 0xff000000;
const unsigned int cdef_color_note_bg           = 0xffffffff;
const unsigned int cdef_color_note_sel          = 0xff0095c6;
const unsigned int cdef_color_note_sel_bg       = 0xffe3eeff;

const unsigned int cdef_color_selrect           = 0xff00857d;
const unsigned int cdef_color_selrect_bg        = 0x2200857d;

const unsigned int cdef_color_snap_line         = 0xaa3effab;

const unsigned int cdef_color_piano_lbl_wh      = 0xff000000; // colors for white and black key lablels
const unsigned int cdef_color_piano_lbl_wh_on   = 0xff00857d;
const unsigned int cdef_color_piano_lbl_bl      = 0xffffffff;
const unsigned int cdef_color_piano_lbl_bl_on   = 0xffeafffe;

const unsigned int cdef_color_logtab_err        = 0xffff0000;

const QString      cdef_color_dynbtn_off        = "#b7b7b7"; // CSS color
const QString      cdef_color_dynbtn_bg         = "#77ded8"; // background graph button color
const QString      cdef_color_dynbtn_on         = "#00857d"; // foreground graph button color
const QString      cdef_color_dynbtn_on_bg      = "#eafffe";

const QPoint c_slideclick_limit = QPoint(3, 3);


// editor config
namespace qne {
    typedef struct _editorNote {
        quint64 id;
        int     keyNumber;
        int     pulseLength;
        int     pulseOffset;
        QString txt;

        bool    cached;
        bool    selected;

        QRect   r; // rectangle in pixels
        QPoint  dragSt; // used to store start topleft pos in pixels at start of dragging
        // settings
        // effects

        _editorNote() : id(0), keyNumber(0), pulseLength(0), pulseOffset(0), txt("TEXT_HERE"),
            cached(false), selected(false), r(0,0,0,0) {}
    } editorNote;

    typedef struct _editorNotes {
        QMap<quint64, editorNote>  idMap;
        QVector<QVector<quint64> > grid;
        QVector<quint64>           selected;
    } editorNotes;

    typedef struct _editorState {
        QRect viewport;

        bool rmbScrollEnabled;
        bool editingEnabled;
        bool gridSnapEnabled;

        QRect selectionRect;
        int snapLine;

        _editorState() : rmbScrollEnabled(true), editingEnabled(false), gridSnapEnabled(true), snapLine(-1) {}
    } editorState;
}


#endif // QTAUCONFIG_H
