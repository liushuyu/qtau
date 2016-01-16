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

#include "mainwindow.h"

#include "Session.h"
#include "Controller.h"
#include "PluginInterfaces.h"
#include "Utils.h"
#include <QJsonDocument>

#include "../audio/jackconnector.h"
#include <QApplication>
#include <QPluginLoader>
#include <QDebug>

qtauController::qtauController(QObject *parent) :
    QObject(parent), _jack(nullptr), _mainWindow(nullptr), _activeSession(nullptr), _audioBuffer(nullptr)
{
    _jack = new JackConnector();
    connect(_jack,&JackConnector::ready,this,&qtauController::jackReady);
    connect(_jack,&JackConnector::transportStateChanged,this,&qtauController::transportStateChanged);
    connect(_jack,&JackConnector::positionChanged,this,&qtauController::positionChanged);

    _jack_buffer = _jack->allocateBuffer();
    _jack_buffer_size = _jack->getBufferSize();

    _useJackTransport=true;
    _localTransportRolling=false;

    //TODO: internal transport

    float bpm=140;
    int numerator=4;//TODO hardcoded in editor
    int denominator=4;
    int fs=44100;
    _samplesToMeasures = (bpm*numerator)/(denominator*fs*60);

    setupTranslations();
    setupPlugins();
    //setupVoicebanks();
}

void qtauController::jackReady()
{

    memset(_jack_buffer,0,_jack_buffer_size*sizeof(float));
    int size=0;

    _jack->setSync(_activeSession->synthNeeded());

    bool isRolling;
    if(_useJackTransport) isRolling=_jack->isRolling();
    else isRolling=_localTransportRolling;

    if(_audioBuffer)
    {
        if(isRolling)
            size = _audioBuffer->readData(_jack_buffer,_jack_buffer_size);
    }

    if(isRolling)
    {
        _jackpos += _jack_buffer_size;
        if(_useJackTransport) _lasttransportjackpos = _jackpos;
        transportPositionChanged(_samplesToMeasures*_jackpos);//FIXME 120 bpm 44100 khz 4/4 time signature

    }

    _jack->writeData(_jack_buffer,_jack_buffer_size*sizeof(float));


}

qtauController::~qtauController()
{
    //TODO -- shutdown jack
    delete _mainWindow;
}

//------------------------------------------

static qtauController* singleton=0;

bool qtauController::run()
{
    _mainWindow = new MainWindow();
    _mainWindow->show();

    // TODO: load previous one from settings, FIXME multi session
    newEmptySession();
    _mainWindow->setController(*this, *this->_activeSession);

    singleton = this;

    return true;
}

qtauController* qtauController::instance()
{
    return singleton;
}


bool qtauController::setupTranslations()
{
    //FIXME: english only
    return false;
}


bool qtauController::setupPlugins()
{
    //FIXME plugins dir, should not be hardcoded
    _pluginsDir = QDir(qApp->applicationDirPath());
    if(_pluginsDir.cd("plugins")==false) return false;

    vsLog::i("Searching for plugins in " + _pluginsDir.absolutePath());
    //TODO: other plugin types

    foreach (QString fileName, _pluginsDir.entryList(QDir::Files))
    {

        QPluginLoader loader(_pluginsDir.absoluteFilePath(fileName));
        QObject *plugin = loader.instance();

        if (plugin)
        {
            ISynth* s = qobject_cast<ISynth*>(plugin);

            qDebug() << fileName;

            if (s)
                initSynth(s);

            //TODO other interfaces
        }
        else vsLog::d("Incompatible plugin: " + fileName);
    }

    return false;
}


void qtauController::initSynth(ISynth *s)
{
    qDebug() << s->name();
    if (!_synths.contains(s->name()))
    {
        //if(activeSession==nullptr) qDebug() << "error needs to be fixed\n";
        SSynthConfig sconf(*vsLog::instance(),this);
        s->setup(sconf);

        vsLog::s("Adding synthesizer " + s->name());
        _synths[s->name()] = s;

        _voices.append(s->listVoices());
        ///TODOx:

    }
    else vsLog::d("Synthesizer " + s->name() + " is already registered!");
}

void qtauController::selectSinger(QString singerName)
{

    foreach (QString synthName, _synths.keys()) {
        if(_synths[synthName]->setVoice(singerName))
        {
            _activeSynth = _synths[synthName];
        }
    }
}

void qtauController::newEmptySession()
{
    _activeSession = new qtauSession(this);
    connect(_activeSession, &qtauSession::requestSynthesis,      this, &qtauController::onRequestSynthesis     );
    connect(_activeSession, &qtauSession::requestStartPlayback,  this, &qtauController::onRequestStartPlayback );
    //connect(_activeSession, &qtauSession::requestPausePlayback,  this, &qtauController::onRequestPausePlayback );
    connect(_activeSession, &qtauSession::requestStopPlayback,   this, &qtauController::onRequestStopPlayback  );
    connect(_activeSession, &qtauSession::audioBufferForPlaybackChanged, this, &qtauController::audioBufferForPlaybackChanged);
    connect(_activeSession, &qtauSession::requestResetPlayback,  this, &qtauController::onRequestResetPlayback );

}

void qtauController::audioBufferForPlaybackChanged(IAudioBuffer* buffer)
{
    //FIXME: old audiobuffer
    _audioBuffer = buffer;
    _jack->changeTranportState(TRANSPORT_START);
}


//------------------------------------------

void qtauController::onLoadUST(QString fileName)
{
    if (!fileName.isEmpty())
    {
        if (!_activeSession)
            newEmptySession();
        _activeSession->loadUST(fileName);
    }
    else vsLog::d("Controller: empty UST file name");
}

void qtauController::onSaveUST(QString fileName, bool rewrite)
{
    if (_activeSession && !_activeSession->isSessionEmpty())
    {
        QFile uf(fileName);

        if (uf.open(QFile::WriteOnly))
        {
            if (uf.size() == 0 || rewrite)
            {
                uf.reset(); // maybe it's redundant?..
                QJsonArray array;
                _activeSession->ustJson(array);
                QJsonDocument doc(array);
                uf.write(doc.toJson());
                uf.close();

                _activeSession->setFilePath(fileName);
                _activeSession->setSaved();

                vsLog::s("UST saved to " + fileName);
            }
            else vsLog::e("File " + fileName + " is not empty, rewriting cancelled");
        }
        else vsLog::e("Could not open file " + fileName + " to save UST");
    }
    else vsLog::e("Trying to save ust from empty session!");
}

void qtauController::onAppMessage(const QString &msg)
{
    vsLog::i("IPC message: " + msg);
    _mainWindow->setWindowState(Qt::WindowActive); // does nothing
}

ISynth* qtauController::getSynth()
{
    return _activeSynth;
}

//new synth api required (madde)
void qtauController::pianoKeyPressed(int keyNum)
{
    //currently ignored:: use hzosc+klattgen, need
    (void)keyNum;
}

void qtauController::pianoKeyReleased(int /*keyNum*/)
{
    //TODO: stop synth
}

void qtauController::onRequestSynthesis()
{

    if (!_activeSession->isSessionEmpty())
    {
        if (!_synths.isEmpty())
        {
            emit playStop();
            _jack->changeTranportState(TRANSPORT_START);
            onRequestStartPlayback();
        }
        else vsLog::e("No synthesizers registered");
    }
    else vsLog::e("Session is empty, nothing to synthesize.");
}

void qtauController::onRequestStartPlayback()
{
    if(_useJackTransport) _jack->changeTranportState(TRANSPORT_START);
    else {
        ///,,,,,,,,,,,,,,,,,,,
        transportStateChanged(TRANSPORT_START);
    }
}

void qtauController::onRequestStopPlayback()
{
    if(_useJackTransport) _jack->changeTranportState(TRANSPORT_STOP);
    else {
        _localTransportRolling=false;
    }
}

void qtauController::onRequestResetPlayback()
{
    if(_useJackTransport) _jack->changeTranportState(TRANSPORT_ZERO);
    else
    {
        _jackpos=0;
        emit transportPositionChanged(0);
    }
    if(_audioBuffer) _audioBuffer->setSeek(0);
}

void qtauController::transportStateChanged(int state)
{
    if(state==TRANSPORT_START)
    {
        if(_activeSession->synthNeeded())
        {

            QJsonArray ust;
            _activeSession->ustJson(ust);
            _activeSynth->setScore(ust);
            int tempo=_activeSession->getTempo();
            qDebug() << "Tempo" << tempo;
            if(tempo==0) tempo=120;
            _activeSynth->setTempo(tempo);
            _activeSynth->synthesizeAsync(_activeSession->getFilePath());

            if(_useJackTransport) _jack->changeTranportState(TRANSPORT_STOP);
            else _localTransportRolling=false;
        }
        else
        {
            if(_useJackTransport) _jack->changeTranportState(TRANSPORT_ROLLING);
            else _localTransportRolling=true;

            if(_audioBuffer) _audioBuffer->setSeek(_jackpos);
        }
    }

}

void qtauController::positionChanged(int pos)
{
    emit transportPositionChanged(_samplesToMeasures*pos); //FIXME
    _jackpos = pos;
    if(_useJackTransport) _lasttransportjackpos = _jackpos;
    if(_audioBuffer) _audioBuffer->setSeek(pos);

}

void qtauController::startPlayback(IAudioBuffer *buffer)
{
    if(_audioBuffer)
    {
        delete _audioBuffer;
    }
    _audioBuffer=buffer;
    _activeSession->resetNeedsSynth();
    _jack->changeTranportState(TRANSPORT_START);
    if(_useJackTransport==false)
        _localTransportRolling=true;

}

void qtauController::setJackTranportEnabled(bool enabled)
{
    if(enabled==false)
        _jack->changeTranportState(TRANSPORT_STOP);

    if(enabled==true)
        positionChanged(_lasttransportjackpos);

    _localTransportRolling=false;

    _jack->setUseTransport(enabled);
    _useJackTransport = enabled;
    _localTransportRolling = false;

}

void  qtauController::updateTempoTimeSignature(int tempo)
{
    float bpm=tempo;
    int fs=44100;
    _samplesToMeasures = bpm/(fs*60);
}
