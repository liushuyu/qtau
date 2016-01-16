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
#include "jackconnector.h"
#include <assert.h>

#include <stdio.h>
#include <QDebug>
#include <QMessageBox>
#include <QIcon>


int JackConnector::process(jack_nframes_t nframes, void *arg)
{

    JackConnector* conn = (JackConnector*) arg;

    static int connected=0;
    if(connected==false)
    {
        jack_connect(conn->_client,"QTau:out","system:playback_2");//FOR testing
        connected=1;
    }

    sample_t* write_samp = (sample_t*)jack_port_get_buffer(conn->_write_port,nframes);

    if(jack_ringbuffer_read_space(conn->_write_rb)>=sizeof(sample_t)*nframes)
    {
            jack_ringbuffer_read(conn->_write_rb,(char*)write_samp,sizeof(sample_t)*nframes);
    }
    else
    {
        for(int i=0;i<nframes;i++) write_samp[i]=0.0;//buffer underrun
    }



    if (conn->_use_transport) {


            conn->_transport_state = jack_transport_query(conn->_client, NULL);

            if (conn->_transport_state == JackTransportStopped) {
                if (conn->_previous_transport_state == JackTransportRolling) {
                    //send_all_sound_off(port_buffers, nframes);
                    emit conn->transportStateChanged(TRANSPORT_STOP);

                }
                //playback_started = -1;
            }

            if (conn->_transport_state == JackTransportStarting
                    && conn->_previous_transport_state != JackTransportStarting) {

                emit conn->transportStateChanged(TRANSPORT_START);
            }

            if (conn->_transport_state == JackTransportRolling) {
                if (conn->_previous_transport_state != JackTransportRolling) {
                   emit conn->transportStateChanged(TRANSPORT_ROLLING);

                }
            }

            conn->_previous_transport_state = conn->_transport_state;
        }




    if(conn->_transport_command==TRANSPORT_START) jack_transport_start(conn->_client);
    if(conn->_transport_command==TRANSPORT_STOP) jack_transport_stop(conn->_client);
    if(conn->_transport_command==TRANSPORT_ZERO) {
        jack_position_t pos;
        pos.valid=(jack_position_bits_t)0;
        pos.frame=0;
        jack_transport_reposition(conn->_client,&pos);
    }
    if(conn->_transport_command==TRANSPORT_ROLLING) conn->_transport_command=TRANSPORT_NONE;

    emit conn->ready();

    return 0;

}

int JackConnector::sync(jack_transport_state_t state, jack_position_t *pos, void *arg)
{
    //start tranport
    JackConnector* conn = (JackConnector*) arg;
    if(conn->_use_transport) emit conn->positionChanged(pos->frame);

    if(conn->_use_transport && conn->_ready==false){

    }
    if(conn->_use_transport) return conn->_ready;
    else return true;


}

JackConnector::JackConnector(QObject *parent) :
    QObject(parent)
{


    _client = jack_client_open("QTau",JackNoStartServer,NULL);
    
    if(_client==NULL) {
        QMessageBox msgbox;
        msgbox.setText("jack is not running");
        msgbox.exec();
        exit(1);
    }

    _buffer_size = jack_get_buffer_size(_client);

    if(jack_get_sample_rate(_client)!=44100) {
        QMessageBox msgbox;
        msgbox.setText("samplerate must be 44100");
        msgbox.exec();
        exit(1);
    }

    //TODO configurable
    _write_port = jack_port_register(_client,"out",JACK_DEFAULT_AUDIO_TYPE,JackPortIsOutput,_buffer_size);

    _write_rb = jack_ringbuffer_create (sizeof(sample_t) * 4096);

    jack_set_process_callback(_client,process,this);
    jack_set_sync_callback(_client,sync,this);

    jack_activate(_client);
    //fprintf(stderr,"%i\n",jack_connect(client,"out","system:playback_1"));//FOR testing
    //qDebug() << "sampleSize" << sizeof(sample_t) << sizeof(float);
    _transport_command = TRANSPORT_NONE;
    _use_transport = true;
    _playback = false;
}


int JackConnector::sampleRate()
{
    return jack_get_sample_rate(_client);
}

int JackConnector::writeData(void *framebuf, int bytes_per_frame)
{  
    if(jack_ringbuffer_write_space(_write_rb)>=bytes_per_frame)
    {
        return jack_ringbuffer_write(_write_rb,(char*)framebuf,bytes_per_frame);
    }
    //fprintf(stderr,"no space left in rb\n");
    return -1;//FIXME
}

float *JackConnector::allocateBuffer()
{
    if(_buffer_size==0) return NULL;
    return new float[_buffer_size];
}

//TODO: fallback to pulseaudio if jack is not installed


