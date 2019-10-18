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

#ifndef _EDITOR_WINDOW_H_
#define _EDITOR_WINDOW_H_

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QActionGroup;
class QAction;
class QLabel;
class QMenu;
class QMdiArea;
class QMdiSubWindow;
class QSignalMapper;
class QDockWidget;
class QListWidgetItem;
QT_END_NAMESPACE

namespace Form { class MiniMap; class TownList; class HeroList; class CustomList; class InfoForm; }
class MapWindow;
class MapTile;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

protected:
    void		closeEvent(QCloseEvent*);

private slots:
    void		newFile(void);
    void		open(void);
    void		save(void);
    void		saveRaw(void);
    void		saveAs(void);
    void		about(void);
    void		updateWindowMenu(void);
    MapWindow*		createMapWindow(void);
    void		setActiveSubWindow(QWidget*);
    void		subWindowActivated(QMdiSubWindow*);
    void		setCoordInfo(const MapTile*);
    void		updateInfoForm(QListWidgetItem*);

private:
    void		createActions(void);
    void		createMenus(void);
    void		createToolBars(void);
    void		createStatusBar(void);
    void		readSettings(void);
    void		writeSettings(void);

    void		updateMenus(void);
    void		updateStatusBar(void);
    void		updateDockWidgets(void);

    MapWindow*		activeMapWindow(void);
    QMdiSubWindow*	findMapWindow(const QString &);

    QMdiArea*		mdiArea;
    QSignalMapper*	windowMapper;

    QMenu*		fileMenu;
    QMenu*		mapMenu;
    QMenu*		windowMenu;
    QToolBar*		fileToolBar;
    QAction*		fileNewAct;
    QAction*		fileOpenAct;
    QAction*		fileSaveAct;
    QAction*		fileSaveAsAct;
    QAction*		fileSaveRawAct;
    QAction*		fileExitAct;
    QAction*		editCopyAct;
    QAction*		editPasteAct;
    QAction*		closeAct;
    QAction*		closeAllAct;
    QAction*		tileAct;
    QAction*		cascadeAct;
    QAction*		nextAct;
    QAction*		previousAct;
    QAction*		separatorAct;
    QAction*		menuAboutAct;

    QAction*		mapOptionsAct;
    QAction*		showPassableAct;

    QDockWidget*	dockMiniMap;
    QDockWidget*	dockTownList;
    QDockWidget*	dockHeroList;
    QDockWidget*	dockObjectsList;
    QDockWidget*	dockInfoWidget;

    QLabel*		labelTileX;
    QLabel*		labelTileY;

    Form::MiniMap*      miniMap;
    Form::TownList*     townList;
    Form::HeroList*     heroList;
    Form::CustomList*	objectsList;
    Form::InfoForm*     infoForm;

    int			sequenceMapNumber;
};

#endif
