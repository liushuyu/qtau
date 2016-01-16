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

#include "espeak_synth.h"
#include <espeak/speak_lib.h>

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

#include <../editor/ustjkeys.h>

#include <QStringList>
#include <smf.h>

#if 1
//TODO move to external file
class EspeakLyricizer
{
    //old one -- works in most cases
    //TODO move to libSinsy
    struct syllable
    {
        int length;
        int vpos;
    };

    std::vector<syllable> syllables;

    public:
    std::vector<std::string> transform(std::string text)
    {
        char obuf[1025];
        std::string tmp;
        espeak_TextToPhonemes(text.c_str(),obuf,sizeof(obuf),espeakCHARS_UTF8,3);
        tmp = obuf;

        std::vector<std::string> strs;
        boost::split(strs, tmp, boost::is_any_of("_ "));
        return strs;

    }

    void start_syllable(std::string s)
    {
            int pos = 0;
            int vpos = 0;
            for(std::string s2 : transform(s))
            {
                if(s2.length()==0) continue;
                //std::cout << s2 << " -- "<<isVowel(s2) << std::endl;
                if(isVowel(s2)) vpos = pos;
                pos ++;
            }
            syllable syl;
            syl.length = pos;
            syl.vpos = vpos;
            syllables.push_back(syl);
            //std::cout << "start: " << pos <<" " << vpos <<std::endl;

    }
    void pseudo_syllable()
    {
        syllable syl;
        syl.length = 0;
        syl.vpos = -1;
        syllables.push_back(syl);
    }
    bool isVowel(std::string code)
    {
        return code.find_first_of ("aeiouAEIOU") != std::string::npos;
    }

    std::vector<std::string> lyricize(std::vector<std::string> input)
    {
        std::vector<std::string> ret;
        syllables.clear();
        std::string current_word;
        int current_note = 0;
        for(std::string s : input)
        {
            if(s=="-" || s== "\\")
            {
                pseudo_syllable();
                //qDebug() << "pseudo";
            }
            else
            if(boost::algorithm::ends_with(s,"-"))
            {
                boost::algorithm::replace_last(s,"-","");//inplace
                current_word += s;
                start_syllable(s);
               // qDebug() << "ends-";

            }
            else
            {
                current_word += s;
                start_syllable(s);
                //std::cout << current_word <<std::endl;

                int current_syl = 0;
                int current_sym = 0;
                std::vector<std::string> note_lyr;
                for(std::string s : transform(current_word))
                {
                    syllable syl = syllables[current_syl];
                    if(s.length())
                    {
                        if(syl.length==0) //skip pseudos
                        {
                            ret.push_back(input[current_note]);
                            current_sym = 0;
                            current_syl ++;
                            current_note ++;
                            syl = syllables[current_syl];

                        }
                        //std::cout << s << " -- " << isVowel(s) << " -- " << current_syl << std::endl;
                        note_lyr.push_back(s);
                        current_sym ++;
                        if(current_sym == syl.length && current_syl < syllables.size()-1)
                        {
                            std::string j = boost::algorithm::join(note_lyr," ");
                            std::cout <<  input[current_note] << " [[" << j << "]]" << std::endl;
                            ret.push_back(input[current_note] + " [[" + j + "]]");
                            current_sym = 0;
                            current_syl ++;
                            current_note ++;
                            note_lyr.clear();
                        }

                    }
                    //end if

                }
                std::string j = boost::algorithm::join(note_lyr," ");
                std::cout << input[current_note] << " [[" << j << "]]" << std::endl;
                ret.push_back(input[current_note] + " [[" + j + "]]");

                current_word = "";
                syllables.clear();
                current_note ++;
            }

        }
        int count = input.size();
        if(input[count-1]=="\\")
        {
            std::cout << "\\" << std::endl;
            ret.push_back("\\");
        }
        return ret;

    }

};
#endif

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

QString eSpeakSynth::name()        { return "eSpeak@LOID"; }
QString eSpeakSynth::description() { return "A multilingual KTH-style singing synthesizer"; }
QString eSpeakSynth::version()     { return "FIXME"; }


void eSpeakSynth::setup(SSynthConfig &cfg)      {
    log = cfg.log;
    ctrl = cfg.controller;

    isResampler=false;

    assert(espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS,0,NULL,espeakINITIALIZE_DONT_EXIT)==22050);

    setVoice("en");

}

bool eSpeakSynth::setVoicebank(const QString &) { return true;   }

bool eSpeakSynth::setScore(const QJsonArray &s)
{
    score = s;
    return false;
}

#if 0
static double midi_freq(unsigned char m) {
/* converts a MIDI note number to a frequency
<http://en.wikipedia.org/wiki/MIDI_Tuning_Standard> */
return 440 * pow(2, (double)(m-69)/12);
}
#endif

bool eSpeakSynth::synthesize(const QString &outFileName)
{
    //synth to file not implemented : TODO
    return false;
}

bool eSpeakSynth::isVbReady()         { return true; }

//TODO tempo support for synths

bool eSpeakSynth::supportsStreaming() { return false; }


QString eSpeakSynth::getTranscription(QString txt)
{
    QString txt2 = txt;
    txt2.replace("-","");
    //clean ????

    char tr_buffer[1025];
    espeak_TextToPhonemes(txt2.toUtf8().data(),tr_buffer,sizeof(tr_buffer),espeakCHARS_AUTO,0x3);

    QString pho = QString::fromUtf8(tr_buffer+1);    
    pho.replace("_"," ");
    return txt+QString(" ["+pho+"]");
}


bool eSpeakSynth::doPhonemeTransformation(QStringList& list)
{
    EspeakLyricizer lyr;

    std::vector<std::string> tmp;

    foreach(QString s, list)
    {
        tmp.push_back(s.toStdString());
     //   qDebug() << "in " << s;
    }

    std::vector<std::string> pho = lyr.lyricize(tmp);

    if(pho.size()!=tmp.size()) return false;

    for(int i=0;i<tmp.size();i++)
    {
        list[i] = QString::fromStdString(pho[i]);
        //qDebug() << "ret" << list[i];
    }

    return true;
}

void eSpeakSynth::synthesizeAsync(const QString &inFileName)
{
    if(proc) return;
    proc = new QProcess(this);
    QStringList args;

    args << "-v";
    args << voiceName;

    args << "-c";
    QString workingdir = "/tmp/utau.cache";
    if(inFileName.length()) workingdir = inFileName+".cache";
    if(!QDir(workingdir).exists()) QDir().mkdir(workingdir);

    //-c $CACHE -O wav -o $WAV $MID
    args << workingdir;
    args << "-O";
    args << "wav";
    args << "-o";
    args << "/tmp/ecantorix.wav";
    args << "/tmp/ecantorix.mid";

    //TODO move midi handler code to common.cpp
    smf_t *smf;
    smf_track_t *track;
    smf_event_t *event;

    unlink("/tmp/ecantorix.mid");

    smf = smf_new();

#if 1
    track = smf_track_new();
    smf_add_track(smf, track);
    char temposig[5];
    int tempo=60*1000*1000/_bpm;
    temposig[0] = 0xFF;
    temposig[1] = 0x51;
    temposig[2] = 0x03;
    temposig[3] = (tempo >> 16) & 0xFF;
    temposig[4] = (tempo >> 8) & 0xFF;
    temposig[5] = (tempo) & 0xFF;
    event = smf_event_new_from_pointer(temposig, 6);
    smf_track_add_event_pulses(track,event,0);
#endif




    track = smf_track_new();
    smf_add_track(smf, track);





    fprintf(stderr,"score.count=%i\n",score.count());

    for (int i = 0; i < score.count(); ++i)
        {
        auto o = score[i];

        double ts = 0.25;
        if(!o.toObject().contains(NOTE_KEY_NUMBER)) continue; //skip setup object

        unsigned char midi[3];
        midi[0]=0x90;
        midi[1]=o.toObject()[NOTE_KEY_NUMBER].toInt();
        midi[2]=100;

        fprintf(stderr,"add note %i\n",midi[1]);

        int noteOffset = o.toObject()[NOTE_PULSE_OFFSET].toInt();
        int noteLength = o.toObject()[NOTE_PULSE_LENGTH].toInt();

        event = smf_event_new_from_pointer(midi, 3);
        smf_track_add_event_pulses(track,event,ts*noteOffset);

        //http://www.ccarh.org/courses/253/handout/smf/

        QString lyric = o.toObject()[NOTE_LYRIC].toString();

      //  qDebug() << lyric;
        int index = lyric.indexOf(" [");
        if(index>1)
        {
            index +=1;
            QString tmp = lyric.right(lyric.length()-index);
            lyric = "["+tmp+"]";
        }


        event = smf_event_new_textual(0x05,lyric.toUtf8());



        smf_track_add_event_pulses(track,event,ts*noteOffset);


        midi[0]=0x80;
        event = smf_event_new_from_pointer(midi, 3);
        smf_track_add_event_pulses(track,event,ts*(noteOffset+noteLength));

    }

     smf_save(smf, "/tmp/ecantorix.mid");



     connect(proc,SIGNAL(started()),this,SLOT(processStarted()));
     connect (proc, SIGNAL(readyReadStandardOutput()), this, SLOT(processOutput()));  // connect process signals with your code
     connect (proc, SIGNAL(readyReadStandardError()), this, SLOT(processOutput()));  // same here
     connect (proc, SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(processFinished(int,QProcess::ExitStatus)));
     isResampler=false;
     proc->start(QString("/opt/ecantorix/ecantorix.pl"),args);



}

void eSpeakSynth::processStarted()
{
    iLog("eCantorix started");
}

void eSpeakSynth::processOutput()
{
    proc->readAllStandardOutput();  // read normal output
    QString str = QString::fromUtf8(proc->readAllStandardError());  // read error channel

   foreach(QString s,str.split("\n"))
   {
    if(s.length()>0) iLog(s);
   }
}

void eSpeakSynth::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    proc->deleteLater();
    proc=nullptr;//will be collected later
    if(exitCode==0)
    {
        if(isResampler)
        {
        iLog("done, starting playback");
        ctrl->startPlayback(new AudioBuffer("/tmp/ecantorix2.wav"));
        }
        else
        {

        proc = new QProcess(this);
        QStringList args;
        args << "-to";
        args << "44100"; //FIXME do not hardcode
        args << "/tmp/ecantorix.wav";
        args << "/tmp/ecantorix2.wav";
        isResampler=true;
        //connect(proc,SIGNAL(started()),this,SLOT(processStarted()));
        connect(proc, SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(processFinished(int,QProcess::ExitStatus)));
        proc->start("sndfile-resample",args);
        iLog("resampling to 44100 hZ");
        }



    }
    else
    {
        QString err = "ecantorix error :"+QVariant(exitCode).toString();
        eLog(err);
    }



}

bool eSpeakSynth::setVoice(QString voiceName)
{
  //  qDebug() << voiceName;
    if(voiceName=="DEFORUTO")
    {
        this->voiceName = "en";
        espeak_SetVoiceByName("en");
        return true;
    }

    if(voiceName=="Fred")
    {
        this->voiceName = "de";
        espeak_SetVoiceByName("de");
        return true;
    }


    return false;
}

bool eSpeakSynth::isVocalsReady()
{
    return true;
}

IAudioBuffer* eSpeakSynth::synthesize()
{
    return NULL;
}

QStringList eSpeakSynth::listVoices()
{
    QStringList ret;
    ret << "DEFORUTO";
    ret << "Fred";
    return ret;
}
