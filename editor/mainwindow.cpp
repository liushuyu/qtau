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
#include "ui_mainwindow.h"
#include "qformlayout.h"

#include <QtGui>
#include <QtCore>
#include <QIcon>

#include "Controller.h"
#include "Session.h"

#include <QGridLayout>
#include <QScrollBar>
#include <QToolBar>
#include <QTabWidget>
#include <QSplitter>
#include <QGroupBox>
#include <QScrollArea>
#include <QTextEdit>
#include <QComboBox>
#include <QDial>
#include <QPushButton>

#include <QFileDialog>

#include "ui/Config.h"
#include "ui/piano.h"
#include "ui/dynDrawer.h"
#include "ui/noteEditor.h"
#include "ui/meter.h"

//#include "audio/Codec.h"

#include "ui/tempodialog.h"

const int cdef_bars             = 128; // 128 bars "is enough for everyone" // TODO: make dynamic

const int c_piano_min_width     = 110;
const int c_piano_min_height    = 40;
const int c_meter_min_height    = 20;
const int c_waveform_min_height = 50;
const int c_drawzone_min_height = 100;
const int c_dynbuttons_num      = 10;

const QString c_dynlbl_css_off = QString("QLabel { color : %1; }").arg(cdef_color_dynbtn_off);
const QString c_dynlbl_css_bg  = QString("QLabel { color : %1; }").arg(cdef_color_dynbtn_bg);
const QString c_dynlbl_css_fg  = QString("QLabel { color : %1; background-color : %2; }")
        .arg(cdef_color_dynbtn_on).arg(cdef_color_dynbtn_on_bg);


QSettings settings("QTau_Devgroup", c_qtau_name);

const QString c_key_dir_score   = QStringLiteral("last_score_dir");
const QString c_key_dir_audio   = QStringLiteral("last_audio_dir");
const QString c_key_win_size    = QStringLiteral("window_size");
const QString c_key_win_max     = QStringLiteral("window_fullscreen");
const QString c_key_show_lognum = QStringLiteral("show_new_log_number");
const QString c_key_dynpanel_on = QStringLiteral("dynamics_panel_visible");
const QString c_key_sound       = QStringLiteral("sould_level");
const QString c_key_audio_codec = QStringLiteral("save_audio_codec");

const QString c_doc_txt         = QStringLiteral(":/tr/documentation_en.txt");
const QString c_icon_app        = QStringLiteral(":/images/appicon_ouka_alice.png");
const QString c_icon_sound      = QStringLiteral(":/images/speaker.png");
const QString c_icon_mute       = QStringLiteral(":/images/speaker_mute.png");
const QString c_icon_editor     = QStringLiteral(":/images/b_notes.png");
const QString c_icon_voices     = QStringLiteral(":/images/b_mic.png");
const QString c_icon_plugins    = QStringLiteral(":/images/b_plug.png");
const QString c_icon_settings   = QStringLiteral(":/images/b_gear.png");
const QString c_icon_doc        = QStringLiteral(":/images/b_manual.png");
const QString c_icon_log        = QStringLiteral(":/images/b_envelope.png");
const QString c_icon_play       = QStringLiteral(":/images/b_play.png");
const QString c_icon_jack       = QStringLiteral(":/images/jack-transport.png");
const QString c_icon_pause      = QStringLiteral(":/images/b_pause.png");


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow),
    _logNewMessages(0), _logHasErrors(false), _showNewLogNumber(true)
{
    ui->setupUi(this);

    setWindowIcon(QIcon(c_icon_app));
    setWindowTitle(c_qtau_name);
    setAcceptDrops(true);
    setContextMenuPolicy(Qt::NoContextMenu);

    //-----------------------------------------

    _meterLabel = new QLabel(QString("%1/%2") .arg(_ns.notesInBar).arg(_ns.noteLength), this);
    _tempoLabel = new QLabel(QString("%1 %2").arg(_ns.tempo).arg(tr("bpm")),           this);
    QPushButton* tempoSelect = new QPushButton(" ");
    tempoSelect->setMaximumHeight(20);
    tempoSelect->setMaximumWidth(20);
    connect(tempoSelect,&QPushButton::clicked,this,&MainWindow::onTempoSelectClicked);

    QHBoxLayout *bpmHBL = new QHBoxLayout();
    bpmHBL->setContentsMargins(0,0,0,0);
    bpmHBL->addSpacing(5);
    bpmHBL->addWidget(tempoSelect);
    bpmHBL->addWidget(_meterLabel);
    bpmHBL->addWidget(_tempoLabel);
    //TODO connectHandler function

    bpmHBL->addSpacing(5);

    QFrame *tempoPanel = new QFrame(this);
    tempoPanel->setMinimumSize(c_piano_min_width, c_meter_min_height);
    tempoPanel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    tempoPanel->setContentsMargins(1,0,1,1);
    tempoPanel->setFrameStyle(QFrame::Panel | QFrame::Raised);

    tempoPanel->setLayout(bpmHBL);

    _meter = new qtauMeterBar(this);
    _meter->setMinimumHeight(c_meter_min_height);
    _meter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    _meter->setContentsMargins(0,0,0,0);

    _piano = new qtauPiano(ui->centralWidget);
    _piano->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    _piano->setMinimumSize(c_piano_min_width, c_piano_min_height);
    _piano->setContentsMargins(0,0,0,0);

    _zoom = new QSlider(Qt::Horizontal, ui->centralWidget);
    _zoom->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    _zoom->setRange(0, c_zoom_num - 1);
    _zoom->setSingleStep(1);
    _zoom->setPageStep(1);
    _zoom->setValue(cdef_zoom_index);
    _zoom->setMinimumWidth(c_piano_min_width);
    _zoom->setGeometry(0,0,c_piano_min_width,10);
    _zoom->setContentsMargins(0,0,0,0);

    _noteEditor = new qtauNoteEditor(ui->centralWidget);
    _noteEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _noteEditor->setContentsMargins(0,0,0,0);




    _hscr = new QScrollBar(Qt::Horizontal, ui->centralWidget);
    _vscr = new QScrollBar(Qt::Vertical,   ui->centralWidget);

    _hscr->setContentsMargins(0,0,0,0);
    _vscr->setContentsMargins(0,0,0,0);
    _hscr->setRange(0, _ns.note.width() * _ns.notesInBar * cdef_bars);
    _vscr->setRange(0, _ns.note.height() * 12 * _ns.numOctaves);
    _hscr->setSingleStep(_ns.note.width());
    _vscr->setSingleStep(_ns.note.height());
    _hscr->setContextMenuPolicy(Qt::NoContextMenu);
    _vscr->setContextMenuPolicy(Qt::NoContextMenu);

    //---- vocal and music waveform panels, hidden until synthesized (vocal wave) and/or loaded (music wave)

    QScrollBar *dummySB = new QScrollBar(this);
    dummySB->setOrientation(Qt::Vertical);
    dummySB->setRange(0,0);
    dummySB->setEnabled(false);

    QFrame *waveControls = new QFrame(this);
    waveControls->setContentsMargins(0,0,0,0);
    waveControls->setMinimumSize(c_piano_min_width, c_waveform_min_height);
    waveControls->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    waveControls->setFrameStyle(QFrame::Panel | QFrame::Raised);

    QGridLayout *waveformL = new QGridLayout();
    waveformL->setContentsMargins(0,0,0,0);
    waveformL->setSpacing(0);
    waveformL->addWidget(waveControls, 0, 0, 2, 1);
    waveformL->addWidget(dummySB,      0, 2, 2, 1);

#if 1
    _wavePanel = new QWidget(this);
    _wavePanel->setContentsMargins(0,0,0,0);
    _wavePanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    _wavePanel->setLayout(waveformL);
    _wavePanel->setVisible(false);
#endif

    //---- notes' dynamics setup area --------------

    QGridLayout *dynBtnL = new QGridLayout();

    QString btnNames[c_dynbuttons_num] = {"VEL", "DYN", "BRE", "BRI", "CLE", "OPE", "GEN", "POR", "PIT", "PBS"};

    for (int i = 0; i < c_dynbuttons_num; ++i)
    {
        qtauDynLabel *l = new qtauDynLabel(btnNames[i], this);
        l->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        dynBtnL->addWidget(l, i / 2, i % 2, 1, 1);

        l->setStyleSheet(c_dynlbl_css_off);
        l->setFrameStyle(QFrame::Box);
        l->setLineWidth(1);

        connect(l, SIGNAL(leftClicked()),  SLOT(dynBtnLClicked()));
        connect(l, SIGNAL(rightClicked()), SLOT(dynBtnRClicked()));
    }

    dynBtnL->setRowStretch(c_dynbuttons_num / 2, 100);

    QFrame *dynBtnPanel = new QFrame(this);
    dynBtnPanel->setContentsMargins(0,0,0,0);
    dynBtnPanel->setMinimumSize(c_piano_min_width, c_drawzone_min_height);
    dynBtnPanel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    dynBtnPanel->setFrameStyle(QFrame::Panel | QFrame::Raised);

    dynBtnPanel->setLayout(dynBtnL);

    _drawZone = new qtauDynDrawer(ui->centralWidget);
    _drawZone->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    _drawZone->setMinimumHeight(c_drawzone_min_height);
    _drawZone->setContentsMargins(0,0,0,0);

    QScrollBar *dummySB3 = new QScrollBar(this);
    dummySB3->setOrientation(Qt::Vertical);
    dummySB3->setRange(0,0);
    dummySB3->setEnabled(false);

    QHBoxLayout *singParamsL = new QHBoxLayout();
    singParamsL->setContentsMargins(0,0,0,0);
    singParamsL->setSpacing(0);
    singParamsL->addWidget(dynBtnPanel);
    singParamsL->addWidget(_drawZone);
    singParamsL->addWidget(dummySB3);

    _drawZonePanel = new QWidget(this);
    _drawZonePanel->setContentsMargins(0,0,0,0);
    _drawZonePanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    _drawZonePanel->setLayout(singParamsL);

    //---- Combining editor panels into hi-level layout ------

    QGridLayout *gl = new QGridLayout();
    gl->setContentsMargins(0,0,0,0);
    gl->setSpacing(0);

    gl->addWidget(tempoPanel, 0, 0, 1, 1);
    gl->addWidget(_meter,      0, 1, 1, 1);
    gl->addWidget(_piano,      1, 0, 1, 1);
    gl->addWidget(_zoom,       2, 0, 1, 1);
    gl->addWidget(_noteEditor, 1, 1, 1, 1);
    gl->addWidget(_hscr,       2, 1, 1, 1);
    gl->addWidget(_vscr,       1, 2, 1, 1);

    QWidget *editorUpperPanel = new QWidget(this);
    editorUpperPanel->setContentsMargins(0,0,0,0);
    editorUpperPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    editorUpperPanel->setMaximumSize(9000,9000);

    editorUpperPanel->setLayout(gl);

    _editorSplitter = new QSplitter(Qt::Vertical, this);
    _editorSplitter->setContentsMargins(0,0,0,0);
    _editorSplitter->addWidget(editorUpperPanel);
    //TODO: rename waveformPanel



    _wavePanel->setVisible(true);
    _editorSplitter->addWidget(_wavePanel);

    _editorSplitter->addWidget(_drawZonePanel);
    _editorSplitter->setStretchFactor(0, 1);
    _editorSplitter->setStretchFactor(1, 0);
    _editorSplitter->setStretchFactor(2, 0);
    _editorSplitter->setStretchFactor(3, 0);

#if 1
    QList<int> sizes = _editorSplitter->sizes();
    sizes[1] = 0;
    _editorSplitter->setSizes(sizes);
#endif

    QVBoxLayout *edVBL = new QVBoxLayout();
    edVBL->setContentsMargins(0,0,0,0);
    edVBL->addWidget(_editorSplitter);

    QWidget *editorPanel = new QWidget(this);
    editorPanel->setContentsMargins(0,0,0,0);
    editorPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    editorPanel->setMaximumSize(9000,9000);

    editorPanel->setLayout(edVBL);

    //---- Voicebank setup tab ---------------------

#if 0
    //---- Plugins setup tab -----------------------

    QWidget *pluginsPanel = new QWidget(this);
    pluginsPanel->setContentsMargins(0,0,0,0);
    pluginsPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    pluginsPanel->setMaximumSize(9000,9000);

    //---- Settings tab ----------------------------

    QWidget *settingsPanel = new QWidget(this);
    settingsPanel->setContentsMargins(0,0,0,0);
    settingsPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    settingsPanel->setMaximumSize(9000,9000);


    //---- Documentation tab -----------------------

    QWidget *docsPanel = new QWidget(this);
    docsPanel->setContentsMargins(0,0,0,0);
    docsPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    docsPanel->setMaximumSize(9000,9000);

    QTextEdit *docpad = new QTextEdit(this);
    docpad->setReadOnly(true);
    docpad->setUndoRedoEnabled(false);
    docpad->setContextMenuPolicy(Qt::NoContextMenu);

    QFile embeddedDocTxt(c_doc_txt);

    if (embeddedDocTxt.open(QFile::ReadOnly))
    {
        QTextStream ts(&embeddedDocTxt);
        ts.setAutoDetectUnicode(true);
        ts.setCodec("UTF-8");

        docpad->setText(ts.readAll());
        embeddedDocTxt.close();
    }

    QGridLayout *docL = new QGridLayout();
    docL->setContentsMargins(0,0,0,0);
    docL->addWidget(docpad, 0, 0, 1, 1);

    docsPanel->setLayout(docL);

#endif

    //---- Log tab ---------------------------------

    QWidget *logPanel = new QWidget(this);
    logPanel->setContentsMargins(0,0,0,0);
    logPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    logPanel->setMaximumSize(9000,9000);

    _logpad = new QTextEdit(this);
    _logpad->setReadOnly(true);
    _logpad->setUndoRedoEnabled(false);
    _logpad->setContextMenuPolicy(Qt::NoContextMenu);
    _logpad->setStyleSheet("p, pre { white-space: 1.2; }");

    QGridLayout *logL = new QGridLayout();
    logL->setContentsMargins(0,0,0,0);
    logL->addWidget(_logpad, 0, 0, 1, 1);

    logPanel->setLayout(logL);

    //---- Combining tabs togeter ------------------

    _tabs = new QTabWidget(this);
    _tabs->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _tabs->setContentsMargins(0,0,0,0);
    _tabs->setMaximumSize(9000, 9000);
    _tabs->setTabPosition(QTabWidget::South);
    _tabs->setMovable(false); // just to be sure

    _tabs->addTab(editorPanel,   QIcon(c_icon_editor),   tr("Editor"));
    //tabs->addTab(voicesPanel,   QIcon(c_icon_voices),   tr("Voices"));
    //tabs->addTab(pluginsPanel,  QIcon(c_icon_plugins),  tr("Plugins"));
    //tabs->addTab(settingsPanel, QIcon(c_icon_settings), tr("Settings"));
    //tabs->addTab(docsPanel,     QIcon(c_icon_doc),      tr("Documentation"));
    _tabs->addTab(logPanel,      QIcon(c_icon_log),      tr("Log"));

    _tabs->widget(0)->setContentsMargins(0,0,0,0);
    _tabs->widget(1)->setContentsMargins(0,0,0,0);
    //tabs->widget(2)->setContentsMargins(0,0,0,0);
    //tabs->widget(3)->setContentsMargins(0,0,0,0);
    //tabs->widget(4)->setContentsMargins(0,0,0,0);
    //tabs->widget(5)->setContentsMargins(0,0,0,0);

    _logTabTextColor = _tabs->tabBar()->tabTextColor(1);

    connect(_tabs, SIGNAL(currentChanged(int)), SLOT(onTabSelected(int)));

    QVBoxLayout *vbl = new QVBoxLayout();
    vbl->setContentsMargins(0,0,0,0);
    vbl->addWidget(_tabs);
    ui->centralWidget->setContentsMargins(0,0,0,0);
    ui->centralWidget->setLayout(vbl);

    //---- Toolbars --------------------------------

    QToolBar *fileTB   = new QToolBar("Fileops",  this);
    QToolBar *playerTB = new QToolBar("Playback", this);
    QToolBar *toolsTB  = new QToolBar("Toolset",  this);

    fileTB  ->setFloatable(false);
    playerTB->setFloatable(false);
    toolsTB ->setFloatable(false);

    fileTB->addAction(ui->actionSave);
    fileTB->addAction(ui->actionSave_audio_as);
    fileTB->addAction(ui->actionUndo);
    fileTB->addAction(ui->actionRedo);

    playerTB->addAction(ui->actionPlay);
    playerTB->addAction(ui->actionStop);
    playerTB->addAction(ui->actionBack);
    playerTB->addAction(ui->actionJack);


    QComboBox *quantizeCombo = new QComboBox(this);
    QComboBox *lengthCombo   = new QComboBox(this);
    quantizeCombo->addItems(QStringList() << "Q/4" << "Q/8" << "Q/16" << "Q/32" << "Q/64");
    lengthCombo  ->addItems(QStringList() << "♪/4" << "♪/8" << "♪/16" << "♪/32" << "♪/64");
    quantizeCombo->setCurrentIndex(3);
    lengthCombo  ->setCurrentIndex(3);

    toolsTB->addAction(ui->actionEdit_Mode);
    toolsTB->addAction(ui->actionGrid_Snap);
    toolsTB->addSeparator();
    toolsTB->addWidget(quantizeCombo);
    toolsTB->addWidget(lengthCombo);

    toolsTB->addSeparator();
    toolsTB->addWidget(new QLabel("Singer: "));
    _singerSelect = new QComboBox();
    _singerSelect->setMinimumWidth(100);
    connect(_singerSelect,   SIGNAL(currentIndexChanged(int)), SLOT(onSingerSelected(int)));
    toolsTB->addWidget(_singerSelect);










    addToolBar(fileTB);
    addToolBar(playerTB);
    addToolBar(toolsTB);

    _toolbars.append(fileTB);
    _toolbars.append(playerTB);
    _toolbars.append(toolsTB);

    //----------------------------------------------
    connect(quantizeCombo, SIGNAL(currentIndexChanged(int)), SLOT(onQuantizeSelected(int)));
    connect(lengthCombo,   SIGNAL(currentIndexChanged(int)), SLOT(onNotelengthSelected(int)));

    connect(vsLog::instance(), &vsLog::message,             this, &MainWindow::onLog);

    connect(_piano,      &qtauPiano      ::heightChanged,    this, &MainWindow::onPianoHeightChanged);
    connect(_noteEditor, &qtauNoteEditor ::widthChanged,     this, &MainWindow::onNoteEditorWidthChanged);

    connect(_meter,      &qtauMeterBar   ::scrolled,         this, &MainWindow::notesHScrolled);
    connect(_piano,      &qtauPiano      ::scrolled,         this, &MainWindow::notesVScrolled);
    connect(_drawZone,   &qtauDynDrawer  ::scrolled,         this, &MainWindow::notesHScrolled);
    connect(_noteEditor, &qtauNoteEditor ::vscrolled,        this, &MainWindow::notesVScrolled);
    connect(_noteEditor, &qtauNoteEditor ::hscrolled,        this, &MainWindow::notesHScrolled);
    connect(_vscr,       &QScrollBar     ::valueChanged,     this, &MainWindow::vertScrolled);
    connect(_hscr,       &QScrollBar     ::valueChanged,     this, &MainWindow::horzScrolled);

    connect(_noteEditor, &qtauNoteEditor ::rmbScrolled,      this, &MainWindow::onEditorRMBScrolled);
    connect(_noteEditor, &qtauNoteEditor ::requestsOffset,   this, &MainWindow::onEditorRequestOffset);

    connect(_zoom,       &QSlider        ::valueChanged,     this, &MainWindow::onZoomed);
    connect(_meter,      &qtauMeterBar   ::zoomed,           this, &MainWindow::onEditorZoomed);
    connect(_noteEditor, &qtauNoteEditor ::zoomed,           this, &MainWindow::onEditorZoomed);
    connect(_drawZone,   &qtauDynDrawer  ::zoomed,           this, &MainWindow::onEditorZoomed);


    connect(ui->actionQuit,      &QAction::triggered, [=]() { this->close(); });
    connect(ui->actionOpen,      &QAction::triggered, this, &MainWindow::onOpenUST);
    connect(ui->actionSave,      &QAction::triggered, this, &MainWindow::onSaveUST);
    connect(ui->actionSave_as,   &QAction::triggered, this, &MainWindow::onSaveUSTAs);

    connect(ui->actionMIDI,&QAction::triggered,this,&MainWindow::onMIDIImport);
    connect(ui->actionMIDI_2,&QAction::triggered,this,&MainWindow::onMIDIExport);

    connect(ui->actionUndo,      &QAction::triggered, this, &MainWindow::onUndo);
    connect(ui->actionRedo,      &QAction::triggered, this, &MainWindow::onRedo);
    connect(ui->actionDelete,    &QAction::triggered, this, &MainWindow::onDelete);

    connect(ui->actionEdit_Mode, &QAction::triggered, this, &MainWindow::onEditMode);
    connect(ui->actionGrid_Snap, &QAction::triggered, this, &MainWindow::onGridSnap);

    //----------------------------------------------

    _lastScoreDir     = settings.value(c_key_dir_score,   "").toString();
    _lastAudioDir     = settings.value(c_key_dir_audio,   "").toString();
    _audioExt         = settings.value(c_key_audio_codec, "").toString();
    _showNewLogNumber = settings.value(c_key_show_lognum, true).toBool();

    if (!settings.value(c_key_dynpanel_on, true).toBool())
    {
        QList<int> panelSizes = _editorSplitter->sizes();
        panelSizes.last() = 0;
        _editorSplitter->setSizes(panelSizes);
    }

    if (settings.value(c_key_win_max, false).toBool())
        showMaximized();
    else
    {
        QRect winGeom = geometry();
        QRect setGeom = settings.value(c_key_win_size, QRect(winGeom.topLeft(), minimumSize())).value<QRect>();

        if (setGeom.width() >= winGeom.width() && setGeom.height() >= setGeom.height())
            setGeometry(setGeom);
    }

    //----------------------------------------------

    vsLog::instance()->enableHistory(false);
    onLog(QString("\t%1 %2 @ %3").arg(tr("Launching QTau")).arg(c_qtau_ver).arg(__DATE__), ELog::success);

    onLog("\t---------------------------------------------", ELog::info);
    vsLog::r(); // print stored messages from program startup
    onLog("\t---------------------------------------------", ELog::info);
    vsLog::n();

    // -- hacks for debugging UI -- remove or refactor

    ui->actionPlay->setText(tr("Play"));
    ui->actionPlay->setIcon(QIcon(c_icon_play));
    ui->actionStop->setEnabled(true);
    ui->actionBack->setEnabled(true);
    ui->actionRepeat->setEnabled(false);
    ui->actionSave_audio_as->setEnabled(false);

    ui->actionPlay->setChecked(false);
    ui->actionRepeat->setChecked(false);

    ui->actionPlay->setEnabled(true);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // store settings
    settings.setValue(c_key_dir_score,   _lastScoreDir);
    settings.setValue(c_key_dir_audio,   _lastAudioDir);
    settings.setValue(c_key_win_size,    geometry());
    settings.setValue(c_key_win_max,     isMaximized());
    settings.setValue(c_key_show_lognum, _showNewLogNumber);
    settings.setValue(c_key_audio_codec, _audioExt);
    settings.setValue(c_key_dynpanel_on, _editorSplitter->sizes().last() > 0);

    event->accept();
}

MainWindow::~MainWindow() { delete ui; }

//========================================================================================


bool MainWindow::setController(qtauController &c, qtauSession &s)
{
    // NOTE: bind what uses qtauSession only here (menu/toolbar button states etc)
    _doc = &s;
    _ctrl = &c;

    connect(_noteEditor, &qtauNoteEditor::editorEvent, _doc, &qtauSession::onUIEvent      );

    connect(&c,&qtauController::transportPositionChanged,this,&MainWindow::onTransportPositionChanged);

    connect(ui->actionNew,&QAction::triggered,&s,&qtauSession::onNewSession);

    connect(_doc, &qtauSession::dataReloaded,   this, &MainWindow::onDocReloaded         );
    connect(_doc, &qtauSession::modifiedStatus, this, &MainWindow::onDocStatus           );
    connect(_doc, &qtauSession::undoStatus,     this, &MainWindow::onUndoStatus          );
    connect(_doc, &qtauSession::redoStatus,     this, &MainWindow::onRedoStatus          );


    connect(_doc, &qtauSession::vocalSet,       this, &MainWindow::onVocalAudioChanged   );
    connect(_doc, &qtauSession::musicSet,       this, &MainWindow::onMusicAudioChanged   );

    connect(_doc, &qtauSession::onEvent,        this, &MainWindow::onDocEvent            );

    connect(ui->actionPlay,   &QAction::triggered, _doc, &qtauSession::startPlayback );
    connect(ui->actionStop,   &QAction::triggered, _doc, &qtauSession::stopPlayback  );
    connect(ui->actionBack,   &QAction::triggered, _doc, &qtauSession::resetPlayback );



    connect(this,   &MainWindow::loadUST,      &c, &qtauController::onLoadUST       );
    connect(this,   &MainWindow::saveUST,      &c, &qtauController::onSaveUST       );

    connect(_piano, &qtauPiano::keyPressed,     &c, &qtauController::pianoKeyPressed );
    connect(_piano, &qtauPiano::keyReleased,    &c, &qtauController::pianoKeyReleased);

    connect(ui->actionJack,   &QAction::triggered, this, &MainWindow::onActionJackTriggered       );
    //-----------------------------------------------------------------------

    // widget configuration - maybe read app settings here?
    _noteEditor->setRMBScrollEnabled(!ui->actionEdit_Mode->isChecked());
    _noteEditor->setEditingEnabled  ( ui->actionEdit_Mode->isChecked());
    _noteEditor->setFocus();

    foreach(QString voice,c.voices())
    {
        _singerSelect->addItem(voice);
    }

    return true;
}

void MainWindow::onOpenUST()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open USTJ"), _lastScoreDir, tr("UTAU JSON Sequence Text Files (*.ustj)"));

    if (!fileName.isEmpty())
    {
        _lastScoreDir = QFileInfo(fileName).absolutePath();
        emit loadUST(fileName);
    }
}

void MainWindow::onSaveUST()
{
    if (_doc->documentFile().isEmpty())
        onSaveUSTAs();
    else
        emit saveUST(_doc->documentFile(), true);
}

void MainWindow::onSaveUSTAs()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save USTJ"), _lastScoreDir, tr("UTAU JSON Sequence Text Files (*.ustj)"));

    if (!fileName.isEmpty())
    {
        _lastScoreDir = QFileInfo(fileName).absolutePath();
        emit saveUST(fileName, true);
    }
}

void MainWindow::onTransportPositionChanged(float pos)
{
    _noteEditor->setPlaybackPosition(pos*480); //FIXME: do not hardcode utau
}

void MainWindow::onMIDIImport()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Import MIDI"),
                                                    QString("/home/hatsunemiku/Music/MIDIs/"), //FIXME do not hardcode
                                                    tr("MIDI Files (*.mid *.mid *.smf)"));
    _doc->importMIDI(fileName);
}

void MainWindow::onMIDIExport()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export as MIDI"),
                                                    QString(),
                                                    tr("MIDI Files (*.mid *.mid *.smf)"));
    _doc->exportMIDI(fileName);
}

void MainWindow::notesVScrolled(int delta)
{
    if (delta > 0 && _vscr->value() > 0) // scroll up
        delta = -_ns.note.height();
    else if (delta < 0 && _vscr->value() < _vscr->maximum()) // scroll down
        delta = _ns.note.height();
    else
        delta = 0;

    if (delta != 0)
        _vscr->setValue(_vscr->value() + delta);
}

void MainWindow::notesHScrolled(int delta)
{
    if (delta > 0 && _hscr->value() > 0) // scroll left
        delta = -_ns.note.width();
    else if (delta < 0 && _hscr->value() < _hscr->maximum()) // scroll right
        delta = _ns.note.width();
    else
        delta = 0;

    if (delta != 0)
        _hscr->setValue(_hscr->value() + delta);
}

void MainWindow::vertScrolled(int delta)
{
    _piano->setOffset(delta);
    _noteEditor->setVOffset(delta);
}

void MainWindow::horzScrolled(int delta)
{
    _noteEditor->setHOffset(delta);
    _meter     ->setOffset (delta);
    _drawZone  ->setOffset (delta);
}

void MainWindow::onEditorRMBScrolled(QPoint mouseDelta, QPoint origOffset)
{
    // moving editor space in reverse of mouse delta
    int hOff = qMax(qMin(origOffset.x() - mouseDelta.x(), _hscr->maximum()), 0);
    int vOff = qMax(qMin(origOffset.y() - mouseDelta.y(), _vscr->maximum()), 0);

    _hscr->setValue(hOff);
    _vscr->setValue(vOff);
}

void MainWindow::onEditorRequestOffset(QPoint off)
{
    off.setX(qMax(qMin(off.x(), _hscr->maximum()), 0));
    off.setY(qMax(qMin(off.y(), _vscr->maximum()), 0));

    _hscr->setValue(off.x());
    _vscr->setValue(off.y());
}

void MainWindow::onPianoHeightChanged(int newHeight)
{
    _vscr->setMaximum(_ns.note.height() * 12 * _ns.numOctaves - newHeight + 1);
    _vscr->setPageStep(_piano->geometry().height());
}

void MainWindow::onNoteEditorWidthChanged(int newWidth)
{
    _hscr->setMaximum(_ns.note.width() * _ns.notesInBar * cdef_bars - newWidth + 1);
    _hscr->setPageStep(_noteEditor->geometry().width());
}

//NEW playback states with JACK

void MainWindow::onUndo()
{
    if (_doc->canUndo())
        _doc->undo();
    else
        ui->actionUndo->setEnabled(false);
}

void MainWindow::onRedo()
{
    if (_doc->canRedo())
        _doc->redo();
    else
        ui->actionRedo->setEnabled(false);
}

void MainWindow::onDelete()
{
    _noteEditor->deleteSelected();
}

void MainWindow::onEditMode(bool toggled)
{
    _noteEditor->setEditingEnabled  ( toggled);
    _noteEditor->setRMBScrollEnabled(!toggled);
}

void MainWindow::onGridSnap(bool toggled)
{
    _noteEditor->setGridSnapEnabled(toggled);
}

void MainWindow::onQuantizeSelected(int index)
{
    int newQuant = 4 * (int)(pow(2, index) + 0.001);

    if (newQuant != _ns.quantize)
    {
        _ns.quantize = newQuant;
        _noteEditor->configure(_ns);
    }
}

void MainWindow::onNotelengthSelected(int index)
{
    int newNoteLength = 4 * (int)(pow(2, index) + 0.001);

    if (newNoteLength != _ns.length)
    {
        _ns.length = newNoteLength;
        _noteEditor->configure(_ns);
    }
}

void MainWindow::dynBtnLClicked()
{
    qtauDynLabel* l= qobject_cast<qtauDynLabel*>(sender());

    if (l && (_fgDynLbl == 0 || l != _fgDynLbl))
    {
        if (_fgDynLbl)
        {
            _fgDynLbl->setState(qtauDynLabel::off);
            _fgDynLbl->setStyleSheet(c_dynlbl_css_off);
        }

        if (l == _bgDynLbl)
        {
            _bgDynLbl->setState(qtauDynLabel::off);
            _bgDynLbl->setStyleSheet(c_dynlbl_css_off);
            _bgDynLbl = 0;
        }

        l->setStyleSheet(c_dynlbl_css_fg);
        _fgDynLbl = l;
    }
}

void MainWindow::dynBtnRClicked()
{
    qtauDynLabel* l= qobject_cast<qtauDynLabel*>(sender());

    if (l)
    {
        if (_bgDynLbl != 0 && l == _bgDynLbl)
        {
            // clicking on same dynkey - switch it off
            _bgDynLbl->setState(qtauDynLabel::off);
            _bgDynLbl->setStyleSheet(c_dynlbl_css_off);
            _bgDynLbl = 0;
        }
        else
        {   // clicking on other dynkey
            if (_bgDynLbl)
            {   // switch off previous one, if any
                _bgDynLbl->setState(qtauDynLabel::off);
                _bgDynLbl->setStyleSheet(c_dynlbl_css_off);
                _bgDynLbl = 0;
            }

            if (l != _fgDynLbl)
            {   // clicking on not-foreground dynkey
                l->setStyleSheet(c_dynlbl_css_bg);
                _bgDynLbl = l;
            }
        }
    }
}

void MainWindow::onLog(const QString &msg, ELog type)
{
    QString color = "black";
    bool viewingLog = _tabs->currentIndex() == _tabs->count() - 1;

    switch(type)
    {
    case ELog::error:
        color = "red";
        break;
    case ELog::success:
        color = "green";
        break;
    default: break;
    }

    if (!viewingLog)
    {
        QTabBar *tb = const_cast<QTabBar *>(_tabs->tabBar()); // dirty hack I know, but no other way atm

        if (_showNewLogNumber)
        {
            tb->setTabText(tb->count() - 1, tr("Log") + QString(" (%1)").arg(_logNewMessages));
            _logNewMessages++;
        }

        if (type == ELog::error)
        {
            tb->setTabTextColor(tb->count() - 1, QColor(cdef_color_logtab_err));
            _logHasErrors = true;
        }
    }

    _logpad->moveCursor(QTextCursor::End);
    _logpad->insertHtml(QString("<pre style=\"color: %1;\">%2</pre><p></p>").arg(color).arg(msg));
}

void MainWindow::enableToolbars(bool enable)
{
    foreach (QToolBar *t, _toolbars)
        t->setVisible(enable);      //t->setEnabled(enable);
}

void MainWindow::onTabSelected(int index)
{
    enableToolbars(index == 0);

    if (index == _tabs->count() - 1)
    {
        QTabBar *tb = const_cast<QTabBar *>(_tabs->tabBar());

        if (_logNewMessages > 0)
        {
            tb->setTabText(tb->count() - 1, tr("Log"));
            _logNewMessages = 0;
        }

        if (_logHasErrors)
        {
            tb->setTabTextColor(tb->count() - 1, _logTabTextColor); // set default color back
            _logHasErrors = false;
        }
    }
}

void MainWindow::onZoomed(int z)
{
    // modify note data and send it to widgets
    _ns.note.setWidth(c_zoom_note_widths[z]);

    _meter     ->configure(_ns);
    _piano     ->configure(_ns);
    _noteEditor->configure(_ns);
    _drawZone  ->configure(_ns);

    // modify scrollbar sizes and position
    double hscr_val = (double)_hscr->value() / _hscr->maximum();
    _hscr->setMaximum(_ns.note.width()  * _ns.notesInBar * cdef_bars - _noteEditor->width() + 1);
    _hscr->setValue(_hscr->maximum() * hscr_val);
    _hscr->setSingleStep(_ns.note.width());

    horzScrolled(_hscr->value());
}

void MainWindow::onEditorZoomed(int delta)
{
    if (delta != 0)
        if ((delta > 0 && _zoom->value() >= 0) ||
                (delta < 0 && _zoom->value() < c_zoom_num))
            _zoom->setValue(_zoom->value() + ((delta > 0) ? 1 : -1));
}

void MainWindow::onDocReloaded()
{
    setWindowTitle(_doc->documentName() + " - QTau");
    _noteEditor->reset();
}

void MainWindow::onDocStatus(bool isModified)
{
    QString newDocName = _doc->documentName();

    if (_docName != newDocName)
        _docName = newDocName;

    setWindowTitle((isModified ? "*" : "") + _docName + " - QTau");

    ui->actionSave->setEnabled(isModified);
}

void MainWindow::onUndoStatus(bool canUndo)
{
    ui->actionUndo->setEnabled(canUndo);
}

void MainWindow::onRedoStatus(bool canRedo)
{
    ui->actionRedo->setEnabled(canRedo);
}

void MainWindow::onDocEvent(qtauEvent* event)
{

    //FIXME: why here
    QString singerName = _doc->getSingerName();
    if(singerName.length())
    {

        int count = _singerSelect->count();
        for(int i=0;i<count;i++)
        {
            if(singerName==_singerSelect->itemText(i))
            {
                _singerSelect->setCurrentIndex(i);
                _ctrl->selectSinger(singerName);
                break;
            }
        }
    }

    if(_doc->getTempo()>0)
    {
        _ns.tempo = _doc->getTempo();
        QJsonArray a = _doc->getTimeSignature();
        _ns.notesInBar=a[0].toInt();
        _ns.noteLength=a[1].toInt();
        updateNoteSetupLabels();
    }



    if (event->type() >= ENoteEvents::add && event->type() <= ENoteEvents::effect)
        _noteEditor->onEvent(event);
    else
        vsLog::e("bad event");
}

void MainWindow::onVocalAudioChanged()
{
    // show vocal waveform panel and send audioSource to it for generation
#if 0
    wavePanel->setVisible(true);
    vocalWave->setVisible(true);
    QList<int> sizes = editorSplitter->sizes();

    if (sizes[1] <= 0)
    {
        sizes[1] = 30;
        editorSplitter->setSizes(sizes);
    }
#endif
}

void MainWindow::onMusicAudioChanged()
{
    // show & fill music waveform panel
#if 0
    wavePanel->setVisible(true);
    musicWave->setVisible(true);
    QList<int> sizes = editorSplitter->sizes();

    if (sizes[1] <= 0)
    {
        sizes[1] = 30;
        editorSplitter->setSizes(sizes);
    }

    //FIXME:musicWave->setAudio(doc->getMusic().musicWave);
#endif
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    // accepting filepaths
    if (event->mimeData()->hasFormat("text/uri-list"))
        event->acceptProposedAction();
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    // accepting filepaths
    if (event->mimeData()->hasFormat("text/uri-list"))
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    QList<QUrl> uris;

    foreach (const QByteArray &uriData, event->mimeData()->data("text/uri-list").split('\n'))
        if (!uriData.isEmpty())
            uris << QUrl::fromEncoded(uriData).toLocalFile().remove('\r');

    if (!uris.isEmpty())
    {
        QFileInfo fi(uris.first().toString());

        if (uris.size() > 1)
            vsLog::i("Multiple URIs dropped, currently using only first one");

        if (fi.exists() && !fi.isDir() && !fi.suffix().isEmpty()) // if it's an existing file with some extension
        {
            // maybe it's a note/lyrics file? (ust/vsq/vsqx/midi)
            if (fi.suffix() == "ust") // TODO: support many, or do something like audio codecs registry
            {
                emit loadUST(fi.absoluteFilePath());
            }
            else
                vsLog::e("File extension not supported: " + fi.suffix());
        }
    }
}

//autoconneccted slot
void MainWindow::on_actionPhoneme_Transformation_triggered()
{
    _noteEditor->doPhonemeTransformation();
}

void MainWindow::onSingerSelected(int index)
{
    _ctrl->selectSinger(_singerSelect->itemText(index));
    _doc->setSingerName(_singerSelect->itemText(index));
}

void MainWindow::onActionJackTriggered()
{
    QAction* a = (QAction*)QObject::sender();
    _ctrl->setJackTranportEnabled(a->isChecked());
}

void MainWindow::onTempoSelectClicked()
{
    TempoDialog dialog(this);
    TempoTimeSig sig;
    sig.tempo=_ns.tempo;
    sig.numerator=_ns.notesInBar;
    sig.denominator=_ns.noteLength;
    dialog.setTempoTimeSignature(sig);
    if(dialog.exec()==QDialog::Accepted)
    {
        sig=dialog.tempoTimeSignature();
        _ns.tempo=sig.tempo;
        _ns.notesInBar=sig.numerator;
        _ns.noteLength=sig.denominator;
        //update func: TODO
        updateNoteSetupLabels();
        //change ustj timesig
        _doc->setTempo(sig.tempo);
        QJsonArray ts;
        ts.append(QJsonValue(_ns.notesInBar));
        ts.append(QJsonValue(_ns.noteLength));
        _doc->setTimeSignature(ts);
    }
}

void MainWindow::updateNoteSetupLabels()
{
    //rename to updateSetup()
    _tempoLabel->setText(QString("%1 %2").arg(_ns.tempo).arg(tr("bpm")));
    _meterLabel->setText(QString("%1/%2").arg(_ns.notesInBar).arg(_ns.noteLength));

    //,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
    _ctrl->updateTempoTimeSignature(_ns.tempo);
    _piano->configure(_ns);
    _meter->configure(_ns);
    _noteEditor->configure(_ns);


}

