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

#ifndef PLUGININTERFACES_H
#define PLUGININTERFACES_H

#include <QtPlugin>
#include <QStringList>
#include "Utils.h"
//#include "utauloid/ust.h"

//class qtauAudioSource;
class IAudioBuffer;

class IController
{
public:
    virtual void startPlayback(IAudioBuffer*)=0;
};

typedef struct SynthConfig {
    vsLog *log = nullptr;
    IController* controller = nullptr;

    SynthConfig(vsLog &l,IController* c) : log(&l), controller(c) {}
} SSynthConfig;




class IAudioBuffer
{
public:
    virtual int bufferSizeHint() {return 0;};
    virtual int readData(float* data,int size) = 0;
    virtual int sampleRate() = 0;
    virtual int setSeek(int pos) = 0;
};

class ISynth
{
public:
    virtual ~ISynth() {}

    virtual QString name()                     = 0;
    virtual QString description()              = 0;
    virtual QString version()                  = 0;

    virtual void setup(SSynthConfig &cfg)      = 0;
    virtual bool setVoicebank(const QString&)  = 0;

    virtual bool setScore(const QJsonArray&)   = 0;


    virtual IAudioBuffer* synthesize()  = 0;
    virtual bool synthesize(const QString&)    = 0;
    virtual void synthesizeAsync(const QString&) = 0;

    virtual bool isVbReady()                   = 0;
    virtual bool isVocalsReady()               = 0;

    // if synth can stream data as it's being created
    virtual bool supportsStreaming()           = 0;
    virtual QString getTranscription(QString)  = 0;
    virtual bool doPhonemeTransformation(QStringList&) = 0;

    virtual bool setVoice(QString voiceName) = 0;
    virtual QStringList listVoices() = 0;

    virtual void setTempo(int tempo)=0;
};

#define c_isynth_comname "moe.ecantorix.qtau.ISynth"

Q_DECLARE_INTERFACE(ISynth, c_isynth_comname)

#endif // PLUGININTERFACES_H
