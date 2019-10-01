/***************************************************************************
 *   Copyright (C) 2013 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <QtGui>
#include <QContextMenuEvent>
#include <QMessageBox>
#include <QtWidgets>
#include <QFileDialog>
#include <QScrollBar>

#include "dialogs.h"
#include "mainwindow.h"
#include "mapwindow.h"

MapWindow::MapWindow(MainWindow* parent) : QGraphicsView(parent), mapData(this)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setMouseTracking(true);

    isUntitled = true;
    isModified = false;

    connect(horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(updateWindowPos(void)));
    connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(updateWindowPos(void)));
    connect(horizontalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(updateWindowPos(void)));
    connect(verticalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(updateWindowPos(void)));
}

void MapWindow::newFile(const QSize & sz, int generate, int sequenceNumber)
{
    isUntitled = true;

    QString fileName;
    QTextStream ss(& fileName);

    ss << "fh2map_";
    ss.setFieldWidth(4);
    ss.setPadChar('0');
    ss << sequenceNumber;
    ss.setFieldWidth(0);
    ss << ".map";

    QApplication::setOverrideCursor(Qt::WaitCursor);

    mapData.newMap(sz, generate, curFile, sequenceNumber);
    setScene(& mapData);

    QApplication::restoreOverrideCursor();
    curFile = fileName;
}

bool MapWindow::loadFile(const QString & fileName)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);

    if(! mapData.loadMap(fileName))
    {
	QApplication::restoreOverrideCursor();
        QMessageBox::warning(this, tr("Map Editor"), tr("Cannot read file %1.").arg(fileName));
        return false;
    }

    setScene(& mapData);

    QApplication::restoreOverrideCursor();
    setCurrentFile(fileName);

    QPair<int, int> vers = mapData.versions();

    if(vers.first != vers.second)
	mapWasModified();
    else
	emit windowModified(& mapData);

    return true;
}

bool MapWindow::save(void)
{
    return isUntitled ? saveAs() : saveFile(curFile, true);
}

bool MapWindow::saveAs(void)
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"), curFile);
    return fileName.isEmpty() ? false : saveFile(fileName, true);
}

bool MapWindow::saveRaw(void)
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Raw As"), curFile);
    return fileName.isEmpty() ? false : saveFile(fileName, false);
}

bool MapWindow::saveFile(const QString & fileName, bool compress)
{
    if(! mapData.saveMapXML(fileName, compress))
    {
        QMessageBox::warning(this, tr("Map Editor"), tr("Cannot write file %1.").arg(fileName));
        return saveAs();
    }

    setCurrentFile(fileName);

    return true;
}

QString MapWindow::userFriendlyCurrentFile(void)
{
    return mapData.name() + " (" + strippedName(curFile) + ")";
}

QString MapWindow::currentFile(void)
{
    return curFile;
}

void MapWindow::closeEvent(QCloseEvent* event)
{
    if(mapData.selectedItems().size())
	mapData.clearSelection();

    if(maybeSave())
        event->accept();
    else
        event->ignore();
}

void MapWindow::mapWasModified(void)
{
    isModified = true;
    setWindowTitle(userFriendlyCurrentFile() + "[*]");
    setWindowModified(true);

    emit windowModified(& mapData);
}

bool MapWindow::maybeSave(void)
{
    if(isModified)
    {
	QMessageBox::StandardButton ret = QMessageBox::warning(this, tr("Map Editor"),
		tr("'%1' has been modified.\nDo you want to save your changes?") .arg(userFriendlyCurrentFile()),
		QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

	switch(ret)
	{
	    case QMessageBox::Save:	return save();
	    case QMessageBox::Cancel:	return false;
	    case QMessageBox::Discard:	return true;
	    default: break;
	}
    }

    return true;
}

void MapWindow::setCurrentFile(const QString & fileName)
{
    QFileInfo fileInfo(fileName);
    curFile = QDir::toNativeSeparators(fileInfo.absolutePath() + QDir::separator() + fileInfo.baseName() + ".map");

    isUntitled = false;
    isModified = false;
    setWindowModified(false);
    setWindowTitle(userFriendlyCurrentFile() + "[*]");
}

QString MapWindow::strippedName(const QString & fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}

void MapWindow::viewportSetPositionFromMiniMap(const QPoint & pos)
{
    if(0 <= pos.x() && 0 <= pos.y() &&
	pos.x() < mapData.size().width() && pos.y() < mapData.size().height())
    {
	    const QSize & ts = EditorTheme::tileSize();
	    centerOn(pos.x() * ts.width(), pos.y() * ts.height());
    }
}

void MapWindow::viewportSetPositionFromListWidget(QListWidgetItem* item)
{
    if(item)
    {
	QPoint pos = qvariant_cast<QPoint>(item->data(Qt::UserRole));
        const QSize & ts = EditorTheme::tileSize();
	centerOn(pos.x() * ts.width(), pos.y() * ts.height());
    }
}

void MapWindow::updateWindowPos(void)
{
    emit windowPosChanged(QRect(QPoint(horizontalScrollBar()->value(), verticalScrollBar()->value()), size()));
}
