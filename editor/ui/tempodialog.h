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
