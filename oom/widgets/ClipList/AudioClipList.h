//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//
//  (C) Copyright 2010 Andrew Williams and Christopher Cherrett
//=========================================================

#ifndef _OOM_AUDIO_CLIPLIST_
#define _OOM_AUDIO_CLIPLIST_

#include "ui_AudioClipListBase.h"
#include <QFrame>
#include <QStringList>
#include <QModelIndex>
#include <QModelIndexList>
#include <QStandardItemModel>

class QListView;
class QMimeData;
class QPoint;
class AudioPlayer;
class Slider;

//Model for the file list
class ClipListModel : public QStandardItemModel
{
public:
	ClipListModel(QObject* parent = 0);
	virtual QStringList mimeTypes() const;
	virtual QMimeData* mimeData(const QModelIndexList&) const;
};


class AudioClipList : public QFrame, public Ui::AudioClipListBase {
	Q_OBJECT
	
	ClipListModel *m_listModel;
	BookmarkListModel *m_bookmarkModel;
	Slider* m_slider;
	QStringList m_filters;
	QString m_currentPath;
	QString m_currentSong;

	bool isSupported(const QString&);
	void loadBookmarks();
	void saveBookmarks();
	void addBookmark(const QString&);
	void updateLabels();

signals:
	void stopPlayback();

private slots:
	void playClicked(bool);
	void stopClicked(bool);
	void homeClicked();
	void forwardClicked();
	void rewindClicked();
	//void addBookmarkClicked();
	void fileItemSelected(const QModelIndex&);
	void fileItemContextMenu(const QPoint&);
	void bookmarkItemSelected(const QModelIndex&);
	void bookmarkContextMenu(const QPoint&);
	void updateTime(const QString&);
	void updateNowPlaying(const QString&);

public slots:
	void refresh();

public:
	AudioClipList(QWidget *parent = 0);
	~AudioClipList();
	void setDir(const QString& path);
};

#endif
