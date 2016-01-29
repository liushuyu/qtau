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

#include "Session.h"
#include "Utils.h"
//#include "audio/Source.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QStringList>

#include <QtAlgorithms>
#include <smf.h>
#include <ustjkeys.h>
#include <QDebug>

#include <QJsonDocument>




qtauSession::qtauSession(QObject *parent) :
    qtauEventManager(parent), _docName(QStringLiteral("Untitled")), _isModified(false), _hadSavePoint(false)
{
    _needsSynth=true;
    _defaults[USER_AGENT] = QString("QTau Debug");
    _defaults[TEMPO]=120;
    QJsonArray ts;
    ts.append(4);
    ts.append(4);
    _defaults[TIME_SIGNATURE]=ts;
    _objectMap[-1] = _defaults;
}

qtauSession::~qtauSession()
{
}

qtauEvent_NoteAddition *util_makeAddNotesEvent(QJsonArray &a)
{
    qtauEvent_NoteAddition::noteAddVector changeset;

    for (int i = 0; i < a.count(); ++i)
    {
        auto o = a[i];

        qtauEvent_NoteAddition::noteAddData d;
        d.id     = i+1;
        d.lyrics = o.toObject()[NOTE_LYRIC].toString();

        d.pulseLength = o.toObject()[NOTE_PULSE_LENGTH].toInt();
        d.pulseOffset = o.toObject()[NOTE_PULSE_OFFSET].toInt();
        d.keyNumber   = o.toObject()[NOTE_KEY_NUMBER].toInt();

        if(d.pulseLength==0 && d.pulseOffset==0)
        {
            //qDebug() << "dummy";
            continue;
        }

        vsLog::d("l: "+QVariant(d.pulseLength).toString());
        vsLog::d("l: "+QVariant(d.pulseOffset).toString());
        vsLog::d("l: "+QVariant(d.keyNumber).toString());
        vsLog::d("l: "+QVariant(d.id).toString());

        changeset.append(d);
    }

    return new qtauEvent_NoteAddition(changeset);
}



bool qtauSession::loadUST(QJsonArray array)
{

    //TODO default tempo



    if (!array.isEmpty())
    {
        _needsSynth=true;
        clearHistory();

        auto o = array[0];
        if(o.toObject().contains(USER_AGENT))
            _objectMap[-1] = o.toObject();
        else
            {/*TODO*/}


        qtauEvent_NoteAddition *loadNotesChangeset = util_makeAddNotesEvent(array);
        applyEvent_NoteAdded(*loadNotesChangeset);
        emit onEvent(loadNotesChangeset);
        delete loadNotesChangeset;
        return true;
    }
    return false;
}


bool qtauSession::loadUST(QString fileName)
{
    bool result = false;
    QFile ustFile(fileName);

    if (ustFile.open(QFile::ReadOnly))
    {
        QJsonDocument doc = QJsonDocument::fromJson(ustFile.readAll());
        if(doc.isArray())
        {
            // qDebug() << "lock";
            _docName  = QFileInfo(fileName).baseName();
            _filePath = fileName;
            emit dataReloaded();
            result = loadUST(doc.array());
            if(result)
                vsLog::d("loaded doc");
        }
        else
            vsLog::e("json document is not an array");
        ustFile.close();
    }
    else
        vsLog::e("Could not open " + fileName);

    return result;
}



inline void qSwap(QJsonValueRef var1, QJsonValueRef var2)
{
    QJsonValue tmp = var2;
    var2 = var1;
    var1 = tmp;
}


inline bool noteNumComparison(const QJsonValueRef o1, const QJsonValueRef& o2)
{
    return o1.toObject()[NOTE_KEY_NUMBER].toInt() < o2.toObject()[NOTE_KEY_NUMBER].toInt();
}

inline bool pulseOffsetComparison(const QJsonValueRef o1, const QJsonValueRef& o2)
{
    return o1.toObject()[NOTE_PULSE_OFFSET].toInt() < o2.toObject()[NOTE_PULSE_OFFSET].toInt();
}



void  qtauSession::ustJson(QJsonArray& ret)
{

    foreach (const quint64 &key, _objectMap.keys())
        ret.append(_objectMap[key]);

    qStableSort(ret.begin(), ret.end(), noteNumComparison);
    qStableSort(ret.begin(), ret.end(), pulseOffsetComparison);
}

void qtauSession::setDocName(const QString &name)
{
    if (name.isEmpty())
        vsLog::e("Shouldn't set empty doc name for session! Ignoring...");
    else
        _docName = name;
}

void qtauSession::setFilePath(const QString &fp)
{
    if (fp.isEmpty())
        vsLog::e("Shouldn't set empty filepath for session! Ignoring...");
    else
    {
        _filePath = fp;
        _docName = QFileInfo(fp).baseName();
    }
}

//----- inner data functions -----------------------------
void qtauSession::applyEvent_NoteAdded(const qtauEvent_NoteAddition &event)
{
    const qtauEvent_NoteAddition::noteAddVector &changeset = event.getAdded();

    // delete event has reversed transformations
    bool reallyForward = (event.isForward() && !event.isDeleteEvent()) ||
            (!event.isForward() &&  event.isDeleteEvent());

    if (reallyForward)
    {
        foreach (const qtauEvent_NoteAddition::noteAddData &change, changeset)
        {
            //noteMap[change.id] = ust_note(change.id, change.lyrics, change.pulseOffset, change.pulseLength, change.keyNumber);
            QJsonObject object;
            object[NOTE_LYRIC] = change.lyrics;
            object[NOTE_PULSE_OFFSET] = change.pulseOffset;
            object[NOTE_PULSE_LENGTH] = change.pulseLength;
            object[NOTE_KEY_NUMBER] = change.keyNumber;
            _objectMap[change.id] = object;
        }
    }
    else
        foreach (const qtauEvent_NoteAddition::noteAddData &change, changeset)
            _objectMap.remove(change.id);
}


void qtauSession::applyEvent_NoteResized(const qtauEvent_NoteResize &event)
{

    const qtauEvent_NoteResize::noteResizeVector &changeset = event.getResized();

    foreach (const qtauEvent_NoteResize::noteResizeData &change, changeset)
    {
        auto &n = _objectMap[change.id];

        if (event.isForward())
        {
            n[NOTE_PULSE_OFFSET] = change.offset;
            n[NOTE_PULSE_LENGTH] = change.length;
        }
        else
        {
            n[NOTE_PULSE_OFFSET] = change.prevOffset;
            n[NOTE_PULSE_LENGTH] = change.prevLength;
        }
    }

}


void qtauSession::applyEvent_NoteMoved(const qtauEvent_NoteMove &event)
{
    const qtauEvent_NoteMove::noteMoveVector &changeset = event.getMoved();

    foreach (const qtauEvent_NoteMove::noteMoveData &change, changeset)
    {
        auto &n = _objectMap[change.id];

        if (event.isForward())
        {
            n[NOTE_PULSE_OFFSET] = n[NOTE_PULSE_OFFSET].toInt() + change.pulseOffDelta;
            n[NOTE_KEY_NUMBER]   =  change.keyNumber;
        }
        else
        {
            n[NOTE_PULSE_OFFSET] = n[NOTE_PULSE_OFFSET].toInt() - change.pulseOffDelta;
            n[NOTE_KEY_NUMBER]   =  change.prevKeyNumber;
        }
    }
}


void qtauSession::applyEvent_NoteLyrics(const qtauEvent_NoteText &event)
{

    const qtauEvent_NoteText::noteTextVector &changeset = event.getText();

    foreach (const qtauEvent_NoteText::noteTextData &change, changeset)
        if (event.isForward()) _objectMap[change.id][NOTE_LYRIC] = change.txt;
        else                   _objectMap[change.id][NOTE_LYRIC] = change.prevTxt;
}


void qtauSession::applyEvent_NoteEffects(const qtauEvent_NoteEffect &/*event*/)
{
    // TODO: or not to do, that is the question
}


//--------- dispatcher -----------------------------
void qtauSession::onUIEvent(qtauEvent *e)
{
    if (e)
    {
        if (processEvent(e))
            storeEvent(e);

        delete e; // if it's valid it was copied on storing, and UI should only create events anyway.
    }
}

// process event is called from both program (ui input) and undo/redo in manager (stack change)
bool qtauSession::processEvent(qtauEvent *e)
{
    //qDebug() << "event" << e->type();

    bool result = false;

    if (e)
    {
        switch (e->type())
        {
        case ENoteEvents::add:
        {
            qtauEvent_NoteAddition *ne = static_cast<qtauEvent_NoteAddition*>(e);

            if (ne)
            {
                applyEvent_NoteAdded(*ne);
                result = true;
            }
            else
                vsLog::e("Session could not convert UI event to noteAdd");

            break;
        }
        case ENoteEvents::move:
        {
            qtauEvent_NoteMove *ne = static_cast<qtauEvent_NoteMove*>(e);

            if (ne)
            {
                applyEvent_NoteMoved(*ne);
                result = true;
            }
            else
                vsLog::e("Session could not convert UI event to noteMove");

            break;
        }
        case ENoteEvents::resize:
        {
            qtauEvent_NoteResize *ne = static_cast<qtauEvent_NoteResize*>(e);

            if (ne)
            {
                applyEvent_NoteResized(*ne);
                result = true;
            }
            else
                vsLog::e("Session could not convert UI event to noteResize");

            break;
        }
        case ENoteEvents::text:
        {
            qtauEvent_NoteText *ne = static_cast<qtauEvent_NoteText*>(e);

            if (ne)
            {
                applyEvent_NoteLyrics(*ne);
                result = true;
            }
            else
                vsLog::e("Session could not convert UI event to noteText");

            break;
        }
        case ENoteEvents::effect:
        {
            qtauEvent_NoteEffect *ne = static_cast<qtauEvent_NoteEffect*>(e);

            if (ne)
            {
                applyEvent_NoteEffects(*ne);
                result = true;
            }
            else
                vsLog::e("Session could not convert UI event to noteEffect");

            break;
        }
        default:
            vsLog::e(QString("Session received unknown event type from UI").arg(e->type()));
        }
    }
    else vsLog::e("Session can't process a zero event! Ignoring...");

    return result;

}

void qtauSession::stackChanged()
{
    if (canUndo())
        _isModified = !events.top()->isSavePoint();
    else
        _isModified = _hadSavePoint;

    emit undoStatus(canUndo());
    emit redoStatus(canRedo());
    emit modifiedStatus(_isModified);

    _needsSynth = !_objectMap.isEmpty();
}

//Ardour is used to manage SynthesizedVocal and BackgroundAudio

void qtauSession::setModified(bool m)
{
    if (m != _isModified)
    {
        _isModified = m;
        emit modifiedStatus(_isModified);
    }
}

void qtauSession::setSaved()
{
    if (canUndo())
    {
        foreach (qtauEvent *e, events)
            e->setSavePoint(false);

        if (!futureEvents.isEmpty())
            foreach (qtauEvent *e, futureEvents)
                e->setSavePoint(false);

        _hadSavePoint = true;
        events.top()->setSavePoint();
        setModified(false);
    }
    else
        vsLog::e("Saving an empty session?");
}

//TODO move midi to external class
void qtauSession::importMIDI(QString fileName)
{

    smf_t *smf;
    smf_event_t *event;
    smf = smf_load(fileName.toUtf8());

    if(smf==NULL) return;

    smf_tempo_t* tempo = smf_get_tempo_by_pulses(smf,0);
    fprintf(stderr,"TT_MIDI: %i %i %i\n",tempo->numerator,tempo->denominator,tempo->microseconds_per_quarter_note);
    QJsonArray ts;
    ts.append(tempo->numerator);
    ts.append(tempo->denominator);
    setTimeSignature(ts);
    float bpm=tempo->microseconds_per_quarter_note/(1000.0*1000.0);
    bpm=60.0/bpm;
    fprintf(stderr,"TEMPO %f\n",bpm);
    setTempo(round(bpm));



    smf_track_t *track = smf_get_track_by_number(smf, smf->number_of_tracks);
    int activeNote = -1;
    int notePos = 0;

    QString text;

    QJsonArray ust;

    for(int i=1;i<track->number_of_events+1;i++)
    {
        event = smf_track_get_event_by_number(track, i);
        if(smf_event_is_textual(event)){
            char* txt = smf_event_extract_text(event);
            text = txt;
            free(txt);
        }
        else
        {
            if(event->midi_buffer_length==3)
            {
                unsigned char status=event->midi_buffer[0];
                unsigned char statusb = 0xF0 & status;
                unsigned char notenum=event->midi_buffer[1];
                unsigned char velocity=event->midi_buffer[2];

                if(statusb == 0x80 || (statusb == 0x90 && velocity==0) )
                {
                    //qDebug()<< "note off " <<notenum << " " <<velocity;

                    int length = (event->time_pulses-notePos);
                    QJsonObject note;
                    //qDebug() << notePos << "::" << length;
                    note[NOTE_PULSE_OFFSET]= notePos*4;
                    note[NOTE_PULSE_LENGTH]= length*4;
                    note[NOTE_KEY_NUMBER] = activeNote;
                    note[NOTE_VELOCITY]=velocity;
                    if(text.length()==0) text="[[a]]";
                    note[NOTE_LYRIC] = text;
                    ust.append(note);
                    activeNote = -1;
                }
                else
                    if(statusb == 0x90)
                    {
                        if(activeNote!=-1) {
                            smf_delete(smf);
                            return;
                        }
                        //qDebug() << "note on " << notenum;
                        activeNote = notenum;
                        notePos = event->time_pulses;
                    }
            }
        }
    }

    smf_delete(smf);

    loadUST(ust);
}

void qtauSession::exportMIDI(QString fileName)
{

    QJsonArray ust;
    ustJson(ust);

    smf_t *smf;
    smf_track_t *track;
    smf_event_t *event;

    smf = smf_new();

    track = smf_track_new();
    smf_add_track(smf, track);

#if 1
    int bpm = getTempo();
    track = smf_track_new();
    smf_add_track(smf, track);
    char temposig[6];
    int tempo=60*1000*1000/bpm;
    temposig[0] = 0xFF;
    temposig[1] = 0x51;
    temposig[2] = 0x03;
    temposig[3] = (tempo >> 16) & 0xFF;
    temposig[4] = (tempo >> 8) & 0xFF;
    temposig[5] = (tempo) & 0xFF;
    event = smf_event_new_from_pointer(temposig, 6);
    smf_track_add_event_pulses(track,event,0);
    char timesig[7];
    timesig[0] = 0xFF;
    timesig[1] = 0x58;
    timesig[2] = 0x04;
    QJsonArray a = getTimeSignature();
    int nn=a[0].toInt();
    int dd=a[1].toInt();
    dd = log(dd)/log(2);
    fprintf(stderr,"LOG: %i %i\n",nn,dd);
    timesig[3] = nn;
    timesig[4] = dd;
    timesig[5] = 0;
    timesig[6] = 8;
    event = smf_event_new_from_pointer(timesig, 7);
    smf_track_add_event_pulses(track,event,0);
#endif


    for (int i = 0; i < ust.count(); ++i)
    {
        auto o = ust[i];


        double ts = 0.25;

        unsigned char midi[3];
        midi[0]=0x90;
        midi[1]=o.toObject()[NOTE_KEY_NUMBER].toInt();
        midi[2]=100;

        int noteOffset = o.toObject()[NOTE_PULSE_OFFSET].toInt();
        int noteLength = o.toObject()[NOTE_PULSE_LENGTH].toInt();

        if(noteLength>0)
        {

        event = smf_event_new_from_pointer(midi, 3);
        smf_track_add_event_pulses(track,event,ts*noteOffset);

        //http://www.ccarh.org/courses/253/handout/smf/

        QString lyric = o.toObject()[NOTE_LYRIC].toString();

        event = smf_event_new_textual(0x05,lyric.toUtf8());



        smf_track_add_event_pulses(track,event,ts*noteOffset);
        midi[0]=0x80;
        event = smf_event_new_from_pointer(midi, 3);
        smf_track_add_event_pulses(track,event,ts*(noteOffset+noteLength));


        }



    }

    smf_save(smf, fileName.toUtf8());//FIXWARN


}



void qtauSession::startPlayback()
{
    emit requestStartPlayback(); // if jack transport is used, rename ?
}


void qtauSession::stopPlayback()
{
    emit requestStopPlayback();
}

void qtauSession::resetPlayback()
{
    emit requestResetPlayback();
}

void qtauSession::repeatPlayback()
{
    emit requestRepeatPlayback();
}

void qtauSession::onNewSession()
{
    //TODO default tempo
    QJsonArray empty;

    clearHistory(); // or make a delete event + settings change event + filepath change event
    _objectMap.clear();
    _objectMap[-1]          = _defaults; //defaults



    _docName  = "Untitled";
    _filePath = "";
    qtauEvent_NoteAddition *loadNotesChangeset = util_makeAddNotesEvent(empty);
    applyEvent_NoteAdded(*loadNotesChangeset);

    emit dataReloaded();//                  UPDATE text
    emit onEvent(loadNotesChangeset);       //LOAD SCORE INTO EDITOR

    delete loadNotesChangeset;
}

// set/get proptery

void qtauSession::setSingerName(QString singerName)
{
    //qDebug() << "setSingerName";
    QJsonObject obj;
    if(_objectMap.contains(-1)) obj=_objectMap[-1];
    obj[SINGER_NAME] = singerName;
    _objectMap[-1] = obj;
    _isModified = true;
    emit modifiedStatus(_isModified);
}

QString qtauSession::getSingerName()
{
    QJsonObject obj;
    if(_objectMap.contains(-1)) obj=_objectMap[-1];
    return obj[SINGER_NAME].toString();
}

int qtauSession::getTempo()
{
    QJsonObject obj;
    if(_objectMap.contains(-1)) obj=_objectMap[-1];
    return obj[TEMPO].toDouble();
}

void qtauSession::setTempo(int tempo)
{
    //qDebug() << "setSingerName";
    QJsonObject obj;
    if(_objectMap.contains(-1)) obj=_objectMap[-1];
    obj[TEMPO] = tempo;
    _objectMap[-1] = obj;
    _isModified = true;
    emit modifiedStatus(_isModified);
}

QJsonArray qtauSession::getTimeSignature()
{
    QJsonObject obj;
    if(_objectMap.contains(-1)) obj=_objectMap[-1];
    return obj[TIME_SIGNATURE].toArray();
}

void qtauSession::setTimeSignature(QJsonArray ts)
{
     QJsonObject obj;
     if(_objectMap.contains(-1)) obj=_objectMap[-1];
      obj[TIME_SIGNATURE] = ts;
     _objectMap[-1] = obj;
     _isModified = true;
     emit modifiedStatus(_isModified);
}

//FIXME global set/get for root json object
