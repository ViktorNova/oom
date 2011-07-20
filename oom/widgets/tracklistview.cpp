//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include <QtGui>
#include "tracklistview.h"
#include "globals.h"
#include "gconfig.h"
#include "app.h"
#include "song.h"
#include "audio.h"
#include "track.h"
#include "part.h"
#include "midieditor.h"
#include "citem.h"
#include "arranger.h"
#include "event.h"

TrackListView::TrackListView(MidiEditor* editor, QWidget* parent)
: QFrame(parent)
{
	m_editor = editor;
	m_displayRole = PartRole;
	m_headers << "V" << "Track List";
	m_layout = new QVBoxLayout(this);
	m_layout->setContentsMargins(8, 2, 8, 2);
	m_model = new QStandardItemModel(0, 2, this);
	m_selmodel = new QItemSelectionModel(m_model);
	m_table = new QTableView(this);
	m_table->setContextMenuPolicy(Qt::CustomContextMenu);
	m_table->setObjectName("TrackListView");
	m_table->setModel(m_model);
	m_table->setSelectionModel(m_selmodel);
	m_table->setAlternatingRowColors(false);
	m_table->setShowGrid(true);
	m_table->setSelectionMode(QAbstractItemView::SingleSelection);
	m_table->setEditTriggers(QAbstractItemView::EditKeyPressed | QAbstractItemView::DoubleClicked);
	m_table->setCornerButtonEnabled(false);
	m_table->horizontalHeader()->setStretchLastSection(true);
	m_table->verticalHeader()->hide();
	m_layout->addWidget(m_table);

	m_buttonBox = new QHBoxLayout;
	m_chkWorkingView = new QCheckBox(tr("Working View"), this);
	m_chkWorkingView->setToolTip(tr("Toggle Working View. Show only tracks with parts in them"));
	m_chkWorkingView->setChecked(true);

	/*m_buttons = new QButtonGroup(this);
	m_buttons->setExclusive(true);
	m_buttons->addButton(m_chkPart, PartRole);
	m_buttons->addButton(m_chkTrack, TrackRole);*/

	m_buttonBox->addWidget(m_chkWorkingView);
	QSpacerItem* hSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
	m_buttonBox->addItem(hSpacer);

	m_layout->addLayout(m_buttonBox);

	songChanged(-1);
	connect(song, SIGNAL(songChanged(int)), this, SLOT(songChanged(int)));
	connect(m_model, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(toggleTrackPart(QStandardItem*)));
	connect(m_selmodel, SIGNAL(currentRowChanged(const QModelIndex, const QModelIndex)), this, SLOT(selectionChanged(const QModelIndex, const QModelIndex)));
	connect(m_chkWorkingView, SIGNAL(stateChanged(int)), this, SLOT(displayRoleChanged(int)));
	connect(m_table, SIGNAL(customContextMenuRequested(QPoint)), SLOT(contextPopupMenu(QPoint)));
}

TrackListView::~TrackListView()
{
}

void TrackListView::displayRoleChanged(int role)
{
	switch(role)
	{
		case Qt::Checked:
			m_displayRole = PartRole;
		break;
		case Qt::Unchecked:
			m_displayRole = TrackRole;
		break;
	}
	songChanged(-1);
}

void TrackListView::songChanged(int flags)/*{{{*/
{
	if(flags == -1 || flags & (SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED | SC_PART_INSERTED | SC_PART_REMOVED | SC_PART_COLOR_MODIFIED))
	{
		if(debugMsg)
			printf("TrackListView::songChanged\n");
		m_model->clear();
		for(iTrack i = song->artracks()->begin(); i != song->artracks()->end(); ++i)
		{
			if(!(*i)->isMidiTrack())
				continue;
			MidiTrack* track = (MidiTrack*)(*i);
			PartList* pl = track->parts();
			if(m_displayRole == PartRole && pl->empty())
			{
				continue;
			}
			QList<QStandardItem*> trackRow;
			QStandardItem* chkTrack = new QStandardItem(true);
			chkTrack->setCheckable(true);
			chkTrack->setBackground(QBrush(QColor(20,20,20)));
			chkTrack->setData(1, TrackRole);
			chkTrack->setData(track->name(), TrackNameRole);
			if(m_selected.contains(track->name()))
				chkTrack->setCheckState(Qt::Checked);
			trackRow.append(chkTrack);
			QStandardItem* trackName = new QStandardItem();
			trackName->setForeground(QBrush(QColor(205,209,205)));
			trackName->setBackground(QBrush(QColor(20,20,20)));
			trackName->setFont(QFont("fixed-width", 10, QFont::Bold));
			trackName->setText(track->name());
			//QFont font = trackName->font();
			trackName->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
			trackName->setData(1, TrackRole);
			trackName->setData(track->name(), TrackNameRole);
			trackName->setEditable(true);
			trackRow.append(trackName);
			m_model->appendRow(trackRow);

			for(iPart ip = pl->begin(); ip != pl->end(); ++ip)
			{
				QList<QStandardItem*> partsRow;
				Part* part = ip->second;
				QStandardItem* chkPart = new QStandardItem(true);
				chkPart->setCheckable(true);
				chkPart->setData(part->sn(), PartRole);
				chkPart->setData(2, TrackRole);
				chkPart->setData(track->name(), TrackNameRole);
				chkPart->setData(part->tick(), TickRole);
				if(m_editor->hasPart(part->sn()))
				{
					chkPart->setCheckState(Qt::Checked);
				}
				QStandardItem* partName = new QStandardItem();

				partName->setFont(QFont("fixed-width", 9, QFont::Bold));
				//if(m_displayRole == TrackRole)
				partName->setText(part->name());
				partName->setData(part->sn(), PartRole);
				partName->setData(2, TrackRole);
				partName->setData(track->name(), TrackNameRole);
				partName->setData(part->tick(), TickRole);
				partName->setEditable(true);
				//else
				//	partName->setText(track->name()+" : "+part->name());

				if(!partColorIcons.isEmpty() && part->colorIndex() < partColorIcons.size())
					partName->setIcon(partColorIcons.at(part->colorIndex()));
				partsRow.append(chkPart);
				partsRow.append(partName);
				m_model->appendRow(partsRow);
			}
		}
		m_model->setHorizontalHeaderLabels(m_headers);
		m_table->setColumnWidth(0, 20);
	}
}/*}}}*/

void TrackListView::contextPopupMenu(QPoint pos)/*{{{*/
{
	QModelIndex index = m_table->indexAt(pos);
	if(!index.isValid())
		return;
	QStandardItem* item = m_model->itemFromIndex(index);
	if(item)
	{
		//Make it works even if you rightclick on the checkbox
		QStandardItem* chkcol = m_model->item(item->row(), 0);
		if(chkcol)
		{
			QString trackName = chkcol->data(TrackNameRole).toString();
			Track* track = song->findTrack(trackName);
			if(!track || !m_editor || chkcol->column() != 0)
				return;
			QMenu* p = new QMenu(this);
			p->addAction(tr("Add Part"))->setData(1);
			p->addAction(tr("Add Part and Select"))->setData(2);

			QAction* act = p->exec(QCursor::pos());
			if (act)
			{
				int selection = act->data().toInt();
				switch(selection)
				{
					case 1:
						oom->arranger->addCanvasPart(track);
					break;
					case 2:
					{
						CItem* citem = oom->arranger->addCanvasPart(track);
						Part* p = citem->part();
						if(p)
						{
							m_editor->addPart(p);
							m_editor->setCurCanvasPart(p);
							movePlaybackToPart(p);
							songChanged(-1);//update check state
						}
					}
					break;
				}
			}
			delete p;
		}
	}
}/*}}}*/

void TrackListView::selectionChanged(const QModelIndex current, const QModelIndex)/*{{{*/
{
	if(!current.isValid())
		return;
	int row = current.row();
	QStandardItem* item = m_model->item(row, 0);
	int type = item->data(TrackRole).toInt();
	bool checked = (item->checkState() == Qt::Checked);
	QString trackName = item->data(TrackNameRole).toString();
	Track* track = song->findTrack(trackName);
	if(!track || !m_editor || type == 1 || !checked)
		return;

	PartList* list = track->parts();
	int sn = item->data(PartRole).toInt();
	unsigned tick = item->data(TickRole).toInt();
	Part* part = list->find(tick, sn);
	if(part)
	{
		m_editor->setCurCanvasPart(part);
		movePlaybackToPart(part);
	}
}/*}}}*/

void TrackListView::movePlaybackToPart(Part* part)
{
	if(audio->isPlaying())
		return;
	if(part)/*{{{*/
	{
		unsigned tick = part->tick();
		EventList* el = part->events();
		if(el->empty())
		{//move pb to part start
			Pos p(tick, true);
			song->setPos(0, p, true, true, true);
		}
		else
		{
			for(iEvent i = el->begin(); i != el->end(); ++i)
			{
				Event ev = i->second;
				if(ev.isNote())
				{
					Pos p(tick+ev.tick(), true);
					song->setPos(0, p, true, true, true);
					break;
				}
			}
		}
	}/*}}}*/
}

void TrackListView::toggleTrackPart(QStandardItem* item)/*{{{*/
{
	int type = item->data(TrackRole).toInt();
	int column = item->column();
	QStandardItem* chkItem = m_model->item(item->row(), 0);
	bool checked = (chkItem->checkState() == Qt::Checked);
	QString trackName = item->data(TrackNameRole).toString();
	Track* track = song->findTrack(trackName);
	if(!track || !m_editor)
		return;

	PartList* list = track->parts();
	if(list->empty())
		return;
	switch(type)
	{
		case 1: //Track
		{
			if(!column)
			{
				if(checked)
				{
					m_editor->addParts(list);
					m_selected.append(trackName);
				}
				else
				{
					m_editor->removeParts(list);
					m_editor->updateCanvas();
					m_selected.removeAll(trackName);
					song->update(SC_SELECTION);
				}
				if(!list->empty())
				{
					if(checked)
					{
						m_editor->setCurCanvasPart(list->begin()->second);
						movePlaybackToPart(list->begin()->second);
					}
					m_model->blockSignals(true);
					songChanged(-1);
					m_model->blockSignals(false);
				}
			}
			else
			{
				QString newName = item->text();
				bool valid = true;
				if(newName.isEmpty())
				{
					valid = false;
				}
				if(valid)
				{
					for (iTrack i = song->tracks()->begin(); i != song->tracks()->end(); ++i)
					{
						if ((*i)->name() == newName)
						{
							valid = false;
							break;
						}
					}
				}
				if(!valid)
				{
					QMessageBox::critical(this, tr("OOMidi: bad trackname"),
							tr("please choose a unique track name"),
							QMessageBox::Ok, Qt::NoButton, Qt::NoButton);
					m_model->blockSignals(true);
					songChanged(-1);
					m_model->blockSignals(false);
					update();
					return;
				}
				Track* newTrack = track->clone(false);
				newTrack->setName(newName);
				track->setName(newName);
				audio->msgChangeTrack(newTrack, track);

				m_model->blockSignals(true);
				songChanged(-1);
				m_model->blockSignals(false);
			}
		}
		break;
		case 2: //Part
		{
			int sn = item->data(PartRole).toInt();
			unsigned tick = item->data(TickRole).toInt();
			Part* part = list->find(tick, sn);
			if(part)
			{
				if(!column)
				{
					if(checked)
					{
						m_editor->addPart(part);
						m_editor->setCurCanvasPart(part);
						movePlaybackToPart(part);
					}
					else
					{
						m_editor->removePart(sn);
						m_editor->updateCanvas();
						m_selected.removeAll(trackName);
						m_model->blockSignals(true);
						songChanged(-1);
						m_model->blockSignals(false);
						song->update(SC_SELECTION);
					}
				}
				else
				{
					QString name = item->text();
					if(name.isEmpty())
					{
						QMessageBox::critical(this, tr("OOMidi: Invalid part name"),
								tr("Please choose a name with at least one charactor"),
								QMessageBox::Ok, Qt::NoButton, Qt::NoButton);
						m_model->blockSignals(true);
						songChanged(-1);
						m_model->blockSignals(false);
						update();
						return;
					}
					Part* newPart = part->clone();
					newPart->setName(name);
					// Indicate do undo, and do port controller values but not clone parts.
					audio->msgChangePart(part, newPart, true, true, false);
					m_model->blockSignals(true);
					songChanged(-1);
					m_model->blockSignals(false);
				}
			}
		}
		break;
	}
	update();
}/*}}}*/
