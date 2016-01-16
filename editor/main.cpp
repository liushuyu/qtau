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

#include <QApplication>
#include "Controller.h"
#include <QIcon>
#include "Utils.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/images/appicon_ouka_alice.png"));

    //TODO: metatypes for plugins and jack
    //qRegisterMetaType<EAudioPlayback>("EAudioPlayback");

    qtauController c;
    c.run(); // create UI that can immediately call back, thus requires already created & inited controller

    return app.exec();
}
