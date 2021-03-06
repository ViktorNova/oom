//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: gconfig.h,v 1.12.2.10 2009/12/01 03:52:40 terminator356 Exp $
//
//  (C) Copyright 1999-2003 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __CONFIG_H__
#define __CONFIG_H__

#define NUM_PARTCOLORS 112
#define NUM_FONTS 7

#include <QColor>
#include <QFont>
#include <QRect>
#include <QString>
#include <QHash>

class Xml;

//---------------------------------------------------------
//   MixerConfig
//---------------------------------------------------------

struct MixerConfig
{
    QString name;
    QRect geometry;
    bool showMidiTracks;
    bool showDrumTracks;
    bool showInputTracks;
    bool showOutputTracks;
    bool showWaveTracks;
    bool showGroupTracks;
    bool showAuxTracks;
    bool showSyntiTracks;

    //void write(Xml&, const char* name);
    //void write(int level, Xml& xml, const char* name);
    void write(int level, Xml & xml);
    //void read(QDomNode);
    //void read(Xml& xml, const QString& name);
    void read(Xml & xml);
};

//---------------------------------------------------------
//   GlobalConfigValues
//---------------------------------------------------------

struct GlobalConfigValues
{
    int globalAlphaBlend;
    QColor palette[16];
    QColor partColors[NUM_PARTCOLORS];
    QColor partWaveColors[NUM_PARTCOLORS];
    QColor partColorsAutomation[NUM_PARTCOLORS];
    QColor partWaveColorsAutomation[NUM_PARTCOLORS];
    QString partColorNames[NUM_PARTCOLORS];
    QColor transportHandleColor;
    QColor bigTimeForegroundColor;
    QColor bigTimeBackgroundColor;
    QColor waveEditBackgroundColor;
    //QFont fonts[6];
    QFont fonts[NUM_FONTS];
    QColor trackBg;
    QColor selectTrackBg;
    QColor selectTrackFg;

    QColor midiTrackLabelBg;
    QColor drumTrackLabelBg;
    QColor waveTrackLabelBg;
    QColor outputTrackLabelBg;
    QColor inputTrackLabelBg;
    QColor groupTrackLabelBg;
    QColor auxTrackLabelBg;
    QColor synthTrackLabelBg;

    QColor midiTrackBg;
    QColor drumTrackBg;
    QColor waveTrackBg;
    QColor outputTrackBg;
    QColor inputTrackBg;
    QColor groupTrackBg;
    QColor auxTrackBg;
    QColor synthTrackBg;

    QColor partCanvasBg;
    QColor ctrlGraphFg;
    QColor mixerBg;

    int division;
    int rtcTicks;
    int minMeter;
    double minSlider;
    bool freewheelMode;
    int guiRefresh;
    QString userInstrumentsDir;

    bool extendedMidi; // extended smf format
    int midiDivision; // division for smf export
    QString copyright; // copyright string for smf export
    int smfFormat; // smf export file type
    bool exp2ByteTimeSigs; // Export 2 byte time sigs instead of 4 bytes
    bool expOptimNoteOffs; // Save space by replacing note offs with note on velocity 0
    bool importMidiSplitParts; // Split imported tracks into multiple parts.

    int startMode; // 0 - start with last song
    // 1 - start with default template
    // 2 - start with song
    QString startSong; // path for start song
    int guiDivision; // division for tick display

    QRect geometryMain;
    QRect geometryTransport;
    QRect geometryBigTime;
    QRect geometryPerformer;
    QRect geometryDrumedit;
    //      QRect geometryMixer;
    MixerConfig mixer1;
    MixerConfig mixer2;
    bool transportVisible;
    bool bigTimeVisible;
    //      bool mixerVisible;
    bool mixer1Visible;
    bool mixer2Visible;
    bool markerVisible;

    bool showSplashScreen;
    int canvasShowPartType; // 1 - names, 2 events
    int canvasShowPartEvent; //
    bool canvasShowGrid;
    QString canvasBgPixmap;
    QStringList canvasCustomBgList;
    QString styleSheetFile;
    QString style;

    QString externalWavEditor;
    bool useOldStyleStopShortCut;
    bool moveArmedCheckBox;
    bool useDenormalBias;
    bool useOutputLimiter;
    bool showDidYouKnow;
    bool vstInPlace; // Enable VST in-place processing
    int dummyAudioSampleRate;
    int dummyAudioBufSize;
    QString projectBaseFolder;
    bool projectStoreInFolder;
	bool useProjectSaveDialog;
	int lsClientPort;
	QString lsClientHost;
	int lsClientRetry;
	int lsClientTimeout;
	bool lsClientBankAsNumber;
	bool lsClientAutoStart;
	bool lsClientResetOnStart;
	bool lsClientResetOnSongStart;
	bool lsClientStartLS;
	QString lsClientLSPath;
	bool loadLV2;
	bool loadLADSPA;
	QString ladspaPaths;
	bool loadVST;
	QString vstPaths;
	int audioRaster;
	int midiRaster;
	bool useAutoCrossFades;
};

extern GlobalConfigValues config;

#endif

