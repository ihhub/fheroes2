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

#ifndef _EDITOR_MAPWINDOW_H_
#define _EDITOR_MAPWINDOW_H_

#include <QGraphicsView>
#include "mapdata.h"

QT_BEGIN_NAMESPACE
class QListWidgetItem;
QT_END_NAMESPACE

class MainWindow;

class MapWindow : public QGraphicsView
{
    Q_OBJECT

public:
    MapWindow(MainWindow*);

    void	newFile(const QSize &, int, int);
    bool	loadFile(const QString &);
    bool	save(void);
    bool	saveAs(void);
    bool	saveRaw(void);
    bool	saveFile(const QString &, bool);
    QString	userFriendlyCurrentFile(void);
    QString	currentFile(void);

signals:
    void	windowModified(MapData*);
    void	windowPosChanged(const QRect &);

protected:
    void	closeEvent(QCloseEvent*);

public slots:
    void	mapWasModified(void);

private slots:
    void	viewportSetPositionFromMiniMap(const QPoint &);
    void	viewportSetPositionFromListWidget(QListWidgetItem*);
    void	updateWindowPos(void);

private:
    bool	maybeSave(void);
    void	setCurrentFile(const QString &);
    QString	strippedName(const QString &);

    QString	curFile;
    bool	isUntitled;
    bool	isModified;

    MapData	mapData;
};

#endif
