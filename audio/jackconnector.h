/*
    This file is part of QTau
    Copyright (C) 2013-2015  Tobias Platen <tobias@platen-software.de>

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

#ifndef JACKCONNECTOR_H
#define JACKCONNECTOR_H

#include <QObject>
#include <jack/jack.h>
#include <jack/ringbuffer.h>
#include "Utils.h"

typedef jack_default_audio_sample_t sample_t;

class JackConnector : public QObject
{
    jack_client_t* _client;
    jack_port_t* _write_port;
    jack_ringbuffer_t* _write_rb;
    bool _playback;

    static int process (jack_nframes_t nframes, void *arg);
    static int sync(jack_transport_state_t state, jack_position_t *pos, void *arg);

    jack_transport_state_t _transport_state;
    jack_transport_state_t _previous_transport_state;
    volatile int _transport_command;
    int _buffer_size;
    bool _use_transport;
    bool _ready;

    Q_OBJECT
public:
    explicit JackConnector(QObject *parent = 0);
    int sampleRate();
    int writeData(void* famebuf,int bytes_per_frame);
    void changeTranportState(int command){_transport_command = command;}
    bool isRolling() {return _transport_state==JackTransportRolling;}
    void setUseTransport(bool use_transport){_use_transport=use_transport;}
    int getBufferSize() {return _buffer_size;}
    float* allocateBuffer();
    void setSync(bool needsSynth){_ready=!needsSynth;}



#define TRANSPORT_NONE 0
#define TRANSPORT_START 1
#define TRANSPORT_STOP 2
#define TRANSPORT_ZERO 3
#define TRANSPORT_ROLLING 4



signals:
    void ready();
    void transportStateChanged(int state);
    void positionChanged(int pos);
    
public slots:
    
};

#endif // JACKCONNECTOR_H

