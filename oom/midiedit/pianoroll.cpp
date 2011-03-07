//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: pianoroll.cpp,v 1.25.2.15 2009/11/16 11:29:33 lunar_shuttle Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include <QLayout>
#include <QSizeGrip>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QToolTip>
#include <QMenu>
#include <QSignalMapper>
#include <QMenuBar>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QAction>
#include <QKeySequence>
#include <QKeyEvent>
#include <QGridLayout>
#include <QResizeEvent>
#include <QCloseEvent>
#include <QMimeData>
#include <QScrollArea>
#include <QWidgetAction>

#include <stdio.h>

#include "xml.h"
#include "mtscale.h"
#include "pcscale.h"
#include "prcanvas.h"
#include "pianoroll.h"
#include "poslabel.h"
#include "pitchlabel.h"
#include "scrollscale.h"
#include "piano.h"
#include "../ctrl/ctrledit.h"
#include "splitter.h"
#include "ttoolbar.h"
//#include "tb1.h"
#include "utils.h"
#include "globals.h"
#include "gconfig.h"
#include "icons.h"
#include "audio.h"
#include "midiport.h"

#include "cmd.h"
#include "quantconfig.h"
#include "shortcuts.h"

#include "mtrackinfo.h"

#include "traverso_shared/TConfig.h"

int PianoRoll::_quantInit = 96;
int PianoRoll::_rasterInit = 96;
int PianoRoll::_widthInit = 600;
int PianoRoll::_heightInit = 400;
int PianoRoll::_quantStrengthInit = 80; // 1 - 100%
int PianoRoll::_quantLimitInit = 50; // tick value
bool PianoRoll::_quantLenInit = false;
int PianoRoll::_toInit = 0;
int PianoRoll::colorModeInit = 0;

static const int xscale = -10;
static const int yscale = 1;
static const int pianoWidth = 40;
static int pianorollTools = PointerTool | PencilTool | RubberTool | DrawTool;


//---------------------------------------------------------
//   PianoRoll
//---------------------------------------------------------

PianoRoll::PianoRoll(PartList* pl, QWidget* parent, const char* name, unsigned initPos)
: MidiEditor(_quantInit, _rasterInit, pl, parent, name)
{
	deltaMode = false;
        // Set size stored in global config, or use defaults.
        int w = tconfig().get_property("PianoRollEdit", "widgetwidth", 800).toInt();
        int h = tconfig().get_property("PianoRollEdit", "widgetheigth", 650).toInt();
		//FIXME: This needs to be checked to make sure its not larger than the current desktop size
        resize(w, h);

	selPart = 0;
	quantConfig = 0;
	_playEvents = false;
	_replay = false;
	replayPos = song->cpos();
	_quantStrength = _quantStrengthInit;
	_quantLimit = _quantLimitInit;
	_quantLen = _quantLenInit;
	_to = _toInit;
	colorMode = colorModeInit;
	_stepQwerty = false;

	QSignalMapper* mapper = new QSignalMapper(this);
	QSignalMapper* colorMapper = new QSignalMapper(this);

	//---------Menu----------------------------------

	menuEdit = menuBar()->addMenu(tr("&Edit"));

	menuEdit->addActions(undoRedo->actions());

	menuEdit->addSeparator();

	editCutAction = menuEdit->addAction(QIcon(*editcutIconSet), tr("C&ut"));
	mapper->setMapping(editCutAction, PianoCanvas::CMD_CUT);
	connect(editCutAction, SIGNAL(triggered()), mapper, SLOT(map()));

	editCopyAction = menuEdit->addAction(QIcon(*editcopyIconSet), tr("&Copy"));
	mapper->setMapping(editCopyAction, PianoCanvas::CMD_COPY);
	connect(editCopyAction, SIGNAL(triggered()), mapper, SLOT(map()));

	editPasteAction = menuEdit->addAction(QIcon(*editpasteIconSet), tr("&Paste"));
	mapper->setMapping(editPasteAction, PianoCanvas::CMD_PASTE);
	connect(editPasteAction, SIGNAL(triggered()), mapper, SLOT(map()));

	menuEdit->addSeparator();

	editDelEventsAction = menuEdit->addAction(tr("Delete &Events"));
	mapper->setMapping(editDelEventsAction, PianoCanvas::CMD_DEL);
	connect(editDelEventsAction, SIGNAL(triggered()), mapper, SLOT(map()));

	menuEdit->addSeparator();

	menuSelect = menuEdit->addMenu(QIcon(*selectIcon), tr("&Select"));

	selectAllAction = menuSelect->addAction(QIcon(*select_allIcon), tr("Select &All"));
	mapper->setMapping(selectAllAction, PianoCanvas::CMD_SELECT_ALL);
	connect(selectAllAction, SIGNAL(triggered()), mapper, SLOT(map()));

	selectNoneAction = menuSelect->addAction(QIcon(*select_deselect_allIcon), tr("&Deselect All"));
	mapper->setMapping(selectNoneAction, PianoCanvas::CMD_SELECT_NONE);
	connect(selectNoneAction, SIGNAL(triggered()), mapper, SLOT(map()));

	selectInvertAction = menuSelect->addAction(QIcon(*select_invert_selectionIcon), tr("Invert &Selection"));
	mapper->setMapping(selectInvertAction, PianoCanvas::CMD_SELECT_INVERT);
	connect(selectInvertAction, SIGNAL(triggered()), mapper, SLOT(map()));

	menuSelect->addSeparator();

	selectInsideLoopAction = menuSelect->addAction(QIcon(*select_inside_loopIcon), tr("&Inside Loop"));
	mapper->setMapping(selectInsideLoopAction, PianoCanvas::CMD_SELECT_ILOOP);
	connect(selectInsideLoopAction, SIGNAL(triggered()), mapper, SLOT(map()));

	selectOutsideLoopAction = menuSelect->addAction(QIcon(*select_outside_loopIcon), tr("&Outside Loop"));
	mapper->setMapping(selectOutsideLoopAction, PianoCanvas::CMD_SELECT_OLOOP);
	connect(selectOutsideLoopAction, SIGNAL(triggered()), mapper, SLOT(map()));

	menuSelect->addSeparator();

	//selectPrevPartAction = select->addAction(tr("&Previous Part"));
	selectPrevPartAction = menuSelect->addAction(QIcon(*select_all_parts_on_trackIcon), tr("&Previous Part"));
	mapper->setMapping(selectPrevPartAction, PianoCanvas::CMD_SELECT_PREV_PART);
	connect(selectPrevPartAction, SIGNAL(triggered()), mapper, SLOT(map()));

	//selNextPartAction = select->addAction(tr("&Next Part"));
	selectNextPartAction = menuSelect->addAction(QIcon(*select_all_parts_on_trackIcon), tr("&Next Part"));
	mapper->setMapping(selectNextPartAction, PianoCanvas::CMD_SELECT_NEXT_PART);
	connect(selectNextPartAction, SIGNAL(triggered()), mapper, SLOT(map()));

	menuConfig = menuBar()->addMenu(tr("&Config"));

	eventColor = menuConfig->addMenu(tr("&Event Color"));

	QActionGroup* actgrp = new QActionGroup(this);
	actgrp->setExclusive(true);

	//evColorBlueAction = eventColor->addAction(tr("&Blue"));
	evColorBlueAction = actgrp->addAction(tr("&Blue"));
	evColorBlueAction->setCheckable(true);
	colorMapper->setMapping(evColorBlueAction, 0);

	//evColorPitchAction = eventColor->addAction(tr("&Pitch colors"));
	evColorPitchAction = actgrp->addAction(tr("&Pitch colors"));
	evColorPitchAction->setCheckable(true);
	colorMapper->setMapping(evColorPitchAction, 1);

	//evColorVelAction = eventColor->addAction(tr("&Velocity colors"));
	evColorVelAction = actgrp->addAction(tr("&Velocity colors"));
	evColorVelAction->setCheckable(true);
	colorMapper->setMapping(evColorVelAction, 2);

	connect(evColorBlueAction, SIGNAL(triggered()), colorMapper, SLOT(map()));
	connect(evColorPitchAction, SIGNAL(triggered()), colorMapper, SLOT(map()));
	connect(evColorVelAction, SIGNAL(triggered()), colorMapper, SLOT(map()));

	eventColor->addActions(actgrp->actions());

	connect(colorMapper, SIGNAL(mapped(int)), this, SLOT(eventColorModeChanged(int)));

	menuFunctions = menuBar()->addMenu(tr("&Functions"));

	menuFunctions->setTearOffEnabled(true);

	funcOverQuantAction = menuFunctions->addAction(tr("Over Quantize"));
	mapper->setMapping(funcOverQuantAction, PianoCanvas::CMD_OVER_QUANTIZE);
	connect(funcOverQuantAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcNoteOnQuantAction = menuFunctions->addAction(tr("Note On Quantize"));
	mapper->setMapping(funcNoteOnQuantAction, PianoCanvas::CMD_ON_QUANTIZE);
	connect(funcNoteOnQuantAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcNoteOnOffQuantAction = menuFunctions->addAction(tr("Note On/Off Quantize"));
	mapper->setMapping(funcNoteOnOffQuantAction, PianoCanvas::CMD_ONOFF_QUANTIZE);
	connect(funcNoteOnOffQuantAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcIterQuantAction = menuFunctions->addAction(tr("Iterative Quantize"));
	mapper->setMapping(funcIterQuantAction, PianoCanvas::CMD_ITERATIVE_QUANTIZE);
	connect(funcIterQuantAction, SIGNAL(triggered()), mapper, SLOT(map()));

	menuFunctions->addSeparator();

	funcConfigQuantAction = menuFunctions->addAction(tr("Config Quant..."));
	connect(funcConfigQuantAction, SIGNAL(triggered()), this, SLOT(configQuant()));

	menuFunctions->addSeparator();

	funcGateTimeAction = menuFunctions->addAction(tr("Modify Gate Time"));
	mapper->setMapping(funcGateTimeAction, PianoCanvas::CMD_MODIFY_GATE_TIME);
	connect(funcGateTimeAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcModVelAction = menuFunctions->addAction(tr("Modify Velocity"));
	mapper->setMapping(funcModVelAction, PianoCanvas::CMD_MODIFY_VELOCITY);
	connect(funcModVelAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcCrescendoAction = menuFunctions->addAction(tr("Crescendo"));
	mapper->setMapping(funcCrescendoAction, PianoCanvas::CMD_CRESCENDO);
	funcCrescendoAction->setEnabled(false);
	connect(funcCrescendoAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcTransposeAction = menuFunctions->addAction(tr("Transpose"));
	mapper->setMapping(funcTransposeAction, PianoCanvas::CMD_TRANSPOSE);
	funcTransposeAction->setEnabled(false);
	connect(funcTransposeAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcThinOutAction = menuFunctions->addAction(tr("Thin Out"));
	mapper->setMapping(funcThinOutAction, PianoCanvas::CMD_THIN_OUT);
	funcThinOutAction->setEnabled(false);
	connect(funcThinOutAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcEraseEventAction = menuFunctions->addAction(tr("Erase Event"));
	mapper->setMapping(funcEraseEventAction, PianoCanvas::CMD_ERASE_EVENT);
	funcEraseEventAction->setEnabled(false);
	connect(funcEraseEventAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcNoteShiftAction = menuFunctions->addAction(tr("Note Shift"));
	mapper->setMapping(funcNoteShiftAction, PianoCanvas::CMD_NOTE_SHIFT);
	funcNoteShiftAction->setEnabled(false);
	connect(funcNoteShiftAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcMoveClockAction = menuFunctions->addAction(tr("Move Clock"));
	mapper->setMapping(funcMoveClockAction, PianoCanvas::CMD_MOVE_CLOCK);
	funcMoveClockAction->setEnabled(false);
	connect(funcMoveClockAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcCopyMeasureAction = menuFunctions->addAction(tr("Copy Measure"));
	mapper->setMapping(funcCopyMeasureAction, PianoCanvas::CMD_COPY_MEASURE);
	funcCopyMeasureAction->setEnabled(false);
	connect(funcCopyMeasureAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcEraseMeasureAction = menuFunctions->addAction(tr("Erase Measure"));
	mapper->setMapping(funcEraseMeasureAction, PianoCanvas::CMD_ERASE_MEASURE);
	funcEraseMeasureAction->setEnabled(false);
	connect(funcEraseMeasureAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcDelMeasureAction = menuFunctions->addAction(tr("Delete Measure"));
	mapper->setMapping(funcDelMeasureAction, PianoCanvas::CMD_DELETE_MEASURE);
	funcDelMeasureAction->setEnabled(false);
	connect(funcDelMeasureAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcCreateMeasureAction = menuFunctions->addAction(tr("Create Measure"));
	mapper->setMapping(funcCreateMeasureAction, PianoCanvas::CMD_CREATE_MEASURE);
	funcCreateMeasureAction->setEnabled(false);
	connect(funcCreateMeasureAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcSetFixedLenAction = menuFunctions->addAction(tr("Set Fixed Length"));
	mapper->setMapping(funcSetFixedLenAction, PianoCanvas::CMD_FIXED_LEN);
	connect(funcSetFixedLenAction, SIGNAL(triggered()), mapper, SLOT(map()));

	funcDelOverlapsAction = menuFunctions->addAction(tr("Delete Overlaps"));
	mapper->setMapping(funcDelOverlapsAction, PianoCanvas::CMD_DELETE_OVERLAPS);
	connect(funcDelOverlapsAction, SIGNAL(triggered()), mapper, SLOT(map()));

	menuPlugins = menuBar()->addMenu(tr("&Plugins"));
	song->populateScriptMenu(menuPlugins, this);

	connect(mapper, SIGNAL(mapped(int)), this, SLOT(cmd(int)));

	//---------ToolBar----------------------------------
	//tools = addToolBar(tr("Pianoroll tools"));
	//tools->setObjectName("tbPRtools");
	//tools->addActions(undoRedo->actions());
	//tools->addSeparator();
	//tools->setIconSize(QSize(22, 22));

    srec = new QToolButton();
	srec->setToolTip(tr("Step Record"));
	srec->setIcon(*steprecIcon);
    srec->setCheckable(true);
	//srec->setObjectName("StepRecord");
	//tools->addWidget(srec);

    midiin = new QToolButton();
	midiin->setToolTip(tr("Midi Input"));
	midiin->setIcon(*midiinIcon);
	midiin->setCheckable(true);
	//tools->addWidget(midiin);

    speaker = new QToolButton();
	speaker->setToolTip(tr("Play Events"));
	speaker->setIcon(*speakerIcon);
	speaker->setCheckable(true);
	//tools->addWidget(speaker);

	solo = new QToolButton();
	solo->setIcon(*soloIconSet2);
	solo->setIconSize(soloIconOn->size());
	solo->setToolTip(tr("Solo"));
	solo->setCheckable(true);

	repPlay = new QToolButton();
	repPlay->setIcon(*auditionIcon);
	repPlay->setIconSize(soloIconOn->size());
	repPlay->setToolTip(tr("Toggle Audition Mode"));
	repPlay->setCheckable(true);

	QToolBar *cursorBar = new QToolBar(tr("Cursor"));
	posLabel = new PosLabel(0, "pos");
	posLabel->setFixedHeight(22);
	posLabel->setObjectName("Cursor");
	cursorBar->addWidget(posLabel);

	pitchLabel = new PitchLabel(0);
	pitchLabel->setFixedHeight(22);
	pitchLabel->setObjectName("pitchLabel");
	cursorBar->addWidget(pitchLabel);

	patchLabel = new QLabel();
	patchLabel->setObjectName("patchLabel");
	patchLabel->setMaximumSize(QSize(180, 22));
	patchLabel->setFixedWidth(180);
	patchLabel->setFixedHeight(22);
	menuBar()->setCornerWidget(patchLabel, Qt::TopRightCorner);
	patchLabel->show();
	//cursorBar->addWidget(patchLabel);

	addToolBar(Qt::BottomToolBarArea, cursorBar);
	cursorBar->setFloatable(false);
	cursorBar->setMovable(false);
	cursorBar->setAllowedAreas(Qt::BottomToolBarArea);

	tools2 = new EditToolBar(this, pianorollTools);
	tools2->setIconSize(QSize(22, 22));
	addToolBar(Qt::BottomToolBarArea, tools2);
	tools2->setFloatable(false);
	tools2->setMovable(false);
	tools2->setAllowedAreas(Qt::BottomToolBarArea);
	QWidget* spacer = new QWidget();
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	spacer->setMaximumWidth(35);
	tools2->addWidget(spacer);


	//-------------------------------------------------------------
	//    Transport Bar
    //QToolBar* transport = new QToolBar(tr("Transport"));
	//addToolBar(Qt::BottomToolBarArea, transport);
	tools2->addWidget(srec);
	tools2->addWidget(speaker);
	QWidget* spacer1 = new QWidget();
	spacer1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	spacer1->setMaximumWidth(10);
	tools2->addWidget(spacer1);
	QWidget* spacer5 = new QWidget();
	spacer5->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	spacer5->setMaximumWidth(10);
	tools2->addWidget(spacer5);
	tools2->addWidget(repPlay);
	tools2->addActions(transportAction->actions());
	tools2->addWidget(solo);
	QWidget* spacer2 = new QWidget();
	spacer2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	spacer2->setMaximumWidth(10);
	tools2->addWidget(spacer2);
	tools2->addAction(panicAction);
    QSizeGrip* corner = new QSizeGrip(mainw);
	QWidget* spacer3 = new QWidget();
	spacer3->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	spacer3->setMaximumWidth(5);
	tools2->addWidget(spacer3);
	tools2->addWidget(corner);


	//transport->setAllowedAreas(Qt::BottomToolBarArea);
	//transport->setFloatable(false);
	//transport->setMovable(false);
	//transport->setIconSize(QSize(22, 22));

	//addToolBarBreak();
	//toolbar = new Toolbar1(this, _rasterInit, _quantInit);
	//addToolBar(toolbar);

	//addToolBarBreak();
	info = new NoteInfo(this);
	addToolBar(Qt::TopToolBarArea, info);
	info->setAllowedAreas(Qt::TopToolBarArea);
	info->setFloatable(false);
	info->setMovable(false);
	info->hide();

	//---------------------------------------------------
	//    split
	//---------------------------------------------------

	splitter = new Splitter(Qt::Vertical, mainw, "splitter");
	splitter->setHandleWidth(2);

	hsplitter = new Splitter(Qt::Horizontal, mainw, "hsplitter");
	hsplitter->setChildrenCollapsible(true);
	hsplitter->setHandleWidth(2);

	QPushButton* ctrl = new QPushButton(tr("ctrl"), mainw);
	ctrl->setObjectName("Ctrl");
	ctrl->setFont(config.fonts[3]);
	ctrl->setToolTip(tr("Add Controller View"));
	hscroll = new ScrollScale(-25, -2, xscale, 20000, Qt::Horizontal, mainw);
	ctrl->setFixedSize(pianoWidth, hscroll->sizeHint().height());
	//ctrl->setFixedSize(pianoWidth / 2, hscroll->sizeHint().height());  // Tim.

	midiTrackInfo = new MidiTrackInfo(this, 0, _rasterInit, _quantInit);
	midiTrackInfo->setObjectName("prTrackInfo");
	int mtiw = 280; //midiTrackInfo->width(); // Save this.
	//midiTrackInfo->setMinimumWidth(100);
	midiTrackInfo->setMinimumSize(QSize(190,100));
	//midiTrackInfo->setMaximumWidth(300);
	// Catch left/right arrow key events for this widget so we
	// can easily move the focus back from this widget to the canvas.
	installEventFilter(this);
	midiTrackInfo->installEventFilter(this);
	midiTrackInfo->getView()->installEventFilter(this);

	connect(hsplitter, SIGNAL(splitterMoved(int, int)), midiTrackInfo, SLOT(updateSize()));
	connect(hsplitter, SIGNAL(splitterMoved(int, int)),  SLOT(splitterMoved(int, int)));

	//midiTrackInfo->setSizePolicy(QSizePolicy(/*QSizePolicy::Ignored*/QSizePolicy::Preferred, QSizePolicy::Expanding));
	/*infoScroll = new QScrollArea;
	infoScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	infoScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	//infoScroll->setMaximumWidth(300);
	infoScroll->setMinimumWidth(100);
	//infoScroll->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));
	infoScroll->setWidget(midiTrackInfo);
	infoScroll->setWidgetResizable(true);*/

	//hsplitter->addWidget(infoScroll);
	hsplitter->addWidget(midiTrackInfo);
	hsplitter->addWidget(splitter);

	mainGrid->setRowStretch(0, 100);
	mainGrid->setColumnStretch(1, 100);
	mainGrid->addWidget(hsplitter, 0, 1, 1, 3);

	QWidget* split1 = new QWidget(splitter);
	split1->setObjectName("split1");
	QGridLayout* gridS1 = new QGridLayout(split1);
	gridS1->setContentsMargins(0, 0, 0, 0);
	gridS1->setSpacing(0);
	//Defined and configure your program change bar here.
	//This may well be a copy of MTScale extended for our needs
	pcbar = new PCScale(&_raster, split1, this, xscale);
	pcbar->setAudio(audio);
	//pcbar->setEditor(this);
	time = new MTScale(&_raster, split1, xscale);
	/*Piano*/ piano = new Piano(split1, yscale);
	canvas = new PianoCanvas(this, split1, xscale, yscale);
	vscroll = new ScrollScale(-1, 7, yscale, KH * 75, Qt::Vertical, split1);

	int offset = -(config.division / 4);
	canvas->setOrigin(offset, 0);
	canvas->setCanvasTools(pianorollTools);
	canvas->setFocus();
	connect(canvas, SIGNAL(toolChanged(int)), tools2, SLOT(set(int)));
	time->setOrigin(offset, 0);
	pcbar->setOrigin(offset, 0);

	gridS1->setRowStretch(2, 100);
	gridS1->setColumnStretch(1, 100);

	gridS1->addWidget(pcbar, 0, 1, 1, 2);
	gridS1->addWidget(time, 1, 1, 1, 2);
	gridS1->addWidget(hLine(split1), 2, 0, 1, 3);
	gridS1->addWidget(piano, 3, 0);
	gridS1->addWidget(canvas, 3, 1);
	gridS1->addWidget(vscroll, 3, 2);

	ctrlLane = new Splitter(Qt::Vertical, splitter, "ctrllane");
	QWidget* split2 = new QWidget(splitter);
	split2->setMaximumHeight(hscroll->sizeHint().height());
	split2->setMinimumHeight(hscroll->sizeHint().height());
	QGridLayout* gridS2 = new QGridLayout(split2);
	gridS2->setContentsMargins(0, 0, 0, 0);
	gridS2->setSpacing(0);
	gridS2->setRowStretch(0, 100);
	gridS2->setColumnStretch(1, 100);
	gridS2->addWidget(ctrl, 0, 0);
	gridS2->addWidget(hscroll, 0, 1);

	//gridS2->addWidget(corner, 0, 2, Qt::AlignBottom | Qt::AlignRight);
	//splitter->setCollapsible(0, true);

	piano->setFixedWidth(pianoWidth);

	// Tim.
	QList<int> mops;
	mops.append(mtiw); // 30 for possible scrollbar
	mops.append(width() - mtiw);
	hsplitter->setSizes(mops);
	hsplitter->setStretchFactor(0, 0);
	hsplitter->setStretchFactor(1, 15);

	connect(tools2, SIGNAL(toolChanged(int)), canvas, SLOT(setTool(int)));

	//connect(midiTrackInfo, SIGNAL(outputPortChanged(int)), list, SLOT(redraw()));
	connect(ctrl, SIGNAL(clicked()), SLOT(addCtrl()));
	//connect(trackInfoButton, SIGNAL(clicked()), SLOT(toggleTrackInfo()));  Tim.
	connect(info, SIGNAL(valueChanged(NoteInfo::ValType, int)), SLOT(noteinfoChanged(NoteInfo::ValType, int)));
	connect(vscroll, SIGNAL(scrollChanged(int)), piano, SLOT(setYPos(int)));
	connect(vscroll, SIGNAL(scrollChanged(int)), canvas, SLOT(setYPos(int)));
	connect(vscroll, SIGNAL(scaleChanged(float)), canvas, SLOT(setYMag(float)));
	connect(vscroll, SIGNAL(scaleChanged(float)), piano, SLOT(setYMag(float)));

	connect(hscroll, SIGNAL(scrollChanged(int)), canvas, SLOT(setXPos(int)));
	connect(hscroll, SIGNAL(scrollChanged(int)), time, SLOT(setXPos(int)));
	connect(hscroll, SIGNAL(scrollChanged(int)), pcbar, SLOT(setXPos(int)));

	connect(hscroll, SIGNAL(scaleChanged(float)), canvas, SLOT(setXMag(float)));
	connect(hscroll, SIGNAL(scaleChanged(float)), time, SLOT(setXMag(float)));
	connect(hscroll, SIGNAL(scaleChanged(float)), pcbar, SLOT(setXMag(float)));

	connect(canvas, SIGNAL(newWidth(int)), SLOT(newCanvasWidth(int)));
	connect(canvas, SIGNAL(pitchChanged(int)), piano, SLOT(setPitch(int)));
	connect(canvas, SIGNAL(verticalScroll(unsigned)), vscroll, SLOT(setPos(unsigned)));
	connect(canvas, SIGNAL(horizontalScroll(unsigned)), hscroll, SLOT(setPos(unsigned)));
	connect(canvas, SIGNAL(horizontalScrollNoLimit(unsigned)), hscroll, SLOT(setPosNoLimit(unsigned)));
	connect(canvas, SIGNAL(selectionChanged(int, Event&, Part*)), this,
			SLOT(setSelection(int, Event&, Part*)));

	connect(piano, SIGNAL(keyPressed(int, int, bool)), canvas, SLOT(pianoPressed(int, int, bool)));
	connect(piano, SIGNAL(keyReleased(int, bool)), canvas, SLOT(pianoReleased(int, bool)));
	connect(srec, SIGNAL(toggled(bool)), SLOT(setSteprec(bool)));
	//connect(midiin, SIGNAL(toggled(bool)), canvas, SLOT(setMidiin(bool)));
	connect(speaker, SIGNAL(toggled(bool)), SLOT(setSpeaker(bool)));
	connect(canvas, SIGNAL(followEvent(int)), SLOT(follow(int)));

	connect(hscroll, SIGNAL(scaleChanged(float)), SLOT(updateHScrollRange()));
	piano->setYPos(KH * 30);
	canvas->setYPos(KH * 30);
	vscroll->setPos(KH * 30);
	//setSelection(0, 0, 0); //Really necessary? Causes segfault when only 1 item selected, replaced by the following:
	info->setEnabled(false);

	connect(song, SIGNAL(songChanged(int)), SLOT(songChanged1(int)));
	connect(song, SIGNAL(playbackStateChanged(bool)), SLOT(playStateChanged(bool)));

	setWindowTitle(canvas->getCaption());

	updateHScrollRange();
	// connect to toolbar
	connect(canvas, SIGNAL(pitchChanged(int)), pitchLabel, SLOT(setPitch(int)));
	connect(canvas, SIGNAL(timeChanged(unsigned)), SLOT(setTime(unsigned)));
	connect(piano, SIGNAL(pitchChanged(int)), pitchLabel, SLOT(setPitch(int)));
	connect(time, SIGNAL(timeChanged(unsigned)), SLOT(setTime(unsigned)));
	//connect(pcbar, SIGNAL(selectInstrument()), midiTrackInfo, SLOT(instrPopup()));
	connect(pcbar, SIGNAL(addProgramChange()), midiTrackInfo, SLOT(insertMatrixEvent()));
	connect(midiTrackInfo, SIGNAL(quantChanged(int)), SLOT(setQuant(int)));
	connect(midiTrackInfo, SIGNAL(rasterChanged(int)), SLOT(setRaster(int)));
	connect(midiTrackInfo, SIGNAL(toChanged(int)), SLOT(setTo(int)));
	connect(midiTrackInfo, SIGNAL(updateCurrentPatch(QString)), patchLabel, SLOT(setText(QString)));
	connect(solo, SIGNAL(toggled(bool)), SLOT(soloChanged(bool)));
	connect(repPlay, SIGNAL(toggled(bool)), SLOT(setReplay(bool)));

	setFocusPolicy(Qt::StrongFocus);
	setEventColorMode(colorMode);
	canvas->setMidiin(true);
	midiin->setChecked(true);
	canvas->playEvents(true);
	speaker->setChecked(true);

	QClipboard* cb = QApplication::clipboard();
	connect(cb, SIGNAL(dataChanged()), SLOT(clipboardChanged()));

	clipboardChanged(); // enable/disable "Paste"
	selectionChanged(); // enable/disable "Copy" & "Paste"
	initShortcuts(); // initialize shortcuts

	const Pos cpos = song->cPos();
	canvas->setPos(0, cpos.tick(), true);
//	canvas->selectAtTick(cpos.tick());
	//canvas->selectFirst();
	//
	if (canvas->track())
	{
		updateTrackInfo();
		solo->blockSignals(true);
		solo->setChecked(canvas->track()->solo());
		solo->blockSignals(false);
	}

	unsigned pos;
	if (initPos >= MAXINT)
		pos = song->cpos();
	else
		pos = initPos;
	if (pos > MAXINT)
		pos = MAXINT;

	// At this point in time the range of the canvas hasn't
	// been calculated right ?
	// Also, why wanting to restore some initPos, what is initPos?
	// To me, it seems to make a lot more sense to use the actual
	// current song cpos.
	// This is now done via the showEvent();

	//      hscroll->setOffset((int)pos); // changed that to:
}

//---------------------------------------------------------
//   songChanged1
//---------------------------------------------------------

void PianoRoll::songChanged1(int bits)
{

	if (bits & SC_SOLO)
	{
		solo->blockSignals(true);
		solo->setChecked(canvas->track()->solo());
		solo->blockSignals(false);
		return;
	}
	songChanged(bits);
	//trackInfo->songChanged(bits);
	// We'll receive SC_SELECTION if a different part is selected.
	if (bits & SC_SELECTION)
		updateTrackInfo();
}

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void PianoRoll::configChanged()
{
	initShortcuts();
	//trackInfo->updateTrackInfo();
}

void PianoRoll::setReplay(bool t)
{
	_replay = t;
	if(t)
	{
		replayPos = song->cpos();
	}
}

void PianoRoll::playStateChanged(bool state)
{
	if(_replay)
	{
		if(!state)
		{
			Pos p(replayPos, true);
			song->setPos(0, p, true, true, true);
		}
	}
}

//---------------------------------------------------------
//   updateHScrollRange
//---------------------------------------------------------

void PianoRoll::updateHScrollRange()
{
	int s, e;
	canvas->range(&s, &e);
	// Show one more measure.
	e += AL::sigmap.ticksMeasure(e);
	// Show another quarter measure due to imprecise drawing at canvas end point.
	e += AL::sigmap.ticksMeasure(e) / 4;
	// Compensate for the fixed piano and vscroll widths.
	e += canvas->rmapxDev(pianoWidth - vscroll->width());
	int s1, e1;
	hscroll->range(&s1, &e1);
	if (s != s1 || e != e1)
		hscroll->setRange(s, e);
}

void PianoRoll::updateTrackInfo()
{
	if(selected != curCanvasPart()->track())
	{
		selected = curCanvasPart()->track();
		if (selected->isMidiTrack())
		{
			midiTrackInfo->setTrack(selected);
			///midiTrackInfo->updateTrackInfo(-1);
		}
	}
}

//---------------------------------------------------------
//   follow
//---------------------------------------------------------

void PianoRoll::follow(int pos)
{
	int s, e;
	canvas->range(&s, &e);

	if (pos < e && pos >= s)
		hscroll->setOffset(pos);
	if (pos < s)
		hscroll->setOffset(s);
}

//---------------------------------------------------------
//   setTime
//---------------------------------------------------------

void PianoRoll::setTime(unsigned tick)
{
	if (tick != MAXINT)
		posLabel->setValue(tick);
	time->setPos(3, tick, false);
	pcbar->setPos(3, tick, false);
}

//---------------------------------------------------------
//   ~Pianoroll
//---------------------------------------------------------

PianoRoll::~PianoRoll()
{
	// undoRedo->removeFrom(tools);  // p4.0.6 Removed
        // store widget size to global config
        tconfig().set_property("PianoRollEdit", "widgetwidth", width());
        tconfig().set_property("PianoRollEdit", "widgetheigth", height());
        tconfig().set_property("PianoRoll", "hscale", hscroll->mag());
        tconfig().set_property("PianoRoll", "yscale", vscroll->mag());
        tconfig().set_property("PianoRoll", "ypos", vscroll->pos());
}

//---------------------------------------------------------
//   cmd
//    pulldown menu commands
//---------------------------------------------------------

void PianoRoll::cmd(int cmd)
{
	((PianoCanvas*) canvas)->cmd(cmd, _quantStrength, _quantLimit, _quantLen, _to);
}

//---------------------------------------------------------
//   setSelection
//    update Info Line
//---------------------------------------------------------

void PianoRoll::setSelection(int tick, Event& e, Part* p)
{
	int selections = canvas->selectionSize();

	selEvent = e;
	selPart = (MidiPart*) p;
	selTick = tick;

	if (selections > 1)
	{
		info->setEnabled(true);
		info->setDeltaMode(true);
		if (!deltaMode)
		{
			deltaMode = true;
			info->setValues(0, 0, 0, 0, 0);
			tickOffset = 0;
			lenOffset = 0;
			pitchOffset = 0;
			veloOnOffset = 0;
			veloOffOffset = 0;
		}
	}
	else if (selections == 1)
	{
		deltaMode = false;
		info->setEnabled(true);
		info->setDeltaMode(false);
		info->setValues(tick,
				selEvent.lenTick(),
				selEvent.pitch(),
				selEvent.velo(),
				selEvent.veloOff());
	}
	else
	{
		deltaMode = false;
		info->setEnabled(false);
	}
	selectionChanged();
}

//---------------------------------------------------------
//    edit currently selected Event
//---------------------------------------------------------

void PianoRoll::noteinfoChanged(NoteInfo::ValType type, int val)
{
	int selections = canvas->selectionSize();

	if (selections == 0)
	{
		printf("noteinfoChanged while nothing selected\n");
	}
	else if (selections == 1)
	{
		Event event = selEvent.clone();
		switch (type)
		{
			case NoteInfo::VAL_TIME:
				event.setTick(val - selPart->tick());
				break;
			case NoteInfo::VAL_LEN:
				event.setLenTick(val);
				break;
			case NoteInfo::VAL_VELON:
				event.setVelo(val);
				break;
			case NoteInfo::VAL_VELOFF:
				event.setVeloOff(val);
				break;
			case NoteInfo::VAL_PITCH:
				event.setPitch(val);
				break;
		}
		// Indicate do undo, and do not do port controller values and clone parts.
		//audio->msgChangeEvent(selEvent, event, selPart);
		audio->msgChangeEvent(selEvent, event, selPart, true, false, false);
	}
	else
	{
		// multiple events are selected; treat noteinfo values
		// as offsets to event values

		int delta = 0;
		switch (type)
		{
			case NoteInfo::VAL_TIME:
				delta = val - tickOffset;
				tickOffset = val;
				break;
			case NoteInfo::VAL_LEN:
				delta = val - lenOffset;
				lenOffset = val;
				break;
			case NoteInfo::VAL_VELON:
				delta = val - veloOnOffset;
				veloOnOffset = val;
				break;
			case NoteInfo::VAL_VELOFF:
				delta = val - veloOffOffset;
				veloOffOffset = val;
				break;
			case NoteInfo::VAL_PITCH:
				delta = val - pitchOffset;
				pitchOffset = val;
				break;
		}
		if (delta)
			canvas->modifySelected(type, delta);
	}
}

//---------------------------------------------------------
//   addCtrl
//---------------------------------------------------------

CtrlEdit* PianoRoll::addCtrl()
{
	///CtrlEdit* ctrlEdit = new CtrlEdit(splitter, this, xscale, false, "pianoCtrlEdit");
	CtrlEdit* ctrlEdit = new CtrlEdit(ctrlLane/*splitter*/, this, xscale, false, "pianoCtrlEdit"); // ccharrett
	connect(tools2, SIGNAL(toolChanged(int)), ctrlEdit, SLOT(setTool(int)));
	connect(hscroll, SIGNAL(scrollChanged(int)), ctrlEdit, SLOT(setXPos(int)));
	connect(hscroll, SIGNAL(scaleChanged(float)), ctrlEdit, SLOT(setXMag(float)));
	connect(ctrlEdit, SIGNAL(timeChanged(unsigned)), SLOT(setTime(unsigned)));
	connect(ctrlEdit, SIGNAL(destroyedCtrl(CtrlEdit*)), SLOT(removeCtrl(CtrlEdit*)));
	connect(ctrlEdit, SIGNAL(yposChanged(int)), pitchLabel, SLOT(setInt(int)));

	ctrlEdit->setTool(tools2->curTool());
	ctrlEdit->setXPos(hscroll->pos());
	ctrlEdit->setXMag(hscroll->getScaleValue());

	ctrlEdit->show();
	ctrlEditList.push_back(ctrlEdit);
	return ctrlEdit;
}

//---------------------------------------------------------
//   removeCtrl
//---------------------------------------------------------

void PianoRoll::removeCtrl(CtrlEdit* ctrl)
{
	for (std::list<CtrlEdit*>::iterator i = ctrlEditList.begin();
			i != ctrlEditList.end(); ++i)
	{
		if (*i == ctrl)
		{
			ctrlEditList.erase(i);
			break;
		}
	}
}

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void PianoRoll::closeEvent(QCloseEvent* e)
{
	emit deleted((unsigned long) this);
	e->accept();
}

//---------------------------------------------------------
//   readConfiguration
//---------------------------------------------------------

void PianoRoll::readConfiguration(Xml& xml)
{
	for (;;)
	{
		Xml::Token token = xml.parse();
		if (token == Xml::Error || token == Xml::End)
			break;
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::TagStart:
				if (tag == "quant")
					_quantInit = xml.parseInt();
				else if (tag == "raster")
					_rasterInit = xml.parseInt();
				else if (tag == "quantStrength")
					_quantStrengthInit = xml.parseInt();
				else if (tag == "quantLimit")
					_quantLimitInit = xml.parseInt();
				else if (tag == "quantLen")
					_quantLenInit = xml.parseInt();
				else if (tag == "to")
					_toInit = xml.parseInt();
				else if (tag == "colormode")
					colorModeInit = xml.parseInt();
				else if (tag == "width")
					_widthInit = xml.parseInt();
				else if (tag == "height")
					_heightInit = xml.parseInt();
				else
					xml.unknown("PianoRoll");
				break;
			case Xml::TagEnd:
				if (tag == "pianoroll")
					return;
			default:
				break;
		}
	}
}

//---------------------------------------------------------
//   writeConfiguration
//---------------------------------------------------------

void PianoRoll::writeConfiguration(int level, Xml& xml)
{
	xml.tag(level++, "pianoroll");
	xml.intTag(level, "quant", _quantInit);
	xml.intTag(level, "raster", _rasterInit);
	xml.intTag(level, "quantStrength", _quantStrengthInit);
	xml.intTag(level, "quantLimit", _quantLimitInit);
	xml.intTag(level, "quantLen", _quantLenInit);
	xml.intTag(level, "to", _toInit);
	xml.intTag(level, "width", _widthInit);
	xml.intTag(level, "height", _heightInit);
	xml.intTag(level, "colormode", colorModeInit);
	xml.etag(level, "pianoroll");
}

//---------------------------------------------------------
//   soloChanged
//    signal from solo button
//---------------------------------------------------------

void PianoRoll::soloChanged(bool flag)
{
	audio->msgSetSolo(canvas->track(), flag);
	song->update(SC_SOLO);
}

//---------------------------------------------------------
//   setRaster
//---------------------------------------------------------

void PianoRoll::setRaster(int val)
{
	_rasterInit = val;
	MidiEditor::setRaster(val);
	canvas->redrawGrid();
	canvas->setFocus(); // give back focus after kb input
}

//---------------------------------------------------------
//   setQuant
//---------------------------------------------------------

void PianoRoll::setQuant(int val)
{
	_quantInit = val;
	MidiEditor::setQuant(val);
	canvas->setFocus();
}

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void PianoRoll::writeStatus(int level, Xml& xml) const
{
	writePartList(level, xml);
	xml.tag(level++, "pianoroll");
	MidiEditor::writeStatus(level, xml);
	splitter->writeStatus(level, xml);
	hsplitter->writeStatus(level, xml);

	for (std::list<CtrlEdit*>::const_iterator i = ctrlEditList.begin();
			i != ctrlEditList.end(); ++i)
	{
		(*i)->writeStatus(level, xml);
	}

	xml.intTag(level, "steprec", canvas->steprec());
	xml.intTag(level, "midiin", canvas->midiin());
	xml.intTag(level, "tool", int(canvas->tool()));
	xml.intTag(level, "quantStrength", _quantStrength);
	xml.intTag(level, "quantLimit", _quantLimit);
	xml.intTag(level, "quantLen", _quantLen);
	xml.intTag(level, "playEvents", _playEvents);
	xml.intTag(level, "xpos", hscroll->pos());
	xml.intTag(level, "xmag", hscroll->mag());
	xml.intTag(level, "ypos", vscroll->pos());
	xml.intTag(level, "ymag", vscroll->mag());
	xml.tag(level, "/pianoroll");
}

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void PianoRoll::readStatus(Xml& xml)
{
        printf("readstatus\n");
	for (;;)
	{
		Xml::Token token = xml.parse();
		if (token == Xml::Error || token == Xml::End)
			break;
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::TagStart:
				if (tag == "steprec")
				{
					int val = xml.parseInt();
					canvas->setSteprec(val);
					srec->setChecked(val);
				}
				else if (tag == "midiin")
				{
					int val = xml.parseInt();
					canvas->setMidiin(val);
					midiin->setChecked(val);
				}
				else if (tag == "tool")
				{
					int tool = xml.parseInt();
					canvas->setTool(tool);
					tools2->set(tool);
				}
				else if (tag == "midieditor")
					MidiEditor::readStatus(xml);
				else if (tag == "ctrledit")
				{
					CtrlEdit* ctrl = addCtrl();
					ctrl->readStatus(xml);
				}
				else if (tag == splitter->objectName())
					splitter->readStatus(xml);
				else if (tag == hsplitter->objectName())
					hsplitter->readStatus(xml);
				else if (tag == "quantStrength")
					_quantStrength = xml.parseInt();
				else if (tag == "quantLimit")
					_quantLimit = xml.parseInt();
				else if (tag == "quantLen")
					_quantLen = xml.parseInt();
				else if (tag == "playEvents")
				{
					_playEvents = xml.parseInt();
					canvas->playEvents(_playEvents);
					speaker->setChecked(_playEvents);
				}
                                else if (tag == "xmag")
                                        hscroll->setMag(xml.parseInt());
                                else if (tag == "xpos")
                                        hscroll->setPos(xml.parseInt());
                                else if (tag == "ymag")
                                        vscroll->setMag(xml.parseInt());
                                else if (tag == "ypos")
                                        vscroll->setPos(xml.parseInt());
				else
					xml.unknown("PianoRoll");
				break;
			case Xml::TagEnd:
				if (tag == "pianoroll")
				{
					_quantInit = _quant;
					_rasterInit = _raster;
					midiTrackInfo->setRaster(_raster);
					midiTrackInfo->setQuant(_quant);
					canvas->redrawGrid();
					return;
				}
			default:
				break;
		}
	}
}

static int rasterTable[] = {
	//-9----8-  7    6     5     4    3(1/4)     2   1
	4, 8, 16, 32, 64, 128, 256, 512, 1024, // triple
	6, 12, 24, 48, 96, 192, 384, 768, 1536,
	9, 18, 36, 72, 144, 288, 576, 1152, 2304 // dot
};


bool PianoRoll::eventFilter(QObject *obj, QEvent *event)
{
	// Force left/right arrow key events to move the focus
	// back on the canvas if it doesn't have the focus.
	// Currently the object that we're filtering is the
	// midiTrackInfo.
	if (event->type() == QEvent::KeyPress) {
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
		int key = keyEvent->key();
		if (key == Qt::Key_Enter || key == Qt::Key_Return)
		{
			canvas->setFocus(Qt::MouseFocusReason);
			return true;
		}
		if (keyEvent->key() == shortcuts[SHRT_TOGGLE_STEPRECORD].key ||
		    keyEvent->key() == shortcuts[SHRT_MIDI_PANIC].key
		    )
		{
			qApp->sendEvent(canvas, event);
			return true;
		}
		if(keyEvent->key() == shortcuts[SHRT_SEL_INSTRUMENT].key)
		{
			midiTrackInfo->addSelectedPatch();
			return true;
		}
	}

	// standard event processing
	return QObject::eventFilter(obj, event);
}

//---------------------------------------------------------
//   viewKeyPressEvent
//---------------------------------------------------------

void PianoRoll::keyPressEvent(QKeyEvent* event)
{

        // Force left/right arrow key events to move the focus
        // back on the canvas if it doesn't have the focus.
        if (!canvas->hasFocus())
	{
                if (event->key() == Qt::Key_Right || event->key() == Qt::Key_Left)
                {
                        canvas->setFocus(Qt::MouseFocusReason);
                        event->accept();
                        return;
                }
	}

	int index;
	int n = sizeof (rasterTable) / sizeof (*rasterTable);
	for (index = 0; index < n; ++index)
		if (rasterTable[index] == raster())
			break;
	if (index == n)
	{
		index = 0;
		// raster 1 is not in table
	}
	int off = (index / 9) * 9;
	index = index % 9;

	int val = 0;

	int key = event->key();

	//if (event->state() & Qt::ShiftButton)
	if (((QInputEvent*) event)->modifiers() & Qt::ShiftModifier)
		key += Qt::SHIFT;
	//if (event->state() & Qt::AltButton)
	if (((QInputEvent*) event)->modifiers() & Qt::AltModifier)
		key += Qt::ALT;
	//if (event->state() & Qt::ControlButton)
	if (((QInputEvent*) event)->modifiers() & Qt::ControlModifier)
		key += Qt::CTRL;
        ///if (event->state() & Qt::MetaButton)
         if (((QInputEvent*) event)->modifiers() & Qt::MetaModifier)
                key += Qt::META;



	 PianoCanvas* pc = (PianoCanvas*) canvas;

	 if (_stepQwerty && pc->steprec())
	 {
		 if (key == shortcuts[SHRT_OCTAVE_QWERTY_0].key) {
			 pc->setOctaveQwerty(0);
			 return;
		 } else if (key == shortcuts[SHRT_OCTAVE_QWERTY_1].key) {
			 pc->setOctaveQwerty(1);
			 return;
		 } else if (key == shortcuts[SHRT_OCTAVE_QWERTY_2].key) {
			 pc->setOctaveQwerty(2);
			 return;
		 } else if (key == shortcuts[SHRT_OCTAVE_QWERTY_3].key) {
			 pc->setOctaveQwerty(3);
			 return;
		 } else if (key == shortcuts[SHRT_OCTAVE_QWERTY_4].key) {
			 pc->setOctaveQwerty(4);
			 return;
		 } else if (key == shortcuts[SHRT_OCTAVE_QWERTY_5].key) {
			 pc->setOctaveQwerty(5);
			 return;
		 } else if (key == shortcuts[SHRT_OCTAVE_QWERTY_6].key) {
			 pc->setOctaveQwerty(6);
			 return;
		 }

		 if(pc->stepInputQwerty(event))
		 {
			 return;
		 }
	 }


	if (key == Qt::Key_Escape)
	{
		close();
		return;
	}
	else if (key == shortcuts[SHRT_TOOL_POINTER].key)
	{
		tools2->set(PointerTool);
		return;
	}
	else if (key == shortcuts[SHRT_TOOL_PENCIL].key)
	{
		tools2->set(PencilTool);
		return;
	}
	else if (key == shortcuts[SHRT_TOOL_RUBBER].key)
	{
		tools2->set(RubberTool);
		return;
	}
	else if (key == shortcuts[SHRT_TOOL_LINEDRAW].key)
	{
		tools2->set(DrawTool);
		return;
	}
	else if (key == shortcuts[SHRT_POS_INC].key)
	{
		pc->pianoCmd(CMD_RIGHT);
		return;
	}
	else if (key == shortcuts[SHRT_POS_DEC].key)
	{
		pc->pianoCmd(CMD_LEFT);
		return;
	}
	else if (key == shortcuts[SHRT_POS_INC_NOSNAP].key)
	{
		pc->pianoCmd(CMD_RIGHT_NOSNAP);
		return;
	}
	else if (key == shortcuts[SHRT_POS_DEC_NOSNAP].key)
	{
		pc->pianoCmd(CMD_LEFT_NOSNAP);
		return;
	}
	else if (key == shortcuts[SHRT_INSERT_AT_LOCATION].key)
	{
		pc->pianoCmd(CMD_INSERT);
		return;
	}
	else if (key == Qt::Key_Delete)
	{
		pc->pianoCmd(CMD_DELETE);
		return;
	}
	else if (key == shortcuts[SHRT_ZOOM_IN].key)
	{
		int mag = hscroll->mag();
		int zoomlvl = ScrollScale::getQuickZoomLevel(mag);
		if (zoomlvl < 23)
			zoomlvl++;

		int newmag = ScrollScale::convertQuickZoomLevelToMag(zoomlvl);
		hscroll->setMag(newmag);
		//printf("mag = %d zoomlvl = %d newmag = %d\n", mag, zoomlvl, newmag);
		return;
	}
	else if (key == shortcuts[SHRT_ZOOM_OUT].key)
	{
		int mag = hscroll->mag();
		int zoomlvl = ScrollScale::getQuickZoomLevel(mag);
		if (zoomlvl > 1)
			zoomlvl--;

		int newmag = ScrollScale::convertQuickZoomLevelToMag(zoomlvl);
		hscroll->setMag(newmag);
		//printf("mag = %d zoomlvl = %d newmag = %d\n", mag, zoomlvl, newmag);
		return;
	}
	else if (key == shortcuts[SHRT_VZOOM_IN].key)
	{
		int mag = vscroll->mag();
		int zoomlvl = ScrollScale::getQuickZoomLevel(mag);
		if (zoomlvl < 23)
			zoomlvl++;

		int newmag = ScrollScale::convertQuickZoomLevelToMag(zoomlvl);
		vscroll->setMag(newmag);
		//printf("mag = %d zoomlvl = %d newmag = %d\n", mag, zoomlvl, newmag);
		return;
	}
	else if (key == shortcuts[SHRT_VZOOM_OUT].key)
	{
		int mag = vscroll->mag();
		int zoomlvl = ScrollScale::getQuickZoomLevel(mag);
		if (zoomlvl > 1)
			zoomlvl--;

		int newmag = ScrollScale::convertQuickZoomLevelToMag(zoomlvl);
		vscroll->setMag(newmag);
		//printf("mag = %d zoomlvl = %d newmag = %d\n", mag, zoomlvl, newmag);
		return;
	}
	else if (key == shortcuts[SHRT_GOTO_CPOS].key)
	{
		PartList* p = this->parts();
		Part* first = p->begin()->second;
		hscroll->setPos(song->cpos() - first->tick());
		return;
	}
	else if (key == shortcuts[SHRT_SCROLL_LEFT].key)
	{
		int pos = hscroll->pos() - config.division;
		if (pos < 0)
			pos = 0;
		hscroll->setPos(pos);
		return;
	}
	else if (key == shortcuts[SHRT_SCROLL_RIGHT].key)
	{
		int pos = hscroll->pos() + config.division;
		hscroll->setPos(pos);
		return;
	}
	else if (key == shortcuts[SHRT_SCROLL_UP].key)
	{
		int pos = vscroll->pos() - (config.division/2);
		if (pos < 0)
			pos = 0;
		vscroll->setPos(pos);
		return;
	}
	else if (key == shortcuts[SHRT_SCROLL_DOWN].key)
	{
		int pos = vscroll->pos() + (config.division/2);
		vscroll->setPos(pos);
		return;
	}
	else if(key == shortcuts[SHRT_PLAY_REPEAT].key)
	{
		repPlay->toggle();
		return;
	}
	else if (key == shortcuts[SHRT_SEL_INSTRUMENT].key)
	{
		midiTrackInfo->addSelectedPatch();
		return;
	}
	else if (key == shortcuts[SHRT_ADD_PROGRAM].key)
	{
		midiTrackInfo->insertMatrixEvent(); //progRecClicked();
		return;
	}
	else if (key == shortcuts[SHRT_DEL_PROGRAM].key)
	{
		//printf("Delete KeyStroke recieved\n");
		int x = song->cpos();
		Track* track = song->findTrack(curCanvasPart()); /*{{{*/
		PartList* parts = track->parts();
		for (iPart p = parts->begin(); p != parts->end(); ++p)
		{
			Part* mprt = p->second;
			EventList* eventList = mprt->events(); //m->second.events();
			if(eventList && !eventList->empty())
			{
				for (iEvent evt = eventList->begin(); evt != eventList->end(); ++evt)
				{
					//Get event type.
					Event pcevt = evt->second;
					//printf("Found events %d \n", pcevt.type());
					if (!pcevt.isNote())
					{
						//printf("Found none Note events of type: %d with dataA: %d\n", pcevt.type(), pcevt.dataA());
						if (pcevt.type() == Controller && pcevt.dataA() == CTRL_PROGRAM)
						{
							//printf("Found Program Change event type\n");
							//printf("Pos x: %d\n", x);
							int xp = pcevt.tick() + mprt->tick();
							//printf("Event x: %d\n", xp);
							if (xp >= x && xp <= (x + 50))
							{
								//printf("Found Program Change to delete at: %d\n", x);
								//song->startUndo();
								song->deleteEvent(pcevt, mprt); //hack
								//audio->msgDeleteEvent(evt->second, p->second, false, true, true);
								//song->endUndo(SC_EVENT_MODIFIED);
							}
						}
					}
				}
			}
		}/*}}}*/
		//pcbar->deleteProgram();
		pcbar->update();
		return;
	}
	else if (key == shortcuts[SHRT_SET_QUANT_1].key)
		val = rasterTable[8 + off];
	else if (key == shortcuts[SHRT_SET_QUANT_2].key)
		val = rasterTable[7 + off];
	else if (key == shortcuts[SHRT_SET_QUANT_3].key)
		val = rasterTable[6 + off];
	else if (key == shortcuts[SHRT_SET_QUANT_4].key)
		val = rasterTable[5 + off];
	else if (key == shortcuts[SHRT_SET_QUANT_5].key)
		val = rasterTable[4 + off];
	else if (key == shortcuts[SHRT_SET_QUANT_6].key)
		val = rasterTable[3 + off];
	else if (key == shortcuts[SHRT_SET_QUANT_7].key)
		val = rasterTable[2 + off];
	else if (key == shortcuts[SHRT_TOGGLE_TRIOL].key)
		val = rasterTable[index + ((off == 0) ? 9 : 0)];
	else if (key == shortcuts[SHRT_EVENT_COLOR].key)
	{
		if (colorMode == 0)
			colorMode = 1;
		else if (colorMode == 1)
			colorMode = 2;
		else
			colorMode = 0;
		setEventColorMode(colorMode);
		return;
	}
	else if (key == shortcuts[SHRT_TOGGLE_PUNCT].key)
		val = rasterTable[index + ((off == 18) ? 9 : 18)];

	else if (key == shortcuts[SHRT_TOGGLE_PUNCT2].key)
	{//CDW
		if ((off == 18) && (index > 2))
		{
			val = rasterTable[index + 9 - 1];
		}
		else if ((off == 9) && (index < 8))
		{
			val = rasterTable[index + 18 + 1];
		}
		else
			return;
	}
        else if (key == shortcuts[SHRT_TOGGLE_STEPRECORD].key)
        {
                srec->toggle();
                return;
        }
	else if (key == shortcuts[SHRT_TOGGLE_STEPQWERTY].key)
	{
		_stepQwerty = !_stepQwerty;
		if (_stepQwerty && !srec->isChecked()) {
			srec->toggle();
		}
		if (!_stepQwerty && srec->isChecked()) {
			srec->toggle();
		}

		return;
	}
        else if (key == shortcuts[SHRT_NOTE_VELOCITY_UP].key)
        {
                CItemList list = canvas->getSelectedItemsForCurrentPart();

                song->startUndo();
                for (iCItem k = list.begin(); k != list.end(); ++k)
                {
                        NEvent* nevent = (NEvent*) (k->second);
                        Event event = nevent->event();
                        if (event.type() != Note)
                                continue;

                        int velo = event.velo();
			velo += 5;

                        if (velo <= 0)
                                velo = 1;
                        if (velo > 127)
                                velo = 127;
                        if (event.velo() != velo)
                        {
                                Event newEvent = event.clone();
                                newEvent.setVelo(velo);
                                // Indicate no undo, and do not do port controller values and clone parts.
                                //audio->msgChangeEvent(event, newEvent, nevent->part(), false);
                                audio->msgChangeEvent(event, newEvent, nevent->part(), false, false, false);
                        }
                }
                song->endUndo(SC_EVENT_MODIFIED);
                return;

        }
        else if (key == shortcuts[SHRT_NOTE_VELOCITY_DOWN].key)
        {
                CItemList list = canvas->getSelectedItemsForCurrentPart();

                song->startUndo();
                for (iCItem k = list.begin(); k != list.end(); ++k)
                {
                        NEvent* nevent = (NEvent*) (k->second);
                        Event event = nevent->event();
                        if (event.type() != Note)
                                continue;

                        int velo = event.velo();
			velo -= 5;

                        if (velo <= 0)
                                velo = 1;
                        if (velo > 127)
                                velo = 127;
                        if (event.velo() != velo)
                        {
                                Event newEvent = event.clone();
                                newEvent.setVelo(velo);
                                // Indicate no undo, and do not do port controller values and clone parts.
                                //audio->msgChangeEvent(event, newEvent, nevent->part(), false);
                                audio->msgChangeEvent(event, newEvent, nevent->part(), false, false, false);
                        }
                }
                song->endUndo(SC_EVENT_MODIFIED);
                return;
        }	
	else if (key == shortcuts[SHRT_TRACK_TOGGLE_SOLO].key)
	{
		if (canvas->part()) {
			Track* t = canvas->part()->track();
			audio->msgSetSolo(t, !t->solo());
			song->update(SC_SOLO);
		}
	}
	else
	{ //Default:
		event->ignore();
		return;
	}
	setQuant(val);
	setRaster(val);
	midiTrackInfo->setQuant(_quant);
	midiTrackInfo->setRaster(_raster);
}

//---------------------------------------------------------
//   configQuant
//---------------------------------------------------------

void PianoRoll::configQuant()
{
	if (!quantConfig)
	{
		quantConfig = new QuantConfig(_quantStrength, _quantLimit, _quantLen);
		connect(quantConfig, SIGNAL(setQuantStrength(int)), SLOT(setQuantStrength(int)));
		connect(quantConfig, SIGNAL(setQuantLimit(int)), SLOT(setQuantLimit(int)));
		connect(quantConfig, SIGNAL(setQuantLen(bool)), SLOT(setQuantLen(bool)));
	}
	quantConfig->show();
}

//---------------------------------------------------------
//   setSteprec
//---------------------------------------------------------

void PianoRoll::setSteprec(bool flag)
{
	canvas->setSteprec(flag);
	//if (flag == false)
	//      midiin->setChecked(flag);
}

//---------------------------------------------------------
//   eventColorModeChanged
//---------------------------------------------------------

void PianoRoll::eventColorModeChanged(int mode)
{
	colorMode = mode;
	colorModeInit = colorMode;

	((PianoCanvas*) (canvas))->setColorMode(colorMode);
}

//---------------------------------------------------------
//   setEventColorMode
//---------------------------------------------------------

void PianoRoll::setEventColorMode(int mode)
{
	colorMode = mode;
	colorModeInit = colorMode;

	///eventColor->setItemChecked(0, mode == 0);
	///eventColor->setItemChecked(1, mode == 1);
	///eventColor->setItemChecked(2, mode == 2);
	evColorBlueAction->setChecked(mode == 0);
	evColorPitchAction->setChecked(mode == 1);
	evColorVelAction->setChecked(mode == 2);

	((PianoCanvas*) (canvas))->setColorMode(colorMode);
}

//---------------------------------------------------------
//   clipboardChanged
//---------------------------------------------------------

void PianoRoll::clipboardChanged()
{
	editPasteAction->setEnabled(QApplication::clipboard()->mimeData()->hasFormat(QString("text/x-oom-eventlist")));
}

//---------------------------------------------------------
//   selectionChanged
//---------------------------------------------------------

void PianoRoll::selectionChanged()
{
	bool flag = canvas->selectionSize() > 0;
	editCutAction->setEnabled(flag);
	editCopyAction->setEnabled(flag);
	editDelEventsAction->setEnabled(flag);
}

//---------------------------------------------------------
//   setSpeaker
//---------------------------------------------------------

void PianoRoll::setSpeaker(bool val)
{
	_playEvents = val;
	canvas->playEvents(_playEvents);
}

#ifdef LSCP_SUPPORT
//---------------------------------------------------------
//   setKeyBindings
//---------------------------------------------------------

void PianoRoll::setKeyBindings(LSCPChannelInfo info)
{
	printf("entering PianoRoll::setKeyBindings");
	if(!selected || audio->isPlaying())
		return;
	printf("not playing and selected");	
	//check if the lscp information is pertaining to this track and port
	Track *t = curCanvasPart()->track();
	RouteList *rl = t->inRoutes();
	for(iRoute ir = rl->begin(); ir != rl->end(); ++ir)
	{
		if((*ir).name() == QString(info.midi_portname))
		{
			printf("port names match");
			if(isCurrentPatch(info.hbank, info.lbank, info.program))
			{
				printf("is current patch calling setMIDIKeyBindings");
				piano->setMIDIKeyBindings(info.key_bindings, info.keyswitch_bindings);
			}	
			break;
		}
	}
}
#endif

bool PianoRoll::isCurrentPatch(int hbank, int lbank, int prog)/*{{{*/
{
	if(!selected)
		return false;
	MidiTrack *tr = (MidiTrack*)selected;
	int outChannel = tr->outChannel();
	int outPort = tr->outPort();
	MidiPort* mp = &midiPorts[outPort];
	int program = mp->hwCtrlState(outChannel, CTRL_PROGRAM);
	if (program == CTRL_VAL_UNKNOWN)
	{
		program = mp->lastValidHWCtrlState(outChannel, CTRL_PROGRAM);
	}
	
	if (program == CTRL_VAL_UNKNOWN)
	{
		return false;
	}
	int hb = ((program >> 16) & 0xff) + 1;
	if (hb == 0x100)
		hb = 0;
	int lb = ((program >> 8) & 0xff) + 1;
	if (lb == 0x100)
		lb = 0;
	int pr = (program & 0xff) + 1;
	if (pr == 0x100)
		pr = 0;

	return (hb == hbank && lb == lbank && pr == prog);

}/*}}}*/

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void PianoRoll::resizeEvent(QResizeEvent* ev)
{
	QWidget::resizeEvent(ev);
	_widthInit = ev->size().width();
	_heightInit = ev->size().height();
}

//---------------------------------------------------------
//   showEvent
//   Now that every gui element is created, including
//   the scroll bars, what about updating the scrollbars
//   so that the play cursor is in the center of the viewport?
//---------------------------------------------------------

void PianoRoll::showEvent(QShowEvent *)
{
	// maybe add a bool flag to follow: centered ?
	// couldn't find a function that does that directly.
	follow(song->cpos());
	// now that the cursor is in the view, move the view
	// half the canvas width so the cursor is centered.
	hscroll->setPos(hscroll->pos() - (canvas->width() / 2));

        int hScale = tconfig().get_property("PianoRoll", "hscale", 346).toInt();
        int vScale = tconfig().get_property("PianoRoll", "yscale", 286).toInt();
        int yPos = tconfig().get_property("PianoRoll", "ypos", 0).toInt();
        hscroll->setMag(hScale);
        vscroll->setMag(vScale);
        vscroll->setPos(yPos);
}

/*
//---------------------------------------------------------
//   trackInfoScroll
//---------------------------------------------------------

void PianoRoll::trackInfoScroll(int y)
	  {
	  if (trackInfo->visibleWidget())
			trackInfo->visibleWidget()->move(0, -y);
	  }
 */

//---------------------------------------------------------
//   initShortcuts
//---------------------------------------------------------

void PianoRoll::initShortcuts()
{
	editCutAction->setShortcut(shortcuts[SHRT_CUT].key);
	editCopyAction->setShortcut(shortcuts[SHRT_COPY].key);
	editPasteAction->setShortcut(shortcuts[SHRT_PASTE].key);
	editDelEventsAction->setShortcut(shortcuts[SHRT_DELETE].key);

	selectAllAction->setShortcut(shortcuts[SHRT_SELECT_ALL].key);
	selectNoneAction->setShortcut(shortcuts[SHRT_SELECT_NONE].key);
	selectInvertAction->setShortcut(shortcuts[SHRT_SELECT_INVERT].key);
	selectInsideLoopAction->setShortcut(shortcuts[SHRT_SELECT_ILOOP].key);
	selectOutsideLoopAction->setShortcut(shortcuts[SHRT_SELECT_OLOOP].key);
	selectPrevPartAction->setShortcut(shortcuts[SHRT_SELECT_PREV_PART].key);
	selectNextPartAction->setShortcut(shortcuts[SHRT_SELECT_NEXT_PART].key);

	eventColor->menuAction()->setShortcut(shortcuts[SHRT_EVENT_COLOR].key);
	//evColorBlueAction->setShortcut(shortcuts[  ].key);
	//evColorPitchAction->setShortcut(shortcuts[  ].key);
	//evColorVelAction->setShortcut(shortcuts[  ].key);

	funcOverQuantAction->setShortcut(shortcuts[SHRT_OVER_QUANTIZE].key);
	funcNoteOnQuantAction->setShortcut(shortcuts[SHRT_ON_QUANTIZE].key);
	funcNoteOnOffQuantAction->setShortcut(shortcuts[SHRT_ONOFF_QUANTIZE].key);
	funcIterQuantAction->setShortcut(shortcuts[SHRT_ITERATIVE_QUANTIZE].key);

	funcConfigQuantAction->setShortcut(shortcuts[SHRT_CONFIG_QUANT].key);

	funcGateTimeAction->setShortcut(shortcuts[SHRT_MODIFY_GATE_TIME].key);
	funcModVelAction->setShortcut(shortcuts[SHRT_MODIFY_VELOCITY].key);
	funcCrescendoAction->setShortcut(shortcuts[SHRT_CRESCENDO].key);
	funcTransposeAction->setShortcut(shortcuts[SHRT_TRANSPOSE].key);
	funcThinOutAction->setShortcut(shortcuts[SHRT_THIN_OUT].key);
	funcEraseEventAction->setShortcut(shortcuts[SHRT_ERASE_EVENT].key);
	funcNoteShiftAction->setShortcut(shortcuts[SHRT_NOTE_SHIFT].key);
	funcMoveClockAction->setShortcut(shortcuts[SHRT_MOVE_CLOCK].key);
	funcCopyMeasureAction->setShortcut(shortcuts[SHRT_COPY_MEASURE].key);
	funcEraseMeasureAction->setShortcut(shortcuts[SHRT_ERASE_MEASURE].key);
	funcDelMeasureAction->setShortcut(shortcuts[SHRT_DELETE_MEASURE].key);
	funcCreateMeasureAction->setShortcut(shortcuts[SHRT_CREATE_MEASURE].key);
	funcSetFixedLenAction->setShortcut(shortcuts[SHRT_FIXED_LEN].key);
	funcDelOverlapsAction->setShortcut(shortcuts[SHRT_DELETE_OVERLAPS].key);

}

//---------------------------------------------------------
//   execDeliveredScript
//---------------------------------------------------------

void PianoRoll::execDeliveredScript(int id)
{
	//QString scriptfile = QString(INSTPREFIX) + SCRIPTSSUFFIX + deliveredScriptNames[id];
	QString scriptfile = song->getScriptPath(id, true);
	song->executeScript(scriptfile.toAscii().data(), parts(), quant(), true);
}

//---------------------------------------------------------
//   execUserScript
//---------------------------------------------------------

void PianoRoll::execUserScript(int id)
{
	QString scriptfile = song->getScriptPath(id, false);
	song->executeScript(scriptfile.toAscii().data(), parts(), quant(), true);
}

//---------------------------------------------------------
//   newCanvasWidth
//---------------------------------------------------------

void PianoRoll::newCanvasWidth(int /*w*/)
{
	/*
		  int nw = w + (vscroll->width() - 18); // 18 is the fixed width of the CtlEdit VScale widget.
		  if(nw < 1)
			nw = 1;
        
		  for (std::list<CtrlEdit*>::iterator i = ctrlEditList.begin();
			 i != ctrlEditList.end(); ++i) {
				// Changed by Tim. p3.3.7
				//(*i)->setCanvasWidth(w);
				(*i)->setCanvasWidth(nw);
				}
            
		  updateHScrollRange();
	 */
}

//---------------------------------------------------------
//   toggleTrackInfo
//---------------------------------------------------------

void PianoRoll::toggleTrackInfo()
{
	bool vis = midiTrackInfo->isVisible();
	infoScroll->setVisible(!vis);
	infoScroll->setEnabled(!vis);
}

void PianoRoll::splitterMoved(int pos, int)
{
	if(pos < midiTrackInfo->minimumSize().width())
	{
		QList<int> def;
		def.append(midiTrackInfo->minimumSize().width());
		def.append(50);
		hsplitter->setSizes(def);
	}
}
