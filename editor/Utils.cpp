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

#include "Utils.h"
#include <QTime>
#include <QDebug>

#include <qmath.h>

//TODO change utils.cpp to use only interface ?

Q_GLOBAL_STATIC(vsLog, vslog_instance)

vsLog* vsLog::instance()
{
    return vslog_instance();
}

void vsLog::r()
{
    vslog_instance()->enableHistory(false);
    auto &hst = vslog_instance()->history;

    for (auto &stored: hst)
    {
        if (stored.second.isEmpty()) vslog_instance()->reemit("", ELog::none);
        else                         vslog_instance()->reemit(stored.second, stored.first);
    }

    hst.clear();
}

void vsLog::reemit(const QString &msg, ELog type) { emit message(msg, type); }

void vsLog::addMessage(const QString &msg, ELog type)
{
    QString m = msg;

    if (type == ELog::none)
        m = " ";
    else
    {
        //qDebug() << m;

        m.prepend("\t");
        QString time = QTime::currentTime().toString();

        if (time.compare(lastTime))
        {
            m.prepend(time);
            lastTime = time;
        }
    }

    emit message(m, type);

    if (saving)
        history.append(QPair<ELog, QString>(type, m));
}
