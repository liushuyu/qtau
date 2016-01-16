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

#ifndef LESYNTH_H
#define LESYNTH_H

#include "PluginInterfaces.h"
#include <QJsonArray>
#include <QProcess>


class eSpeakSynth : public QObject, public ISynth
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID c_isynth_comname FILE "leSynth.json")
    Q_INTERFACES(ISynth)

public:
    eSpeakSynth() {proc=nullptr;}
    QString name()                              override;
    QString description()                       override;
    QString version()                           override;

    void setup(SSynthConfig &cfg)               override;
    bool setVoicebank(const QString &path)      override;

    bool setScore(const QJsonArray&)            override;

    IAudioBuffer* synthesize()                  override;
    bool synthesize(const QString &outFileName) override;
    void synthesizeAsync(const QString &inFileName) override;

    bool isVbReady()                            override;
    bool isVocalsReady()                        override;

    bool supportsStreaming()                    override;
    QString getTranscription(QString txt)       override;
    bool doPhonemeTransformation(QStringList& lyrics) override;

    bool setVoice(QString voiceName) override;
    QStringList listVoices() override;
    void setTempo(int tempo){_bpm=tempo;}


protected:
    vsLog*  log;
    QJsonArray score;
    QProcess* proc;
    IController* ctrl;
    bool isResampler;
    QString voiceName;
    float _bpm;

    inline void sLog(const QString &msg) { log->addMessage(msg, ELog::success); }
    inline void dLog(const QString &msg) { log->addMessage(msg, ELog::debug);   }
    inline void iLog(const QString &msg) { log->addMessage(msg, ELog::info);    }
    inline void eLog(const QString &msg) { log->addMessage(msg, ELog::error);   }

public slots:
    void processStarted();
    //void processError(QProcess::ProcessError);
    void processOutput();
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
};

#endif // LESYNTH_H
