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

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include <QMap>
#include <QDir>
#include <QThread>
#include "Utils.h"

class MainWindow;
class qtauSynth;
class qtmmPlayer;
class qtauAudioSource;
class qtauSession;
class ISynth;
class IAudioBuffer;
class JackConnector;
#include "PluginInterfaces.h"


// main class of QTau that ties everything together
class qtauController : public QObject, public IController
{
    Q_OBJECT


public:
    explicit qtauController(QObject *parent = 0);
    ~qtauController();

    bool run(); // app startup & setup, window creation
    ISynth* getSynth(); //used by the transcriber
    static qtauController* instance();

    void startPlayback(IAudioBuffer* buffer);
    void selectSinger(QString singerName);
    void updateTempoTimeSignature(int tempo);

signals:
    void setEffect(qtauAudioSource *e, bool replace, bool smoothly, bool copy);
    void setTrack (qtauAudioSource *t, bool replace, bool smoothly, bool copy);


    void playStart();
    void playPause();
    void playStop();

    void playerSetVolume(int level);
    void transportPositionChanged(float pos);

public slots:
    void onAppMessage(const QString& msg);

    void onLoadUST(QString fileName);
    void onSaveUST(QString fileName, bool rewrite);

    //void onAudioPlaybackEnded();
    //void onAudioPlaybackTick(qint64 mcsecElapsed);

    void onRequestSynthesis();
    void onRequestStartPlayback();
    //void onRequestPausePlayback();
    void onRequestStopPlayback();
    void onRequestResetPlayback();

    void pianoKeyPressed(int);
    void pianoKeyReleased(int);

    void transportStateChanged(int state);
    void positionChanged(int frame);
    //void voiceChanged(QString voice);
private slots:
    void jackReady();
    void audioBufferForPlaybackChanged(IAudioBuffer* buffer);

protected:

    JackConnector* _jack;
    IAudioBuffer* _audioBuffer;
    int _jackpos;
    MainWindow *_mainWindow;

    //for playback func
    float* _jack_buffer;
    int _jack_buffer_size;

    qtauSession *_activeSession;
    float _samplesToMeasures;

    bool setupTranslations();
    bool setupPlugins();
    bool setupVoicebanks();

    void initSynth(ISynth *s);
    QMap<QString, ISynth*> _synths;
    ISynth* _activeSynth;

    QDir _pluginsDir;
    QStringList _voices;

    bool _useJackTransport;
    bool _localTransportRolling;
    int _lasttransportjackpos;

    void newEmptySession();
public:
    const QStringList& voices(){return _voices;}
    void setJackTranportEnabled(bool enabled);
};

#endif // CONTROLLER_H
