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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUrl>

#include "Utils.h"

class qtauController;
class qtauSession;
class qtauEvent;
class qtauPiano;
class qtauNoteEditor;
class qtauMeterBar;
class qtauDynDrawer;
class qtauDynLabel;
class qtauWaveform;

class QAction;
class QScrollBar;
class QSlider;
class QToolBar;
class QTabWidget;
class QTextEdit;
class QToolBar;
class QSplitter;
class QComboBox;
class QLabel;


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    bool setController(qtauController &c, qtauSession &s);

private:
    Ui::MainWindow *ui;

signals:
    void loadUST  (QString fileName);
    void saveUST  (QString fileName, bool rewrite);
    void saveAudio(QString fileName, bool rewrite);

    void loadAudio(QString fileName);
    void setVolume(int);

public slots:
    void onLog(const QString&, ELog);
    void onOpenUST();
    void onSaveUST();
    void onSaveUSTAs();

    void onVocalAudioChanged();
    void onMusicAudioChanged();

    void notesVScrolled(int);
    void notesHScrolled(int);
    void vertScrolled(int);
    void horzScrolled(int);

    void onEditorRMBScrolled(QPoint, QPoint);
    void onEditorRequestOffset(QPoint);

    void onPianoHeightChanged    (int newHeight);
    void onNoteEditorWidthChanged(int newWidth);

    void onUndo();
    void onRedo();
    void onDelete();

    void onEditMode(bool);
    void onGridSnap(bool);
    void onQuantizeSelected(int);
    void onNotelengthSelected(int);

    void dynBtnLClicked();
    void dynBtnRClicked();

    void onTabSelected(int);
    void onZoomed(int);
    void onEditorZoomed(int);

    void onSingerSelected(int);


    void onDocReloaded();
    void onDocStatus(bool);
    void onUndoStatus(bool);
    void onRedoStatus(bool);
    void onDocEvent(qtauEvent*);

    void onTransportPositionChanged(float pos);

    void onMIDIImport();
    void onMIDIExport();
    void onActionJackTriggered();
    void onTempoSelectClicked();

protected:
    qtauSession    *_doc;
    qtauController *_ctrl;
    SNoteSetup      _ns;
    QTabWidget     *_tabs;
    QComboBox*      _voiceNameCbo;

    qtauPiano      *_piano;
    qtauNoteEditor *_noteEditor;
    qtauDynDrawer  *_drawZone;
    qtauMeterBar   *_meter;

    QWidget        *_wavePanel; // used to switch its visibility, hidden by default
    QWidget        *_drawZonePanel;
    QSplitter      *_editorSplitter;

    qtauDynLabel   *_fgDynLbl;
    qtauDynLabel   *_bgDynLbl;

    QScrollBar     *_hscr;
    QScrollBar     *_vscr;
    QSlider        *_zoom;
    QComboBox* _singerSelect;

    QTextEdit      *_logpad;
    QList<QToolBar*> _toolbars;

    void enableToolbars(bool enable = true);

    QColor  _logTabTextColor;
    int     _logNewMessages;
    bool    _logHasErrors;
    bool    _showNewLogNumber;

    QString _docName;
    QString _lastScoreDir;
    QString _lastAudioDir;
    QString _audioExt;

    QLabel* _tempoLabel;
    QLabel *_meterLabel;

    void updateNoteSetupLabels();

    void dragEnterEvent (QDragEnterEvent*);
    void dragMoveEvent  (QDragMoveEvent *);
    void dropEvent      (QDropEvent     *);
    void closeEvent     (QCloseEvent    *);

private slots:
    void on_actionPhoneme_Transformation_triggered();
};

#endif // MAINWINDOW_H

