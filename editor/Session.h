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

#ifndef SESSION_H
#define SESSION_H

#include "Utils.h"
#include "NoteEvents.h"
#include <QJsonArray>
#include "PluginInterfaces.h"
#include "ustjkeys.h"

#include <QMap>






/** Work session that contains one voice setup (notes/lyrics/effects)
 and voicebank selection+setup to synthesize one song.
 */
class qtauSession : public qtauEventManager
{
    Q_OBJECT

public:
    explicit qtauSession(QObject *parent = 0);
    ~qtauSession();

    bool loadUST(QString fileName);
    bool loadUST(QJsonArray array);

    QString documentName() { return _docName; }
    QString documentFile() { return _filePath; }

    void setDocName(const QString &name);
    void setFilePath(const QString &fp);
    QString getFilePath(){return _filePath;}

    bool isSessionEmpty()    const { return _objectMap.isEmpty(); } /// returns true if doesn't contain any data
    bool isSessionModified() const { return _isModified; }        /// if has changes from last save/load

    void setModified(bool m);
    void setSaved(); // if doc was saved at this point

    void importMIDI(QString fileName);
    void exportMIDI(QString fileName);
    bool synthNeeded(){return _needsSynth;}
    void resetNeedsSynth(){_needsSynth=false;}

    void ustJson(QJsonArray& ret);

    void setSingerName(QString singerName);
    QString getSingerName();

    void setTempo(int tempo);
    int getTempo();

    QJsonArray getTimeSignature();
    void setTimeSignature(QJsonArray ts);


signals:
    void modifiedStatus(bool); /// if document is modified
    void undoStatus    (bool); /// if can undo last stored action
    void redoStatus    (bool); /// if can apply previously reverted action

    void dataReloaded();       /// when data is changed completely
    //void playbackStateChanged(EAudioPlayback);

    void vocalSet(); // when session gets synthesized audio from score
    void musicSet(); // when user adds bg (off-vocal?) music to play with synthesized vocals

    void audioBufferForPlaybackChanged(IAudioBuffer* buffer);

    // signals to controller
    void requestSynthesis(); // means synth & play
    void requestStartPlayback();
    void requestPausePlayback();
    void requestStopPlayback();
    void requestResetPlayback();
    void requestRepeatPlayback();


    //void setJackTransportEnabled(bool enabled);

public slots:
    void onUIEvent(qtauEvent *);

    // received from UI
    void startPlayback();
    void stopPlayback();
    void resetPlayback();
    void repeatPlayback();
    void onNewSession();

protected:
    bool    parseUSTStrings(QStringList ustStrings);
    QString _filePath;
    QString _docName;
    bool    _isModified;
    bool    _hadSavePoint; // if was saved having a non-empty event stack
    bool    _needsSynth;
    QMap<qint64, QJsonObject> _objectMap; // need to store copies until changing data structure to something better


    void applyEvent_NoteAdded  (const qtauEvent_NoteAddition &event);
    void applyEvent_NoteMoved  (const qtauEvent_NoteMove     &event);
    void applyEvent_NoteResized(const qtauEvent_NoteResize   &event);
    void applyEvent_NoteLyrics (const qtauEvent_NoteText     &event);
    void applyEvent_NoteEffects(const qtauEvent_NoteEffect   &event);

    bool processEvent(qtauEvent *) override;
    void stackChanged()            override;


    //EAudioPlayback playSt;
};

#endif // SESSION_H
