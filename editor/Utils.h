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

#ifndef UTILS_H
#define UTILS_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QPoint>
#include <QSize>
#include <QPair>

#if 0
#define DEVLOG_BEGIN() if(1){FILE* logchan=stderr; fprintf(logchan,"DEBUG: %s:%i ",__FILE__,__LINE__)
#define DEVLOG_S(x) if(logchan) fprintf(logchan,"%s ",x);
#define DEVLOG_U(x) if(logchan) fprintf(logchan,"%u ",x);
#define DEVLOG_CR() if(logchan) fprintf(logchan,"\n");
#define DEVLOG_END() fprintf(logchan,"\n");} else {}
#endif



const QString c_qtau_name = QStringLiteral("QTau");
const QString c_qtau_ver  = QStringLiteral("α1");

/* Pulses Per Quarter note - standard timing resolution of a MIDI sequencer,
     needs to be divisible by 16 and 3 to support 1/64 notes and triplets.
     Used to determine note offset (in bars/seconds) and duration */
const int c_midi_ppq       = 480;
const int c_zoom_num       = 17;
const int cdef_zoom_index  = 4;
const int cdef_note_height = 14;

const int c_zoom_note_widths[c_zoom_num] = {16,  32,  48,  64,  80,
                                            96,  112, 128, 144, 160,
                                            176, 208, 240, 304, 368, 480, 608};

struct SNoteSetup {
    QSize note;     // gui dimensions (in pixels)

    int baseOctave;
    int numOctaves;

    int noteLength; // denominator of note length 1
    int notesInBar; // noteLength=4 and notesInBar=4 means music time signature 4/4
    int tempo;      // bpm
    int quantize;   // 1/x note length, used as unit of offset for singing notes
    int length;     // 1/x singing note length unit (len=4 means 1/4, 2/4 etc +1/4)

    int barWidth;
    int octHeight;

    SNoteSetup() : note(c_zoom_note_widths[cdef_zoom_index], cdef_note_height),
        baseOctave(1), numOctaves(7), noteLength(4), notesInBar(4), tempo(120), quantize(32), length(32),
        barWidth(note.width() * notesInBar), octHeight(note.height() * 12) {}
};
//----------------------------------------------------

enum class ELog : char {
    none,
    info,
    debug,
    error,
    success
};


class vsLog : public QObject
{
    Q_OBJECT

public:
    vsLog() : lastTime(QStringLiteral("none")), saving(true) {}
    static vsLog* instance();

    static void i(const QString &msg) { instance()->addMessage(msg, ELog::info);    }
    static void d(const QString &msg) { instance()->addMessage(msg, ELog::debug);   }
    static void e(const QString &msg) { instance()->addMessage(msg, ELog::error);   }
    static void s(const QString &msg) { instance()->addMessage(msg, ELog::success); }
    static void n()                   { instance()->addMessage("",  ELog::none);    }
    static void r(); // reemit stored message (removing it from history)

    void enableHistory(bool enable) { saving = enable; }

public slots:
    void addMessage(const QString&, ELog);

signals:
    void message(QString, ELog);  // int is msg type from enum "vsLog::msgType"

protected:
    QString lastTime;
    bool    saving;

    QList<QPair<ELog, QString>> history;
    void reemit(const QString &msg, ELog type);

};

union uichar {
    char    c[4];
    quint32 i;
};


#endif // UTILS_H
