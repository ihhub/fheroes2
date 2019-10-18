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

#include <QCoreApplication>
#include <QProcessEnvironment>
#include <QRegExp>
#include <QDebug>
#include <QDir>
#include <QtGui>
#include <QTextStream>
#include <ctime>
#include <QtWidgets>

#include "engine.h"
#include "mainwindow.h"
#include "dialogs.h"
#include "program.h"

#define PROGRAM_NAME "fh2editor"

struct ProgramShareEnv : QRegExp
{
    ProgramShareEnv() : QRegExp(QString(PROGRAM_NAME).toUpper() + "=(.+)"){}

    bool operator() (const QString & str) const { return exactMatch(str); }
};

namespace Resource
{
    QStringList shares;
    QSettings   settings(QSettings::IniFormat, QSettings::UserScope, PROGRAM_NAME, "settings");

    QSettings & localSettings(void)
    {
        return settings;
    }

    const QStringList & ShareDirs(void)
    {
	return shares;
    }

    QString FindFile(const QString & dir, const QString & file)
    {
	for(QStringList::const_iterator
	    it = shares.begin(); it != shares.end(); ++it)
	{
	    const QString path = QDir::toNativeSeparators(*it + QDir::separator() + dir + QDir::separator() + file);
	    if(QFileInfo(path).isReadable()) return path;
	}

	return NULL;
    }

    QStringList FindFiles(const QString & dir, const QString & file)
    {
	QStringList res;

	for(QStringList::const_iterator
	    it = shares.begin(); it != shares.end(); ++it)
	{
	    QDir fullDir(QDir::toNativeSeparators(*it + QDir::separator() + dir));
	    QStringList files = fullDir.entryList(QStringList() << file, QDir::Files | QDir::Readable);

	    for(QStringList::const_iterator
		ft = files.begin(); ft != files.end(); ++ft)
		res << QDir::toNativeSeparators(fullDir.path() + QDir::separator() + *ft);
	}

	return res;
    }

    void InitShares(void)
    {
	QStringList list;

#ifdef BUILD_PROGRAM_NAME
	list.push_back(QDir::toNativeSeparators(QString(BUILD_PROGRAM_NAME)));
#endif

	const QStringList & envs = QProcess::systemEnvironment();
	struct ProgramShareEnv regExp;

	for(QStringList::const_iterator
	    it = envs.begin(); it != envs.end(); ++it)
	if(regExp.exactMatch(*it)){ list.push_back(QDir::toNativeSeparators(regExp.cap(1))); break; }

	list.push_back(QDir::toNativeSeparators(QCoreApplication::applicationDirPath()));
	list.push_back(QDir::toNativeSeparators(QDir::homePath() + QDir::separator() + "." + QString(PROGRAM_NAME).toLower()));

	shares.clear();

	for(QStringList::const_iterator
	    it = list.begin(); it != list.end(); ++it)
	{
	    qDebug() << "registry sharedir:" << *it;
    	    shares.push_front(*it);
	}
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Resource::InitShares();
    qsrand(std::time(0));
    QPixmapCache::setCacheLimit(40000);

    QSettings & settings = Resource::localSettings();
    QString dataFile = settings.value("Main/dataFile", "").toString();

    if(! QFile(dataFile).exists())
    {
	const QString resourceAgg = "HEROES2.AGG";
	const QString subFolder = "data";

	dataFile = Resource::FindFile(subFolder, resourceAgg);

	if(dataFile.isEmpty())
	    dataFile = Resource::FindFile(subFolder, resourceAgg.toLower());

	if(dataFile.isEmpty())
	{
	    QStringList list = Resource::ShareDirs();
	    Form::SelectDataFile form(resourceAgg, list.replaceInStrings(QRegExp("$"),
					    QDir::toNativeSeparators(QDir::separator() + subFolder)));

	    if(QDialog::Accepted != form.exec())
		return 0;

	    dataFile = form.result;
	}

	settings.setValue("Main/dataFile", dataFile);
    }

    if(! EditorTheme::load(dataFile))
	return -1;

    MainWindow main;
    main.show();

    return app.exec();
}
