/*
    This file is part of QTau
    Copyright (C) 2015  Tobias Platen <tobias@platen-software.de>

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

#ifndef TEMPODIALOG_H
#define TEMPODIALOG_H

#include <QDialog>

namespace Ui {
class TempoDialog;
}

struct TempoTimeSig
{
    float tempo;
    int numerator;
    int denominator;
};

class TempoDialog : public QDialog
{
    Q_OBJECT
    TempoTimeSig _timeSig;
public:
    explicit TempoDialog(QWidget *parent = 0);
    ~TempoDialog();
    void setTempoTimeSignature(TempoTimeSig sig);
    TempoTimeSig tempoTimeSignature(){return _timeSig;}



private slots:
    void on_tempo_valueChanged(int arg1);

    void on_timeSig_valueChanged(int arg1);

    void on_timeSig2_valueChanged(int arg1);

private:
    Ui::TempoDialog *ui;
};

#endif // TEMPODIALOG_H
