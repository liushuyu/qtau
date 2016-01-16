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

#include "vconnect_synth.h"

#include <unistd.h>
#include <assert.h>
#include <sndfile.h>
#include <samplerate.h>
#include <math.h>
#include <stdlib.h>
#include <QDebug>

#include <stdio.h>
#include <assert.h>
#include <boost/algorithm/string.hpp>
#include <vector>
#include <iostream>
#include <qdiriterator.h>

#include "../editor/ustjkeys.h"
#include <QJsonDocument>

#include "libvsq/VSQFileWriter.hpp"
#include "libvsq/ByteArrayOutputStream.hpp"
#include "libvsq/Sequence.hpp"
#include "libvsq/TextStream.hpp"
#include "libvsq/StringUtil.hpp"

using namespace std;
using namespace vsq;





bool createVSQMetaText(QString oto_ini,QJsonArray score, int tempo)
{
        Track track("DummyTrackName", "DummySingerName");
        //one global singer event
        Event singerEvent(0, EventType::SINGER);
        singerEvent.singerHandle = Handle(HandleType::SINGER);   //h#0000
        singerEvent.singerHandle.iconId = "$07010002";
        singerEvent.singerHandle.ids = "Miku";
        singerEvent.singerHandle.original = 1;
        singerEvent.singerHandle.caption = "caption for miku";
        singerEvent.singerHandle.language = 1;
        singerEvent.singerHandle.program = 2;
        track.events().set(0, singerEvent);


    int counter=0;

    for (int i = 0; i < score.count(); ++i)
    {
        auto o = score[i];

        if(!o.toObject().contains(NOTE_KEY_NUMBER)) continue; //skip setup object

        int keyNumber=o.toObject()[NOTE_KEY_NUMBER].toInt();
        int noteOffset = o.toObject()[NOTE_PULSE_OFFSET].toInt();
        int noteLength = o.toObject()[NOTE_PULSE_LENGTH].toInt();
        QString lyric = o.toObject()[NOTE_LYRIC].toString();

        Event noteEvent(noteOffset, EventType::NOTE);
        noteEvent.note = keyNumber;
        noteEvent.dynamics = 100;//FIXME
        noteEvent.pmBendDepth = 0;
        noteEvent.pmBendLength = 0;
        noteEvent.pmbPortamentoUse = 0;
        noteEvent.demDecGainRate = 71;
        noteEvent.demAccent = 72;
        noteEvent.length(noteLength);
        noteEvent.lyricHandle = Handle(HandleType::LYRIC);
        noteEvent.lyricHandle.set(0, Lyric(lyric.toStdString(), "a"));    //h#0005


        noteEvent.vibratoHandle = Handle(HandleType::VIBRATO);  //h#0006
        noteEvent.vibratoDelay = 73;
        noteEvent.vibratoHandle.iconId = "$04040004";
        noteEvent.vibratoHandle.ids = "vibrato";
        noteEvent.vibratoHandle.original = 1;
        noteEvent.vibratoHandle.caption = "caption for vibrato";
        noteEvent.vibratoHandle.length(407);
        noteEvent.vibratoHandle.startDepth = 0;
        noteEvent.vibratoHandle.startRate = 0;
        noteEvent.noteHeadHandle = Handle(HandleType::NOTE_HEAD);  //h#0007
        noteEvent.noteHeadHandle.iconId = "$05030000";
        noteEvent.noteHeadHandle.ids = "attack";
        noteEvent.noteHeadHandle.original = 15;
        noteEvent.noteHeadHandle.caption = "caption for attack";
        noteEvent.noteHeadHandle.length(120);
        noteEvent.noteHeadHandle.duration = 62;
        noteEvent.noteHeadHandle.depth = 65;
        track.events().add(noteEvent, 1+counter);
        counter++;
    }

    if(counter==0) return false;

        Master master;
        master.preMeasure = 1;

        Mixer mixer;
        mixer.masterFeder = 1;
        mixer.masterPanpot = 2;
        mixer.masterMute = 3;
        mixer.outputMode = 4;
        mixer.slave.push_back(MixerItem(5, 6, 7, 8));

#if 0
        //understand vocaloid curves
        track.common().version = "DSB301";
        track.common().name = "foo";
        track.common().color = "1,2,3";
        track.common().dynamicsMode = DynamicsMode::STANDARD;
        track.common().playMode(PlayMode::PLAY_WITH_SYNTH);


        vector<string> curves;
        curves.push_back("pit");
        curves.push_back("pbs");
        curves.push_back("dyn");
        curves.push_back("bre");
        curves.push_back("bri");
        curves.push_back("cle");
        curves.push_back("RESO1FREQ");
        curves.push_back("RESO2FREQ");
        curves.push_back("RESO3FREQ");
        curves.push_back("RESO4FREQ");
        curves.push_back("RESO1BW");
        curves.push_back("RESO2BW");
        curves.push_back("RESO3BW");
        curves.push_back("RESO4BW");
        curves.push_back("RESO1amp");
        curves.push_back("RESO2amp");
        curves.push_back("RESO3amp");
        curves.push_back("RESO4amp");
        curves.push_back("HARMONICS");
        curves.push_back("fx2depth");
        curves.push_back("GEN");
        curves.push_back("pOr");
        curves.push_back("OPE");
        for (int i = 0; i < curves.size(); i++) {
            string curveName = curves[i];
            track.curve(curveName)->add(480 + i, i);
        }
#endif

        TextStream stream;
        VSQFileWriter writer;
        writer._printMetaText(track, stream, 2400, 0, false, &master, &mixer);
        QString str = QString::fromStdString(stream.toString());
        QString header = "[Tempo]\n";
        QString t=QVariant(tempo).toString();
        qDebug() << "Tempo" << t;
        header+=t+"\n";
        header+="[oto.ini]\n";
        header+=oto_ini+"\n";
        str = header + str;
        QFile file("/tmp/vconnect.vsq");

        if(file.open(QFile::WriteOnly))
        {
            file.write(str.toUtf8());
            file.close();
        }
        return true;
}


//FIXME -- audio buffer gets never freed
class AudioBuffer : public IAudioBuffer
{
    int m_sampleRate;
    float* m_data;
    int m_size;
    int m_position;
public:
    AudioBuffer(double* data,int size,int samplerate);
    AudioBuffer(char* fileName);
    int readData(float *data, int size);
    int sampleRate();
    int setSeek(int pos);
};

AudioBuffer::AudioBuffer(double *data, int size, int samplerate)
{
    m_sampleRate = samplerate;
    m_data = new float[size];
    m_size = size;
    m_position = 0;
    for(int i=0;i<size;i++)
    {
        m_data[i] = data[i];
    }
}

AudioBuffer::AudioBuffer(char* filePath)
{
    SF_INFO info={0};
    SNDFILE* sf = sf_open(filePath,SFM_READ,&info);
    assert(info.channels==1);
    m_size = info.frames;
    m_sampleRate = info.samplerate;
    m_data = new float[m_size];
    m_position = 0;
    sf_read_float(sf,m_data,m_size);
    sf_close(sf);

}

int AudioBuffer::setSeek(int pos)
{
    if(pos>=0 && pos<m_size) m_position = pos;
    return 0; //FIXME
}

int AudioBuffer::readData(float *data, int size)
{
    int bytes = size;
    if(m_size-m_position<size) bytes = m_size-m_position;
    for(int i=0;i<bytes;i++)
    {
        data[i] = m_data[i+m_position];
    }
    m_position += bytes;
    return bytes;
}

int AudioBuffer::sampleRate()
{
    return m_sampleRate;
}


#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>

QString vConnectSynth::name()        { return "vconnect-stand"; }
QString vConnectSynth::description() { return "Japanese Vocal Synthesis Tool"; }
QString vConnectSynth::version()     { return "FIXME"; }


void vConnectSynth::setup(SSynthConfig &cfg)      {
    log = cfg.log;
    ctrl = cfg.controller;

    isResampler=false;

}

bool vConnectSynth::setVoicebank(const QString &) { return true;   }

bool vConnectSynth::setScore(const QJsonArray &s)
{
    score = s;
    return false;
}

bool vConnectSynth::synthesize(const QString &outFileName)
{
    //synth to file not implemented : TODO
    return false;
}

bool vConnectSynth::isVbReady()         { return true; }

//TODO tempo support for synths

bool vConnectSynth::supportsStreaming() { return false; }


QString vConnectSynth::getTranscription(QString txt)
{
    return "";
}


bool vConnectSynth::doPhonemeTransformation(QStringList& list)
{
   return false;//does nothing
}


void vConnectSynth::synthesizeAsync(const QString &inFileName)
{

    if(proc) return;
    proc = new QProcess(this);
    QStringList args;

    args << "/tmp/vconnect.vsq";
    args << "/tmp/vconnect.wav";

    if(createVSQMetaText(voicesMap[voiceName],score,_tempo))
    {
     connect (proc, SIGNAL(started()),this,SLOT(processStarted()));
     connect (proc, SIGNAL(readyReadStandardOutput()), this, SLOT(processOutput()));  // connect process signals with your code
     connect (proc, SIGNAL(readyReadStandardError()), this, SLOT(processOutput()));  // same here
     connect (proc, SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(processFinished(int,QProcess::ExitStatus)));
     isResampler=false;
     proc->start(QString("/usr/bin/vconnect_stand"),args);
    }




}

void vConnectSynth::processStarted()
{
    iLog("eCantorix started");
}

void vConnectSynth::processOutput()
{
    proc->readAllStandardOutput();  // read normal output
    QString str = QString::fromUtf8(proc->readAllStandardError());  // read error channel

   foreach(QString s,str.split("\n"))
   {
    if(s.length()>0) iLog(s);
   }
}

void vConnectSynth::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    proc->deleteLater();
    proc=nullptr;//will be collected later
    if(exitCode==0 && QFileInfo("/tmp/vconnect.wav").exists())
    {
        iLog("done, starting playback");
        ctrl->startPlayback(new AudioBuffer("/tmp/vconnect.wav"));
    }
    else
    {
        QString err = "ecantorix error :"+QVariant(exitCode).toString();
        eLog(err);
    }



}

bool vConnectSynth::setVoice(QString voiceName)
{
    if(voicesMap.contains(voiceName))
    {
        this->voiceName = voiceName;
        return true;
    }
}

bool vConnectSynth::isVocalsReady()
{
    return true;
}

IAudioBuffer* vConnectSynth::synthesize()
{
    return NULL;
}

//selects first voice by default
QStringList vConnectSynth::listVoices()
{
    QStringList ret;
    QString voicePath = QDir::home().filePath(".local/share/utau/voice");
    QDir dir(voicePath);
    QDirIterator it(dir);

    //TODO: add support for voices in /usr/share/utau/voice

    while (it.hasNext()) {
        QString vdir = it.next();
        if(QFileInfo(dir,vdir).isDir())
        {
            if(QFileInfo(dir,vdir+"/voices.json").isFile())
            {
                QFile jsonFile(dir.absoluteFilePath(vdir+"/voices.json"));
                if (jsonFile.open(QFile::ReadOnly))
                {
                    QJsonDocument doc = QJsonDocument::fromJson(jsonFile.readAll());
                    if(doc.isArray())
                    {
                        QJsonArray a = doc.array();
                        for (int i = 0; i < a.count(); ++i)
                            {
                                auto o = a[i];
                                QString name = o.toObject()["name"].toString();
                                QString otoIni = o.toObject()["oto.ini"].toString();
                                voicesMap[name] = vdir+"/"+otoIni;
                                ret << name;
                            }
                    }
                }

            }

        }

    }
    if(ret.size()>0)
    {
        this->voiceName = ret[0];
    }
    //else no voices installed


    return ret;

}
