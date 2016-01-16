#include "tempodialog.h"
#include "ui_tempodialog.h"

TempoDialog::TempoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TempoDialog)
{
    ui->setupUi(this);
}

TempoDialog::~TempoDialog()
{
    delete ui;
}

void TempoDialog::setTempoTimeSignature(TempoTimeSig sig)
{
    _timeSig = sig;
    this->ui->tempo->setValue(sig.tempo);
    this->ui->timeSig->setValue(sig.numerator);
    this->ui->timeSig2->setValue(sig.denominator);
}

void TempoDialog::on_tempo_valueChanged(int arg1)
{
    _timeSig.tempo = arg1;
}

void TempoDialog::on_timeSig_valueChanged(int value)
{
    if(value>_timeSig.denominator) {
        value=_timeSig.denominator;
        this->ui->timeSig->setValue(value);
    }
    _timeSig.numerator=value;
}

void TempoDialog::on_timeSig2_valueChanged(int value)
{
      if(_timeSig.denominator==4 && value==5) value=8;
      if(_timeSig.denominator==4 && value==3) value=2;
      if(_timeSig.denominator==2 && value==3) value=4;
      if(_timeSig.denominator==8 && value==7) value=4;
      if(_timeSig.denominator != value)
      {
          this->ui->timeSig2->setValue(value);
      }
      _timeSig.denominator=value;
}
