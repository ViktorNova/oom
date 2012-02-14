//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include "CreateTrackDialog.h"
#include "globals.h"
#include "gconfig.h"
#include "track.h"
#include "song.h"
#include "app.h"
#include "mididev.h"
#include "midiport.h"
#include "minstrument.h"
#include "audio.h"
#include "midiseq.h"
#include "driver/jackaudio.h"
#include "driver/jackmidi.h"
#include "driver/alsamidi.h"
#include "network/lsclient.h"
#include "icons.h"
#include "midimonitor.h"
#include "plugin.h"
#include "utils.h"
#include "genset.h"
#include "knob.h"
#include "doublelabel.h"


CreateTrackDialog::CreateTrackDialog(int type, int pos, QWidget* parent)
: QDialog(parent),
m_insertType(type),
m_insertPosition(pos),
m_templateMode(false)
{
	initDefaults();
	m_vtrack = new VirtualTrack;
    //m_lastSynth = 0;
}

CreateTrackDialog::CreateTrackDialog(VirtualTrack** vt, int type, int pos, QWidget* parent)
: QDialog(parent),
m_insertType(type),
m_insertPosition(pos),
m_templateMode(true)
{
	initDefaults();
	m_vtrack = new VirtualTrack;
    //m_lastSynth = 0;
	*vt = m_vtrack;
}

void CreateTrackDialog::initDefaults()
{
	setupUi(this);

	m_createMidiInputDevice = false;
	m_createMidiOutputDevice = false;
	
	m_midiSameIO = false;
	m_createTrackOnly = false;
	m_instrumentLoaded = false;
	m_showJackAliases = -1;
	
	m_midiInPort = -1;
	m_midiOutPort = -1;

    m_height = 290;
	m_width = 450;

	m_allChannelBit = (1 << MIDI_CHANNELS) - 1;

	cmbType->addItem(*addMidiIcon, tr("Midi"), Track::MIDI);
	cmbType->addItem(*addAudioIcon, tr("Audio"), Track::WAVE);
	cmbType->addItem(*addOutputIcon, tr("Output"), Track::AUDIO_OUTPUT);
	cmbType->addItem(*addInputIcon, tr("Input"), Track::AUDIO_INPUT);
	cmbType->addItem(*addBussIcon, tr("Buss"), Track::AUDIO_BUSS);
	cmbType->addItem(*addAuxIcon, tr("Aux Send"), Track::AUDIO_AUX);

	cmbInChannel->addItem(tr("All"), m_allChannelBit);
	for(int i = 0; i < 16; ++i)
	{
		cmbInChannel->addItem(QString(tr("Chan ")).append(QString::number(i+1)), 1 << i);
	}
	cmbInChannel->setCurrentIndex(1);
	
	//cmbOutChannel->addItem(tr("All"), m_allChannelBit);
	for(int i = 0; i < 16; ++i)
	{
		cmbOutChannel->addItem(QString(tr("Chan ")).append(QString::number(i+1)), i);
	}
	cmbOutChannel->setCurrentIndex(0);
	
	int row = cmbType->findData(m_insertType);
	cmbType->setCurrentIndex(row);

	m_panKnob = new Knob(this);/*{{{*/
	m_panKnob->setRange(-1.0, +1.0);
	m_panKnob->setId(9);
	m_panKnob->setKnobImage(QString(":images/knob_buss_new.png"));

	m_panKnob->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	m_panKnob->setBackgroundRole(QPalette::Mid);
	m_panKnob->setToolTip("Panorama");
	m_panKnob->setEnabled(true);
	m_panKnob->setIgnoreWheel(false);
	
	m_panLabel = new DoubleLabel(0, -1.0, +1.0, this);
	m_panLabel->setSlider(m_panKnob);
	m_panLabel->setFont(config.fonts[1]);
	m_panLabel->setBackgroundRole(QPalette::Mid);
	m_panLabel->setFrame(true);
	m_panLabel->setAlignment(Qt::AlignCenter);
	m_panLabel->setPrecision(2);
	m_panLabel->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

	connect(m_panKnob, SIGNAL(valueChanged(double, int)), m_panLabel, SLOT(setValue(double)));
	connect(m_panLabel, SIGNAL(valueChanged(double, int)), m_panKnob, SLOT(setValue(double)));
	QLabel *pl = new QLabel(tr("Pan"));
	pl->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);

	panBox->addWidget(m_panKnob);
	panBox->addWidget(m_panLabel);
	panBox->addWidget(pl);


	m_auxKnob = new Knob(this);
	m_auxKnob->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));

	m_auxKnob->setRange(config.minSlider - 0.1, 10.0);
	m_auxKnob->setKnobImage(":images/knob_aux_new.png");
	m_auxKnob->setToolTip(tr("Reverb Sends level"));
	m_auxKnob->setBackgroundRole(QPalette::Mid);
	m_auxKnob->setValue(config.minSlider);

	m_auxLabel = new DoubleLabel(config.minSlider, config.minSlider, 10.1, this);
	m_auxLabel->setSlider(m_auxKnob);
	m_auxLabel->setFont(config.fonts[1]);
	m_auxLabel->setBackgroundRole(QPalette::Mid);
	m_auxLabel->setFrame(true);
	m_auxLabel->setAlignment(Qt::AlignCenter);
	m_auxLabel->setPrecision(0);
	m_auxLabel->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

	connect(m_auxKnob, SIGNAL(valueChanged(double, int)), m_auxLabel, SLOT(setValue(double)));
	connect(m_auxLabel, SIGNAL(valueChanged(double, int)), m_auxKnob, SLOT(setValue(double)));
	QLabel *al = new QLabel(tr("Verb"));
	al->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);

	auxBox->addWidget(m_auxKnob);
	auxBox->addWidget(m_auxLabel);
	auxBox->addWidget(al);/*}}}*/

	connect(chkInput, SIGNAL(toggled(bool)), this, SLOT(updateInputSelected(bool)));
	connect(chkOutput, SIGNAL(toggled(bool)), this, SLOT(updateOutputSelected(bool)));
	connect(chkBuss, SIGNAL(toggled(bool)), this, SLOT(updateBussSelected(bool)));
	connect(cmbType, SIGNAL(currentIndexChanged(int)), this, SLOT(trackTypeChanged(int)));
	//connect(cmbInstrument, SIGNAL(currentIndexChanged(int)), this, SLOT(updateInstrument(int)));
	connect(cmbInstrument, SIGNAL(activated(int)), this, SLOT(updateInstrument(int)));
	connect(btnAdd, SIGNAL(clicked()), this, SLOT(addTrack()));
	connect(txtName, SIGNAL(editingFinished()/*textEdited(QString)*/), this, SLOT(trackNameEdited()));
	connect(btnCancel, SIGNAL(clicked()), this, SLOT(cancelSelected()));
	connect(btnMyInput, SIGNAL(clicked()), this, SLOT(showInputSettings()));
	txtName->setFocus(Qt::OtherFocusReason);
}

//Add button slot
void CreateTrackDialog::addTrack()/*{{{*/
{
	if(txtName->text().isEmpty())
		return;
	
	int inputIndex = cmbInput->currentIndex();
	int outputIndex = cmbOutput->currentIndex();
	int instrumentIndex = cmbInstrument->currentIndex();
	int monitorIndex = cmbMonitor->currentIndex();
	int inChanIndex = cmbInChannel->currentIndex();
	int outChanIndex = cmbOutChannel->currentIndex();
	int bussIndex = cmbBuss->currentIndex();
	bool valid = true;

	m_vtrack->name = txtName->text();
	m_vtrack->type = m_insertType;
	Track::TrackType type = (Track::TrackType)m_insertType;
	switch(type)
	{
		case Track::MIDI:
		case Track::DRUM:
		{
            int instrumentType = cmbInstrument->itemData(instrumentIndex, CTDInstrumentTypeRole).toInt();
            QString instrumentName = cmbInstrument->itemData(instrumentIndex, CTDInstrumentNameRole).toString();
            QString selectedInput, selectedInput2;
            
            if (instrumentType == TrackManager::SYNTH_INSTRUMENT)
            {
                for (iMidiDevice i = midiDevices.begin(); i != midiDevices.end(); ++i)
                {
                    if ((*i)->isSynthPlugin() && (*i)->name() == instrumentName)
                    {
						m_vtrack->instrumentType = instrumentType;
						m_vtrack->instrumentName = instrumentName;

                        m_vtrack->useOutput = true;
                        m_vtrack->createMidiOutputDevice = false;
                        m_vtrack->useMonitor = true;
                        break;
                    }
                }
            }

			m_vtrack->autoCreateInstrument = chkAutoCreate->isChecked();
			//Process Input connections
			if(inputIndex >= 0 && chkInput->isChecked())
			{
				m_vtrack->useInput = true;
				int chanbit = cmbInChannel->itemData(inChanIndex).toInt();
				if(inputIndex == 0)
				{
					m_vtrack->useGlobalInputs = true;
					m_vtrack->inputChannel = chanbit;
				}
				else
				{
					QString devname = cmbInput->itemText(inputIndex);
					int inDevType = cmbInput->itemData(inputIndex).toInt();
					if(m_currentMidiInputList.isEmpty())
					{
						m_createMidiInputDevice = true;
					}
					else
					{
						m_createMidiInputDevice = !(m_currentMidiInputList.contains(inputIndex) && m_currentMidiInputList.value(inputIndex) == devname);
					}
					m_vtrack->createMidiInputDevice = m_createMidiInputDevice;
					m_vtrack->inputConfig = qMakePair(inDevType, devname);
					m_vtrack->inputChannel = chanbit;
				}
			}
			
			//Process Output connections
			if(outputIndex >= 0 && chkOutput->isChecked())
			{
				m_vtrack->useOutput = true;
				QString devname = cmbOutput->itemText(outputIndex);
				int devtype = cmbOutput->itemData(outputIndex).toInt();
				int outChan = cmbOutChannel->itemData(outChanIndex).toInt();
				if(m_currentMidiOutputList.isEmpty())
				{
					m_createMidiOutputDevice = true;
				}
				else
				{
					m_createMidiOutputDevice = !(m_currentMidiOutputList.contains(outputIndex) && m_currentMidiOutputList.value(outputIndex) == devname);
				}
				m_vtrack->createMidiOutputDevice = m_createMidiOutputDevice;
				m_vtrack->outputConfig = qMakePair(devtype, devname);
				m_vtrack->outputChannel = outChan;
				if(m_createMidiOutputDevice)
				{
					//QString instrumentName = cmbInstrument->itemData(instrumentIndex, CTDInstrumentNameRole).toString();
					m_vtrack->instrumentType = instrumentType; //cmbInstrument->itemData(instrumentIndex, CTDInstrumentTypeRole).toInt();
					m_vtrack->instrumentName = instrumentName;
				}
			}

			if(monitorIndex >= 0 && midiBox->isChecked())
			{
				int iBuss = cmbBuss->itemData(bussIndex).toInt();
				QString selectedBuss = cmbBuss->itemText(bussIndex);
                if (selectedInput.isEmpty())
                    selectedInput = cmbMonitor->itemText(monitorIndex);
                if (selectedInput2.isEmpty())
                    selectedInput2 = selectedInput;
				m_vtrack->useMonitor = true;
				m_vtrack->monitorConfig  = qMakePair(0, selectedInput);
                m_vtrack->monitorConfig2 = qMakePair(0, selectedInput2);
				m_vtrack->instrumentPan = m_panKnob->value();
				m_vtrack->instrumentVerb = m_auxKnob->value();
				if(chkBuss->isChecked())
				{
					m_vtrack->useBuss = true;
					m_vtrack->bussConfig = qMakePair(iBuss, selectedBuss);
				}
			}
		}
		break;
		case Track::WAVE:
		{
			if(inputIndex >= 0 && chkInput->isChecked())
			{
				QString selectedInput = cmbInput->itemText(inputIndex);
				int addNewRoute = cmbInput->itemData(inputIndex).toInt();
				m_vtrack->useInput = true;
				m_vtrack->inputConfig = qMakePair(addNewRoute, selectedInput);
			}
			if(outputIndex >= 0 && chkOutput->isChecked())
			{
				//Route to the Output or Buss
				QString selectedOutput = cmbOutput->itemText(outputIndex);
				m_vtrack->useOutput = true;
				m_vtrack->outputConfig = qMakePair(0, selectedOutput);
			}
		}
		break;
		case Track::AUDIO_OUTPUT:
		{
			if(inputIndex >= 0 && chkInput->isChecked())
			{
				QString selectedInput = cmbInput->itemText(inputIndex);
				m_vtrack->useInput = true;
				m_vtrack->inputConfig = qMakePair(0, selectedInput);
			}

			if(outputIndex >= 0 && chkOutput->isChecked())
			{
				QString jackPlayback("system:playback");
				QString selectedOutput = cmbOutput->itemText(outputIndex);
				m_vtrack->useOutput = true;
				m_vtrack->outputConfig = qMakePair(0, selectedOutput);
			}
		}
		break;
		case Track::AUDIO_INPUT:
		{
			if(inputIndex >= 0 && chkInput->isChecked())
			{
				QString selectedInput = cmbInput->itemText(inputIndex);
				m_vtrack->useInput = true;
				m_vtrack->inputConfig = qMakePair(0, selectedInput);
			}
			if(outputIndex >= 0 && chkOutput->isChecked())
			{
				QString selectedOutput = cmbOutput->itemText(outputIndex);
				m_vtrack->useOutput = true;
				m_vtrack->outputConfig = qMakePair(0, selectedOutput);
			}
		}
		break;
		case Track::AUDIO_BUSS:
		{
			if(inputIndex >= 0 && chkInput->isChecked())
			{
				QString selectedInput = cmbInput->itemText(inputIndex);
				m_vtrack->useInput = true;
				m_vtrack->inputConfig = qMakePair(0, selectedInput);
			}
			if(outputIndex >= 0 && chkOutput->isChecked())
			{
				QString selectedOutput = cmbOutput->itemText(outputIndex);
				m_vtrack->useOutput = true;
				m_vtrack->outputConfig = qMakePair(0, selectedOutput);
			}
		}
		break;
		case Track::AUDIO_AUX:
			//Just add the track type and rename it
		break;
		case Track::AUDIO_SOFTSYNTH:
			valid = false;
        break;
		default:
			valid = false;
	}
	if(valid)
	{
		cleanup();
		if(m_templateMode)
		{
			//hide();
			done(1);
		}
		else
		{
			connect(trackManager, SIGNAL(trackAdded(qint64)), this, SIGNAL(trackAdded(qint64)));
			if(trackManager->addTrack(m_vtrack, m_insertPosition))
			{
				qDebug("Sucessfully added track");
				done(1);
			}
		}
	}
}/*}}}*/

void CreateTrackDialog::lockType(bool lock)
{
	cmbType->setEnabled(!lock);
}

void CreateTrackDialog::cancelSelected()
{
	cleanup();
	reject();
}

void CreateTrackDialog::showInputSettings()
{
	GlobalSettingsConfig* genSetConfig = new GlobalSettingsConfig(this);
	genSetConfig->setCurrentTab(2);
	genSetConfig->show();
}

void CreateTrackDialog::cleanup()/*{{{*/
{
	if(m_instrumentLoaded)
	{
		int insType = cmbInstrument->itemData(cmbInstrument->currentIndex(), CTDInstrumentTypeRole).toInt();
		switch(insType)
		{
			case TrackManager::LS_INSTRUMENT:
			{
				if(!lsClient)
				{
					lsClient = new LSClient(config.lsClientHost, config.lsClientPort);
					lsClientStarted = lsClient->startClient();
				}
				else if(!lsClientStarted)
				{
					lsClientStarted = lsClient->startClient();
				}
				if(lsClientStarted)
				{
					lsClient->removeLastChannel();
					m_instrumentLoaded = false;
				}
			}
			break;
			case TrackManager::SYNTH_INSTRUMENT:
			{
                if (m_instrumentLoaded)
                {
#if 0
                    if (m_lastSynth)
                    {
                        if (m_lastSynth->duplicated())
                        {
                            midiDevices.remove(m_lastSynth);
                            //delete m_lastSynth;
                        }
                        m_lastSynth->close();
                        m_lastSynth = 0;
                    }
#endif
                    m_instrumentLoaded = false;
                }
			}
			break;
			default:
			break;
		}
	}
}/*}}}*/

void CreateTrackDialog::updateInstrument(int index)/*{{{*/
{
	QString instrumentName = cmbInstrument->itemData(index, CTDInstrumentNameRole).toString();
	QString trackName = txtName->text();
	if(btnAdd->isEnabled())
	{
        chkInput->setEnabled(true);
        cmbInput->setEnabled(true);
        cmbInChannel->setEnabled(true);
        chkOutput->setEnabled(true);
        cmbOutput->setEnabled(true);
        cmbOutChannel->setEnabled(true);
        midiBox->setChecked(true);
        midiBox->setEnabled(true);

		int insType = cmbInstrument->itemData(index, CTDInstrumentTypeRole).toInt();
		switch(insType)
		{
			case TrackManager::LS_INSTRUMENT:
			{
				for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i)
				{
					if ((*i)->iname() == instrumentName && (*i)->isOOMInstrument())
					{
						//m_panKnob->blockSignals(true);
						m_panKnob->setValue((*i)->defaultPan());
						//m_panLabel->setValue((*i)->defaultPan());
						//m_panKnob->blockSignals(false);
						//m_auxKnob->blockSignals(true);
						m_auxKnob->setValue((*i)->defaultVerb());
						//m_auxLabel->setValue((*i)->defaultVerb());
						//m_auxKnob->blockSignals(false);
						if(m_instrumentLoaded)
						{//unload the last one
							cleanup();
						}
						if(!lsClient)
						{
							lsClient = new LSClient(config.lsClientHost, config.lsClientPort);
							lsClientStarted = lsClient->startClient();
							if(config.lsClientResetOnStart && lsClientStarted)
							{
								lsClient->resetSampler();
							}
						}
						else if(!lsClientStarted)
						{
							lsClientStarted = lsClient->startClient();
						}
						if(lsClientStarted)
						{
							qDebug("Loading Instrument to LinuxSampler");
							if(lsClient->loadInstrument(*i))
							{
								qDebug("Instrument Map Loaded");
								if(chkAutoCreate->isChecked())
								{
									int map = lsClient->findMidiMap((*i)->iname().toUtf8().constData());
									Patch* p = (*i)->getDefaultPatch();
									if(p && map >= 0)
									{
										if(lsClient->createInstrumentChannel(txtName->text().toUtf8().constData(), p->engine.toUtf8().constData(), p->filename.toUtf8().constData(), p->index, map))
										{
											qDebug("Create Channel for track");
											QString prefix("LinuxSampler:");
											QString postfix("-audio");
											QString audio(QString(prefix).append(trackName).append(postfix));
											QString midi(QString(prefix).append(trackName));
											//reload input/output list and select the coresponding ports respectively
											updateVisibleElements();
											//populateInputList();
											populateOutputList();
											populateMonitorList();
											cmbOutput->setCurrentIndex(cmbOutput->findText(midi));
											cmbMonitor->setCurrentIndex(cmbMonitor->findText(audio));
											m_instrumentLoaded = true;
										}
									}
								}
							}
						}
						break;
					}
				}
			}
			break;
			case TrackManager::SYNTH_INSTRUMENT:
			{
                for (iMidiDevice i = midiDevices.begin(); i != midiDevices.end(); ++i)
                {
                    if ((*i)->isSynthPlugin() && (*i)->name() == instrumentName)
                    {
                        if(m_instrumentLoaded)
						{   //unload the last one
							cleanup();
						}

                        updateVisibleElements();
                        populateMonitorList();
                        chkInput->setChecked(true);
                        chkInput->setEnabled(true);
                        chkOutput->setChecked(false);
                        chkOutput->setEnabled(false);
                        midiBox->setChecked(false);
                        midiBox->setEnabled(false);

                        m_instrumentLoaded = true;
                        break;
                    }
                }
			}
			break;
			default:
			break;
		}
	}
}/*}}}*/

//Input raw slot
void CreateTrackDialog::updateInputSelected(bool raw)/*{{{*/
{
	cmbInput->setEnabled(raw);
	cmbInChannel->setEnabled(raw);
}/*}}}*/

//Output raw slot
void CreateTrackDialog::updateOutputSelected(bool raw)/*{{{*/
{
	cmbOutput->setEnabled(raw);
	cmbOutChannel->setEnabled(raw);
}/*}}}*/

void CreateTrackDialog::updateBussSelected(bool raw)/*{{{*/
{
	cmbBuss->setEnabled(raw);
	if(raw)
	{
		m_panKnob->setKnobImage(QString(":images/knob_buss_new.png"));
	}
	else
	{
		m_panKnob->setKnobImage(QString(":images/knob_midi_new.png"));
	}
	m_panKnob->update();
}/*}}}*/

//Track type combo slot
void CreateTrackDialog::trackTypeChanged(int type)
{
	m_insertType = cmbType->itemData(type).toInt();
	updateVisibleElements();
	populateInputList();
	populateOutputList();
	populateInstrumentList();
	populateMonitorList();
	populateBussList();
}

void CreateTrackDialog::trackNameEdited()
{
	bool enabled = false;
	if(!txtName->text().isEmpty())
	{
		Track* t = song->findTrack(txtName->text());
		if(!t)
			enabled = true;
	}
	btnAdd->setEnabled(enabled);
	Track::TrackType type = (Track::TrackType)m_insertType;
	if(type == Track::MIDI && m_instrumentLoaded && enabled)
	{
		cleanup();
		updateInstrument(cmbInstrument->currentIndex());
	}
}

//Populate input combo based on type
void CreateTrackDialog::populateInputList()/*{{{*/
{
	while(cmbInput->count())
		cmbInput->removeItem(cmbInput->count()-1);
	Track::TrackType type = (Track::TrackType)m_insertType;
	switch(type)
	{
		case Track::MIDI:
		case Track::DRUM:
		{
			m_currentMidiInputList.clear();
			cmbInput->addItem(tr("My Inputs"), 1025);
			for (int i = 0; i < MIDI_PORTS; ++i)
			{
				MidiPort* mp = &midiPorts[i];
				MidiDevice* md = mp->device();
				
				if (!md)
					continue;

				if ((md->openFlags() & 2))
				{
					QString mdname(md->name());
					if(md->deviceType() == MidiDevice::ALSA_MIDI)
					{
						mdname = QString("(OOMidi) ").append(mdname);
					}
					cmbInput->addItem(mdname, i);
					m_currentMidiInputList.insert(cmbInput->count()-1, mdname);
				}
			}
			populateNewInputList();

			if (!cmbInput->count())
			{
				chkInput->setChecked(false);
				chkInput->setEnabled(false);
			}
		}
		break;
		case Track::WAVE:
		{
			for(iTrack it = song->inputs()->begin(); it != song->inputs()->end(); ++it)
			{
				cmbInput->addItem((*it)->name(), 0);
			}
			importOutputs();
			if (!cmbInput->count())
			{//TODO: Not sure what we could do here except notify the user
				chkInput->setChecked(false);
				chkInput->setEnabled(false);
			}
		}
		break;
		case Track::AUDIO_OUTPUT:
		{
			for(iTrack t = song->tracks()->begin(); t != song->tracks()->end(); ++t)
			{
				if((*t)->isMidiTrack() || (*t)->type() == Track::AUDIO_OUTPUT)
					continue;
				AudioTrack* track = (AudioTrack*) (*t);
				Route r(track, -1);
				cmbInput->addItem(r.name());
			}

			if (!cmbInput->count())
			{//TODO: Not sure what we could do here except notify the user
				chkInput->setChecked(false);
				chkInput->setEnabled(false);
			}
		}
		break;
		case Track::AUDIO_INPUT:
		{
			importOutputs();
			if (!cmbInput->count())
			{//TODO: Not sure what we could do here except notify the user
				chkInput->setChecked(false);
				chkInput->setEnabled(false);
			}
		}
		break;
		case Track::AUDIO_BUSS:
		{
			for(iTrack t = song->tracks()->begin(); t != song->tracks()->end(); ++t)
			{
				if((*t)->isMidiTrack())
					continue;
				AudioTrack* track = (AudioTrack*) (*t);
				if(track->type() == Track::AUDIO_INPUT || track->type() == Track::WAVE || track->type() == Track::AUDIO_SOFTSYNTH)
				{
					cmbInput->addItem(Route(track, -1).name());
				}
			}
			if (!cmbInput->count())
			{//TODO: Not sure what we could do here except notify the user
				chkInput->setChecked(false);
				chkInput->setEnabled(false);
			}
		}
		break;
		case Track::AUDIO_AUX:
        break;
		case Track::AUDIO_SOFTSYNTH:
            chkInput->setChecked(false);
            chkInput->setEnabled(false);
		break;
	}
}/*}}}*/

void CreateTrackDialog::populateOutputList()/*{{{*/
{
	while(cmbOutput->count())
		cmbOutput->removeItem(cmbOutput->count()-1);
	Track::TrackType type = (Track::TrackType)m_insertType;
	switch(type)
	{
		case Track::MIDI:
		case Track::DRUM:
		{
			m_currentMidiOutputList.clear();
			for (int i = 0; i < MIDI_PORTS; ++i)
			{
				MidiPort* mp = &midiPorts[i];
				MidiDevice* md = mp->device();
				
				if (!md)
					continue;

				if((md->openFlags() & 1))
				{
					QString mdname(md->name());
					if(md->deviceType() == MidiDevice::ALSA_MIDI)
					{
						mdname = QString("(OOMidi) ").append(mdname);
					}
					cmbOutput->addItem(mdname, i);
					m_currentMidiOutputList.insert(cmbOutput->count()-1, mdname);
				}
			}

			populateNewOutputList();
			if (!cmbOutput->count())
			{
				chkOutput->setChecked(false);
				chkOutput->setEnabled(false);
			}
		}
		break;
		case Track::WAVE:
		{
			for(iTrack t = song->tracks()->begin(); t != song->tracks()->end(); ++t)
			{
				if((*t)->isMidiTrack())
					continue;
				AudioTrack* track = (AudioTrack*) (*t);
				if(track->type() == Track::AUDIO_OUTPUT || track->type() == Track::AUDIO_BUSS)
				{
					cmbOutput->addItem(Route(track, -1).name());
				}
			}
			if (!cmbOutput->count())
			{
				chkOutput->setChecked(false);
				chkOutput->setEnabled(false);
			}
		}
		break;
		case Track::AUDIO_OUTPUT:
		{
			importInputs();
			if (!cmbOutput->count())
			{
				chkOutput->setChecked(false);
				chkOutput->setEnabled(false);
			}
		}
		break;
		case Track::AUDIO_INPUT:
		{
			for(iTrack t = song->tracks()->begin(); t != song->tracks()->end(); ++t)
			{
				if((*t)->isMidiTrack())
					continue;
				AudioTrack* track = (AudioTrack*) (*t);
				switch(track->type())
				{
					case Track::AUDIO_OUTPUT:
					case Track::AUDIO_BUSS:
					case Track::WAVE:
						cmbOutput->addItem(Route(track, -1).name());
					break;
					default:
					break;
				}
			}
			if (!cmbOutput->count())
			{
				chkOutput->setChecked(false);
				chkOutput->setEnabled(false);
			}
		}
		break;
		case Track::AUDIO_BUSS:
		{
			for(iTrack t = song->outputs()->begin(); t != song->outputs()->end(); ++t)
			{
				AudioTrack* track = (AudioTrack*) (*t);
				cmbOutput->addItem(Route(track, -1).name());
			}
			if (!cmbOutput->count())
			{
				chkOutput->setChecked(false);
				chkOutput->setEnabled(false);
			}
		}
		break;
		case Track::AUDIO_AUX:
        break;
		case Track::AUDIO_SOFTSYNTH:
            chkOutput->setChecked(false);
            chkOutput->setEnabled(false);
		break;
	}
}/*}}}*/

void CreateTrackDialog::importOutputs()/*{{{*/
{
	if (checkAudioDevice()) 
	{
		std::list<QString> sl = audioDevice->outputPorts();
		for (std::list<QString>::iterator i = sl.begin(); i != sl.end(); ++i) {
			cmbInput->addItem(*i, 1);
		}
	}
}/*}}}*/

void CreateTrackDialog::importInputs()/*{{{*/
{
	if (checkAudioDevice()) 
	{
		std::list<QString> sl = audioDevice->inputPorts();
		for (std::list<QString>::iterator i = sl.begin(); i != sl.end(); ++i) {
			cmbOutput->addItem(*i, 1);
		}
	}
}/*}}}*/

void CreateTrackDialog::populateMonitorList()/*{{{*/
{
	while(cmbMonitor->count())
		cmbMonitor->removeItem(cmbMonitor->count()-1);
	if (checkAudioDevice()) 
	{
		std::list<QString> sl = audioDevice->outputPorts();
		for (std::list<QString>::iterator i = sl.begin(); i != sl.end(); ++i) {
			cmbMonitor->addItem(*i, 1);
		}
	}
}/*}}}*/

void CreateTrackDialog::populateBussList()/*{{{*/
{
	while(cmbBuss->count())
		cmbBuss->removeItem(cmbBuss->count()-1);
	cmbBuss->addItem(tr("Add New Buss"), 1);
	for(iTrack it = song->groups()->begin(); it != song->groups()->end(); ++it)
	{
		cmbBuss->addItem((*it)->name(), 0);
	}
}/*}}}*/

void CreateTrackDialog::populateNewInputList()/*{{{*/
{
	//while(cmbInput->count())
	//	cmbInput->removeItem(cmbInput->count()-1);
	//m_createMidiInputDevice = true;
	for (iMidiDevice i = midiDevices.begin(); i != midiDevices.end(); ++i)
	{
		if ((*i)->deviceType() == MidiDevice::ALSA_MIDI)
		{
			cmbInput->addItem((*i)->name(), MidiDevice::ALSA_MIDI);
		}
	}
	if(audioDevice->deviceType() != AudioDevice::JACK_AUDIO)
		return;
	std::list<QString> sl = audioDevice->outputPorts(true, m_showJackAliases);
	for (std::list<QString>::iterator ip = sl.begin(); ip != sl.end(); ++ip)
	{
		cmbInput->addItem(*ip, MidiDevice::JACK_MIDI);
	}
}/*}}}*/

void CreateTrackDialog::populateNewOutputList()/*{{{*/
{
	//while(cmbOutput->count())
	//	cmbOutput->removeItem(cmbOutput->count()-1);
	//m_createMidiOutputDevice = true;
	if(audioDevice->deviceType() != AudioDevice::JACK_AUDIO)
		return;
	std::list<QString> sl = audioDevice->inputPorts(true, m_showJackAliases);
	for (std::list<QString>::iterator ip = sl.begin(); ip != sl.end(); ++ip)
	{
		cmbOutput->addItem(*ip, MidiDevice::JACK_MIDI);
	}
}/*}}}*/

void CreateTrackDialog::populateInstrumentList()/*{{{*/
{
    cmbInstrument->clear();

    if (m_insertType == Track::MIDI)
    {
        // add GM first, then LS, then SYNTH
        for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i)
        {
			if((*i)->isOOMInstrument() == false)
			{
            	cmbInstrument->addItem(QString("(GM) ").append((*i)->iname()));
				cmbInstrument->setItemData(cmbInstrument->count()-1, (*i)->iname(), CTDInstrumentNameRole);
				cmbInstrument->setItemData(cmbInstrument->count()-1, TrackManager::GM_INSTRUMENT, CTDInstrumentTypeRole);
			}
        }
        
        for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i)
        {
			if((*i)->isOOMInstrument())
			{
            	cmbInstrument->addItem(QString("(LS) ").append((*i)->iname()));
				cmbInstrument->setItemData(cmbInstrument->count()-1, (*i)->iname(), CTDInstrumentNameRole);
				cmbInstrument->setItemData(cmbInstrument->count()-1, TrackManager::LS_INSTRUMENT, CTDInstrumentTypeRole);
			}
        }

        for (iMidiDevice i = midiDevices.begin(); i != midiDevices.end(); ++i)
        {
            if ((*i)->isSynthPlugin())
			{
                if (((SynthPluginDevice*)(*i))->duplicated() == false)
                {
                    cmbInstrument->addItem(QString("(SYNTH) ").append((*i)->name()));
                    cmbInstrument->setItemData(cmbInstrument->count()-1, (*i)->name(), CTDInstrumentNameRole);
                    cmbInstrument->setItemData(cmbInstrument->count()-1, TrackManager::SYNTH_INSTRUMENT, CTDInstrumentTypeRole);
                }
			}
        }

        //Default to the GM instrument
        int gm = cmbInstrument->findText("(GM) GM");
        if (gm >= 0)
            cmbInstrument->setCurrentIndex(gm);
    }
}/*}}}*/

void CreateTrackDialog::updateVisibleElements()/*{{{*/
{
	chkInput->setEnabled(true);
	chkOutput->setEnabled(true);
	chkInput->setChecked(true);
	chkOutput->setChecked(true);
	chkBuss->setChecked(true);
	chkAutoCreate->setChecked(true);
	trackNameEdited();

	Track::TrackType type = (Track::TrackType)m_insertType;
	switch (type)
	{
		case Track::MIDI:
		case Track::DRUM:
		{
			cmbInChannel->setVisible(true);
			cmbOutChannel->setVisible(true);
			lblInstrument->setVisible(true);
			cmbInstrument->setVisible(true);
			cmbInstrument->setEnabled(true);
			cmbMonitor->setVisible(true);
			cmbInput->setVisible(true);
			chkInput->setVisible(true);
			cmbOutput->setVisible(true);
			chkOutput->setVisible(true);
			midiBoxFrame->setVisible(true);
			midiBoxLabelFrame->setVisible(true);
			chkAutoCreate->setVisible(true);
			trackNameEdited();

            //m_height = 290;
            m_height = 400;
			m_width = width();
		}
		break;
		case Track::AUDIO_OUTPUT:
		{
			cmbInChannel->setVisible(false);
			cmbOutChannel->setVisible(false);
			lblInstrument->setVisible(false);
			cmbInstrument->setVisible(false);
			cmbMonitor->setVisible(false);
			midiBoxFrame->setVisible(false);
			midiBoxLabelFrame->setVisible(false);
			chkAutoCreate->setVisible(false);

			cmbInput->setVisible(true);
			chkInput->setVisible(true);
			cmbOutput->setVisible(true);
			chkOutput->setVisible(true);
			
			//m_height = 160;
			m_height = 260;
			m_width = width();
		}
		break;
		case Track::AUDIO_INPUT:
		{
			cmbInChannel->setVisible(false);
			cmbOutChannel->setVisible(false);
			lblInstrument->setVisible(false);
			cmbInstrument->setVisible(false);
			cmbMonitor->setVisible(false);
			midiBoxFrame->setVisible(false);
			midiBoxLabelFrame->setVisible(false);
			chkAutoCreate->setVisible(false);

			cmbInput->setVisible(true);
			chkInput->setVisible(true);
			cmbOutput->setVisible(true);
			chkOutput->setVisible(true);
			
			//m_height = 160;
			m_height = 260;
			m_width = width();
		}
		break;
		case Track::WAVE:
		{
			cmbInChannel->setVisible(false);
			cmbOutChannel->setVisible(false);
			lblInstrument->setVisible(false);
			cmbInstrument->setVisible(false);
			cmbMonitor->setVisible(false);
			midiBoxFrame->setVisible(false);
			midiBoxLabelFrame->setVisible(false);
			chkAutoCreate->setVisible(false);

			cmbInput->setVisible(true);
			chkInput->setVisible(true);
			cmbOutput->setVisible(true);
			chkOutput->setVisible(true);
			
			//m_height = 160;
			m_height = 260;
			m_width = width();
		}
		break;
		case Track::AUDIO_BUSS:
		{
			cmbInChannel->setVisible(false);
			cmbOutChannel->setVisible(false);
			lblInstrument->setVisible(false);
			cmbInstrument->setVisible(false);
			cmbMonitor->setVisible(false);
			midiBoxFrame->setVisible(false);
			midiBoxLabelFrame->setVisible(false);
			chkAutoCreate->setVisible(false);

			cmbInput->setVisible(true);
			chkInput->setVisible(true);
			cmbOutput->setVisible(true);
			chkOutput->setVisible(true);
			
			//m_height = 160;
			m_height = 260;
			m_width = width();
		}
		break;
		case Track::AUDIO_AUX:
		{
			cmbInChannel->setVisible(false);
			cmbOutChannel->setVisible(false);
			lblInstrument->setVisible(false);
			cmbInstrument->setVisible(false);
			cmbMonitor->setVisible(false);
			chkAutoCreate->setVisible(false);
			
			midiBoxFrame->setVisible(false);
			midiBoxLabelFrame->setVisible(false);
			
			cmbInput->setVisible(false);
			chkInput->setVisible(false);

			cmbOutput->setVisible(false);
			chkOutput->setVisible(false);
			
			//m_height = 100;
			m_height = 200;
			m_width = width();
		}
		default:
		break;
	}
	setFixedHeight(m_height);
	updateGeometry();
}/*}}}*/

void CreateTrackDialog::showEvent(QShowEvent*)
{
	qDebug("Inside CreateTrackDialog::showEvent trackType: %i, position: %i", m_insertType, m_insertPosition);
	updateVisibleElements();
	populateInputList();
	populateOutputList();
	populateInstrumentList();
	populateMonitorList();
	populateBussList();
	/*if(!gInputList.size())//TODO: popup a messagebox first telling them what is happening
	{
		GlobalSettingsConfig* genSetConfig = new GlobalSettingsConfig(this);
		genSetConfig->setCurrentTab(2);
		genSetConfig->show();
	}*/
}

