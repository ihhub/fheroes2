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

#include <algorithm>
#include <QtGui>
#include <QApplication>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QSpacerItem>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QLineEdit>
#include <QGroupBox>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QTreeWidget>
#include <QListWidget>
#include <QSpinBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QScrollBar>
#include <QRadioButton>
#include <QProxyStyle>

#include "global.h"
#include "program.h"
#include "engine.h"
#include "mapdata.h"
#include "dialogs.h"

QVariant comboBoxCurrentData(const QComboBox* box)
{
    return box->itemData(box->currentIndex());
}

Form::SelectMapSize::SelectMapSize()
{
    setWindowTitle(QApplication::translate("SelectMapSize", "Select Size", 0));

    vboxLayout = new QVBoxLayout(this);
    vboxLayout->setSpacing(6);
    vboxLayout->setMargin(9);

    // select size
    comboBoxSize = new QComboBox(this);
    comboBoxSize->addItem(QApplication::translate("SelectMapSize", "Small (36x36)", 0), 36);
    comboBoxSize->addItem(QApplication::translate("SelectMapSize", "Medium (72x72)", 0), 72);
    comboBoxSize->addItem(QApplication::translate("SelectMapSize", "Large (108x108)", 0), 108);
    comboBoxSize->addItem(QApplication::translate("SelectMapSize", "Extra Large (144x144)", 0), 144);
    comboBoxSize->setCurrentIndex(1);
    vboxLayout->addWidget(comboBoxSize);

    // width
    spinBoxWidth = new QSpinBox(this);
    spinBoxWidth->setMaximum(1024);
    spinBoxWidth->setMinimum(36);
    spinBoxWidth->setSingleStep(2);
    spinBoxWidth->setVisible(false);

    labelWidth = new QLabel(this);
    labelWidth->setEnabled(true);
    labelWidth->setVisible(false);
    labelWidth->setText(QApplication::translate("SelectMapSize", "width", 0));
    labelWidth->setBuddy(spinBoxWidth);

    spacerItem1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    spacerItem2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    hboxLayout = new QHBoxLayout();
    hboxLayout->setSpacing(6);
    hboxLayout->setMargin(0);
    hboxLayout->addItem(spacerItem1);
    hboxLayout->addWidget(labelWidth);
    hboxLayout->addWidget(spinBoxWidth);
    hboxLayout->addItem(spacerItem2);
    vboxLayout->addLayout(hboxLayout);

    // height
    spinBoxHeight = new QSpinBox(this);
    spinBoxHeight->setMaximum(1024);
    spinBoxHeight->setMinimum(36);
    spinBoxHeight->setSingleStep(2);
    spinBoxHeight->setVisible(false);

    labelHeight = new QLabel(this);
    labelHeight->setText(QApplication::translate("SelectMapSize", "height", 0));
    labelHeight->setVisible(false);
    labelHeight->setBuddy(spinBoxHeight);

    spacerItem3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    spacerItem4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    hboxLayout1 = new QHBoxLayout();
    hboxLayout1->setSpacing(6);
    hboxLayout1->setMargin(0);
    hboxLayout1->addItem(spacerItem3);
    hboxLayout1->addWidget(labelHeight);
    hboxLayout1->addWidget(spinBoxHeight);

    hboxLayout1->addItem(spacerItem4);
    vboxLayout->addLayout(hboxLayout1);

    // generate map
    checkBoxGenerateMap = new QCheckBox(this);
    checkBoxGenerateMap->setText(QApplication::translate("SelectMapSize", "Generate map", 0));
    vboxLayout->addWidget(checkBoxGenerateMap);

    labelArea = new QLabel(this);
    labelArea->setText(QApplication::translate("SelectMapSize", "Area size", 0));
    labelArea->setVisible(false);
    vboxLayout->addWidget(labelArea);

    sliderAreaSize = new QSlider(this);
    sliderAreaSize->setMinimum(16);
    sliderAreaSize->setMaximum(64);
    sliderAreaSize->setSingleStep(16);
    sliderAreaSize->setValue(32);
    sliderAreaSize->setOrientation(Qt::Horizontal);
    sliderAreaSize->setVisible(false);
    vboxLayout->addWidget(sliderAreaSize);

    //
    spacerItem5 = new QSpacerItem(20, 16, QSizePolicy::Minimum, QSizePolicy::Expanding);
    vboxLayout->addItem(spacerItem5);

    hboxLayout2 = new QHBoxLayout();
    hboxLayout2->setSpacing(6);
    hboxLayout2->setMargin(0);

    spacerItem6 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    hboxLayout2->addItem(spacerItem6);

    pushButtonOk = new QPushButton(this);
    pushButtonOk->setText(QApplication::translate("SelectMapSize", "Ok", 0));
    hboxLayout2->addWidget(pushButtonOk);

    spacerItem7 = new QSpacerItem(61, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    hboxLayout2->addItem(spacerItem7);

    pushButtonExpert = new QPushButton(this);
    pushButtonExpert->setText(QApplication::translate("SelectMapSize", "Expert", 0));
    hboxLayout2->addWidget(pushButtonExpert);

    spacerItem8 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    hboxLayout2->addItem(spacerItem8);
    vboxLayout->addLayout(hboxLayout2);

    QSize minSize = minimumSizeHint();

    resize(minSize);
    setFixedSize(minSize);

    connect(pushButtonExpert, SIGNAL(clicked()), this, SLOT(clickExpert()));
    connect(pushButtonOk, SIGNAL(clicked()), this, SLOT(clickOk()));
    connect(checkBoxGenerateMap, SIGNAL(clicked()), this, SLOT(generateWidgetVisible()));
}

void Form::SelectMapSize::generateWidgetVisible(void)
{
    labelArea->setVisible(checkBoxGenerateMap->isChecked());
    sliderAreaSize->setVisible(checkBoxGenerateMap->isChecked());
    resize(minimumSizeHint());
}

void Form::SelectMapSize::clickExpert(void)
{
    // switch to expert
    if(comboBoxSize->isVisible())
    {
	comboBoxSize->hide();
	labelWidth->show();
	spinBoxWidth->show();
	labelHeight->show();
	spinBoxHeight->show();

	pushButtonExpert->setText("Simple");

	resize(minimumSizeHint());
    }
    else
    {
	labelWidth->hide();
	spinBoxWidth->hide();
	labelHeight->hide();
	spinBoxHeight->hide();
	comboBoxSize->show();

	pushButtonExpert->setText("Expert");

	resize(minimumSizeHint());
    }
}

void Form::SelectMapSize::clickOk(void)
{
    if(comboBoxSize->isVisible())
    {
	int size = qvariant_cast<int>(comboBoxCurrentData(comboBoxSize));
	result = QSize(size, size);
    }
    else
    {
	result = QSize(spinBoxWidth->value(), spinBoxHeight->value());
    }

    accept();
}

Form::SelectDataFile::SelectDataFile(const QString & dataFile, const QStringList & dirList)
{
    setWindowTitle(QApplication::translate("DialogSelectDataFile", "Warning", 0));

    verticalLayout = new QVBoxLayout(this);

    labelHeader = new QLabel(this);
    labelHeader->setAlignment(Qt::AlignCenter);
    labelHeader->setText(QApplication::translate("DialogSelectDataFile", "Cannot find resource file: ", 0) + dataFile);
    verticalLayout->addWidget(labelHeader);

    horizontalLayout2 = new QHBoxLayout();

    labelImage = new QLabel(this);
    labelImage->setPixmap(QPixmap(QString::fromUtf8(":/images/cancel.png")));
    labelImage->setScaledContents(false);
    horizontalLayout2->addWidget(labelImage);

    labelBody = new QLabel(this);
    labelBody->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
    labelBody->setText(QApplication::translate("DialogSelectDataFile", "Scan directories: ", 0) + "\n" + dirList.join("\n"));
    horizontalLayout2->addWidget(labelBody);
    verticalLayout->addLayout(horizontalLayout2);

    verticalSpacer = new QSpacerItem(288, 6, QSizePolicy::Minimum, QSizePolicy::Expanding);
    verticalLayout->addItem(verticalSpacer);

    horizontalLayout1 = new QHBoxLayout();

    pushButtonSelect = new QPushButton(this);
    pushButtonSelect->setText(QApplication::translate("DialogSelectDataFile", "Select", 0));
    horizontalLayout1->addWidget(pushButtonSelect);

    pushButtonSave = new QPushButton(this);
    pushButtonSave->setText(QApplication::translate("DialogSelectDataFile", "Save", 0));
    pushButtonSave->setEnabled(false);
    horizontalLayout1->addWidget(pushButtonSave);

    horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    horizontalLayout1->addItem(horizontalSpacer);

    pushButtonExit = new QPushButton(this);
    pushButtonExit->setText(QApplication::translate("DialogSelectDataFile", "Exit", 0));
    horizontalLayout1->addWidget(pushButtonExit);
    verticalLayout->addLayout(horizontalLayout1);

    QSize minSize = minimumSizeHint();

    resize(minSize);
    setFixedSize(minSize);

    connect(pushButtonExit, SIGNAL(clicked()), this, SLOT(reject()));
    connect(pushButtonSave, SIGNAL(clicked()), this, SLOT(accept()));
    connect(pushButtonSelect, SIGNAL(clicked()), this, SLOT(clickSelect()));
}

void Form::SelectDataFile::clickSelect(void)
{
    result = QFileDialog::getOpenFileName(this, tr("Open data file"), "", "heroes2.agg");
    pushButtonSave->setEnabled(true);
}

class SelectImageItem : public QListWidgetItem
{
public:
    SelectImageItem(const CompositeObject & obj, const QMap<int, QString> & ids)
    {
	setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	setData(Qt::UserRole, QVariant::fromValue(obj));
	setIcon(EditorTheme::getImage(obj));
	setText(obj.name);
#ifndef QT_NO_TOOLTIP
	setToolTip(QString("class id: ") + (ids[obj.classId].isEmpty() ? QString("unknown") : ids[obj.classId]));
#endif
	setSizeHint(QSize(132, 80));
    }
};

Form::SelectImageTab::SelectImageTab(const QDomElement & groupElem, const QString & dataFolder)
{
    verticalLayout = new QVBoxLayout(this);
    listWidget = new QListWidget(this);
    verticalLayout->addWidget(listWidget);
    listWidget->setIconSize(QSize(64, 64));
    listWidget->setViewMode(QListView::IconMode);
    listWidget->setWrapping(true);
    listWidget->setResizeMode(QListView::Adjust);

    QMap<int, QString> objectsID;

    for(int index = 0; index < 0x80; ++index)
	objectsID[index] = MapObj::transcribe(index);

    Editor::MyObjectsXML objectsElem(EditorTheme::resourceFile(dataFolder, groupElem.attribute("file")));

    for(Editor::MyObjectsXML::const_iterator
	it = objectsElem.begin(); it != objectsElem.end(); ++it)
    {
        CompositeObject obj(*it);

        if(obj.isValid())
	    listWidget->addItem(new SelectImageItem(obj, objectsID));
    }
}

bool Form::SelectImageTab::isSelected(void) const
{
    return listWidget->selectedItems().size();
}

class TabWidgetWestStyle : public QProxyStyle
{
public:
    QSize sizeFromContents(ContentsType type, const QStyleOption* option, const QSize & size, const QWidget* widget) const
    {
	QSize res = QProxyStyle::sizeFromContents(type, option, size, widget);

	if(type == QStyle::CT_TabBarTab)
	    res.transpose();

	return res;
    }
 
    void drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
    {
	const QStyleOptionTab* tab = qstyleoption_cast<const QStyleOptionTab*>(option);

	if(element == CE_TabBarTabLabel && tab)
	{
	    QStyleOptionTab opt(*tab);
	    opt.shape = QTabBar::RoundedNorth;
	    QProxyStyle::drawControl(element, &opt, painter, widget);
	}
	else
	    QProxyStyle::drawControl(element, option, painter, widget);
    }
};

Form::TabWidgetWest::TabWidgetWest(QWidget* parent) : QTabWidget(parent)
{
    setTabPosition(QTabWidget::West);
    tabBar()->setStyle(new TabWidgetWestStyle);
    tabBar()->setShape(QTabBar::RoundedWest);
}

Form::SelectImageObject::SelectImageObject()
{
    setWindowTitle(QApplication::translate("SelectImage", "Select Object", 0));

    tabWidget = new TabWidgetWest(this);

    const QString dataFolder("objects");
    Editor::MyXML groupsElem(EditorTheme::resourceFile(dataFolder, "groups.xml"), "groups");

    if(! groupsElem.isNull())
    {
        QDomNodeList groupsList = groupsElem.elementsByTagName("group");

        for(int pos1 = 0; pos1 < groupsList.size(); ++pos1)
        {
            QDomElement groupElem = groupsList.item(pos1).toElement();
            QString name = groupElem.attribute("name");

            if(! name.isEmpty())
		tabWidget->addTab(new SelectImageTab(groupElem, dataFolder), name);
	}

	tabWidget->setCurrentIndex(0);
    }

    verticalLayout = new QVBoxLayout(this);
    verticalLayout->addWidget(tabWidget);

    pushButtonSelect = new QPushButton(this);
    pushButtonSelect->setText(QApplication::translate("SelectImage", "Select", 0));
    pushButtonSelect->setEnabled(false);

    horizontalLayout = new QHBoxLayout();
    horizontalLayout->addWidget(pushButtonSelect);

    horizontalSpacer = new QSpacerItem(268, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    horizontalLayout->addItem(horizontalSpacer);

    pushButtonClose = new QPushButton(this);
    pushButtonClose->setText(QApplication::translate("SelectImage", "Close", 0));

    horizontalLayout->addWidget(pushButtonClose);
    verticalLayout->addLayout(horizontalLayout);

    // set size
    QSettings & settings = Resource::localSettings();
    setMinimumSize(QSize(550, 410));
    resize(settings.value("SelectImageDialog/size", minimumSize()).toSize());

    tabSwitched(settings.value("SelectImageDialog/lastTab", 0).toInt());

    connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabSwitched(int)));
    connect(pushButtonClose, SIGNAL(clicked()), this, SLOT(reject()));
    connect(pushButtonSelect, SIGNAL(clicked()), this, SLOT(clickSelect()));

    connect(this, SIGNAL(finished(int)), this, SLOT(saveSettings()));
}

void Form::SelectImageObject::saveSettings(void)
{
    QSettings & settings = Resource::localSettings();
    settings.setValue("SelectImageDialog/size", size());
    settings.setValue("SelectImageDialog/lastTab", tabWidget->currentIndex());
}

void Form::SelectImageObject::tabSwitched(int num)
{
    tabWidget->setCurrentIndex(num);

    SelectImageTab* tab = qobject_cast<SelectImageTab*>(tabWidget->widget(num));

    disconnect(this, SLOT(accept(QListWidgetItem*)));
    disconnect(this, SLOT(selectionChanged()));

    if(tab)
    {
	pushButtonSelect->setEnabled(tab->isSelected());

	connect(tab->listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(accept(QListWidgetItem*)));
	connect(tab->listWidget, SIGNAL(itemSelectionChanged()), this, SLOT(selectionChanged()));
    }
}

void Form::SelectImageObject::selectionChanged(void)
{
    SelectImageTab* tab = qobject_cast<SelectImageTab*>(tabWidget->currentWidget());

    if(tab)
	pushButtonSelect->setEnabled(tab->isSelected());
}


void Form::SelectImageObject::clickSelect(void)
{
    SelectImageTab* tab = qobject_cast<SelectImageTab*>(tabWidget->currentWidget());

    if(tab)
	accept(tab->listWidget->currentItem());
}

void Form::SelectImageObject::accept(QListWidgetItem* item)
{
    if(item)
    {
	result = qvariant_cast<CompositeObject>(item->data(Qt::UserRole));
	QDialog::accept();
    }
}

void fillComboBox(QComboBox & box, const ListStringPos & list)
{
    box.clear();
    for(ListStringPos::const_iterator
	it = list.begin(); it != list.end(); ++it)
    {
	QString str; QTextStream ts(&str);
	ts << (*it).first << " - " << "(" << (*it).second.x() << ", " << (*it).second.y() << ")";
	box.addItem(str, (*it).second);
    }
}

Form::PlayerStatus::PlayerStatus(int c, int v, QWidget* parent) : QLabel(parent), col(c), stat(v)
{
    updatePlayers();
}

void Form::PlayerStatus::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    stat += 1;
    if(0 == (stat % 4)) stat += 1;
    updatePlayers();
    emit mousePressed();
}

void Form::PlayerStatus::updatePlayers(void)
{
    switch(col)
    {
	case Color::Blue:	setPixmap(EditorTheme::getImageICN("CELLWIN.ICN", 19 + status()).first); break;
    	case Color::Green:	setPixmap(EditorTheme::getImageICN("CELLWIN.ICN", 23 + status()).first); break;
    	case Color::Red:	setPixmap(EditorTheme::getImageICN("CELLWIN.ICN", 27 + status()).first); break;
    	case Color::Yellow:	setPixmap(EditorTheme::getImageICN("CELLWIN.ICN", 31 + status()).first); break;
    	case Color::Orange:	setPixmap(EditorTheme::getImageICN("CELLWIN.ICN", 35 + status()).first); break;
    	case Color::Purple:	setPixmap(EditorTheme::getImageICN("CELLWIN.ICN", 39 + status()).first); break;
    	default: break;
    }
}

Form::MapOptions::MapOptions(MapData & map)
{
    QSettings & settings = Resource::localSettings();
    setWindowTitle(QApplication::translate("MapOptions", "Map Options", 0));

    /* tab info block */
    tabInfo = new QWidget();
    verticalLayout = new QVBoxLayout(tabInfo);

    labelName = new QLabel(tabInfo);
    labelName->setAlignment(Qt::AlignCenter);
    labelName->setText(QApplication::translate("MapOptions", "Map Name:", 0));

    lineEditName = new QLineEdit(tabInfo);
    lineEditName->setText(map.name());

    labelDifficulty = new QLabel(tabInfo);
    labelDifficulty->setAlignment(Qt::AlignCenter);
    labelDifficulty->setText(QApplication::translate("MapOptions", "Map Difficulty:", 0));

    comboBoxDifficulty = new QComboBox(tabInfo);
    comboBoxDifficulty->addItem(QApplication::translate("MapOptions", "Easy", 0), Difficulty::Easy);
    comboBoxDifficulty->addItem(QApplication::translate("MapOptions", "Normal", 0), Difficulty::Normal);
    comboBoxDifficulty->addItem(QApplication::translate("MapOptions", "Tough", 0), Difficulty::Tough);
    comboBoxDifficulty->addItem(QApplication::translate("MapOptions", "Expert", 0), Difficulty::Expert);
    comboBoxDifficulty->setCurrentIndex(map.difficulty());

    labelDescription = new QLabel(tabInfo);
    labelDescription->setAlignment(Qt::AlignCenter);
    labelDescription->setText(QApplication::translate("MapOptions", "Map Description:", 0));

    plainTextEditDescription = new QPlainTextEdit(tabInfo);
    plainTextEditDescription->setPlainText(map.description());

    verticalLayout->addWidget(labelName);
    verticalLayout->addWidget(lineEditName);
    verticalLayout->addWidget(labelDifficulty);
    verticalLayout->addWidget(comboBoxDifficulty);
    verticalLayout->addWidget(labelDescription);
    verticalLayout->addWidget(plainTextEditDescription);

    /* tab condition block */
    tabConditions = new QWidget();

    groupBoxWinsCond = new QGroupBox(tabConditions);
    groupBoxWinsCond->setTitle(QApplication::translate("MapOptions", "Victory Condition", 0));

    comboBoxWinsCond = new QComboBox(groupBoxWinsCond);
    comboBoxWinsCond->addItem(QApplication::translate("MapOptions", "Default", 0), Conditions::Wins);
    comboBoxWinsCond->addItem(QApplication::translate("MapOptions", "Capture a particular castle", 0), Conditions::CaptureTown);
    comboBoxWinsCond->addItem(QApplication::translate("MapOptions", "Defeat a particular hero", 0), Conditions::DefeatHero);
    comboBoxWinsCond->addItem(QApplication::translate("MapOptions", "Find a particular artifact", 0), Conditions::FindArtifact);
    comboBoxWinsCond->addItem(QApplication::translate("MapOptions", "One side defeats another", 0), Conditions::SideWins);
    comboBoxWinsCond->addItem(QApplication::translate("MapOptions", "Accumulate gold", 0), Conditions::AccumulateGold);

    comboBoxWinsCondExt = new QComboBox(groupBoxWinsCond);
    comboBoxWinsCondExt->setEnabled(false);

    horizontalLayoutVictorySlct = new QHBoxLayout();
    horizontalLayoutVictorySlct->addWidget(comboBoxWinsCond);
    horizontalLayoutVictorySlct->addWidget(comboBoxWinsCondExt);

    checkBoxAllowNormalVictory = new QCheckBox(groupBoxWinsCond);
    checkBoxAllowNormalVictory->setEnabled(false);
    checkBoxAllowNormalVictory->setText(QApplication::translate("MapOptions", "Allow normal victory condition", 0));

    checkBoxCompAlsoWins = new QCheckBox(groupBoxWinsCond);
    checkBoxCompAlsoWins->setEnabled(false);
    checkBoxCompAlsoWins->setText(QApplication::translate("MapOptions", "Comp also wins via Special VC", 0));

    horizontalLayoutVictoryCheck = new QHBoxLayout();
    horizontalLayoutVictoryCheck->addWidget(checkBoxAllowNormalVictory);
    horizontalLayoutVictoryCheck->addWidget(checkBoxCompAlsoWins);

    verticalLayout3 = new QVBoxLayout(groupBoxWinsCond);
    verticalLayout3->addLayout(horizontalLayoutVictorySlct);
    verticalLayout3->addLayout(horizontalLayoutVictoryCheck);

    groupBoxLossCond = new QGroupBox(tabConditions);
    groupBoxLossCond->setTitle(QApplication::translate("MapOptions", "Loss Condition", 0));

    comboBoxLossCond = new QComboBox(groupBoxLossCond);
    comboBoxLossCond->addItem(QApplication::translate("MapOptions", "Default", 0), Conditions::Loss);
    comboBoxLossCond->addItem(QApplication::translate("MapOptions", "Lose a particuclar castle", 0), Conditions::LoseTown);
    comboBoxLossCond->addItem(QApplication::translate("MapOptions", "Lose a particular hero", 0), Conditions::LoseHero);
    comboBoxLossCond->addItem(QApplication::translate("MapOptions", "Run out of time", 0), Conditions::OutTime);

    horizontalLayoutLossCond = new QHBoxLayout();
    horizontalLayoutLossCond->addWidget(comboBoxLossCond);

    comboBoxLossCondExt = new QComboBox(groupBoxLossCond);
    comboBoxLossCondExt->setEnabled(false);

    horizontalLayoutLossCond = new QHBoxLayout();
    horizontalLayoutLossCond->addWidget(comboBoxLossCond);
    horizontalLayoutLossCond->addWidget(comboBoxLossCondExt);

    verticalLayout4 = new QVBoxLayout(groupBoxLossCond);
    verticalLayout4->addLayout(horizontalLayoutLossCond);
    groupBoxPlayers = new QGroupBox(tabConditions);
    groupBoxPlayers->setTitle(QApplication::translate("MapOptions", "Players", 0));

    horizontalSpacerPlayersLeft = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    horizontalSpacerPlayersRight = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    verticalSpacerPage2 = new QSpacerItem(20, 17, QSizePolicy::Minimum, QSizePolicy::Expanding);

    horizontalLayoutPlayers = new QHBoxLayout();
    horizontalLayoutPlayers->addItem(horizontalSpacerPlayersLeft);

    // create players labels
    QVector<int> colors = Color::colors(Color::All);

    for(QVector<int>::const_iterator
	it = colors.begin(); it != colors.end(); ++it)
    {
	int stat = 0; /* 0: n/a, 1: human only, 2: comp only, 3: comp or human */
	if((*it) & map.kingdomColors())
	{
	    if(((*it) & map.computerColors()) &&
		((*it) & map.humanColors()))
		stat = 3;
	    else
	    if((*it) & map.humanColors())
		stat = 1;
	    else
	    if((*it) & map.computerColors())
		stat = 2;
	}
        labelPlayers.push_back(new PlayerStatus(*it, stat, groupBoxPlayers));
	labelPlayers.back()->setEnabled((*it) & map.kingdomColors());
        horizontalLayoutPlayers->addWidget(labelPlayers.back());
    }

    horizontalLayoutPlayers->addItem(horizontalSpacerPlayersRight);

    checkBoxStartWithHero = new QCheckBox(groupBoxPlayers);
    checkBoxStartWithHero->setText(QApplication::translate("MapOptions", "Start with hero in each player's main castle", 0));
    checkBoxStartWithHero->setChecked(map.startWithHero());

    verticalLayout5 = new QVBoxLayout(groupBoxPlayers);
    verticalLayout5->addLayout(horizontalLayoutPlayers);
    verticalLayout5->addWidget(checkBoxStartWithHero);

    verticalLayout6 = new QVBoxLayout(tabConditions);
    verticalLayout6->addWidget(groupBoxWinsCond);
    verticalLayout6->addWidget(groupBoxLossCond);
    verticalLayout6->addWidget(groupBoxPlayers);
    verticalLayout6->addItem(verticalSpacerPage2);

    /* tab rumors/events block */
    tabRumorsEvents = new QWidget();

    groupBoxRumors = new QGroupBox(tabRumorsEvents);
    groupBoxRumors->setTitle(QApplication::translate("MapOptions", "Rumors", 0));

    groupBoxEvents = new QGroupBox(tabRumorsEvents);
    groupBoxEvents->setTitle(QApplication::translate("MapOptions", "Events", 0));

    listWidgetRumors = new RumorsList(groupBoxRumors);
    listWidgetEvents = new DayEventsList(map.kingdomColors(), groupBoxEvents);

    verticalLayout7 = new QVBoxLayout(groupBoxRumors);
    verticalLayout7->addWidget(listWidgetRumors);

    verticalLayout8 = new QVBoxLayout(groupBoxEvents);
    verticalLayout8->addWidget(listWidgetEvents);

    horizontalLayout6 = new QHBoxLayout(tabRumorsEvents);
    horizontalLayout6->addWidget(groupBoxRumors);
    horizontalLayout6->addWidget(groupBoxEvents);

    /* tab: default values */
/*
    tabDefaults = new QWidget();
    verticalLayoutDefaults = new QVBoxLayout(tabDefaults);

    spinBoxResourceGoldMin = new QSpinBox(tabDefaults);
    spinBoxResourceGoldMin->setMaximum(1000);
    spinBoxResourceGoldMin->setMinimum(100);
    spinBoxResourceGoldMin->setSingleStep(100);
    spinBoxResourceGoldMin->setValue(Default::resourceGoldMin());

    spinBoxResourceGoldMax = new QSpinBox(tabDefaults);
    spinBoxResourceGoldMax->setMaximum(5000);
    spinBoxResourceGoldMax->setMinimum(300);
    spinBoxResourceGoldMax->setSingleStep(100);
    spinBoxResourceGoldMax->setValue(Default::resourceGoldMax());

    labelResourceGold = new QLabel(this);
    labelResourceGold->setText(QApplication::translate("MapOptions", "Resource (gold)", 0));

    horizontalLayoutResourceGold = new QHBoxLayout();
    horizontalLayoutResourceGold->addWidget(labelResourceGold);
    horizontalLayoutResourceGold->addWidget(spinBoxResourceGoldMin);
    horizontalLayoutResourceGold->addWidget(spinBoxResourceGoldMax);

    spinBoxResourceWoodOreMin = new QSpinBox(tabDefaults);
    spinBoxResourceWoodOreMin->setMaximum(10);
    spinBoxResourceWoodOreMin->setMinimum(1);
    spinBoxResourceWoodOreMin->setSingleStep(1);
    spinBoxResourceWoodOreMin->setValue(Default::resourceWoodOreMin());

    spinBoxResourceWoodOreMax = new QSpinBox(tabDefaults);
    spinBoxResourceWoodOreMax->setMaximum(50);
    spinBoxResourceWoodOreMax->setMinimum(3);
    spinBoxResourceWoodOreMax->setSingleStep(1);
    spinBoxResourceWoodOreMax->setValue(Default::resourceWoodOreMax());

    labelResourceWoodOre = new QLabel(this);
    labelResourceWoodOre->setText(QApplication::translate("MapOptions", "Resource (wood, ore)", 0));

    horizontalLayoutResourceWoodOre = new QHBoxLayout();
    horizontalLayoutResourceWoodOre->addWidget(labelResourceWoodOre);
    horizontalLayoutResourceWoodOre->addWidget(spinBoxResourceWoodOreMin);
    horizontalLayoutResourceWoodOre->addWidget(spinBoxResourceWoodOreMax);

    spinBoxResourceOtherMin = new QSpinBox(tabDefaults);
    spinBoxResourceOtherMin->setMaximum(10);
    spinBoxResourceOtherMin->setMinimum(1);
    spinBoxResourceOtherMin->setSingleStep(1);
    spinBoxResourceOtherMin->setValue(Default::resourceOtherMin());

    spinBoxResourceOtherMax = new QSpinBox(tabDefaults);
    spinBoxResourceOtherMax->setMaximum(30);
    spinBoxResourceOtherMax->setMinimum(3);
    spinBoxResourceOtherMax->setSingleStep(1);
    spinBoxResourceOtherMax->setValue(Default::resourceOtherMax());

    labelResourceOther = new QLabel(this);
    labelResourceOther->setText(QApplication::translate("MapOptions", "Resource (other)", 0));

    horizontalLayoutResourceOther = new QHBoxLayout();
    horizontalLayoutResourceOther->addWidget(labelResourceOther);
    horizontalLayoutResourceOther->addWidget(spinBoxResourceOtherMin);
    horizontalLayoutResourceOther->addWidget(spinBoxResourceOtherMax);

    verticalSpacerDefaults = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayoutDefaults->addLayout(horizontalLayoutResourceGold);
    verticalLayoutDefaults->addLayout(horizontalLayoutResourceWoodOre);
    verticalLayoutDefaults->addLayout(horizontalLayoutResourceOther);
    verticalLayoutDefaults->addItem(verticalSpacerDefaults);
*/
    /* tab: authors/license */
    tabAuthorsLicense = new QWidget();
    verticalLayout9 = new QVBoxLayout(tabAuthorsLicense);

    labelAuthors = new QLabel(tabAuthorsLicense);
    labelAuthors->setAlignment(Qt::AlignCenter);
    labelAuthors->setText(QApplication::translate("MapOptions", "Authors:", 0));

    plainTextEditAuthors = new QPlainTextEdit(tabAuthorsLicense);
    plainTextEditAuthors->setPlainText(map.authors());

    labelLicense = new QLabel(tabAuthorsLicense);
    labelLicense->setAlignment(Qt::AlignCenter);
    labelLicense->setText(QApplication::translate("MapOptions", "License:", 0));

    plainTextEditLicense = new QPlainTextEdit(tabAuthorsLicense);
    plainTextEditLicense->setPlainText(map.license());

    verticalLayout9->addWidget(labelAuthors);
    verticalLayout9->addWidget(plainTextEditAuthors);
    verticalLayout9->addWidget(labelLicense);
    verticalLayout9->addWidget(plainTextEditLicense);

    /* end */
    tabWidget = new QTabWidget(this);
    tabWidget->addTab(tabInfo, QApplication::translate("MapOptions", "General Info", 0));
    tabWidget->addTab(tabConditions, QApplication::translate("MapOptions", "Wins/Loss Condition", 0));
    tabWidget->addTab(tabRumorsEvents, QApplication::translate("MapOptions", "Rumors and Events", 0));
    //tabWidget->addTab(tabDefaults, QApplication::translate("MapOptions", "Default Values", 0));
    tabWidget->addTab(tabAuthorsLicense, QApplication::translate("MapOptions", "Authors and License", 0));

    pushButtonSave = new QPushButton(this);
    pushButtonSave->setText(QApplication::translate("MapOptions", "Save", 0));

    pushButtonCancel = new QPushButton(this);
    pushButtonCancel->setText(QApplication::translate("MapOptions", "Cancel", 0));
    pushButtonSave->setEnabled(false);

    horizontalLayoutButton = new QHBoxLayout();
    horizontalLayoutButton->addWidget(pushButtonSave);
    horizontalSpacerButton = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    horizontalLayoutButton->addItem(horizontalSpacerButton);
    horizontalLayoutButton->addWidget(pushButtonCancel);

    verticalLayout2 = new QVBoxLayout(this);
    verticalLayout2->addWidget(tabWidget);
    verticalLayout2->addLayout(horizontalLayoutButton);

    // contents test
    winsCondHeroList << map.conditionHeroList(Conditions::Wins);
    winsCondTownList << map.conditionTownList(Conditions::Wins);
    winsCondArtifactList << map.conditionArtifactList();
    winsCondSideList << map.conditionSideList();
    lossCondHeroList << map.conditionHeroList(Conditions::Loss);
    lossCondTownList << map.conditionTownList(Conditions::Loss);

    listWidgetRumors->addItems(map.tavernRumorsList());

    for(DayEvents::const_iterator
        it = map.dayEvents().begin(); it != map.dayEvents().end(); ++it)
    {
	QListWidgetItem* item = new QListWidgetItem((*it).header());
	item->setData(Qt::UserRole, QVariant::fromValue(*it));
	static_cast<QListWidget*>(listWidgetEvents)->addItem(item);
    }
    listWidgetEvents->sortItems();

    setConditionsBoxesMapValues(map);

    // set size
    setMinimumSize(minimumSizeHint());
    resize(settings.value("MapOptions/size", minimumSize()).toSize());

    connect(lineEditName, SIGNAL(textChanged(const QString &)), this, SLOT(setEnableSaveButton(const QString &)));
    connect(plainTextEditDescription, SIGNAL(textChanged()), this, SLOT(setEnableSaveButton()));
    connect(comboBoxDifficulty, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(setEnableSaveButton(const QString &)));

    connect(comboBoxWinsCond, SIGNAL(currentIndexChanged(int)), this, SLOT(winsConditionsSelected(int)));
    connect(comboBoxLossCond, SIGNAL(currentIndexChanged(int)), this, SLOT(lossConditionsSelected(int)));

    connect(comboBoxWinsCondExt, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(setEnableSaveButton(const QString &)));
    connect(comboBoxLossCondExt, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(setEnableSaveButton(const QString &)));

    connect(checkBoxAllowNormalVictory, SIGNAL(clicked()), this, SLOT(setEnableSaveButton()));
    connect(checkBoxCompAlsoWins, SIGNAL(clicked()), this, SLOT(setEnableSaveButton()));
    connect(checkBoxStartWithHero, SIGNAL(clicked()), this, SLOT(setEnableSaveButton()));

    connect(listWidgetRumors, SIGNAL(mousePressed()), listWidgetEvents, SLOT(clearSelection()));
    connect(listWidgetRumors, SIGNAL(listChanged()), this, SLOT(setEnableSaveButton()));

    connect(listWidgetEvents, SIGNAL(mousePressed()), listWidgetRumors, SLOT(clearSelection()));
    connect(listWidgetEvents, SIGNAL(listChanged()), this, SLOT(setEnableSaveButton()));

/*
    connect(spinBoxResourceGoldMin, SIGNAL(valueChanged(int)), this, SLOT(setEnableSaveButton()));
    connect(spinBoxResourceGoldMax, SIGNAL(valueChanged(int)), this, SLOT(setEnableSaveButton()));
    connect(spinBoxResourceWoodOreMin, SIGNAL(valueChanged(int)), this, SLOT(setEnableSaveButton()));
    connect(spinBoxResourceWoodOreMax, SIGNAL(valueChanged(int)), this, SLOT(setEnableSaveButton()));
    connect(spinBoxResourceOtherMin, SIGNAL(valueChanged(int)), this, SLOT(setEnableSaveButton()));
    connect(spinBoxResourceOtherMax, SIGNAL(valueChanged(int)), this, SLOT(setEnableSaveButton()));
*/

    connect(plainTextEditAuthors, SIGNAL(textChanged()), this, SLOT(setEnableSaveButton()));
    connect(plainTextEditLicense, SIGNAL(textChanged()), this, SLOT(setEnableSaveButton()));

    for(QVector<PlayerStatus*>::const_iterator
	it = labelPlayers.begin(); it != labelPlayers.end(); ++it)
	connect(*it, SIGNAL(mousePressed()), this, SLOT(setEnableSaveButton()));

    connect(pushButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(pushButtonSave, SIGNAL(clicked()), this, SLOT(accept()));

    connect(this, SIGNAL(finished(int)), this, SLOT(saveSettings()));
}

void Form::MapOptions::saveSettings(void)
{
    QSettings & settings = Resource::localSettings();
    settings.setValue("MapOptions/size", size());
/*
    Default::resourceGoldMin() = spinBoxResourceGoldMin->value();
    Default::resourceGoldMax() = spinBoxResourceGoldMax->value();
    Default::resourceWoodOreMin() = spinBoxResourceWoodOreMin->value();
    Default::resourceWoodOreMax() = spinBoxResourceWoodOreMax->value();
    Default::resourceOtherMin() = spinBoxResourceOtherMin->value();
    Default::resourceOtherMin() = spinBoxResourceOtherMax->value();
*/
}

void Form::MapOptions::setConditionsBoxesMapValues(const MapData & map)
{
    const CondWins & condWins = map.conditionWins();
    const CondLoss & condLoss = map.conditionLoss();

    comboBoxWinsCond->setCurrentIndex(condWins.index());
    comboBoxLossCond->setCurrentIndex(condLoss.index());

    winsConditionsSelected(comboBoxWinsCond->currentIndex());

    switch(condWins.condition())
    {
	case Conditions::CaptureTown:
	    qDebug() << qvariant_cast<QPoint>(condWins.variant());
	    checkBoxAllowNormalVictory->setChecked(condWins.allowNormalVictory());
	    checkBoxCompAlsoWins->setChecked(condWins.compAlsoWins());
	    break;

	case Conditions::DefeatHero:
	    qDebug() << qvariant_cast<QPoint>(condWins.variant());
	    break;

	case Conditions::FindArtifact:
	    checkBoxAllowNormalVictory->setChecked(condWins.allowNormalVictory());
	    qDebug() << qvariant_cast<int>(condWins.variant());
	    break;

	case Conditions::SideWins:
	    qDebug() << qvariant_cast<int>(condWins.variant());
	    break;

	case Conditions::AccumulateGold:
	{
	    checkBoxAllowNormalVictory->setChecked(condWins.allowNormalVictory());
	    checkBoxCompAlsoWins->setChecked(condWins.compAlsoWins());
	    int find = comboBoxWinsCondExt->findData(condWins.variant());
	    if(0 > find)
		comboBoxWinsCondExt->clear();
	    else
		comboBoxWinsCondExt->setCurrentIndex(find);
	}
	    break;

	default: break;
    }

    lossConditionsSelected(comboBoxLossCond->currentIndex());

    switch(condLoss.condition())
    {
	case Conditions::LoseTown:
	case Conditions::LoseHero:
	    qDebug() << qvariant_cast<int>(condLoss.variant());
	    break;

	case Conditions::OutTime:
	{
	    int find = comboBoxLossCondExt->findData(condLoss.variant());
	    if(0 > find)
		comboBoxLossCondExt->clear();
	    else
		comboBoxLossCondExt->setCurrentIndex(find + 1);
	}
	break;

	default: break;
    }

    pushButtonSave->setEnabled(false);
}

void Form::MapOptions::winsConditionsSelected(int index)
{
    comboBoxWinsCondExt->clear();
    comboBoxWinsCondExt->setEnabled(false);
    checkBoxAllowNormalVictory->setEnabled(false);
    checkBoxCompAlsoWins->setEnabled(false);
    checkBoxAllowNormalVictory->setCheckState(Qt::Unchecked);
    checkBoxCompAlsoWins->setCheckState(Qt::Unchecked);

    switch(index)
    {
	// capture castle
	case 1:
	    comboBoxWinsCondExt->setEnabled(true);
	    fillComboBox(*comboBoxWinsCondExt, winsCondTownList);
	    checkBoxAllowNormalVictory->setEnabled(true);
	    checkBoxCompAlsoWins->setEnabled(true);
	    break;

	// defeat hero
	case 2:
	    comboBoxWinsCondExt->setEnabled(true);
	    fillComboBox(*comboBoxWinsCondExt, winsCondHeroList);
	    break;

	// find artifact
	case 3:
	    comboBoxWinsCondExt->setEnabled(true);
	    for(ListStringPos::const_iterator
		it = winsCondArtifactList.begin(); it != winsCondArtifactList.end(); ++it)
		comboBoxWinsCondExt->addItem((*it).first, (*it).second.x());
	    checkBoxAllowNormalVictory->setEnabled(true);
	    break;

	// defeat side
	case 4:
	    comboBoxWinsCondExt->setEnabled(true);
	    for(QList<QString>::const_iterator
		it = winsCondSideList.begin(); it != winsCondSideList.end(); ++it)
		comboBoxWinsCondExt->addItem(*it);
	    break;

	// accumulate gold
	case 5:
	    comboBoxWinsCondExt->setEnabled(true);
	    for(int ii = 50000; ii < 1005000; ii += 50000)
		comboBoxWinsCondExt->addItem(QApplication::translate("MapOptions", "%n golds", 0, ii), ii);
	    checkBoxAllowNormalVictory->setEnabled(true);
	    checkBoxCompAlsoWins->setEnabled(true);
	    break;

	default: break;
    }

    setEnableSaveButton();
}

void Form::MapOptions::lossConditionsSelected(int index)
{
    comboBoxLossCondExt->clear();
    comboBoxLossCondExt->setEnabled(false);

    switch(index)
    {
	// lose castle
	case 1:
	    comboBoxLossCondExt->setEnabled(true);
	    fillComboBox(*comboBoxLossCondExt, lossCondTownList);
	    break;
	// lose chero
	case 2:
	    comboBoxLossCondExt->setEnabled(true);
	    fillComboBox(*comboBoxLossCondExt, lossCondHeroList);
	    break;
	// out of time
	case 3:
	    comboBoxLossCondExt->setEnabled(true);
	    for(int ii = 2; ii < 8; ++ii) // 2-7 days
		comboBoxLossCondExt->addItem(QApplication::translate("MapOptions", "%n days", 0, ii), ii);
	    for(int ii = 2; ii < 9; ++ii) // 2-8 weeks
		comboBoxLossCondExt->addItem(QApplication::translate("MapOptions", "%n weeks", 0, ii), ii * 7);
	    for(int ii = 3; ii < 13; ++ii) // 3-12 months
		comboBoxLossCondExt->addItem(QApplication::translate("MapOptions", "%n months", 0, ii), ii * 7 * 4);
	    break;
	default: break;
    }

    setEnableSaveButton();
}

void Form::MapOptions::setEnableSaveButton(void)
{
    pushButtonSave->setEnabled(true);
}

void Form::MapOptions::setEnableSaveButton(const QString & val)
{
    Q_UNUSED(val);
    pushButtonSave->setEnabled(true);
}

Form::ItemsList::ItemsList(QWidget* parent) : QListWidget(parent)
{
    setViewMode(QListView::ListMode);

    addItemAct = new QAction(tr("Add"), this);
    addItemAct->setEnabled(true);

    editItemAct = new QAction(tr("Edit"), this);
    editItemAct->setEnabled(false);

    delItemAct = new QAction(tr("Delete"), this);
    delItemAct->setEnabled(false);

    connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(editCurrentItem()));
    connect(addItemAct, SIGNAL(triggered()), this, SLOT(addNewItem()));
    connect(editItemAct, SIGNAL(triggered()), this, SLOT(editCurrentItem()));
    connect(delItemAct, SIGNAL(triggered()), this, SLOT(deleteCurrentItem()));
    connect(this, SIGNAL(listChanged()), this, SLOT(slotCheckLimit()));
}

void Form::ItemsList::slotCheckLimit(void)
{
    checkLimit();
}

void Form::ItemsList::addNewItem(void)
{
    addItem();
    emit listChanged();
}

void Form::ItemsList::editCurrentItem(void)
{
    editItem(currentItem());
    emit listChanged();
}

void Form::ItemsList::deleteCurrentItem(void)
{
    takeItem(currentRow());
    emit listChanged();
}

void Form::ItemsList::setCurrentItem(int row)
{
    setCurrentRow(row);
    emit listChanged();
}

void Form::ItemsList::mousePressEvent(QMouseEvent* event)
{
    if(event->buttons() & Qt::RightButton)
    {
	QMenu menu(this);
	createMenuItems(&menu);
	menu.exec(event->globalPos());
    }

    event->accept();

    QListWidget::mousePressEvent(event);

    emit mousePressed();
}

void Form::ItemsList::createMenuItems(QMenu* menu)
{
    bool selected = selectedItems().size();

    editItemAct->setEnabled(selected);
    delItemAct->setEnabled(selected);

    menu->addAction(addItemAct);
    menu->addAction(editItemAct);
    menu->addSeparator();
    menu->addAction(delItemAct);
}

QStringList Form::ItemsList::results(void) const
{
    QStringList res;

    for(int pos = 0; pos < count(); ++pos)
        res << item(pos)->text();

    return res;
}

Form::RumorsList::RumorsList(QWidget* parent) : ItemsList(parent)
{
    addItemAct->setStatusTip(tr("Add new rumor"));
    editItemAct->setStatusTip(tr("Edit rumor"));
    delItemAct->setStatusTip(tr("Delete rumor"));
}

void Form::RumorsList::addItem(void)
{
    MessageDialog dialog;

    if(QDialog::Accepted == dialog.exec())
    {
	QListWidget::addItem(dialog.message());
	setCurrentRow(count() - 1);
    }
}

void Form::RumorsList::editItem(QListWidgetItem* item)
{
    MessageDialog dialog(item->text());

    if(QDialog::Accepted == dialog.exec())
	item->setText(dialog.message());
}

TavernRumors Form::RumorsList::results(void) const
{
    TavernRumors res;

    for(int pos = 0; pos < count(); ++pos)
	res.push_back(item(pos)->text());

    return res;
}

Form::DayEventsList::DayEventsList(int colors, QWidget* parent) : ItemsList(parent), kingdomColors(colors)
{
    addItemAct->setStatusTip(tr("Add new event"));
    editItemAct->setStatusTip(tr("Edit event"));
    delItemAct->setStatusTip(tr("Delete event"));
}

void Form::DayEventsList::addItem(void)
{
    DayEventDialog dialog(DayEvent(), kingdomColors);

    if(QDialog::Accepted == dialog.exec())
    {
	DayEvent event = dialog.result();
	QListWidgetItem* item = new QListWidgetItem(event.header());
	item->setData(Qt::UserRole, QVariant::fromValue(event));
	QListWidget::addItem(item);
	setCurrentRow(count() - 1);
    }
}

void Form::DayEventsList::editItem(QListWidgetItem* item)
{
    DayEvent event = qvariant_cast<DayEvent>(item->data(Qt::UserRole));
    DayEventDialog dialog(event, kingdomColors);

    if(QDialog::Accepted == dialog.exec())
    {
	event = dialog.result();
	item->setText(event.header());
	item->setData(Qt::UserRole, QVariant::fromValue(event));
    }
}

DayEvents Form::DayEventsList::results(void) const
{
    DayEvents res;

    for(int pos = 0; pos < count(); ++pos)
	res.push_back(qvariant_cast<DayEvent>(item(pos)->data(Qt::UserRole)));

    return res;
}

Form::MessageDialog::MessageDialog(const QString & msg)
{
    setWindowTitle(QApplication::translate("MessageDialog", "Message Detail", 0));

    plainText = new QPlainTextEdit(this);
    plainText->setPlainText(msg);

    verticalLayout = new QVBoxLayout(this);
    verticalLayout->addWidget(plainText);

    pushButtonOk = new QPushButton(this);
    pushButtonOk->setText(QApplication::translate("MessageDialog", "Ok", 0));
    pushButtonOk->setEnabled(false);

    horizontalSpacer = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    pushButtonCancel = new QPushButton(this);
    pushButtonCancel->setText(QApplication::translate("MessageDialog", "Cancel", 0));

    horizontalLayout = new QHBoxLayout();
    horizontalLayout->addWidget(pushButtonOk);
    horizontalLayout->addItem(horizontalSpacer);
    horizontalLayout->addWidget(pushButtonCancel);
    verticalLayout->addLayout(horizontalLayout);

    resize(250, 160);

    connect(plainText, SIGNAL(textChanged()), this, SLOT(enableButtonOK()));
    connect(pushButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(pushButtonOk, SIGNAL(clicked()), this, SLOT(accept()));
}

void Form::MessageDialog::enableButtonOK(void)
{
    pushButtonOk->setEnabled(! plainText->toPlainText().isEmpty());
}

QString Form::MessageDialog::message(void) const
{
    return plainText->toPlainText();
}

Form::PlayerAllow::PlayerAllow(int c, bool v, QWidget* parent) : QLabel(parent), col(c), stat(v)
{
    updatePlayers();
}

void Form::PlayerAllow::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    stat = stat ? false : true;
    updatePlayers();
    emit mousePressed();
}

void Form::PlayerAllow::updatePlayers(void)
{
    QPixmap pix = Editor::pixmapBorder(QSize(26, 26), Color::convert(col), QColor(0, 0, 0));

    if(stat)
    {
	QPainter paint(& pix);
	paint.drawPixmap(QPoint(6, 6), EditorTheme::getImageICN("CELLWIN.ICN", 2).first);
    }

    setPixmap(pix);
}

Form::AccessGroup::AccessGroup(QWidget* parent, int kingdomColors, int checkedColors) : QGroupBox(parent)
{
    setFlat(true);

    horizontalLayoutPlayers = new QHBoxLayout();
    horizontalSpacerPlayersLeft = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    horizontalSpacerPlayersRight = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    horizontalLayoutPlayers->addItem(horizontalSpacerPlayersLeft);

    // create allow players labels
    QVector<int> colors = Color::colors(Color::All);

    for(QVector<int>::const_iterator
	it = colors.begin(); it != colors.end(); ++it)
    if((*it) & kingdomColors)
    {
	labelPlayers.push_back(new PlayerAllow(*it, (*it) & checkedColors, this));
	horizontalLayoutPlayers->addWidget(labelPlayers.back());
    }
    horizontalLayoutPlayers->addItem(horizontalSpacerPlayersRight);

    checkBoxAllowComp = new QCheckBox(this);
    checkBoxAllowComp->setLayoutDirection(Qt::LeftToRight);
    checkBoxAllowComp->setText(QApplication::translate("AccessGroup", "Allow computer", 0));

    checkBoxCancelAfterFirstVisit = new QCheckBox(this);
    checkBoxCancelAfterFirstVisit->setLayoutDirection(Qt::LeftToRight);
    checkBoxCancelAfterFirstVisit->setText(QApplication::translate("AccessGroup", "Cancel after first visit", 0));

    verticalLayoutAllowCols = new QVBoxLayout(this);
    verticalLayoutAllowCols->addLayout(horizontalLayoutPlayers);
    verticalLayoutAllowCols->addWidget(checkBoxAllowComp);
    verticalLayoutAllowCols->addWidget(checkBoxCancelAfterFirstVisit);

   for(QVector<PlayerAllow*>::const_iterator
        it = labelPlayers.begin(); it != labelPlayers.end(); ++it)
        connect(*it, SIGNAL(mousePressed()), this, SLOT(setFormChanged()));
}

void Form::AccessGroup::setFormChanged(void)
{
    emit formChanged();
}

void Form::AccessGroup::setAllowComputer(bool f)
{
    checkBoxAllowComp->setChecked(f);
}

void Form::AccessGroup::setCancelAfterFirstVisit(bool f)
{
    checkBoxCancelAfterFirstVisit->setChecked(f);
}

int Form::AccessGroup::colors(void) const
{
    int res = 0;

    for(QVector<PlayerAllow*>::const_iterator
        it = labelPlayers.begin(); it != labelPlayers.end(); ++it)
    if((*it)->allow())
    {
        res |= (*it)->color();
    }

    return res;
}

bool Form::AccessGroup::allowComputer(void) const
{
    return checkBoxAllowComp->isChecked();
}

bool Form::AccessGroup::cancelAfterFirstVisit(void) const
{
    return checkBoxCancelAfterFirstVisit->isChecked();
}

Form::ArtifactGroup::ArtifactGroup(QWidget* parent, int artifact) : QGroupBox(parent)
{
    setFlat(true);

    labelArtifact = new QLabel(this);
    changeLabelArtifact(artifact);

    comboBoxArtifact = new QComboBox(this);
    for(int index = Artifact::None; index < Artifact::Unknown; ++index)
	comboBoxArtifact->addItem(Artifact::transcribe(index), index);
    comboBoxArtifact->setCurrentIndex(artifact);

    horizontalLayout = new QHBoxLayout(this);
    horizontalLayout->addWidget(labelArtifact);
    horizontalLayout->addWidget(comboBoxArtifact);

    connect(comboBoxArtifact, SIGNAL(currentIndexChanged(int)), this, SLOT(setFormChanged()));
    connect(comboBoxArtifact, SIGNAL(currentIndexChanged(int)), this, SLOT(changeLabelArtifact(int)));
}

void Form::ArtifactGroup::setFormChanged(void)
{
    emit formChanged();
}

void Form::ArtifactGroup::setValue(int art)
{
    if(art >= Artifact::Random && art <= Artifact::Random3)
	art = Artifact::None;

    comboBoxArtifact->setCurrentIndex(art >= Artifact::None && art < Artifact::Unknown ? art : Artifact::None);
    emit formChanged();
}

void Form::ArtifactGroup::changeLabelArtifact(int index)
{
    labelArtifact->setPixmap(EditorTheme::getImageICN("ARTIFACT.ICN", index).first.scaled(48, 48));
#ifndef QT_NO_TOOLTIP
    labelArtifact->setToolTip(Artifact::transcribe(index));
#endif
}

int Form::ArtifactGroup::result(void) const
{
    return comboBoxArtifact->currentIndex();
}

Form::SpellGroup::SpellGroup(QWidget* parent, int spell) : QGroupBox(parent)
{
    setFlat(true);

    comboBoxSpell = new QComboBox(this);
    for(int index = Spell::None; index < Spell::Unknown; ++index)
    {
	comboBoxSpell->addItem(Spell::transcribe(index), index);
#ifndef QT_NO_TOOLTIP
	comboBoxSpell->setItemData(index, Spell::tips(index), Qt::ToolTipRole);
#endif
    }
    comboBoxSpell->setCurrentIndex(spell);

    horizontalLayout = new QHBoxLayout(this);
    horizontalLayout->addWidget(comboBoxSpell);

    connect(comboBoxSpell, SIGNAL(currentIndexChanged(int)), this, SLOT(setFormChanged()));
}

void Form::SpellGroup::setFormChanged(void)
{
    emit formChanged();
}

void Form::SpellGroup::setValue(int spell)
{
    comboBoxSpell->setCurrentIndex(spell >= Spell::None && spell < Spell::Unknown ? spell : Spell::None);
    emit formChanged();
}

int Form::SpellGroup::result(void) const
{
    return comboBoxSpell->currentIndex();
}

Form::ResourcesGroup::ResourcesGroup(QWidget* parent, const Resources & resources) : QGroupBox(parent)
{
    setFlat(true);

    int resMin = -65535;
    int resMax = 65535;

    spinBoxResWood = new QSpinBox(this);
    spinBoxResWood->setMinimum(resMin);
    spinBoxResWood->setMaximum(resMax);
    spinBoxResWood->setValue(resources.wood);

    labelResWood = new QLabel(this);
    labelResWood->setPixmap(EditorTheme::getImageICN("RESOURCE.ICN", 0).first);
    labelResWood->setBuddy(spinBoxResWood);

    horizontalSpacerWoodSulfur = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    spinBoxResSulfur = new QSpinBox(this);
    spinBoxResSulfur->setMinimum(resMin);
    spinBoxResSulfur->setMaximum(resMax);
    spinBoxResSulfur->setValue(resources.sulfur);

    labelResSulfur = new QLabel(this);
    labelResSulfur->setPixmap(EditorTheme::getImageICN("RESOURCE.ICN", 3).first);
    labelResSulfur->setBuddy(spinBoxResSulfur);

    horizontalLayoutWoodSulfur = new QHBoxLayout();
    horizontalLayoutWoodSulfur->addWidget(labelResWood);
    horizontalLayoutWoodSulfur->addWidget(spinBoxResWood);
    horizontalLayoutWoodSulfur->addItem(horizontalSpacerWoodSulfur);
    horizontalLayoutWoodSulfur->addWidget(labelResSulfur);
    horizontalLayoutWoodSulfur->addWidget(spinBoxResSulfur);

    spinBoxResMercury = new QSpinBox(this);
    spinBoxResMercury->setMinimum(resMin);
    spinBoxResMercury->setMaximum(resMax);
    spinBoxResMercury->setValue(resources.mercury);

    labelResMercury = new QLabel(this);
    labelResMercury->setPixmap(EditorTheme::getImageICN("RESOURCE.ICN", 1).first);
    labelResMercury->setBuddy(spinBoxResMercury);

    horizontalSpacerMercuryCristal = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    spinBoxResCrystal = new QSpinBox(this);
    spinBoxResCrystal->setMinimum(resMin);
    spinBoxResCrystal->setMaximum(resMax);
    spinBoxResCrystal->setValue(resources.crystal);

    labelResCrystal = new QLabel(this);
    labelResCrystal->setPixmap(EditorTheme::getImageICN("RESOURCE.ICN", 4).first);
    labelResCrystal->setBuddy(spinBoxResCrystal);

    horizontalLayoutMercuryCristal = new QHBoxLayout();
    horizontalLayoutMercuryCristal->addWidget(labelResMercury);
    horizontalLayoutMercuryCristal->addWidget(spinBoxResMercury);
    horizontalLayoutMercuryCristal->addItem(horizontalSpacerMercuryCristal);
    horizontalLayoutMercuryCristal->addWidget(labelResCrystal);
    horizontalLayoutMercuryCristal->addWidget(spinBoxResCrystal);

    spinBoxResOre = new QSpinBox(this);
    spinBoxResOre->setMinimum(resMin);
    spinBoxResOre->setMaximum(resMax);
    spinBoxResOre->setValue(resources.ore);

    labelResOre = new QLabel(this);
    labelResOre->setPixmap(EditorTheme::getImageICN("RESOURCE.ICN", 2).first);
    labelResOre->setBuddy(spinBoxResOre);

    horizontalSpacerOreGems = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    spinBoxResGems = new QSpinBox(this);
    spinBoxResGems->setMinimum(resMin);
    spinBoxResGems->setMaximum(resMax);
    spinBoxResGems->setValue(resources.gems);

    labelResGems = new QLabel(this);
    labelResGems->setPixmap(EditorTheme::getImageICN("RESOURCE.ICN", 5).first);
    labelResGems->setBuddy(spinBoxResGems);

    horizontalLayoutOreGems = new QHBoxLayout();
    horizontalLayoutOreGems->addWidget(labelResOre);
    horizontalLayoutOreGems->addWidget(spinBoxResOre);
    horizontalLayoutOreGems->addItem(horizontalSpacerOreGems);
    horizontalLayoutOreGems->addWidget(labelResGems);
    horizontalLayoutOreGems->addWidget(spinBoxResGems);

    horizontalSpacerGoldLeft = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    horizontalSpacerGoldRight = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    spinBoxResGold = new QSpinBox(this);
    spinBoxResGold->setMinimum(resMin);
    spinBoxResGold->setMaximum(resMax);
    spinBoxResGold->setValue(resources.gold);

    labelResGold = new QLabel(this);
    labelResGold->setPixmap(EditorTheme::getImageICN("RESOURCE.ICN", 6).first);
    labelResGold->setBuddy(spinBoxResGold);

    horizontalLayoutGold = new QHBoxLayout();
    horizontalLayoutGold->addItem(horizontalSpacerGoldLeft);
    horizontalLayoutGold->addWidget(labelResGold);
    horizontalLayoutGold->addWidget(spinBoxResGold);
    horizontalLayoutGold->addItem(horizontalSpacerGoldRight);

    verticalLayoutBox = new QVBoxLayout(this);
    verticalLayoutBox->addLayout(horizontalLayoutWoodSulfur);
    verticalLayoutBox->addLayout(horizontalLayoutMercuryCristal);
    verticalLayoutBox->addLayout(horizontalLayoutOreGems);
    verticalLayoutBox->addLayout(horizontalLayoutGold);

#ifndef QT_NO_TOOLTIP
    labelResWood->setToolTip(QApplication::translate("DayEventDialog", "wood", 0));
    spinBoxResWood->setToolTip(QApplication::translate("DayEventDialog", "wood", 0));
    labelResSulfur->setToolTip(QApplication::translate("DayEventDialog", "sulfur", 0));
    spinBoxResSulfur->setToolTip(QApplication::translate("DayEventDialog", "sulfur", 0));
    labelResMercury->setToolTip(QApplication::translate("DayEventDialog", "mercury", 0));
    spinBoxResMercury->setToolTip(QApplication::translate("DayEventDialog", "mercury", 0));
    labelResCrystal->setToolTip(QApplication::translate("DayEventDialog", "crystal", 0));
    spinBoxResCrystal->setToolTip(QApplication::translate("DayEventDialog", "crystal", 0));
    labelResOre->setToolTip(QApplication::translate("DayEventDialog", "ore", 0));
    spinBoxResOre->setToolTip(QApplication::translate("DayEventDialog", "ore", 0));
    labelResGems->setToolTip(QApplication::translate("DayEventDialog", "gems", 0));
    spinBoxResGems->setToolTip(QApplication::translate("DayEventDialog", "gems", 0));
    labelResGold->setToolTip(QApplication::translate("DayEventDialog", "gold", 0));
    spinBoxResGold->setToolTip(QApplication::translate("DayEventDialog", "gold", 0));
#endif // QT_NO_TOOLTIP

    connect(spinBoxResWood, SIGNAL(valueChanged(int)), this, SLOT(setFormChanged()));
    connect(spinBoxResMercury, SIGNAL(valueChanged(int)), this, SLOT(setFormChanged()));
    connect(spinBoxResOre, SIGNAL(valueChanged(int)), this, SLOT(setFormChanged()));
    connect(spinBoxResSulfur, SIGNAL(valueChanged(int)), this, SLOT(setFormChanged()));
    connect(spinBoxResCrystal, SIGNAL(valueChanged(int)), this, SLOT(setFormChanged()));
    connect(spinBoxResGems, SIGNAL(valueChanged(int)), this, SLOT(setFormChanged()));
    connect(spinBoxResGold, SIGNAL(valueChanged(int)), this, SLOT(setFormChanged()));
}

void Form::ResourcesGroup::setFormChanged(void)
{
    emit formChanged();
}

Resources Form::ResourcesGroup::result(void) const
{
    Resources res;

    res.wood = spinBoxResWood->value();
    res.mercury = spinBoxResMercury->value();
    res.ore = spinBoxResOre->value();
    res.sulfur = spinBoxResSulfur->value();
    res.crystal = spinBoxResCrystal->value();
    res.gems = spinBoxResGems->value();
    res.gold = spinBoxResGold->value();

    return res;
}


Form::DayEventDialog::DayEventDialog(const DayEvent & event, int kingdomColors)
{
    setWindowTitle(QApplication::translate("DayEventDialog", "Event Detail", 0));

    // tab 1
    tabDay = new QWidget();

    labelDayFirst = new QLabel(tabDay);
    labelDayFirst->setText(QApplication::translate("DayEventDialog", "Day of first occurent", 0));

    horizontalLayout = new QHBoxLayout();
    horizontalLayout->addWidget(labelDayFirst);

    spinBoxDayFirst = new QSpinBox(tabDay);
    spinBoxDayFirst->setMinimum(1);
    spinBoxDayFirst->setMaximum(65535);
    spinBoxDayFirst->setValue(event.dayFirstOccurent);
    labelDayFirst->setBuddy(spinBoxDayFirst);
    horizontalLayout->addWidget(spinBoxDayFirst);

    labelSubsequent = new QLabel(tabDay);
    labelSubsequent->setText(QApplication::translate("DayEventDialog", "Subsequent occurrences", 0));

    horizontalLayout2 = new QHBoxLayout();
    horizontalLayout2->addWidget(labelSubsequent);

    comboBoxSubsequent = new QComboBox(tabDay);
    comboBoxSubsequent->addItem(QApplication::translate("DayEventDialog", "Never", 0), 0);
    comboBoxSubsequent->addItem(QApplication::translate("DayEventDialog", "Every Day", 0), 1);
    comboBoxSubsequent->addItem(QApplication::translate("DayEventDialog", "Every 2 Days", 0), 2);
    comboBoxSubsequent->addItem(QApplication::translate("DayEventDialog", "Every 3 Days", 0), 3);
    comboBoxSubsequent->addItem(QApplication::translate("DayEventDialog", "Every 4 Days", 0), 4);
    comboBoxSubsequent->addItem(QApplication::translate("DayEventDialog", "Every 5 Days", 0), 5);
    comboBoxSubsequent->addItem(QApplication::translate("DayEventDialog", "Every 6 Days", 0), 6);
    comboBoxSubsequent->addItem(QApplication::translate("DayEventDialog", "Every 7 Days", 0), 7);
    comboBoxSubsequent->addItem(QApplication::translate("DayEventDialog", "Every 14 Days", 0), 14);
    comboBoxSubsequent->addItem(QApplication::translate("DayEventDialog", "Every 21 Days", 0), 21);
    comboBoxSubsequent->addItem(QApplication::translate("DayEventDialog", "Every 28 Days", 0), 28);
    int find = comboBoxSubsequent->findData(event.daySubsequentOccurrences);
    if(0 <= find)
	comboBoxSubsequent->setCurrentIndex(find);
    horizontalLayout2->addWidget(comboBoxSubsequent);

    groupBoxAllowedColors = new QGroupBox(tabDay);
    groupBoxAllowedColors->setTitle(QApplication::translate("DayEventDialog", "Colors allowed to get event", 0));

    horizontalLayout4 = new QHBoxLayout();
    horizontalSpacerPlayersLeft = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    horizontalSpacerPlayersRight = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    horizontalLayout4->addItem(horizontalSpacerPlayersLeft);

    // create allow players labels
    QVector<int> colors = Color::colors(Color::All);

    for(QVector<int>::const_iterator
	it = colors.begin(); it != colors.end(); ++it)
    if((*it) & kingdomColors)
    {
	labelPlayers.push_back(new PlayerAllow(*it, (*it) & event.colors, groupBoxAllowedColors));
	horizontalLayout4->addWidget(labelPlayers.back());
    }
    horizontalLayout4->addItem(horizontalSpacerPlayersRight);

    checkBoxAllowComp = new QCheckBox(groupBoxAllowedColors);
    checkBoxAllowComp->setLayoutDirection(Qt::LeftToRight);
    checkBoxAllowComp->setText(QApplication::translate("DayEventDialog", "Allow computer", 0));
    checkBoxAllowComp->setChecked(event.allowComputer);

    verticalLayout3 = new QVBoxLayout(groupBoxAllowedColors);
    verticalLayout3->addLayout(horizontalLayout4);
    verticalLayout3->addWidget(checkBoxAllowComp);

    verticalLayout = new QVBoxLayout(tabDay);
    verticalLayout->addLayout(horizontalLayout);
    verticalLayout->addLayout(horizontalLayout2);
    verticalLayout->addWidget(groupBoxAllowedColors);

    // tab 2
    tabResource = new QWidget();

    resourcesGroup = new ResourcesGroup(tabResource, event.resources);

    // tab 3
    tabMessage = new QWidget();

    plainTextMessage = new QPlainTextEdit(tabMessage);
    plainTextMessage->setPlainText(event.message);

    verticalLayout5 = new QVBoxLayout(tabMessage);
    verticalLayout5->addWidget(plainTextMessage);

    // end
    tabWidget = new QTabWidget(this);

    tabWidget->addTab(tabDay, "Days");
    tabWidget->addTab(tabResource, "Resources");
    tabWidget->addTab(tabMessage, "Message");

    pushButtonOk = new QPushButton(this);
    pushButtonOk->setText(QApplication::translate("DayEventDialog", "Ok", 0));
    pushButtonOk->setEnabled(false);

    horizontalSpacer = new QSpacerItem(238, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    pushButtonCancel = new QPushButton(this);
    pushButtonCancel->setText(QApplication::translate("DayEventDialog", "Cancel", 0));

    horizontalLayout3 = new QHBoxLayout();
    horizontalLayout3->addWidget(pushButtonOk);
    horizontalLayout3->addItem(horizontalSpacer);
    horizontalLayout3->addWidget(pushButtonCancel);

    verticalLayout2 = new QVBoxLayout(this);
    verticalLayout2->addWidget(tabWidget);
    verticalLayout2->addLayout(horizontalLayout3);

    tabWidget->setCurrentIndex(0);

    QSize minSize = minimumSizeHint();

    resize(minSize);
    setMinimumSize(minSize);

    connect(spinBoxDayFirst, SIGNAL(valueChanged(const QString &)), this, SLOT(setEnableOKButton(const QString &)));
    connect(comboBoxSubsequent , SIGNAL(currentIndexChanged(const QString &)), this, SLOT(setEnableOKButton(const QString &)));
    connect(checkBoxAllowComp, SIGNAL(clicked()), this, SLOT(setEnableOKButton()));
    connect(resourcesGroup, SIGNAL(formChanged()), this, SLOT(setEnableOKButton()));

    connect(plainTextMessage, SIGNAL(textChanged()), this, SLOT(setEnableOKButton()));

    for(QVector<PlayerAllow*>::const_iterator
	it = labelPlayers.begin(); it != labelPlayers.end(); ++it)
	connect(*it, SIGNAL(mousePressed()), this, SLOT(setEnableOKButton()));

    connect(pushButtonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(pushButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));
}

void Form::DayEventDialog::setEnableOKButton(void)
{
    pushButtonOk->setEnabled(true);
}

void Form::DayEventDialog::setEnableOKButton(const QString & val)
{
    Q_UNUSED(val);
    pushButtonOk->setEnabled(true);
}

DayEvent Form::DayEventDialog::result(void) const
{
    DayEvent res;

    res.resources = resourcesGroup->result();
    res.colors = 0;

    for(QVector<PlayerAllow*>::const_iterator
	it = labelPlayers.begin(); it != labelPlayers.end(); ++it)
    if((*it)->allow())
    {
	res.colors |= (*it)->color();
    }

    res.allowComputer = checkBoxAllowComp->isChecked();
    res.dayFirstOccurent = spinBoxDayFirst->value();
    res.daySubsequentOccurrences = qvariant_cast<int>(comboBoxCurrentData(comboBoxSubsequent));
    res.message = plainTextMessage->toPlainText();

    return res;
}

Form::MiniMap::MiniMap(QWidget* parent) : QLabel(parent), miniMapSize(144, 144)
{
    setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
}

void Form::MiniMap::generate(const MapData* data)
{
    if(!data) return;

    const MapTiles & tiles = data->tiles();
    QApplication::setOverrideCursor(Qt::WaitCursor);

    mapSize = tiles.mapSize();
    QImage image(mapSize, QImage::Format_RGB32);

    for(MapTiles::const_iterator
	it = tiles.begin(); it != tiles.end(); ++it)
    {
	const QPoint & pos = (*it).mapPos();

	switch((*it).groundType())
	{
	    case Ground::Desert:	image.setPixel(pos, qRgb(0xD0, 0xC0, 0x48)); break;
	    case Ground::Snow:		image.setPixel(pos, qRgb(0xE0, 0xE0, 0xE0)); break;
	    case Ground::Swamp:		image.setPixel(pos, qRgb(0x58, 0x94, 0xA0)); break;
	    case Ground::Wasteland:	image.setPixel(pos, qRgb(0xE0, 0x48, 0)); break;
	    case Ground::Beach:		image.setPixel(pos, qRgb(0xE0, 0xD0, 0x80)); break;
	    case Ground::Lava:		image.setPixel(pos, qRgb(0x58, 0x58, 0x58)); break;
	    case Ground::Dirt:		image.setPixel(pos, qRgb(0x80, 0x58, 0x28)); break;
	    case Ground::Grass:		image.setPixel(pos, qRgb(0x18, 0x68, 0x18)); break;
	    case Ground::Water:		image.setPixel(pos, qRgb(0, 0x48, 0xD0)); break;
	    default: break;
	}
    }

    QImage scaled;

    if(mapSize.width() > mapSize.height())
	scaled = image.scaledToWidth(miniMapSize.width());
    else
    if(mapSize.width() < mapSize.height())
        scaled = image.scaledToHeight(miniMapSize.height());
    else
        scaled = image.scaled(miniMapSize);

    QPixmap border(scaled.size() + QSize(2, 2));
    border.fill(QColor(0x10, 0x10, 0x10));

    QPainter paint(& border);
    paint.drawImage(1, 1, scaled);
    setPixmap(border);

    QSize minSize = minimumSizeHint();
    resize(minSize);
    setFixedSize(minSize);

    QApplication::restoreOverrideCursor();
}

void Form::MiniMap::changeWindowPos(const QRect & windowRect)
{
    const QSize & ts = EditorTheme::tileSize();
    const QSize absSize = QSize(mapSize.width() * ts.width(), mapSize.height() * ts.height());
    const QSize tmpSize = QSize(windowRect.width() * miniMapSize.width(), windowRect.height() * miniMapSize.height());

    int mw = tmpSize.width() / absSize.width();
    int mh = tmpSize.height() / absSize.height();
    
    QRect newPos = QRect(windowRect.x() * miniMapSize.width() / absSize.width(),
                                windowRect.y() * miniMapSize.height() / absSize.height(),
                                mw ? mw : 1, mh ? mh : 1);

    if(windowPos != newPos)
    {
	windowPos = newPos;

	if(windowPixmap.isNull() || windowPixmap.size() != windowPos.size())
	    windowPixmap = Editor::pixmapBorder(windowPos.size(), Qt::transparent, QColor(220, 0, 220));

	update();
    }
}

void Form::MiniMap::paintEvent(QPaintEvent* event)
{
    QLabel::paintEvent(event);
    QPainter painter(this);
    painter.drawPixmap(windowPos.topLeft() + QPoint(1, 1), windowPixmap);
}

void Form::MiniMap::mouseMoveEvent(QMouseEvent* event)
{
    if(event->buttons() & Qt::LeftButton)
    {
	QPoint miniPos = event->pos() - QPoint(1, 1);
	QPointF pos(mapSize.width() * miniPos.x(), mapSize.height() * miniPos.y());

	pos.rx() /= miniMapSize.width();
	pos.ry() /= miniMapSize.height();

	emit windowPositionNeedChange(pos.toPoint());
    }
}

void Form::MiniMap::mousePressEvent(QMouseEvent* event)
{
    if(event->buttons() & Qt::LeftButton)
    {
	QPoint miniPos = event->pos() - QPoint(1, 1);
	QPointF pos(mapSize.width() * miniPos.x(), mapSize.height() * miniPos.y());

	pos.rx() /= miniMapSize.width();
	pos.ry() /= miniMapSize.height();

	emit windowPositionNeedChange(pos.toPoint());
    }
}

Form::MapEventDialog::MapEventDialog(const MapEvent & event, int kingdomColors)
{
    setWindowTitle(QApplication::translate("MapEventDialog", "Event Detail", 0));

    // tab 1
    tabAccess = new QWidget();

    accessGroup = new AccessGroup(tabAccess, kingdomColors, event.colors);
    accessGroup->setTitle(QApplication::translate("MapEventDialog", "Colors allowed to get event", 0));
    accessGroup->setFlat(false);
    accessGroup->setAllowComputer(event.allowComputer);
    accessGroup->setCancelAfterFirstVisit(event.cancelAfterFirstVisit);

    spacerItemAccess = new QSpacerItem(40, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayoutTabAcs = new QVBoxLayout(tabAccess);
    verticalLayoutTabAcs->addWidget(accessGroup);
    verticalLayoutTabAcs->addItem(spacerItemAccess);

    // tab 2
    tabGift = new QWidget();

    resourcesGroup = new ResourcesGroup(tabGift, event.resources);
    resourcesGroup->setFlat(false);
    resourcesGroup->setTitle(QApplication::translate("MapEventDialog", "Resources", 0));

    artifactGroup = new ArtifactGroup(tabGift, event.artifact);
    artifactGroup->setTitle(QApplication::translate("MapEventDialog", "Artifact to give", 0));
    artifactGroup->setFlat(false);

    verticalLayoutGift = new QVBoxLayout(tabGift);
    verticalLayoutGift->addWidget(resourcesGroup);
    verticalLayoutGift->addWidget(artifactGroup);

    // tab 3
    tabMessage = new QWidget();

    plainTextMessage = new QPlainTextEdit(tabMessage);
    plainTextMessage->setPlainText(event.message);

    verticalLayoutTabMsg = new QVBoxLayout(tabMessage);
    verticalLayoutTabMsg->addWidget(plainTextMessage);

    // end
    tabWidget = new QTabWidget(this);

    tabWidget->addTab(tabAccess, "Access");
    tabWidget->addTab(tabGift, "Gifts");
    tabWidget->addTab(tabMessage, "Message");

    pushButtonOk = new QPushButton(this);
    pushButtonOk->setText(QApplication::translate("MapEventDialog", "Ok", 0));
    pushButtonOk->setEnabled(false);

    horizontalSpacerButtons = new QSpacerItem(238, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    pushButtonCancel = new QPushButton(this);
    pushButtonCancel->setText(QApplication::translate("MapEventDialog", "Cancel", 0));

    horizontalLayoutButtons = new QHBoxLayout();
    horizontalLayoutButtons->addWidget(pushButtonOk);
    horizontalLayoutButtons->addItem(horizontalSpacerButtons);
    horizontalLayoutButtons->addWidget(pushButtonCancel);

    verticalLayoutForm = new QVBoxLayout(this);
    verticalLayoutForm->addWidget(tabWidget);
    verticalLayoutForm->addLayout(horizontalLayoutButtons);

    tabWidget->setCurrentIndex(0);

    QSize minSize = minimumSizeHint();

    resize(minSize);
    setMinimumSize(minSize);

    connect(accessGroup, SIGNAL(formChanged()), this, SLOT(setEnableOKButton()));
    connect(resourcesGroup, SIGNAL(formChanged()), this, SLOT(setEnableOKButton()));
    connect(artifactGroup, SIGNAL(formChanged()), this, SLOT(setEnableOKButton()));
    connect(plainTextMessage, SIGNAL(textChanged()), this, SLOT(setEnableOKButton()));

    connect(pushButtonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(pushButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));
}

void Form::MapEventDialog::setEnableOKButton(void)
{
    pushButtonOk->setEnabled(true);
}

MapEvent Form::MapEventDialog::result(const QPoint & pos, quint32 uid) const
{
    MapEvent res(pos, uid);

    res.resources = resourcesGroup->result();
    res.colors = accessGroup->colors();
    res.artifact = artifactGroup->result();

    res.allowComputer = accessGroup->allowComputer();
    res.cancelAfterFirstVisit = accessGroup->cancelAfterFirstVisit();
    res.message = plainTextMessage->toPlainText();

    return res;
}

Form::MapTownDialog::MapTownDialog(const MapTown & town)
{
    setWindowTitle(QApplication::translate("MapTownDialog", "Town Detail", 0));

    // tab: info
    tabInfo = new QWidget();

    labelName = new QLabel(tabInfo);
    labelName->setText(QApplication::translate("MapTownDialog", "Name", 0));

    comboBoxName = new QComboBox(tabInfo);
    comboBoxName->setEditable(true);
    comboBoxName->addItems(Editor::townNames());
    comboBoxName->lineEdit()->setText(town.nameTown);

    horizontalLayoutName = new QHBoxLayout();
    horizontalLayoutName->addWidget(labelName);
    horizontalLayoutName->addWidget(comboBoxName);

    labelColor = new QLabel(tabInfo);
    labelColor->setText(QApplication::translate("MapTownDialog", "Color", 0));

    comboBoxColor = new QComboBox(tabInfo);
    comboBoxColor->addItem(Editor::pixmapBorder(QSize(24, 24), QColor(130, 130, 130), QColor(0, 0, 0)), "Gray", Color::None);

    QVector<int> colors = Color::colors(Color::All);
    for(QVector<int>::const_iterator
	it = colors.begin(); it != colors.end(); ++it)
	comboBoxColor->addItem(Editor::pixmapBorder(QSize(24, 24), Color::convert(*it), QColor(0, 0, 0)), Color::transcribe(*it), *it);

    comboBoxColor->setCurrentIndex(Color::index(town.color()));

    horizontalLayoutColor = new QHBoxLayout();
    horizontalLayoutColor->addWidget(labelColor);
    horizontalLayoutColor->addWidget(comboBoxColor);

    checkBoxCaptain = new QCheckBox(tabInfo);
    checkBoxCaptain->setText(QApplication::translate("MapTownDialog", "Captain", 0));
    checkBoxCaptain->setChecked(town.captainPresent);
#ifndef QT_NO_TOOLTIP
    checkBoxCaptain->setToolTip(Building::description(Building::Captain, town.race));
#endif

    checkBoxAllowCastle = new QCheckBox(tabInfo);
    checkBoxAllowCastle->setVisible(! town.isCastle);
    checkBoxAllowCastle->setChecked(! town.forceTown);
    checkBoxAllowCastle->setText(QApplication::translate("MapTownDialog", "Allow castle", 0));

    verticalSpacerInfo = new QSpacerItem(20, 142, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayoutInfo = new QVBoxLayout(tabInfo);
    verticalLayoutInfo->addLayout(horizontalLayoutName);
    verticalLayoutInfo->addLayout(horizontalLayoutColor);
    verticalLayoutInfo->addWidget(checkBoxCaptain);
    verticalLayoutInfo->addWidget(checkBoxAllowCastle);
    verticalLayoutInfo->addItem(verticalSpacerInfo);

    // tab: troops
    tabTroops = new QWidget();
    bool defaultTroops = ! town.customTroops;

    checkBoxTroopsDefault = new QCheckBox(tabTroops);
    checkBoxTroopsDefault->setChecked(defaultTroops);
    checkBoxTroopsDefault->setText(QApplication::translate("MapTownDialog", "Default", 0));

    // troop 1
    labelSlot1 = new QLabel(tabTroops);
    labelSlot1->setEnabled(! defaultTroops);
    labelSlot1->setText(QApplication::translate("MapTownDialog", "Slot 1", 0));

    comboBoxTroop1 = new QComboBox(tabTroops);
    comboBoxTroop1->setEnabled(! defaultTroops);

    spinBoxCount1 = new QSpinBox(tabTroops);
    spinBoxCount1->setEnabled(! defaultTroops);
    spinBoxCount1->setMaximumWidth(61);
    spinBoxCount1->setMaximum(65535);

    horizontalLayoutT1 = new QHBoxLayout();
    horizontalLayoutT1->addWidget(labelSlot1);
    horizontalLayoutT1->addWidget(comboBoxTroop1);
    horizontalLayoutT1->addWidget(spinBoxCount1);

    // troop 2
    labelSlot2 = new QLabel(tabTroops);
    labelSlot2->setEnabled(! defaultTroops);
    labelSlot2->setText(QApplication::translate("MapTownDialog", "Slot 2", 0));

    comboBoxTroop2 = new QComboBox(tabTroops);
    comboBoxTroop2->setEnabled(! defaultTroops);

    spinBoxCount2 = new QSpinBox(tabTroops);
    spinBoxCount2->setEnabled(! defaultTroops);
    spinBoxCount2->setMaximumWidth(61);
    spinBoxCount2->setMaximum(65535);

    horizontalLayoutT2 = new QHBoxLayout();
    horizontalLayoutT2->addWidget(labelSlot2);
    horizontalLayoutT2->addWidget(comboBoxTroop2);
    horizontalLayoutT2->addWidget(spinBoxCount2);

    // troop 3
    labelSlot3 = new QLabel(tabTroops);
    labelSlot3->setEnabled(! defaultTroops);
    labelSlot3->setText(QApplication::translate("MapTownDialog", "Slot 3", 0));

    comboBoxTroop3 = new QComboBox(tabTroops);
    comboBoxTroop3->setEnabled(! defaultTroops);

    spinBoxCount3 = new QSpinBox(tabTroops);
    spinBoxCount3->setEnabled(! defaultTroops);
    spinBoxCount3->setMaximumWidth(61);
    spinBoxCount3->setMaximum(65535);

    horizontalLayoutT3 = new QHBoxLayout();
    horizontalLayoutT3->addWidget(labelSlot3);
    horizontalLayoutT3->addWidget(comboBoxTroop3);
    horizontalLayoutT3->addWidget(spinBoxCount3);

    // troop 4
    labelSlot4 = new QLabel(tabTroops);
    labelSlot4->setEnabled(! defaultTroops);
    labelSlot4->setText(QApplication::translate("MapTownDialog", "Slot 4", 0));

    comboBoxTroop4 = new QComboBox(tabTroops);
    comboBoxTroop4->setEnabled(! defaultTroops);

    spinBoxCount4 = new QSpinBox(tabTroops);
    spinBoxCount4->setEnabled(! defaultTroops);
    spinBoxCount4->setMaximumWidth(61);
    spinBoxCount4->setMaximum(65535);

    horizontalLayoutT4 = new QHBoxLayout();
    horizontalLayoutT4->addWidget(labelSlot4);
    horizontalLayoutT4->addWidget(comboBoxTroop4);
    horizontalLayoutT4->addWidget(spinBoxCount4);

    // troop 5
    labelSlot5 = new QLabel(tabTroops);
    labelSlot5->setEnabled(! defaultTroops);
    labelSlot5->setText(QApplication::translate("MapTownDialog", "Slot 5", 0));

    comboBoxTroop5 = new QComboBox(tabTroops);
    comboBoxTroop5->setEnabled(! defaultTroops);

    spinBoxCount5 = new QSpinBox(tabTroops);
    spinBoxCount5->setEnabled(! defaultTroops);
    spinBoxCount5->setMaximumWidth(61);
    spinBoxCount5->setMaximum(65535);

    horizontalLayoutT5 = new QHBoxLayout();
    horizontalLayoutT5->addWidget(labelSlot5);
    horizontalLayoutT5->addWidget(comboBoxTroop5);
    horizontalLayoutT5->addWidget(spinBoxCount5);

    for(int index = Monster::None; index < Monster::Unknown; ++index)
    {
	QPixmap mons32 = EditorTheme::getImageICN(ICN::MONS32, index - 1).first;
	comboBoxTroop1->addItem(mons32, Monster::transcribe(index));
	comboBoxTroop2->addItem(mons32, Monster::transcribe(index));
	comboBoxTroop3->addItem(mons32, Monster::transcribe(index));
	comboBoxTroop4->addItem(mons32, Monster::transcribe(index));
	comboBoxTroop5->addItem(mons32, Monster::transcribe(index));
#ifndef QT_NO_TOOLTIP
	comboBoxTroop1->setItemData(index, Monster::tips(index), Qt::ToolTipRole);
	comboBoxTroop2->setItemData(index, Monster::tips(index), Qt::ToolTipRole);
	comboBoxTroop3->setItemData(index, Monster::tips(index), Qt::ToolTipRole);
	comboBoxTroop4->setItemData(index, Monster::tips(index), Qt::ToolTipRole);
	comboBoxTroop5->setItemData(index, Monster::tips(index), Qt::ToolTipRole);
#endif
    }

    if(! defaultTroops)
    {
	comboBoxTroop1->setCurrentIndex(town.troops[0].type());
	comboBoxTroop2->setCurrentIndex(town.troops[1].type());
	comboBoxTroop3->setCurrentIndex(town.troops[2].type());
	comboBoxTroop4->setCurrentIndex(town.troops[3].type());
	comboBoxTroop5->setCurrentIndex(town.troops[4].type());

	spinBoxCount1->setValue(town.troops[0].count());
	spinBoxCount2->setValue(town.troops[1].count());
	spinBoxCount3->setValue(town.troops[2].count());
	spinBoxCount4->setValue(town.troops[3].count());
	spinBoxCount5->setValue(town.troops[4].count());
    }

    verticalSpacerTroops = new QSpacerItem(20, 37, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayoutTroops = new QVBoxLayout(tabTroops);
    verticalLayoutTroops->addWidget(checkBoxTroopsDefault);
    verticalLayoutTroops->addLayout(horizontalLayoutT1);
    verticalLayoutTroops->addLayout(horizontalLayoutT2);
    verticalLayoutTroops->addLayout(horizontalLayoutT3);
    verticalLayoutTroops->addLayout(horizontalLayoutT4);
    verticalLayoutTroops->addLayout(horizontalLayoutT5);
    verticalLayoutTroops->addItem(verticalSpacerTroops);

    // tab: buildings
    tabBuildings = new QWidget();
    bool defaultBuildings = ! town.customBuildings;

    checkBoxBuildingsDefault = new QCheckBox(tabBuildings);
    checkBoxBuildingsDefault->setChecked(defaultBuildings);
    checkBoxBuildingsDefault->setText(QApplication::translate("MapTownDialog", "Default", 0));

    labelMageGuild = new QLabel(tabBuildings);
    labelMageGuild->setText(QApplication::translate("MapTownDialog", "Mage Guild", 0));
    labelMageGuild->setEnabled(! defaultBuildings);

    comboBoxMageGuild = new QComboBox(tabBuildings);
    comboBoxMageGuild->setEnabled(! defaultBuildings);
    comboBoxMageGuild->addItem(QApplication::translate("MapTownDialog", "None", 0), 0);
    comboBoxMageGuild->addItem(QApplication::translate("MapTownDialog", "Level 1", 0), 1);
    comboBoxMageGuild->addItem(QApplication::translate("MapTownDialog", "Level 2", 0), 2);
    comboBoxMageGuild->addItem(QApplication::translate("MapTownDialog", "Level 3", 0), 3);
    comboBoxMageGuild->addItem(QApplication::translate("MapTownDialog", "Level 4", 0), 4);
    comboBoxMageGuild->addItem(QApplication::translate("MapTownDialog", "Level 5", 0), 5);
    comboBoxMageGuild->setCurrentIndex(0);

    horizontalLayoutMageGuild = new QHBoxLayout();
    horizontalLayoutMageGuild->addWidget(labelMageGuild);
    horizontalLayoutMageGuild->addWidget(comboBoxMageGuild);

    checkBoxMarket = new QCheckBox(tabBuildings);
    checkBoxMarket->setEnabled(! defaultBuildings);
    checkBoxMarket->setText(QApplication::translate("MapTownDialog", "Marketplace", 0));

    checkBoxLeftTurret = new QCheckBox(tabBuildings);
    checkBoxLeftTurret->setEnabled(! defaultBuildings);
    checkBoxLeftTurret->setText(QApplication::translate("MapTownDialog", "Left Turret", 0));

    horizontalLayoutB1 = new QHBoxLayout();
    horizontalLayoutB1->addWidget(checkBoxMarket);
    horizontalLayoutB1->addWidget(checkBoxLeftTurret);

    checkBoxTavern = new QCheckBox(tabBuildings);
    checkBoxTavern->setEnabled(! defaultBuildings);
    checkBoxTavern->setText(QApplication::translate("MapTownDialog", "Tavern", 0));

    checkBoxRightTurret = new QCheckBox(tabBuildings);
    checkBoxRightTurret->setEnabled(! defaultBuildings);
    checkBoxRightTurret->setText(QApplication::translate("MapTownDialog", "Right Turret", 0));

    horizontalLayoutB2 = new QHBoxLayout();
    horizontalLayoutB2->addWidget(checkBoxTavern);
    horizontalLayoutB2->addWidget(checkBoxRightTurret);

    checkBoxShipyard = new QCheckBox(tabBuildings);
    checkBoxShipyard->setEnabled(! defaultBuildings);
    checkBoxShipyard->setText(QApplication::translate("MapTownDialog", "Shipyard", 0));

    checkBoxMoat = new QCheckBox(tabBuildings);
    checkBoxMoat->setEnabled(! defaultBuildings);
    checkBoxMoat->setText(QApplication::translate("MapTownDialog", "Moat", 0));

    horizontalLayoutB3 = new QHBoxLayout();
    horizontalLayoutB3->addWidget(checkBoxShipyard);
    horizontalLayoutB3->addWidget(checkBoxMoat);

    checkBoxWell = new QCheckBox(tabBuildings);
    checkBoxWell->setEnabled(! defaultBuildings);
    checkBoxWell->setText(QApplication::translate("MapTownDialog", "Well", 0));

    checkBoxExt = new QCheckBox(tabBuildings);
    checkBoxExt->setEnabled(! defaultBuildings);
    checkBoxExt->setText(Building::extraWel2(town.race));

    horizontalLayoutB4 = new QHBoxLayout();
    horizontalLayoutB4->addWidget(checkBoxWell);
    horizontalLayoutB4->addWidget(checkBoxExt);

    checkBoxStatue = new QCheckBox(tabBuildings);
    checkBoxStatue->setEnabled(! defaultBuildings);
    checkBoxStatue->setText(QApplication::translate("MapTownDialog", "Statue", 0));

    checkBoxSpec = new QCheckBox(tabBuildings);
    checkBoxSpec->setEnabled(! defaultBuildings);
    checkBoxSpec->setText(Building::extraSpec(town.race));

    horizontalLayoutB5 = new QHBoxLayout();
    horizontalLayoutB5->addWidget(checkBoxStatue);
    horizontalLayoutB5->addWidget(checkBoxSpec);

    checkBoxThievesGuild = new QCheckBox(tabBuildings);
    checkBoxThievesGuild->setEnabled(! defaultBuildings);
    checkBoxThievesGuild->setText(QApplication::translate("MapTownDialog", "Thieves Guild", 0));

    verticalSpacerBuildings = new QSpacerItem(20, 45, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayoutBuildings = new QVBoxLayout(tabBuildings);
    verticalLayoutBuildings->addWidget(checkBoxBuildingsDefault);
    verticalLayoutBuildings->addLayout(horizontalLayoutMageGuild);
    verticalLayoutBuildings->addLayout(horizontalLayoutB1);
    verticalLayoutBuildings->addLayout(horizontalLayoutB2);
    verticalLayoutBuildings->addLayout(horizontalLayoutB3);
    verticalLayoutBuildings->addLayout(horizontalLayoutB4);
    verticalLayoutBuildings->addLayout(horizontalLayoutB5);
    verticalLayoutBuildings->addWidget(checkBoxThievesGuild);
    verticalLayoutBuildings->addItem(verticalSpacerBuildings);

    if(!defaultBuildings)
    {
	if(town.buildings & Building::MageGuild5)
	    comboBoxMageGuild->setCurrentIndex(5);
	else
	if(town.buildings & Building::MageGuild4)
	    comboBoxMageGuild->setCurrentIndex(4);
	else
	if(town.buildings & Building::MageGuild3)
	    comboBoxMageGuild->setCurrentIndex(3);
	else
	if(town.buildings & Building::MageGuild2)
	    comboBoxMageGuild->setCurrentIndex(2);
	else
	if(town.buildings & Building::MageGuild1)
	    comboBoxMageGuild->setCurrentIndex(1);

	checkBoxThievesGuild->setChecked(town.buildings & Building::ThievesGuild);
	checkBoxTavern->setChecked(town.buildings & Building::Tavern);
	checkBoxShipyard->setChecked(town.buildings & Building::Shipyard);
	checkBoxWell->setChecked(town.buildings & Building::Well);
	checkBoxStatue->setChecked(town.buildings & Building::Statue);
	checkBoxLeftTurret->setChecked(town.buildings & Building::LeftTurret);
	checkBoxRightTurret->setChecked(town.buildings & Building::RightTurret);
	checkBoxMarket->setChecked(town.buildings & Building::Marketplace);
	checkBoxMoat->setChecked(town.buildings & Building::Moat);
	checkBoxExt->setChecked(town.buildings & Building::ExtraWel2);
	checkBoxSpec->setChecked(town.buildings & Building::ExtraSpec);
    }

#ifndef QT_NO_TOOLTIP
    labelMageGuild->setToolTip(Building::description(Building::MageGuild, town.race));
    comboBoxMageGuild->setToolTip(Building::description(Building::MageGuild, town.race));
    checkBoxMarket->setToolTip(Building::description(Building::Marketplace, town.race));
    checkBoxLeftTurret->setToolTip(Building::description(Building::LeftTurret, town.race));
    checkBoxTavern->setToolTip(Building::description(Building::Tavern, town.race));
    checkBoxRightTurret->setToolTip(Building::description(Building::RightTurret, town.race));
    checkBoxShipyard->setToolTip(Building::description(Building::Shipyard, town.race));
    checkBoxMoat->setToolTip(Building::description(Building::Moat, town.race));
    checkBoxWell->setToolTip(Building::description(Building::Well, town.race));
    checkBoxExt->setToolTip(Building::description(Building::ExtraWel2, town.race));
    checkBoxStatue->setToolTip(Building::description(Building::Statue, town.race));
    checkBoxSpec->setToolTip(Building::description(Building::ExtraSpec, town.race));
    checkBoxThievesGuild->setToolTip(Building::description(Building::ThievesGuild, town.race));
#endif

    // tab: dwellings
    tabDwellings = new QWidget();
    bool defaultDwellings = ! town.customDwellings;
    int dwellingMap = Building::dwellingMap(town.race);

    checkBoxDwellingsDefault = new QCheckBox(tabDwellings);
    checkBoxDwellingsDefault->setChecked(defaultDwellings);
    checkBoxDwellingsDefault->setText(QApplication::translate("MapTownDialog", "Default", 0));

    checkBoxDwelling1 = new QCheckBox(tabDwellings);
    checkBoxDwelling1->setEnabled(! defaultDwellings);
    checkBoxDwelling1->setText(QApplication::translate("MapTownDialog", "Dwelling 1", 0));

    checkBoxDwelling2 = new QCheckBox(tabDwellings);
    checkBoxDwelling2->setEnabled(! defaultDwellings);
    checkBoxDwelling2->setText(QApplication::translate("MapTownDialog", "Dwelling 2", 0));

    checkBoxUpgrade2 = new QCheckBox(tabDwellings);
    checkBoxUpgrade2->setEnabled(! defaultDwellings);
    checkBoxUpgrade2->setVisible(dwellingMap & Building::Upgrade2);
    checkBoxUpgrade2->setText(QApplication::translate("MapTownDialog", "Upgrade 2", 0));

    horizontalLayoutD2 = new QHBoxLayout();
    horizontalLayoutD2->addWidget(checkBoxDwelling2);
    horizontalLayoutD2->addWidget(checkBoxUpgrade2);

    checkBoxDwelling3 = new QCheckBox(tabDwellings);
    checkBoxDwelling3->setEnabled(! defaultDwellings);
    checkBoxDwelling3->setText(QApplication::translate("MapTownDialog", "Dwelling 3", 0));

    checkBoxUpgrade3 = new QCheckBox(tabDwellings);
    checkBoxUpgrade3->setEnabled(! defaultDwellings);
    checkBoxUpgrade3->setVisible(dwellingMap & Building::Upgrade3);
    checkBoxUpgrade3->setText(QApplication::translate("MapTownDialog", "Upgrade 3", 0));

    horizontalLayoutD3 = new QHBoxLayout();
    horizontalLayoutD3->addWidget(checkBoxDwelling3);
    horizontalLayoutD3->addWidget(checkBoxUpgrade3);

    checkBoxDwelling4 = new QCheckBox(tabDwellings);
    checkBoxDwelling4->setEnabled(! defaultDwellings);
    checkBoxDwelling4->setText(QApplication::translate("MapTownDialog", "Dwelling 4", 0));

    checkBoxUpgrade4 = new QCheckBox(tabDwellings);
    checkBoxUpgrade4->setEnabled(! defaultDwellings);
    checkBoxUpgrade4->setVisible(dwellingMap & Building::Upgrade4);
    checkBoxUpgrade4->setText(QApplication::translate("MapTownDialog", "Upgrade 4", 0));

    horizontalLayoutD4 = new QHBoxLayout();
    horizontalLayoutD4->addWidget(checkBoxDwelling4);
    horizontalLayoutD4->addWidget(checkBoxUpgrade4);

    checkBoxDwelling5 = new QCheckBox(tabDwellings);
    checkBoxDwelling5->setEnabled(! defaultDwellings);
    checkBoxDwelling5->setText(QApplication::translate("MapTownDialog", "Dwelling 5", 0));

    checkBoxUpgrade5 = new QCheckBox(tabDwellings);
    checkBoxUpgrade5->setEnabled(! defaultDwellings);
    checkBoxUpgrade5->setVisible(dwellingMap & Building::Upgrade5);
    checkBoxUpgrade5->setText(QApplication::translate("MapTownDialog", "Upgrade 5", 0));

    horizontalLayoutD5 = new QHBoxLayout();
    horizontalLayoutD5->addWidget(checkBoxDwelling5);
    horizontalLayoutD5->addWidget(checkBoxUpgrade5);

    checkBoxDwelling6 = new QCheckBox(tabDwellings);
    checkBoxDwelling6->setEnabled(! defaultDwellings);
    checkBoxDwelling6->setText(QApplication::translate("MapTownDialog", "Dwelling 6", 0));

    checkBoxUpgrade6 = new QCheckBox(tabDwellings);
    checkBoxUpgrade6->setEnabled(! defaultDwellings);
    checkBoxUpgrade6->setVisible(dwellingMap & Building::Upgrade6);
    checkBoxUpgrade6->setText(QApplication::translate("MapTownDialog", "Upgrade 6", 0));

    horizontalLayoutD6 = new QHBoxLayout();
    horizontalLayoutD6->addWidget(checkBoxDwelling6);
    horizontalLayoutD6->addWidget(checkBoxUpgrade6);

    verticalSpacerDwellings = new QSpacerItem(20, 30, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayoutDwellings = new QVBoxLayout(tabDwellings);
    verticalLayoutDwellings->addWidget(checkBoxDwellingsDefault);
    verticalLayoutDwellings->addWidget(checkBoxDwelling1);
    verticalLayoutDwellings->addLayout(horizontalLayoutD2);
    verticalLayoutDwellings->addLayout(horizontalLayoutD3);
    verticalLayoutDwellings->addLayout(horizontalLayoutD4);
    verticalLayoutDwellings->addLayout(horizontalLayoutD5);
    verticalLayoutDwellings->addLayout(horizontalLayoutD6);
    verticalLayoutDwellings->addItem(verticalSpacerDwellings);

    if(!defaultBuildings)
    {
	checkBoxDwelling1->setChecked(town.dwellings & Building::Dwelling1);
	checkBoxDwelling2->setChecked(town.dwellings & Building::Dwelling2);
	checkBoxDwelling3->setChecked(town.dwellings & Building::Dwelling3);
	checkBoxDwelling4->setChecked(town.dwellings & Building::Dwelling4);
	checkBoxDwelling5->setChecked(town.dwellings & Building::Dwelling5);
	checkBoxDwelling6->setChecked(town.dwellings & Building::Dwelling6);

	checkBoxUpgrade2->setChecked(town.dwellings & Building::Upgrade2);
	checkBoxUpgrade3->setChecked(town.dwellings & Building::Upgrade3);
	checkBoxUpgrade4->setChecked(town.dwellings & Building::Upgrade4);
	checkBoxUpgrade5->setChecked(town.dwellings & Building::Upgrade5);
	checkBoxUpgrade6->setChecked(town.dwellings & Building::Upgrade6);
    }

    // add tabs
    tabWidget = new QTabWidget(this);
    tabWidget->addTab(tabInfo, "Info");
    tabWidget->addTab(tabTroops, "Troops");
    tabWidget->addTab(tabBuildings, "Buildings");
    tabWidget->addTab(tabDwellings, "Dwellings");
    tabWidget->setCurrentIndex(0);

    pushButtonOk = new QPushButton(this);
    pushButtonOk->setEnabled(false);
    pushButtonOk->setText(QApplication::translate("MapTownDialog", "Ok", 0));

    horizontalSpacerButton = new QSpacerItem(48, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    pushButtonCancel = new QPushButton(this);
    pushButtonCancel->setText(QApplication::translate("MapTownDialog", "Cancel", 0));

    horizontalLayoutButtons = new QHBoxLayout();
    horizontalLayoutButtons->addWidget(pushButtonOk);
    horizontalLayoutButtons->addItem(horizontalSpacerButton);
    horizontalLayoutButtons->addWidget(pushButtonCancel);

    verticalLayoutWidget = new QVBoxLayout(this);
    verticalLayoutWidget->addWidget(tabWidget);
    verticalLayoutWidget->addLayout(horizontalLayoutButtons);

    QSize minSize = minimumSizeHint();

    resize(minSize);
    setMinimumSize(minSize);

    connect(pushButtonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(pushButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));

    connect(checkBoxCaptain, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));
    connect(checkBoxAllowCastle, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));
    connect(comboBoxName, SIGNAL(editTextChanged(QString)), this, SLOT(setEnableOKButton()));
    connect(comboBoxName, SIGNAL(currentIndexChanged(int)), this, SLOT(setEnableOKButton()));
    connect(comboBoxColor, SIGNAL(currentIndexChanged(int)), this, SLOT(setEnableOKButton()));
    connect(checkBoxCaptain, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));

    connect(checkBoxTroopsDefault, SIGNAL(toggled(bool)), this, SLOT(setDefaultTroops(bool)));
    connect(comboBoxTroop1, SIGNAL(currentIndexChanged(int)), this, SLOT(setEnableOKButton()));
    connect(comboBoxTroop2, SIGNAL(currentIndexChanged(int)), this, SLOT(setEnableOKButton()));
    connect(comboBoxTroop3, SIGNAL(currentIndexChanged(int)), this, SLOT(setEnableOKButton()));
    connect(comboBoxTroop4, SIGNAL(currentIndexChanged(int)), this, SLOT(setEnableOKButton()));
    connect(comboBoxTroop5, SIGNAL(currentIndexChanged(int)), this, SLOT(setEnableOKButton()));
    connect(spinBoxCount1, SIGNAL(valueChanged(int)), this, SLOT(setEnableOKButton()));
    connect(spinBoxCount2, SIGNAL(valueChanged(int)), this, SLOT(setEnableOKButton()));
    connect(spinBoxCount3, SIGNAL(valueChanged(int)), this, SLOT(setEnableOKButton()));
    connect(spinBoxCount4, SIGNAL(valueChanged(int)), this, SLOT(setEnableOKButton()));
    connect(spinBoxCount5, SIGNAL(valueChanged(int)), this, SLOT(setEnableOKButton()));

    connect(checkBoxBuildingsDefault, SIGNAL(toggled(bool)), this, SLOT(setDefaultBuildings(bool)));
    connect(comboBoxMageGuild, SIGNAL(currentIndexChanged(int)), this, SLOT(setEnableOKButton()));
    connect(checkBoxMarket, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));
    connect(checkBoxLeftTurret, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));
    connect(checkBoxTavern, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));
    connect(checkBoxRightTurret, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));
    connect(checkBoxShipyard, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));
    connect(checkBoxMoat, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));
    connect(checkBoxWell, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));
    connect(checkBoxExt, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));
    connect(checkBoxStatue, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));
    connect(checkBoxSpec, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));
    connect(checkBoxThievesGuild, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));

    connect(checkBoxDwellingsDefault, SIGNAL(toggled(bool)), this, SLOT(setDefaultDwellings(bool)));
    connect(checkBoxDwelling1, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));
    connect(checkBoxDwelling2, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));
    connect(checkBoxDwelling3, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));
    connect(checkBoxDwelling4, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));
    connect(checkBoxDwelling5, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));
    connect(checkBoxDwelling6, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));
    connect(checkBoxUpgrade2, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));
    connect(checkBoxUpgrade3, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));
    connect(checkBoxUpgrade4, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));
    connect(checkBoxUpgrade5, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));
    connect(checkBoxUpgrade6, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));
}

Troops Form::MapTownDialog::troops(void) const
{
    Troops res;

    res[0] = Troop(comboBoxTroop1->currentIndex(), spinBoxCount1->value());
    res[1] = Troop(comboBoxTroop2->currentIndex(), spinBoxCount2->value());
    res[2] = Troop(comboBoxTroop3->currentIndex(), spinBoxCount3->value());
    res[3] = Troop(comboBoxTroop4->currentIndex(), spinBoxCount4->value());
    res[4] = Troop(comboBoxTroop5->currentIndex(), spinBoxCount5->value());

    return res;
}

uint Form::MapTownDialog::buildings(void) const
{
    uint res = 0;

    switch(comboBoxMageGuild->currentIndex())
    {
	case 1: res |= Building::MageGuild1; break;
	case 2: res |= Building::MageGuild2; break;
	case 3: res |= Building::MageGuild3; break;
	case 4: res |= Building::MageGuild4; break;
	case 5: res |= Building::MageGuild5; break;
	default: break;
    }

    if(checkBoxMarket->isChecked()) res |= Building::Marketplace;
    if(checkBoxLeftTurret->isChecked()) res |= Building::LeftTurret;
    if(checkBoxTavern->isChecked()) res |= Building::Tavern;
    if(checkBoxRightTurret->isChecked()) res |= Building::RightTurret;
    if(checkBoxShipyard->isChecked()) res |= Building::Shipyard;
    if(checkBoxMoat->isChecked()) res |= Building::Moat;
    if(checkBoxWell->isChecked()) res |= Building::Well;
    if(checkBoxExt->isChecked()) res |= Building::ExtraWel2;
    if(checkBoxStatue->isChecked()) res |= Building::Statue;
    if(checkBoxSpec->isChecked()) res |= Building::ExtraSpec;
    if(checkBoxThievesGuild->isChecked()) res |= Building::ThievesGuild;

    if(checkBoxCaptain->isChecked()) res |= Building::Captain;

    return res;
}

uint Form::MapTownDialog::dwellings(void) const
{
    uint res = 0;

    if(checkBoxDwelling1->isChecked()) res |= Building::Dwelling1;
    if(checkBoxDwelling2->isChecked()) res |= Building::Dwelling2;
    if(checkBoxDwelling3->isChecked()) res |= Building::Dwelling3;
    if(checkBoxDwelling4->isChecked()) res |= Building::Dwelling4;
    if(checkBoxDwelling5->isChecked()) res |= Building::Dwelling5;
    if(checkBoxDwelling6->isChecked()) res |= Building::Dwelling6;

    if(checkBoxUpgrade2->isChecked()) res |= Building::Upgrade2;
    if(checkBoxUpgrade3->isChecked()) res |= Building::Upgrade3;
    if(checkBoxUpgrade4->isChecked()) res |= Building::Upgrade4;
    if(checkBoxUpgrade5->isChecked()) res |= Building::Upgrade5;
    if(checkBoxUpgrade6->isChecked()) res |= Building::Upgrade6;

    return res;
}

void Form::MapTownDialog::setDefaultBuildings(bool f)
{
    if(f)
    {
	comboBoxMageGuild->setCurrentIndex(0);

	checkBoxMarket->setChecked(false);
	checkBoxLeftTurret->setChecked(false);
	checkBoxTavern->setChecked(false);
	checkBoxRightTurret->setChecked(false);
	checkBoxShipyard->setChecked(false);
	checkBoxMoat->setChecked(false);
	checkBoxWell->setChecked(false);
	checkBoxExt->setChecked(false);
	checkBoxStatue->setChecked(false);
	checkBoxSpec->setChecked(false);
	checkBoxThievesGuild->setChecked(false);
    }

    labelMageGuild->setDisabled(f);
    comboBoxMageGuild->setDisabled(f);

    checkBoxMarket->setDisabled(f);
    checkBoxLeftTurret->setDisabled(f);
    checkBoxTavern->setDisabled(f);
    checkBoxRightTurret->setDisabled(f);
    checkBoxShipyard->setDisabled(f);
    checkBoxMoat->setDisabled(f);
    checkBoxWell->setDisabled(f);
    checkBoxExt->setDisabled(f);
    checkBoxStatue->setDisabled(f);
    checkBoxSpec->setDisabled(f);
    checkBoxThievesGuild->setDisabled(f);

    setEnableOKButton();
}

void Form::MapTownDialog::setDefaultDwellings(bool f)
{
    if(f)
    {
	checkBoxDwelling1->setChecked(false);
	checkBoxDwelling2->setChecked(false);
	checkBoxDwelling3->setChecked(false);
        checkBoxDwelling4->setChecked(false);
	checkBoxDwelling5->setChecked(false);
	checkBoxDwelling6->setChecked(false);

	checkBoxUpgrade2->setChecked(false);
	checkBoxUpgrade3->setChecked(false);
	checkBoxUpgrade4->setChecked(false);
	checkBoxUpgrade5->setChecked(false);
	checkBoxUpgrade6->setChecked(false);
    }

    checkBoxDwelling1->setDisabled(f);
    checkBoxDwelling2->setDisabled(f);
    checkBoxDwelling3->setDisabled(f);
    checkBoxDwelling4->setDisabled(f);
    checkBoxDwelling5->setDisabled(f);
    checkBoxDwelling6->setDisabled(f);

    checkBoxUpgrade2->setDisabled(f);
    checkBoxUpgrade3->setDisabled(f);
    checkBoxUpgrade4->setDisabled(f);
    checkBoxUpgrade5->setDisabled(f);
    checkBoxUpgrade6->setDisabled(f);

    setEnableOKButton();
}

void Form::MapTownDialog::setDefaultTroops(bool f)
{
    if(f)
    {
	comboBoxTroop1->setCurrentIndex(0);
	comboBoxTroop2->setCurrentIndex(0);
	comboBoxTroop3->setCurrentIndex(0);
	comboBoxTroop4->setCurrentIndex(0);
	comboBoxTroop5->setCurrentIndex(0);

	spinBoxCount1->setValue(0);
	spinBoxCount2->setValue(0);
	spinBoxCount3->setValue(0);
	spinBoxCount4->setValue(0);
	spinBoxCount5->setValue(0);
    }

    labelSlot1->setDisabled(f);
    labelSlot2->setDisabled(f);
    labelSlot3->setDisabled(f);
    labelSlot4->setDisabled(f);
    labelSlot5->setDisabled(f);

    comboBoxTroop1->setDisabled(f);
    comboBoxTroop2->setDisabled(f);
    comboBoxTroop3->setDisabled(f);
    comboBoxTroop4->setDisabled(f);
    comboBoxTroop5->setDisabled(f);

    spinBoxCount1->setDisabled(f);
    spinBoxCount2->setDisabled(f);
    spinBoxCount3->setDisabled(f);
    spinBoxCount4->setDisabled(f);
    spinBoxCount5->setDisabled(f);

    setEnableOKButton();
}

void Form::MapTownDialog::setEnableOKButton(void)
{
    pushButtonOk->setEnabled(true);
}

Form::SignDialog::SignDialog(const QString & msg)
{
    setWindowTitle(QApplication::translate("SignDialog", "Sign Detail", 0));

    plainTextEdit = new QPlainTextEdit(this);
    plainTextEdit->setPlainText(msg);

    pushButtonOk = new QPushButton(this);
    pushButtonOk->setEnabled(false);
    pushButtonOk->setText(QApplication::translate("SignDialog", "Ok", 0));

    horizontalSpacerButtons = new QSpacerItem(88, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    pushButtonCancel = new QPushButton(this);
    pushButtonCancel->setText(QApplication::translate("SignDialog", "Cancel", 0));

    horizontalLayout = new QHBoxLayout();
    horizontalLayout->addWidget(pushButtonOk);
    horizontalLayout->addItem(horizontalSpacerButtons);
    horizontalLayout->addWidget(pushButtonCancel);

    verticalLayout = new QVBoxLayout(this);
    verticalLayout->addWidget(plainTextEdit);
    verticalLayout->addLayout(horizontalLayout);

    QSize minSize(240, 170);

    resize(minSize);
    setMinimumSize(minSize);

    connect(pushButtonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(pushButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(plainTextEdit, SIGNAL(modificationChanged(bool)), this, SLOT(setEnableOKButton()));
}

void Form::SignDialog::setEnableOKButton(void)
{
    pushButtonOk->setEnabled(true);
}

Form::MapHeroDialog::MapHeroDialog(const MapHero & hero)
{
    setWindowTitle(QApplication::translate("MapHeroDialog", "Hero Detail", 0));

    // tab: info
    tabInfo = new QWidget();

    labelName = new QLabel(tabInfo);
    labelName->setText(QApplication::translate("MapHeroDialog", "Name", 0));

    lineEditName = new QLineEdit(tabInfo);
    lineEditName->setText(hero.nameHero);

    horizontalLayoutName = new QHBoxLayout();
    horizontalLayoutName->addWidget(labelName);
    horizontalLayoutName->addWidget(lineEditName);

    labelColor = new QLabel(tabInfo);
    labelColor->setText(QApplication::translate("MapHeroDialog", "Color", 0));

    comboBoxColor = new QComboBox(tabInfo);

    QVector<int> colors = Color::colors(Color::All);
    if(hero.jailMode)
    {
	comboBoxColor->setDisabled(true);
	comboBoxColor->addItem(Editor::pixmapBorder(QSize(24, 24), Color::convert(Color::None), QColor(0, 0, 0)), "Jail", Color::None);
	comboBoxColor->setCurrentIndex(0);
    }
    else
    {
	for(QVector<int>::const_iterator
    	    it = colors.begin(); it != colors.end(); ++it)
    	    comboBoxColor->addItem(Editor::pixmapBorder(QSize(24, 24), Color::convert(*it), QColor(0, 0, 0)), Color::transcribe(*it), *it);
	comboBoxColor->setCurrentIndex(Color::index(hero.color()) - 1);
    }

    horizontalLayoutColor = new QHBoxLayout();
    horizontalLayoutColor->addWidget(labelColor);
    horizontalLayoutColor->addWidget(comboBoxColor);

    labelRace = new QLabel(tabInfo);
    labelRace->setText(QApplication::translate("MapHeroDialog", "Type", 0));

    comboBoxRace = new QComboBox(tabInfo);
    comboBoxRace->addItem(Race::transcribe(Race::Knight), Race::Knight);
    comboBoxRace->addItem(Race::transcribe(Race::Barbarian), Race::Barbarian);
    comboBoxRace->addItem(Race::transcribe(Race::Sorceress), Race::Sorceress);
    comboBoxRace->addItem(Race::transcribe(Race::Warlock), Race::Warlock);
    comboBoxRace->addItem(Race::transcribe(Race::Wizard), Race::Wizard);
    comboBoxRace->addItem(Race::transcribe(Race::Necromancer), Race::Necromancer);
    comboBoxRace->addItem(Race::transcribe(Race::Random), Race::Random);
    comboBoxRace->setCurrentIndex(Race::index(hero.race) - 1);

    horizontalLayoutRace = new QHBoxLayout();
    horizontalLayoutRace->addWidget(labelRace);
    horizontalLayoutRace->addWidget(comboBoxRace);

    labelExperience = new QLabel(tabInfo);
    labelExperience->setText(QApplication::translate("MapHeroDialog", "Experience", 0));

    lineEditExperience = new QLineEdit(tabInfo);
    //lineEditExperience->setInputMethodHints(Qt::ImhDigitsOnly);
    lineEditExperience->setValidator(new QDoubleValidator(0, 100000, 1, this));
    lineEditExperience->setText(QString::number(hero.experience));

    horizontalLayoutExp = new QHBoxLayout();
    horizontalLayoutExp->addWidget(labelExperience);
    horizontalLayoutExp->addWidget(lineEditExperience);

    verticalLayoutNameExp = new QVBoxLayout();
    verticalLayoutNameExp->addLayout(horizontalLayoutName);
    verticalLayoutNameExp->addLayout(horizontalLayoutColor);
    verticalLayoutNameExp->addLayout(horizontalLayoutRace);
    verticalLayoutNameExp->addLayout(horizontalLayoutExp);

    horizontalSpacerCenter = new QSpacerItem(18, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    labelPortrait = new QLabel(tabInfo);
    labelPortrait->setPixmap(EditorTheme::getImageICN("PORTMEDI.ICN", hero.portrait).first);

    verticalScrollBarPort = new QScrollBar(tabInfo);
    verticalScrollBarPort->setPageStep(1);
    verticalScrollBarPort->setTracking(false);
    verticalScrollBarPort->setMinimum(Portrait::Unknown);
    verticalScrollBarPort->setMaximum(Portrait::Random - 1);
    verticalScrollBarPort->setValue(hero.portrait);
    verticalScrollBarPort->setOrientation(Qt::Vertical);
    verticalScrollBarPort->setInvertedAppearance(false);

    horizontalLayout = new QHBoxLayout();
    horizontalLayout->addLayout(verticalLayoutNameExp);
    horizontalLayout->addItem(horizontalSpacerCenter);
    horizontalLayout->addWidget(labelPortrait);
    horizontalLayout->addWidget(verticalScrollBarPort);

    verticalSpacerInfo = new QSpacerItem(20, 121, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayoutInfo = new QVBoxLayout(tabInfo);
    verticalLayoutInfo->addLayout(horizontalLayout);
    verticalLayoutInfo->addItem(verticalSpacerInfo);

    // tab: primary skills
    tabPrimarySkills = new QWidget();

    verticalLayoutAttack = new QVBoxLayout();
    verticalLayoutDefence = new QVBoxLayout();
    verticalLayoutPower = new QVBoxLayout();
    verticalLayoutKnowledge = new QVBoxLayout();

    spinBoxAttack = new QSpinBox(tabPrimarySkills);
    spinBoxAttack->setMaximum(20);
    spinBoxAttack->setValue(hero.attack);

    labelAttack = new QLabel("Attack", tabPrimarySkills);
    labelAttack->setPixmap(EditorTheme::getImageICN("PRIMSKIL.ICN", 0).first);

    verticalLayoutAttack->addWidget(labelAttack);
    verticalLayoutAttack->addWidget(spinBoxAttack);

    spinBoxDefence = new QSpinBox(tabPrimarySkills);
    spinBoxDefence->setMaximum(20);
    spinBoxDefence->setValue(hero.defence);

    labelDefence = new QLabel("Defence", tabPrimarySkills);
    labelDefence->setPixmap(EditorTheme::getImageICN("PRIMSKIL.ICN", 1).first);

    verticalLayoutDefence->addWidget(labelDefence);
    verticalLayoutDefence->addWidget(spinBoxDefence);

    spinBoxPower = new QSpinBox(tabPrimarySkills);
    spinBoxPower->setMaximum(20);
    spinBoxPower->setValue(hero.power);

    labelPower = new QLabel("Power", tabPrimarySkills);
    labelPower->setPixmap(EditorTheme::getImageICN("PRIMSKIL.ICN", 2).first);

    verticalLayoutPower->addWidget(labelPower);
    verticalLayoutPower->addWidget(spinBoxPower);

    spinBoxKnowledge = new QSpinBox(tabPrimarySkills);
    spinBoxKnowledge->setMaximum(20);
    spinBoxKnowledge->setValue(hero.knowledge);

    labelKnowledge = new QLabel("Knowledge", tabPrimarySkills);
    labelKnowledge->setPixmap(EditorTheme::getImageICN("PRIMSKIL.ICN", 3).first);

    verticalLayoutKnowledge->addWidget(labelKnowledge);
    verticalLayoutKnowledge->addWidget(spinBoxKnowledge);

#ifndef QT_NO_TOOLTIP
    labelAttack->setToolTip("Attack");
    labelDefence->setToolTip("Defence");
    labelPower->setToolTip("Power");
    labelKnowledge->setToolTip("Knowledge");
    spinBoxAttack->setToolTip("Attack");
    spinBoxDefence->setToolTip("Defence");
    spinBoxPower->setToolTip("Power");
    spinBoxKnowledge->setToolTip("Knowledge");
#endif

    horizontalLayoutPrimarySkills = new QHBoxLayout();
    horizontalLayoutPrimarySkills->addLayout(verticalLayoutAttack);
    horizontalLayoutPrimarySkills->addLayout(verticalLayoutDefence);
    horizontalLayoutPrimarySkills->addLayout(verticalLayoutPower);
    horizontalLayoutPrimarySkills->addLayout(verticalLayoutKnowledge);

    verticalSpacerPrimSkills = new QSpacerItem(20, 121, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayoutPrimSkills = new QVBoxLayout(tabPrimarySkills);
    verticalLayoutPrimSkills->addLayout(horizontalLayoutPrimarySkills);
    verticalLayoutPrimSkills->addItem(verticalSpacerPrimSkills);

    // tab: troops
    tabTroops = new QWidget();
    bool defaultTroops = 0 == hero.troops.validCount();

    checkBoxTroopsDefault = new QCheckBox(tabTroops);
    checkBoxTroopsDefault->setChecked(defaultTroops);
    checkBoxTroopsDefault->setText(QApplication::translate("MapHeroDialog", "Default", 0));

    labelSlot1 = new QLabel(tabTroops);
    labelSlot1->setEnabled(! defaultTroops);
    labelSlot1->setText(QApplication::translate("MapHeroDialog", "Slot 1", 0));

    comboBoxTroop1 = new QComboBox(tabTroops);
    comboBoxTroop1->setEnabled(! defaultTroops);

    spinBoxCount1 = new QSpinBox(tabTroops);
    spinBoxCount1->setEnabled(! defaultTroops);
    spinBoxCount1->setMaximumWidth(61);
    spinBoxCount1->setMaximum(65535);

    horizontalLayoutT1 = new QHBoxLayout();
    horizontalLayoutT1->addWidget(labelSlot1);
    horizontalLayoutT1->addWidget(comboBoxTroop1);
    horizontalLayoutT1->addWidget(spinBoxCount1);

    labelSlot2 = new QLabel(tabTroops);
    labelSlot2->setEnabled(! defaultTroops);
    labelSlot2->setText(QApplication::translate("MapHeroDialog", "Slot 2", 0));

    comboBoxTroop2 = new QComboBox(tabTroops);
    comboBoxTroop2->setEnabled(! defaultTroops);

    spinBoxCount2 = new QSpinBox(tabTroops);
    spinBoxCount2->setEnabled(! defaultTroops);
    spinBoxCount2->setMaximumWidth(61);
    spinBoxCount2->setMaximum(65535);

    horizontalLayoutT2 = new QHBoxLayout();
    horizontalLayoutT2->addWidget(labelSlot2);
    horizontalLayoutT2->addWidget(comboBoxTroop2);
    horizontalLayoutT2->addWidget(spinBoxCount2);

    labelSlot3 = new QLabel(tabTroops);
    labelSlot3->setEnabled(! defaultTroops);
    labelSlot3->setText(QApplication::translate("MapHeroDialog", "Slot 3", 0));

    comboBoxTroop3 = new QComboBox(tabTroops);
    comboBoxTroop3->setEnabled(! defaultTroops);

    spinBoxCount3 = new QSpinBox(tabTroops);
    spinBoxCount3->setEnabled(! defaultTroops);
    spinBoxCount3->setMaximumWidth(61);
    spinBoxCount3->setMaximum(65535);

    horizontalLayoutT3 = new QHBoxLayout();
    horizontalLayoutT3->addWidget(labelSlot3);
    horizontalLayoutT3->addWidget(comboBoxTroop3);
    horizontalLayoutT3->addWidget(spinBoxCount3);

    labelSlot4 = new QLabel(tabTroops);
    labelSlot4->setEnabled(! defaultTroops);
    labelSlot4->setText(QApplication::translate("MapHeroDialog", "Slot 4", 0));

    comboBoxTroop4 = new QComboBox(tabTroops);
    comboBoxTroop4->setEnabled(! defaultTroops);

    spinBoxCount4 = new QSpinBox(tabTroops);
    spinBoxCount4->setEnabled(! defaultTroops);
    spinBoxCount4->setMaximumWidth(61);
    spinBoxCount4->setMaximum(65535);

    horizontalLayoutT4 = new QHBoxLayout();
    horizontalLayoutT4->addWidget(labelSlot4);
    horizontalLayoutT4->addWidget(comboBoxTroop4);
    horizontalLayoutT4->addWidget(spinBoxCount4);

    labelSlot5 = new QLabel(tabTroops);
    labelSlot5->setEnabled(! defaultTroops);
    labelSlot5->setText(QApplication::translate("MapHeroDialog", "Slot 5", 0));

    comboBoxTroop5 = new QComboBox(tabTroops);
    comboBoxTroop5->setEnabled(! defaultTroops);

    spinBoxCount5 = new QSpinBox(tabTroops);
    spinBoxCount5->setEnabled(! defaultTroops);
    spinBoxCount5->setMaximumWidth(61);
    spinBoxCount5->setMaximum(65535);

    horizontalLayoutT5 = new QHBoxLayout();
    horizontalLayoutT5->addWidget(labelSlot5);
    horizontalLayoutT5->addWidget(comboBoxTroop5);
    horizontalLayoutT5->addWidget(spinBoxCount5);

    for(int index = Monster::None; index < Monster::Unknown; ++index)
    {
	QPixmap mons32 = EditorTheme::getImageICN(ICN::MONS32, index - 1).first;
	comboBoxTroop1->addItem(mons32, Monster::transcribe(index));
	comboBoxTroop2->addItem(mons32, Monster::transcribe(index));
	comboBoxTroop3->addItem(mons32, Monster::transcribe(index));
	comboBoxTroop4->addItem(mons32, Monster::transcribe(index));
	comboBoxTroop5->addItem(mons32, Monster::transcribe(index));
#ifndef QT_NO_TOOLTIP
	comboBoxTroop1->setItemData(index, Monster::tips(index), Qt::ToolTipRole);
	comboBoxTroop2->setItemData(index, Monster::tips(index), Qt::ToolTipRole);
	comboBoxTroop3->setItemData(index, Monster::tips(index), Qt::ToolTipRole);
	comboBoxTroop4->setItemData(index, Monster::tips(index), Qt::ToolTipRole);
	comboBoxTroop5->setItemData(index, Monster::tips(index), Qt::ToolTipRole);
#endif
    }

    if(! defaultTroops)
    {
	comboBoxTroop1->setCurrentIndex(hero.troops[0].type());
	comboBoxTroop2->setCurrentIndex(hero.troops[1].type());
	comboBoxTroop3->setCurrentIndex(hero.troops[2].type());
	comboBoxTroop4->setCurrentIndex(hero.troops[3].type());
	comboBoxTroop5->setCurrentIndex(hero.troops[4].type());

	spinBoxCount1->setValue(hero.troops[0].count());
	spinBoxCount2->setValue(hero.troops[1].count());
	spinBoxCount3->setValue(hero.troops[2].count());
	spinBoxCount4->setValue(hero.troops[3].count());
	spinBoxCount5->setValue(hero.troops[4].count());
    }

    verticalSpacerTroops = new QSpacerItem(20, 19, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayoutTroops = new QVBoxLayout(tabTroops);
    verticalLayoutTroops->addWidget(checkBoxTroopsDefault);
    verticalLayoutTroops->addLayout(horizontalLayoutT1);
    verticalLayoutTroops->addLayout(horizontalLayoutT2);
    verticalLayoutTroops->addLayout(horizontalLayoutT3);
    verticalLayoutTroops->addLayout(horizontalLayoutT4);
    verticalLayoutTroops->addLayout(horizontalLayoutT5);
    verticalLayoutTroops->addItem(verticalSpacerTroops);

    // tab: artifacts
    tabArtifacts = new QWidget();

    listWidgetArtifacts = new ArtifactsList(tabArtifacts);
    verticalSpacerArtifacts = new QSpacerItem(20, 36, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayoutArtifacts = new QVBoxLayout(tabArtifacts);
    verticalLayoutArtifacts->addWidget(listWidgetArtifacts);
    verticalLayoutArtifacts->addItem(verticalSpacerArtifacts);

    for(QVector<int>::const_iterator
	it = hero.artifacts.begin(); it != hero.artifacts.end(); ++it)
    {
	QListWidgetItem* item = new QListWidgetItem(EditorTheme::getImageICN("ARTFX.ICN", *it - 1).first, Artifact::transcribe(*it));
	item->setData(Qt::UserRole, *it);
#ifndef QT_NO_TOOLTIP
	item->setToolTip(Artifact::description(*it));
#endif
	static_cast<QListWidget*>(listWidgetArtifacts)->addItem(item);
    }

    // tab: skills
    tabSecSkills = new QWidget();
    bool defaultSkills = 0 == hero.skills.size();

    checkBoxDefaultSkills = new QCheckBox(tabSecSkills);
    checkBoxDefaultSkills->setChecked(defaultSkills);
    checkBoxDefaultSkills->setText(QApplication::translate("MapHeroDialog", "Default", 0));

    listWidgetSkills = new SkillsList(tabSecSkills);
    listWidgetSkills->setVisible(! defaultSkills);

    verticalSpacerSkills = new QSpacerItem(20, 6, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayoutSkills = new QVBoxLayout(tabSecSkills);
    verticalLayoutSkills->addWidget(checkBoxDefaultSkills);
    verticalLayoutSkills->addWidget(listWidgetSkills);
    verticalLayoutSkills->addItem(verticalSpacerSkills);

    for(Skills::const_iterator
	it = hero.skills.begin(); it != hero.skills.end(); ++it)
    {
	QListWidgetItem* item = new QListWidgetItem((*it).pixmap(), (*it).name());
	item->setData(Qt::UserRole, QVariant::fromValue(*it));
#ifndef QT_NO_TOOLTIP
	item->setToolTip((*it).description());
#endif
	static_cast<QListWidget*>(listWidgetSkills)->addItem(item);
    }

    // tab: spells
    tabSpells = new QWidget();

    checkBoxHaveMagicBook = new QCheckBox(tabSpells);
    checkBoxHaveMagicBook->setChecked(hero.haveMagicBook());
    checkBoxHaveMagicBook->setText(QApplication::translate("MapHeroDialog", "Have magic book", 0));

    listWidgetSpells = new SpellsList(tabSpells);
    listWidgetSpells->setEnabled(hero.haveMagicBook());
    verticalSpacerSpells = new QSpacerItem(20, 6, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayoutSpells = new QVBoxLayout(tabSpells);
    verticalLayoutSpells->addWidget(checkBoxHaveMagicBook);
    verticalLayoutSpells->addWidget(listWidgetSpells);
    verticalLayoutSpells->addItem(verticalSpacerSpells);

    if(hero.haveMagicBook())
    for(QVector<int>::const_iterator
	it = hero.spells.begin(); it != hero.spells.end(); ++it)
    {
	QListWidgetItem* item = new QListWidgetItem(Spell::pixmap(*it), Spell::transcribe(*it));
	item->setData(Qt::UserRole, QVariant::fromValue(*it));
#ifndef QT_NO_TOOLTIP
	item->setToolTip(Spell::tips(*it));
#endif
	static_cast<QListWidget*>(listWidgetSpells)->addItem(item);
    }

    // tab: other
    tabOther = new QWidget();

    groupBoxPatrol = new QGroupBox(tabOther);
    groupBoxPatrol->setTitle(QApplication::translate("MapHeroDialog", "Patrol", 0));

    checkBoxEnablePatrol = new QCheckBox(groupBoxPatrol);
    checkBoxEnablePatrol->setChecked(hero.patrolMode);

    checkBoxEnablePatrol->setText(QApplication::translate("MapHeroDialog", "Enable", 0));

    comboBoxPatrol = new QComboBox(groupBoxPatrol);
    comboBoxPatrol->setEnabled(hero.patrolMode);
    comboBoxPatrol->addItem(QApplication::translate("MapHeroDialog", "Stand still", 0), 0);
    comboBoxPatrol->addItem(QApplication::translate("MapHeroDialog", "Radius 1 square", 0), 1);
    comboBoxPatrol->addItem(QApplication::translate("MapHeroDialog", "Radius 2 squares", 0), 2);
    comboBoxPatrol->addItem(QApplication::translate("MapHeroDialog", "Radius 3 squares", 0), 3);
    comboBoxPatrol->addItem(QApplication::translate("MapHeroDialog", "Radius 4 squares", 0), 4);
    comboBoxPatrol->addItem(QApplication::translate("MapHeroDialog", "Radius 5 squares", 0), 5);
    comboBoxPatrol->addItem(QApplication::translate("MapHeroDialog", "Radius 6 squares", 0), 6);
    comboBoxPatrol->addItem(QApplication::translate("MapHeroDialog", "Radius 7 squares", 0), 7);
    comboBoxPatrol->addItem(QApplication::translate("MapHeroDialog", "Radius 8 squares", 0), 8);
    comboBoxPatrol->addItem(QApplication::translate("MapHeroDialog", "Radius 9 squares", 0), 9);
    comboBoxPatrol->addItem(QApplication::translate("MapHeroDialog", "Radius 10 squares", 0), 10);
    comboBoxPatrol->setCurrentIndex(hero.patrolSquare < comboBoxPatrol->count() ? hero.patrolSquare : 0);

    verticalLayoutPatrol = new QVBoxLayout(groupBoxPatrol);
    verticalLayoutPatrol->addWidget(checkBoxEnablePatrol);
    verticalLayoutPatrol->addWidget(comboBoxPatrol);

    verticalSpacerOther = new QSpacerItem(20, 96, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayoutOther = new QVBoxLayout(tabOther);
    verticalLayoutOther->addWidget(groupBoxPatrol);
    verticalLayoutOther->addItem(verticalSpacerOther);

    // buttons
    pushButtonOk = new QPushButton(this);
    pushButtonOk->setEnabled(false);
    pushButtonOk->setText(QApplication::translate("MapHeroDialog", "Ok", 0));

    horizontalSpacerButtons = new QSpacerItem(68, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    pushButtonCancel = new QPushButton(this);
    pushButtonCancel->setText(QApplication::translate("MapHeroDialog", "Cancel", 0));

    horizontalLayoutButtons = new QHBoxLayout();
    horizontalLayoutButtons->addWidget(pushButtonOk);
    horizontalLayoutButtons->addItem(horizontalSpacerButtons);
    horizontalLayoutButtons->addWidget(pushButtonCancel);

    tabWidget = new QTabWidget(this);
    tabWidget->addTab(tabInfo, "Info");
    tabWidget->addTab(tabPrimarySkills, "Primary Skills");
    tabWidget->addTab(tabSecSkills, "Secondary Skills");
    tabWidget->addTab(tabTroops, "Troops");
    tabWidget->addTab(tabArtifacts, "Artifacts");
    tabWidget->addTab(tabSpells, "Spells");
    tabWidget->addTab(tabOther, "Other");
    tabWidget->setCurrentIndex(0);

    tabSpellsIndex = 5;

    verticalLayoutWidget = new QVBoxLayout(this);
    verticalLayoutWidget->addWidget(tabWidget);
    verticalLayoutWidget->addLayout(horizontalLayoutButtons);

    QSize minSize = minimumSizeHint();

    resize(minSize);
    setMinimumSize(minSize);

    connect(pushButtonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(pushButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));

    connect(lineEditName, SIGNAL(textChanged(QString)), this, SLOT(setEnableOKButton()));
    connect(lineEditExperience, SIGNAL(textChanged(QString)), this, SLOT(setEnableOKButton()));
    connect(comboBoxColor, SIGNAL(currentIndexChanged(int)), this, SLOT(setEnableOKButton()));
    connect(comboBoxRace, SIGNAL(currentIndexChanged(int)), this, SLOT(setEnableOKButton()));
    connect(verticalScrollBarPort, SIGNAL(valueChanged(int)), this, SLOT(setPortrait(int)));

    connect(spinBoxAttack, SIGNAL(valueChanged(int)), this, SLOT(setEnableOKButton()));
    connect(spinBoxDefence, SIGNAL(valueChanged(int)), this, SLOT(setEnableOKButton()));
    connect(spinBoxPower, SIGNAL(valueChanged(int)), this, SLOT(setEnableOKButton()));
    connect(spinBoxKnowledge, SIGNAL(valueChanged(int)), this, SLOT(setEnableOKButton()));

    connect(checkBoxTroopsDefault, SIGNAL(toggled(bool)), this, SLOT(setDefaultTroops(bool)));
    connect(comboBoxTroop1, SIGNAL(currentIndexChanged(int)), this, SLOT(setEnableOKButton()));
    connect(comboBoxTroop2, SIGNAL(currentIndexChanged(int)), this, SLOT(setEnableOKButton()));
    connect(comboBoxTroop3, SIGNAL(currentIndexChanged(int)), this, SLOT(setEnableOKButton()));
    connect(comboBoxTroop4, SIGNAL(currentIndexChanged(int)), this, SLOT(setEnableOKButton()));
    connect(comboBoxTroop5, SIGNAL(currentIndexChanged(int)), this, SLOT(setEnableOKButton()));
    connect(spinBoxCount1, SIGNAL(valueChanged(int)), this, SLOT(setEnableOKButton()));
    connect(spinBoxCount2, SIGNAL(valueChanged(int)), this, SLOT(setEnableOKButton()));
    connect(spinBoxCount3, SIGNAL(valueChanged(int)), this, SLOT(setEnableOKButton()));
    connect(spinBoxCount4, SIGNAL(valueChanged(int)), this, SLOT(setEnableOKButton()));
    connect(spinBoxCount5, SIGNAL(valueChanged(int)), this, SLOT(setEnableOKButton()));

    connect(checkBoxDefaultSkills, SIGNAL(toggled(bool)), this, SLOT(widgetSkillsVisible(bool)));
    connect(listWidgetArtifacts, SIGNAL(listChanged()), this, SLOT(setEnableOKButton()));

    connect(checkBoxHaveMagicBook, SIGNAL(toggled(bool)), this, SLOT(widgetSpellsVisible(bool)));
    connect(listWidgetSpells, SIGNAL(listChanged()), this, SLOT(setEnableOKButton()));

    connect(checkBoxEnablePatrol, SIGNAL(toggled(bool)), comboBoxPatrol, SLOT(setEnabled(bool)));
    connect(checkBoxEnablePatrol, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));
    connect(comboBoxPatrol, SIGNAL(currentIndexChanged(int)), this, SLOT(setEnableOKButton()));
}

void Form::MapHeroDialog::widgetSpellsVisible(bool f)
{
    listWidgetSpells->setEnabled(f);
    setEnableOKButton();
}

void Form::MapHeroDialog::widgetSkillsVisible(bool f)
{
    if(f) listWidgetSkills->clear();
    listWidgetSkills->setVisible(! f);
    setEnableOKButton();
}

void Form::MapHeroDialog::setPortrait(int val)
{
    labelPortrait->setPixmap(EditorTheme::getImageICN("PORTMEDI.ICN", val).first);
    if(val)
    {
	int curRace = (val - 1) / 9;
	comboBoxRace->setCurrentIndex(curRace > 5 ? 6 : curRace);
	lineEditName->setText(Portrait::transcribe(val));
    }
    else
    {
	comboBoxRace->setCurrentIndex(6);
	lineEditName->setText("Random");
    }

    setEnableOKButton();
}

void Form::MapHeroDialog::setEnableOKButton(void)
{
    pushButtonOk->setEnabled(true);
}

void Form::MapHeroDialog::setDefaultTroops(bool f)
{
    if(f)
    {
	comboBoxTroop1->setCurrentIndex(0);
	comboBoxTroop2->setCurrentIndex(0);
	comboBoxTroop3->setCurrentIndex(0);
	comboBoxTroop4->setCurrentIndex(0);
	comboBoxTroop5->setCurrentIndex(0);

	spinBoxCount1->setValue(0);
	spinBoxCount2->setValue(0);
	spinBoxCount3->setValue(0);
	spinBoxCount4->setValue(0);
	spinBoxCount5->setValue(0);
    }

    labelSlot1->setDisabled(f);
    labelSlot2->setDisabled(f);
    labelSlot3->setDisabled(f);
    labelSlot4->setDisabled(f);
    labelSlot5->setDisabled(f);

    comboBoxTroop1->setDisabled(f);
    comboBoxTroop2->setDisabled(f);
    comboBoxTroop3->setDisabled(f);
    comboBoxTroop4->setDisabled(f);
    comboBoxTroop5->setDisabled(f);

    spinBoxCount1->setDisabled(f);
    spinBoxCount2->setDisabled(f);
    spinBoxCount3->setDisabled(f);
    spinBoxCount4->setDisabled(f);
    spinBoxCount5->setDisabled(f);

    setEnableOKButton();
}

Troops Form::MapHeroDialog::troops(void) const
{
    Troops res;

    res[0] = Troop(comboBoxTroop1->currentIndex(), spinBoxCount1->value());
    res[1] = Troop(comboBoxTroop2->currentIndex(), spinBoxCount2->value());
    res[2] = Troop(comboBoxTroop3->currentIndex(), spinBoxCount3->value());
    res[3] = Troop(comboBoxTroop4->currentIndex(), spinBoxCount4->value());
    res[4] = Troop(comboBoxTroop5->currentIndex(), spinBoxCount5->value());

    return res;
}

QVector<int> Form::MapHeroDialog::artifacts(void) const
{
    QVector<int> res;

    for(int index = 0; index < listWidgetArtifacts->count(); ++index)
	res.push_back(listWidgetArtifacts->item(index)->data(Qt::UserRole).toInt());

    return res;
}

bool Form::MapHeroDialog::book(void) const
{
    return checkBoxHaveMagicBook->isChecked();
}

QVector<int> Form::MapHeroDialog::spells(void) const
{
    QVector<int> res;

    if(checkBoxHaveMagicBook->isChecked())
    for(int index = 0; index < listWidgetSpells->count(); ++index)
	res.push_back(listWidgetSpells->item(index)->data(Qt::UserRole).toInt());

    return res;
}

Skills Form::MapHeroDialog::skills(void) const
{
    Skills res;

    for(int index = 0; index < listWidgetSkills->count(); ++index)
	res.push_back(qvariant_cast<Skill>(listWidgetSkills->item(index)->data(Qt::UserRole)));

    return res;
}

Form::ArtifactsList::ArtifactsList(QWidget* parent) : ItemsList(parent)
{
    addItemAct->setStatusTip(tr("Add artifact"));
    editItemAct->setStatusTip(tr("Edit artifact"));
    delItemAct->setStatusTip(tr("Delete artifact"));
}

void Form::ArtifactsList::addItem(void)
{
    SelectArtifactDialog dialog;

    if(QDialog::Accepted == dialog.exec())
    {
	QListWidget::addItem(new QListWidgetItem(*dialog.listWidget->currentItem()));
	setCurrentRow(count() - 1);
    }
}

bool Form::ArtifactsList::limit(void) const
{
    return count() >= 14;
}

void Form::ArtifactsList::editItem(QListWidgetItem* item)
{
    SelectArtifactDialog dialog(item->data(Qt::UserRole).toInt());

    if(QDialog::Accepted == dialog.exec())
	*item = *dialog.listWidget->currentItem();
}

void Form::ArtifactsList::checkLimit(void)
{
    addItemAct->setDisabled(limit());
}

Form::SkillsList::SkillsList(QWidget* parent) : ItemsList(parent)
{
    addItemAct->setStatusTip(tr("Add skill"));
    editItemAct->setStatusTip(tr("Edit skill"));
    delItemAct->setStatusTip(tr("Delete skill"));
}

void Form::SkillsList::addItem(void)
{
    SelectSkillDialog dialog;

    if(QDialog::Accepted == dialog.exec())
    {
	QListWidget::addItem(new QListWidgetItem(*dialog.listWidget->currentItem()));
	setCurrentRow(count() - 1);
    }
}

bool Form::SkillsList::limit(void) const
{
    return count() >= 8;
}

void Form::SkillsList::editItem(QListWidgetItem* item)
{
    SelectSkillDialog dialog(qvariant_cast<Skill>(item->data(Qt::UserRole)));

    if(QDialog::Accepted == dialog.exec())
	*item = *dialog.listWidget->currentItem();
}

void Form::SkillsList::checkLimit(void)
{
    addItemAct->setDisabled(limit());
}

Form::SpellsList::SpellsList(QWidget* parent) : ItemsList(parent)
{
    addItemAct->setStatusTip(tr("Add spell"));
    editItemAct->setStatusTip(tr("Edit spell"));
    delItemAct->setStatusTip(tr("Delete spell"));
}

void Form::SpellsList::addItem(void)
{
    SelectSpellDialog dialog;

    if(QDialog::Accepted == dialog.exec())
    {
	QListWidget::addItem(new QListWidgetItem(*dialog.listWidget->currentItem()));
	setCurrentRow(count() - 1);
    }
}

bool Form::SpellsList::limit(void) const
{
    return count() >= 20;
}

void Form::SpellsList::editItem(QListWidgetItem* item)
{
    SelectSpellDialog dialog(qvariant_cast<int>(item->data(Qt::UserRole)));

    if(QDialog::Accepted == dialog.exec())
	*item = *dialog.listWidget->currentItem();
}

void Form::SpellsList::checkLimit(void)
{
    addItemAct->setDisabled(limit());
}

Form::ListDialog::ListDialog()
{
    listWidget = new QListWidget(this);

    pushButtonOk = new QPushButton(this);
    pushButtonOk->setEnabled(false);
    pushButtonOk->setText(QApplication::translate("ListDialog", "Ok", 0));

    horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    pushButtonCancel = new QPushButton(this);
    pushButtonCancel->setText(QApplication::translate("ListDialog", "Cancel", 0));

    horizontalLayout = new QHBoxLayout();
    horizontalLayout->addWidget(pushButtonOk);
    horizontalLayout->addItem(horizontalSpacer);
    horizontalLayout->addWidget(pushButtonCancel);

    verticalLayout = new QVBoxLayout(this);
    verticalLayout->addWidget(listWidget);
    verticalLayout->addLayout(horizontalLayout);

    QSize minSize(270, 240);

    resize(minSize);
    setMinimumSize(minSize);

    connect(pushButtonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(pushButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(listWidget, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(setEnableOKButton()));
    connect(listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(accept()));
}

void Form::ListDialog::setEnableOKButton(void)
{
    pushButtonOk->setEnabled(true);
}

Form::SelectArtifactDialog::SelectArtifactDialog(int current)
{
    setWindowTitle(QApplication::translate("SelectArtifactDialog", "Select artifact", 0));

    for(int index = Artifact::None + 1; index < Artifact::Random; ++index)
    {
	QListWidgetItem* item = new QListWidgetItem(EditorTheme::getImageICN("ARTIFACT.ICN", index).first, Artifact::transcribe(index));
	item->setData(Qt::UserRole, index);
#ifndef QT_NO_TOOLTIP
	item->setToolTip(Artifact::description(index));
#endif
	listWidget->addItem(item);
    }

    for(int index = Artifact::Random3 + 1; index < Artifact::Unknown; ++index)
    {
	QListWidgetItem* item = new QListWidgetItem(EditorTheme::getImageICN("ARTIFACT.ICN", index).first, Artifact::transcribe(index));
	item->setData(Qt::UserRole, index);
#ifndef QT_NO_TOOLTIP
	item->setToolTip(Artifact::description(index));
#endif
	listWidget->addItem(item);
    }

    if(current && current < listWidget->count()) listWidget->setCurrentRow(current);
}

Form::SelectSkillDialog::SelectSkillDialog(const Skill & current)
{
    setWindowTitle(QApplication::translate("SelectSkillDialog", "Select skill", 0));

    for(int skill = SkillType::None + 1; skill < SkillType::Unknown; ++skill)
    {
	for(int level = SkillLevel::Basic; level <= SkillLevel::Expert; ++level)
	{
	    Skill obj(skill, level);
	    QListWidgetItem* item = new QListWidgetItem(obj.pixmap(), obj.name());
	    item->setData(Qt::UserRole, QVariant::fromValue(obj));
#ifndef QT_NO_TOOLTIP
	    item->setToolTip(obj.description());
#endif
	    listWidget->addItem(item);
	}
    }

    if(current.isValid()) listWidget->setCurrentRow(3 * (current.skill() - 1) + (current.level() - 1));
}

Form::SelectSpellDialog::SelectSpellDialog(int spell)
{
    setWindowTitle(QApplication::translate("SelectSpellDialog", "Select spell", 0));

    for(int sp = Spell::None + 1; sp < Spell::Random; ++sp)
    {
	QListWidgetItem* item = new QListWidgetItem(Spell::pixmap(sp), Spell::transcribe(sp));
	item->setData(Qt::UserRole, QVariant::fromValue(sp));
#ifndef QT_NO_TOOLTIP
	item->setToolTip(Spell::tips(sp));
#endif
	listWidget->addItem(item);
    }

    if(Spell::None < spell && spell < Spell::Random) listWidget->setCurrentRow(spell - 1);
}

Form::RiddlesList::RiddlesList(QWidget* parent) : ItemsList(parent)
{
    addItemAct->setStatusTip(tr("Add new riddle"));
    editItemAct->setStatusTip(tr("Edit riddle"));
    delItemAct->setStatusTip(tr("Delete riddle"));
}

void Form::RiddlesList::addItem(void)
{
    MessageDialog dialog;

    if(QDialog::Accepted == dialog.exec())
    {
	QListWidget::addItem(dialog.message());
	setCurrentRow(count() - 1);
    }
}

void Form::RiddlesList::editItem(QListWidgetItem* item)
{
    MessageDialog dialog(item->text());

    if(QDialog::Accepted == dialog.exec())
	item->setText(dialog.message());
}

Form::MapSphinxDialog::MapSphinxDialog(const MapSphinx & sphinx)
{
    setWindowTitle(QApplication::translate("MapSphinxDialog", "Sphinx Detail", 0));

    // tab message
    tabMessage = new QWidget();

    plainTextMessage = new QPlainTextEdit(tabMessage);
    plainTextMessage->setPlainText(sphinx.message);

    verticalLayoutTabMsg = new QVBoxLayout(tabMessage);
    verticalLayoutTabMsg->addWidget(plainTextMessage);

    // tab resources
    tabGift = new QWidget();

    resourcesGroup = new ResourcesGroup(tabGift, sphinx.resources);
    resourcesGroup->setFlat(false);
    resourcesGroup->setTitle(QApplication::translate("MapSphinxDialog", "Resources", 0));

    artifactGroup = new ArtifactGroup(tabGift, sphinx.artifact);
    artifactGroup->setTitle(QApplication::translate("MapSphinxDialog", "Artifact to give", 0));
    artifactGroup->setFlat(false);

    verticalLayoutGift = new QVBoxLayout(tabGift);
    verticalLayoutGift->addWidget(resourcesGroup);
    verticalLayoutGift->addWidget(artifactGroup);

    // tab riddles
    tabAnswers = new QWidget();
    listWidgetAnswers = new RiddlesList(tabAnswers);
    listWidgetAnswers->addItems(sphinx.answers);
    verticalLayoutAnswers = new QVBoxLayout(tabAnswers);
    verticalLayoutAnswers->addWidget(listWidgetAnswers);

    // end
    tabWidget = new QTabWidget(this);

    tabWidget->addTab(tabMessage, "Message");
    tabWidget->addTab(tabGift, "Gifts");
    tabWidget->addTab(tabAnswers, "Answers");

    pushButtonOk = new QPushButton(this);
    pushButtonOk->setText(QApplication::translate("MapSphinxDialog", "Ok", 0));
    pushButtonOk->setEnabled(false);

    horizontalSpacerButtons = new QSpacerItem(238, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    pushButtonCancel = new QPushButton(this);
    pushButtonCancel->setText(QApplication::translate("MapSphinxDialog", "Cancel", 0));

    horizontalLayoutButtons = new QHBoxLayout();
    horizontalLayoutButtons->addWidget(pushButtonOk);
    horizontalLayoutButtons->addItem(horizontalSpacerButtons);
    horizontalLayoutButtons->addWidget(pushButtonCancel);

    verticalLayoutForm = new QVBoxLayout(this);
    verticalLayoutForm->addWidget(tabWidget);
    verticalLayoutForm->addLayout(horizontalLayoutButtons);

    tabWidget->setCurrentIndex(0);

    QSize minSize = minimumSizeHint();

    resize(minSize);
    setMinimumSize(minSize);

    connect(resourcesGroup, SIGNAL(formChanged()), this, SLOT(setEnableOKButton()));
    connect(artifactGroup, SIGNAL(formChanged()), this, SLOT(setEnableOKButton()));

    connect(plainTextMessage, SIGNAL(textChanged()), this, SLOT(setEnableOKButton()));
    connect(listWidgetAnswers, SIGNAL(listChanged()), this, SLOT(setEnableOKButton()));

    connect(pushButtonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(pushButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));
}

void Form::MapSphinxDialog::setEnableOKButton(void)
{
    pushButtonOk->setEnabled(true);
}

MapSphinx Form::MapSphinxDialog::result(const QPoint & pos, quint32 uid) const
{
    MapSphinx res(pos, uid);

    res.resources = resourcesGroup->result();
    res.artifact = artifactGroup->result();
    res.message = plainTextMessage->toPlainText();
    res.answers = listWidgetAnswers->results();

    return res;
}

Form::ObjectEventsDialog::ObjectEventsDialog(const MapActions* actions)
{
    setWindowTitle(QApplication::translate("ObjectEventsDialog", "Object Events", 0));

    pushButtonUp = new QPushButton(this);
    pushButtonUp->setIcon(QIcon(":/images/uparrow.png"));
    pushButtonUp->setEnabled(false);

    pushButtonDown = new QPushButton(this);
    pushButtonDown->setIcon(QIcon(":/images/downarrow.png"));
    pushButtonDown->setEnabled(false);

    verticalSpacerButtons = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayoutButtons = new QVBoxLayout();
    verticalLayoutButtons->addWidget(pushButtonUp);
    verticalLayoutButtons->addWidget(pushButtonDown);
    verticalLayoutButtons->addItem(verticalSpacerButtons);

    listWidgetEvents = new ObjectEventsList(this);

    if(actions)
	listWidgetEvents->addItems(actions->list);
    else
    {
	QListWidgetItem* item = new QListWidgetItem();
        DefaultActionDialog::fillItem(*item, ActionDefault(QString(), true));
        qobject_cast<QListWidget*>(listWidgetEvents)->addItem(item);
	listWidgetEvents->setCurrentItem(0);
    }

    horizontalLayoutList = new QHBoxLayout();
    horizontalLayoutList->addWidget(listWidgetEvents);
    horizontalLayoutList->addLayout(verticalLayoutButtons);

    verticalSpacerForm = new QSpacerItem(20, 21, QSizePolicy::Minimum, QSizePolicy::Expanding);

    pushButtonOk = new QPushButton(this);
    pushButtonOk->setEnabled(false);
    pushButtonOk->setText(QApplication::translate("ObjectEventsDialog", "Ok", 0));

    horizontalSpacerButtons = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    pushButtonCancel = new QPushButton(this);
    pushButtonCancel->setText(QApplication::translate("ObjectEventsDialog", "Cancel", 0));

    horizontalLayoutButtons = new QHBoxLayout();
    horizontalLayoutButtons->addWidget(pushButtonOk);
    horizontalLayoutButtons->addItem(horizontalSpacerButtons);
    horizontalLayoutButtons->addWidget(pushButtonCancel);

    verticalLayoutForm = new QVBoxLayout(this);
    verticalLayoutForm->addLayout(horizontalLayoutList);
    verticalLayoutForm->addItem(verticalSpacerForm);
    verticalLayoutForm->addLayout(horizontalLayoutButtons);

    QSize minSize(250, 200); // = minimumSizeHint();

    resize(minSize);
    setMinimumSize(minSize);

    connect(pushButtonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(pushButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(pushButtonUp, SIGNAL(clicked()), this, SLOT(moveCurrentItemUp()));
    connect(pushButtonDown, SIGNAL(clicked()), this, SLOT(moveCurrentItemDown()));
    connect(listWidgetEvents, SIGNAL(mousePressed()), this, SLOT(checkUpDownButtons()));
    connect(listWidgetEvents, SIGNAL(listChanged()), this, SLOT(setEnableOKButton()));
}

void Form::ObjectEventsDialog::checkUpDownButtons(void)
{
    int row = listWidgetEvents->currentRow();

    if(0 <=row)
    {
	pushButtonUp->setEnabled(0 != row);
	pushButtonDown->setEnabled(listWidgetEvents->count() - 1 != row);
    }
}

void Form::ObjectEventsDialog::moveCurrentItemUp(void)
{
    int row = listWidgetEvents->currentRow();

    if(0 <=row)
    {
	QListWidgetItem* crnt = listWidgetEvents->item(row);
	QListWidgetItem* next = listWidgetEvents->item(row - 1);

	if(next)
	{
	    qSwap(*crnt, *next);
	    listWidgetEvents->setCurrentItem(row - 1);
	}
    }
}

void Form::ObjectEventsDialog::moveCurrentItemDown(void)
{
    int row = listWidgetEvents->currentRow();

    if(0 <=row)
    {
	QListWidgetItem* crnt = listWidgetEvents->item(row);
	QListWidgetItem* next = listWidgetEvents->item(row + 1);

	if(next)
	{
	    qSwap(*crnt, *next);
	    listWidgetEvents->setCurrentItem(row + 1);
	}
    }
}

void Form::ObjectEventsDialog::setEnableOKButton(void)
{
    checkUpDownButtons();
    pushButtonOk->setEnabled(true);
}

Form::ObjectEventsList::ObjectEventsList(QWidget* parent) : ItemsList(parent)
{
    editItemAct->setStatusTip(tr("Edit event"));
    delItemAct->setStatusTip(tr("Delete event"));

    eventsGroupAct = new QActionGroup(this);

    for(int ii = MapActions::DefaultAction; ii < 5/* MapActions::Unknown */; ++ii)
    {
	QAction* curAct = new QAction(MapActions::transcribe(ii), this);
	curAct->setData(ii);
	eventsGroupAct->addAction(curAct);
    }

    connect(eventsGroupAct, SIGNAL(triggered(QAction*)), this, SLOT(addEventsAction(QAction*)));
}

void Form::ObjectEventsList::createMenuItems(QMenu* menu)
{
    bool selected = selectedItems().size();

    editItemAct->setEnabled(selected);
    delItemAct->setEnabled(selected);

    QMenu* eventsSubMenu = menu->addMenu(tr("Add"));

    menu->addAction(editItemAct);
    menu->addSeparator();
    menu->addAction(delItemAct);

    QList<QAction*> actions = eventsGroupAct->actions();

    for(QList<QAction*>::const_iterator
	it = actions.begin(); it != actions.end(); ++it)
        eventsSubMenu->addAction(*it);
}

bool Form::ObjectEventsList::limit(void) const
{
    return count() >= 15;
}

void Form::ObjectEventsList::checkLimit(void)
{
    addItemAct->setDisabled(limit());
}

Form::EditPassableDialog::EditPassableDialog(const MapTile & tile)
{
    setWindowTitle(QApplication::translate("EditPassableDialog", "Edit Passable", 0));

    horizontalSpacer2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    labelText = new QLabel(this);
    labelText->setText("TEST");

    horizontalSpacer3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout5 = new QHBoxLayout();
    horizontalLayout5->addItem(horizontalSpacer2);
    horizontalLayout5->addWidget(labelText);
    horizontalLayout5->addItem(horizontalSpacer3);

    verticalSpacer2 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Expanding);

    horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    labelTopLeft = new QLabel(this);
    labelTop = new QLabel(this);
    labelTopRight = new QLabel(this);
    horizontalSpacer4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout2 = new QHBoxLayout();
    horizontalLayout2->addItem(horizontalSpacer);
    horizontalLayout2->addWidget(labelTopLeft);
    horizontalLayout2->addWidget(labelTop);
    horizontalLayout2->addWidget(labelTopRight);
    horizontalLayout2->addItem(horizontalSpacer4);

    horizontalSpacer5 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    labelLeft = new QLabel(this);
    labelCenter = new QLabel(this);
    labelRight = new QLabel(this);
    horizontalSpacer7 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout3 = new QHBoxLayout();
    horizontalLayout3->addItem(horizontalSpacer5);
    horizontalLayout3->addWidget(labelLeft);
    horizontalLayout3->addWidget(labelCenter);
    horizontalLayout3->addWidget(labelRight);
    horizontalLayout3->addItem(horizontalSpacer7);

    horizontalSpacer8 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    labelBottomLeft = new QLabel(this);
    labelBottom = new QLabel(this);
    labelBottomRight = new QLabel(this);
    horizontalSpacer11 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout4 = new QHBoxLayout();
    horizontalLayout4->addItem(horizontalSpacer8);
    horizontalLayout4->addWidget(labelBottomLeft);
    horizontalLayout4->addWidget(labelBottom);
    horizontalLayout4->addWidget(labelBottomRight);
    horizontalLayout4->addItem(horizontalSpacer11);

    verticalSpacer = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Expanding);

    pushButtonOk = new QPushButton(this);
    pushButtonOk->setText(QApplication::translate("EditPassableDialog", "Ok", 0));

    horizontalSpacerButtons = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    pushButtonCancel = new QPushButton(this);
    pushButtonCancel->setText(QApplication::translate("EditPassableDialog", "Cancel", 0));

    horizontalLayout = new QHBoxLayout();
    horizontalLayout->addWidget(pushButtonOk);
    horizontalLayout->addItem(horizontalSpacerButtons);
    horizontalLayout->addWidget(pushButtonCancel);

    verticalLayout = new QVBoxLayout(this);
    verticalLayout->addLayout(horizontalLayout5);
    verticalLayout->addItem(verticalSpacer2);
    verticalLayout->addLayout(horizontalLayout2);
    verticalLayout->addLayout(horizontalLayout3);
    verticalLayout->addLayout(horizontalLayout4);
    verticalLayout->addItem(verticalSpacer);
    verticalLayout->addLayout(horizontalLayout);

    connect(pushButtonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(pushButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));
}

int Form::EditPassableDialog::result(void) const
{
    return Direction::All;
}

Form::TownList::TownList(QWidget* parent) : QListWidget(parent), mapData(NULL)
{
    connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(open(QListWidgetItem*)));
}

void Form::TownList::update(MapData* data)
{
    clear();

    if(data)
    {
	QList<SharedMapObject> listCastles = data->objects().list(MapObj::Castle);

	for(QList<SharedMapObject>::const_iterator
    	    it = listCastles.begin(); it != listCastles.end(); ++it)
	{
	    QListWidgetItem* item = new QListWidgetItem(Editor::pixmapBorder(QSize(10, 10), Color::convert((*it).data()->color()), QColor(0, 0, 0)), (*it).data()->name());
	    item->setData(Qt::UserRole, (*it).data()->pos());
	    addItem(item);
	}
    }

    mapData = data;
}

void Form::TownList::open(QListWidgetItem* item)
{
    if(item && mapData)
	mapData->editTownDialog(item->data(Qt::UserRole).toPoint());
}

Form::HeroList::HeroList(QWidget* parent) : QListWidget(parent), mapData(NULL)
{
    connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(open(QListWidgetItem*)));
}

void Form::HeroList::update(MapData* data)
{
    clear();

    if(data)
    {
	QList<SharedMapObject> listHeroes = data->objects().list(MapObj::Heroes);

	for(QList<SharedMapObject>::const_iterator
    	    it = listHeroes.begin(); it != listHeroes.end(); ++it)
	{
	    QListWidgetItem* item = new QListWidgetItem(Editor::pixmapBorder(QSize(10, 10), Color::convert((*it).data()->color()), QColor(0, 0, 0)), (*it).data()->name());
	    item->setData(Qt::UserRole, (*it).data()->pos());
	    addItem(item);
	}
    }

    mapData = data;
}

void Form::HeroList::open(QListWidgetItem* item)
{
    if(item && mapData)
	mapData->editHeroDialog(item->data(Qt::UserRole).toPoint());
}

Form::CustomList::CustomList(QWidget* parent) : QListWidget(parent), mapData(NULL)
{
    connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(open(QListWidgetItem*)));
}

void Form::CustomList::update(MapData* data)
{
    clear();

    if(data)
    {
	const MapObjects & listObjects = data->objects();

	for(MapObjects::const_iterator
    	    it = listObjects.begin(); it != listObjects.end(); ++it)
	{
	    switch((*it).data()->type())
	    {
    		case MapObj::Event:
    		case MapObj::RndCastle:
    		case MapObj::RndTown:
    		case MapObj::Castle:
    		case MapObj::Bottle:
    		case MapObj::Sign:
    		case MapObj::Jail:
    		case MapObj::Heroes:
    		case MapObj::Sphinx:
		    break;

		default:
		{
		    const MapActions* obj = dynamic_cast<const MapActions*>((*it).data());
		    if(obj && ! obj->isDefault())
		    {
			const QPoint & pos = (*it).data()->pos();
			const MapTile* tile = data->tiles().tileConst(pos);
			QString str; QTextStream ts(& str);
			ts << "(" << pos.x() << "," << pos.y() << ")" << "\t" << MapObj::transcribe(tile ? tile->object() : MapObj::None);
			QListWidgetItem* item = new QListWidgetItem(str);
			item->setData(Qt::UserRole, pos);
			addItem(item);
		    }
		}
	        break;
	    }
	}
    }

    mapData = data;
}

void Form::CustomList::open(QListWidgetItem* item)
{
    if(item && mapData)
	mapData->editOtherMapEventsDialog(item->data(Qt::UserRole).toPoint());
}

Form::InfoForm::InfoForm(QWidget* parent) : QFrame(parent)
{
    verticalLayoutForm = new QVBoxLayout(this);
    labelInfo = new QLabel(this);

    verticalLayoutForm->addWidget(labelInfo);
}

void Form::InfoForm::update(const MapTile* tile)
{
    if(tile)
    {
	labelInfo->setText(MapObj::transcribe(tile->object()));
    }
}

MapActionList Form::ObjectEventsDialog::results(void) const
{
    MapActionList res;

    for(int ii = 0; ii < listWidgetEvents->count(); ++ii)
    {
	const QListWidgetItem* item = listWidgetEvents->item(ii);

	if(item)
	{
	    TypeVariant v = qvariant_cast<TypeVariant>(item->data(Qt::UserRole));
	    ActionSimple* ptr = NULL;

	    switch(v.type)
	    {
		case MapActions::DefaultAction:	ptr = new ActionDefault(qvariant_cast<ActionDefault>(v.variant)); break;
		case MapActions::Access:	ptr = new ActionAccess(qvariant_cast<ActionAccess>(v.variant)); break;
		case MapActions::Message:	ptr = new ActionMessage(qvariant_cast<ActionMessage>(v.variant)); break;
		case MapActions::Resources:	ptr = new ActionResources(qvariant_cast<ActionResources>(v.variant)); break;
		case MapActions::Artifact:	ptr = new ActionArtifact(qvariant_cast<ActionArtifact>(v.variant)); break;
		default: break;
	    }

	    res.push_back(SharedActionSimple(ptr));
	}
    }

    return res;
}

void Form::ObjectEventsList::addEventsAction(QAction* act)
{
    if(act)
    {
        int type = act->data().toInt();
	QListWidgetItem* item = NULL;

	switch(type)
	{
	    case MapActions::DefaultAction:
	    {
	        DefaultActionDialog dialog;
		if(QDialog::Accepted == dialog.exec())
		{
		    item = new QListWidgetItem();
		    dialog.fillItem(*item);
		}
	    }
	    break;
	    case MapActions::Access:
	    {
	        AccessDialog dialog;
		if(QDialog::Accepted == dialog.exec())
		{
		    item = new QListWidgetItem();
		    dialog.fillItem(*item);
		}
	    }
	    break;
	    case MapActions::Message:
	    {
	        MessageTabDialog dialog;
		if(QDialog::Accepted == dialog.exec())
		{
		    item = new QListWidgetItem();
		    dialog.fillItem(*item);
		}
	    }
	    break;
	    case MapActions::Resources:
	    {
	        ResourcesDialog dialog;
		if(QDialog::Accepted == dialog.exec())
		{
		    item = new QListWidgetItem();
		    dialog.fillItem(*item);
		}
	    }
	    break;
	    case MapActions::Artifact:
	    {
	        ArtifactDialog dialog;
		if(QDialog::Accepted == dialog.exec())
		{
		    item = new QListWidgetItem();
		    dialog.fillItem(*item);
		}
	    }
	    break;
	    case MapActions::Troops:
	    break;
	    case MapActions::Morale:
	    break;
	    case MapActions::Luck:
	    break;
	    case MapActions::Experience:
	    break;
	    case MapActions::Skill:
	    break;
	    // unknown
	    default: break;
	}

	if(item)
	{
    	    QListWidget::addItem(item);
    	    setCurrentItem(count() - 1);
	}
    }
}

void Form::ObjectEventsList::editItem(QListWidgetItem* item)
{
    TypeVariant data = qvariant_cast<TypeVariant>(item->data(Qt::UserRole));

    switch(data.type)
    {
        case MapActions::DefaultAction:
	{
	    ActionDefault act = qvariant_cast<ActionDefault>(data.variant);
	    DefaultActionDialog dialog(act);
	    if(QDialog::Accepted == dialog.exec()) dialog.fillItem(*item);
	}
	break;
        case MapActions::Access:
	{
	    ActionAccess act = qvariant_cast<ActionAccess>(data.variant);
	    AccessDialog dialog(act);
	    if(QDialog::Accepted == dialog.exec()) dialog.fillItem(*item);
	}
	break;
        case MapActions::Message:
	{
	    ActionMessage act = qvariant_cast<ActionMessage>(data.variant);
	    MessageTabDialog dialog(act);
	    if(QDialog::Accepted == dialog.exec()) dialog.fillItem(*item);
	}
	break;
	case MapActions::Resources:
	{
	    ActionResources act = qvariant_cast<ActionResources>(data.variant);
	    ResourcesDialog dialog(act);
	    if(QDialog::Accepted == dialog.exec()) dialog.fillItem(*item);
	}
	break;
        case MapActions::Artifact:
	{
	    ActionArtifact act = qvariant_cast<ActionArtifact>(data.variant);
	    ArtifactDialog dialog(act);
	    if(QDialog::Accepted == dialog.exec()) dialog.fillItem(*item);
	}
	break;

        case MapActions::Troops:
        break;
        case MapActions::Morale:
        break;
        case MapActions::Luck:
        break;
        case MapActions::Experience:
        break;
        case MapActions::Skill:
	break;

	default: break;
    }
}

void Form::ObjectEventsList::addItems(const MapActionList & list)
{
    for(MapActionList::const_iterator
	it = list.begin(); it != list.end(); ++it)
    {
	QListWidgetItem* item = NULL;

	switch((*it).type())
	{
	    case MapActions::DefaultAction:
	    {
		item = new QListWidgetItem();
		const ActionDefault* act = dynamic_cast<ActionDefault*>((*it).data());
		if(act) DefaultActionDialog::fillItem(*item, *act);
	    }
	    break;
	    case MapActions::Access:
	    {
	        item = new QListWidgetItem();
		const ActionAccess* act = dynamic_cast<ActionAccess*>((*it).data());
		if(act) AccessDialog::fillItem(*item, *act);
	    }
	    break;
	    case MapActions::Message:
	    {
		item = new QListWidgetItem();
		const ActionMessage* act = dynamic_cast<ActionMessage*>((*it).data());
		if(act) MessageTabDialog::fillItem(*item, *act);
	    }
	    break;
	    case MapActions::Resources:
	    {
		item = new QListWidgetItem();
		const ActionResources* act = dynamic_cast<ActionResources*>((*it).data());
		if(act) ResourcesDialog::fillItem(*item, *act);
	    }
	    break;
	    case MapActions::Artifact:
	    {
		item = new QListWidgetItem();
		const ActionArtifact* act = dynamic_cast<ActionArtifact*>((*it).data());
		if(act) ArtifactDialog::fillItem(*item, *act);
	    }
	    break;
	    case MapActions::Troops:
	    break;
	    case MapActions::Morale:
	    break;
	    case MapActions::Luck:
	    break;
	    case MapActions::Experience:
	    break;
	    case MapActions::Skill:
	    break;
	    // unknown
	    default: break;
	}

	if(item)
    	    QListWidget::addItem(item);
    }

    setCurrentItem(count() - 1);
}

Form::MessageTabDialog::MessageTabDialog(const ActionMessage & act)
{
    setWindowTitle(QApplication::translate("MessageTabDialog", "Message Detail", 0));

    tabMessage = new QWidget();

    plainText = new QPlainTextEdit(tabMessage);
    plainText->setPlainText(act.message);

    verticalLayoutMessage = new QVBoxLayout(tabMessage);
    verticalLayoutMessage->addWidget(plainText);

    pushButtonOk = new QPushButton(this);
    pushButtonOk->setText(QApplication::translate("MessageTabDialog", "Ok", 0));
    pushButtonOk->setEnabled(false);

    horizontalSpacer = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    pushButtonCancel = new QPushButton(this);
    pushButtonCancel->setText(QApplication::translate("MessageTabDialog", "Cancel", 0));

    horizontalLayoutButton = new QHBoxLayout();
    horizontalLayoutButton->addWidget(pushButtonOk);
    horizontalLayoutButton->addItem(horizontalSpacer);
    horizontalLayoutButton->addWidget(pushButtonCancel);

    tabWidget = new QTabWidget(this);
    tabWidget->addTab(tabMessage, "Message");

    verticalLayoutForm = new QVBoxLayout(this);
    verticalLayoutForm->addWidget(tabWidget);
    verticalLayoutForm->addLayout(horizontalLayoutButton);

    resize(250, 160);

    connect(plainText, SIGNAL(textChanged()), this, SLOT(enableButtonOK()));
    connect(pushButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(pushButtonOk, SIGNAL(clicked()), this, SLOT(accept()));
}

void Form::MessageTabDialog::enableButtonOK(void)
{
    pushButtonOk->setEnabled(true);
}

QString Form::MessageTabDialog::message(void) const
{
    return plainText->toPlainText();
}

ActionMessage Form::MessageTabDialog::result(void) const
{
    return ActionMessage(message());
}

void Form::MessageTabDialog::fillItem(QListWidgetItem & item) const
{
    fillItem(item, result());
}

void Form::MessageTabDialog::fillItem(QListWidgetItem & item, const ActionMessage & act)
{
    const int type = MapActions::Message;
    item.setData(Qt::UserRole, QVariant::fromValue(TypeVariant(type, QVariant::fromValue(act))));
    item.setText(MapActions::transcribe(type).append(" - ").append(act.message));
}

Form::DefaultActionDialog::DefaultActionDialog(const ActionDefault & act) : MessageTabDialog(act.msg)
{
    setWindowTitle(QApplication::translate("DefaultActionDialog", "Default Action", 0));

    tabAction = new QWidget();

    comboBoxResult = new QComboBox(tabAction);
    comboBoxResult->addItem("disable", 0);
    comboBoxResult->addItem("enable", 1);
    comboBoxResult->setCurrentIndex(act.result ? 1 : 0);

    verticalLayoutAction = new QVBoxLayout(tabAction);
    verticalLayoutAction->addWidget(comboBoxResult);

    tabWidget->addTab(tabAction, "Default Action");
    tabWidget->setCurrentIndex(1);

    QSize minSize = minimumSizeHint();

    resize(minSize);
    setMinimumSize(minSize);

    connect(comboBoxResult, SIGNAL(currentIndexChanged(int)), this, SLOT(enableButtonOK()));
}

ActionDefault Form::DefaultActionDialog::result(void) const
{
    return ActionDefault(message(), comboBoxResult->currentIndex());
}

void Form::DefaultActionDialog::fillItem(QListWidgetItem & item) const
{
    fillItem(item, result());
}

void Form::DefaultActionDialog::fillItem(QListWidgetItem & item, const ActionDefault & act)
{
    const int type = MapActions::DefaultAction;
    item.setData(Qt::UserRole, QVariant::fromValue(TypeVariant(type, QVariant::fromValue(act))));
    item.setText(MapActions::transcribe(type).append(" - ").append(act.result ? "enable" : "disable"));
}

Form::AccessDialog::AccessDialog(const ActionAccess & act) : MessageTabDialog(act.msg)
{
    setWindowTitle(QApplication::translate("AccessDialog", "Access", 0));

    tabAccess = new QWidget();

    accessGroup = new AccessGroup(tabAccess, Color::All, act.access.allowPlayers);
    accessGroup->setAllowComputer(act.access.allowComputer);
    accessGroup->setCancelAfterFirstVisit(act.access.cancelAfterFirstVisit);

    verticalLayoutAccess = new QVBoxLayout(tabAccess);
    verticalLayoutAccess->addWidget(accessGroup);

    tabWidget->addTab(tabAccess, "Access");
    tabWidget->setCurrentIndex(1);

    QSize minSize = minimumSizeHint();

    resize(minSize);
    setMinimumSize(minSize);

    connect(accessGroup, SIGNAL(formChanged()), this, SLOT(enableButtonOK()));
}

ActionAccess Form::AccessDialog::result(void) const
{
    return ActionAccess(message(), AccessResult(accessGroup->colors(), accessGroup->allowComputer(), accessGroup->cancelAfterFirstVisit()));
}

void Form::AccessDialog::fillItem(QListWidgetItem & item) const
{
    fillItem(item, result());
}

void Form::AccessDialog::fillItem(QListWidgetItem & item, const ActionAccess & act)
{
    const int type = MapActions::Access;
    item.setData(Qt::UserRole, QVariant::fromValue(TypeVariant(type, QVariant::fromValue(act))));
    item.setText(MapActions::transcribe(type).append(" - ").append(act.access.transcribe()));
}

Form::ResourcesDialog::ResourcesDialog(const ActionResources & act) : MessageTabDialog(act.msg)
{
    setWindowTitle(QApplication::translate("ResourcesDialog", "Resources", 0));

    tabResources = new QWidget();
    resourcesGroup = new ResourcesGroup(tabResources, act.resources);

    verticalLayoutResources = new QVBoxLayout(tabResources);
    verticalLayoutResources->addWidget(resourcesGroup);

    tabWidget->addTab(tabResources, "Resources");
    tabWidget->setCurrentIndex(1);

    QSize minSize = minimumSizeHint();

    resize(minSize);
    setMinimumSize(minSize);

    connect(resourcesGroup, SIGNAL(formChanged()), this, SLOT(enableButtonOK()));
}

ActionResources Form::ResourcesDialog::result(void) const
{
    return ActionResources(message(), resourcesGroup->result());
}

void Form::ResourcesDialog::fillItem(QListWidgetItem & item) const
{
    fillItem(item, result());
}

void Form::ResourcesDialog::fillItem(QListWidgetItem & item, const ActionResources & act)
{
    const int type = MapActions::Resources;
    item.setData(Qt::UserRole, QVariant::fromValue(TypeVariant(type, QVariant::fromValue(act))));
    item.setText(MapActions::transcribe(type).append(" - ").append(act.resources.describe()));
}

Form::ArtifactDialog::ArtifactDialog(const ActionArtifact & act) : MessageTabDialog(act.msg)
{
    setWindowTitle(QApplication::translate("ArtifactDialog", "Artifact", 0));

    tabArtifact = new QWidget();
    artifactGroup = new ArtifactGroup(tabArtifact, act.artifact);
    spellGroup = new SpellGroup(tabArtifact, Spell::Random);

    verticalLayoutArtifact = new QVBoxLayout(tabArtifact);
    verticalLayoutArtifact->addWidget(artifactGroup);
    verticalLayoutArtifact->addWidget(spellGroup);

    tabWidget->addTab(tabArtifact, "Artifact");
    tabWidget->setCurrentIndex(1);

    QSize minSize = minimumSizeHint();

    resize(minSize);
    setMinimumSize(minSize);

    if(act.artifact == Artifact::SpellScroll)
    {
	spellGroup->setVisible(true);
	spellGroup->setValue(Spell::None < act.spell && Spell::Unknown > act.spell ? act.spell : Spell::Random);
    }
    else
    {
	spellGroup->setVisible(false);
	spellGroup->setValue(Spell::None);
    }

    connect(artifactGroup, SIGNAL(formChanged()), this, SLOT(artifactFormChanged()));
    connect(spellGroup, SIGNAL(formChanged()), this, SLOT(enableButtonOK()));
}

ActionArtifact Form::ArtifactDialog::result(void) const
{
    ActionArtifact res(message(), artifactGroup->result());

    if(res.artifact == Artifact::SpellScroll)
	res.spell = spellGroup->result();
    else
	res.spell = 0;

    return res;
}

void Form::ArtifactDialog::fillItem(QListWidgetItem & item) const
{
    fillItem(item, result());
}

void Form::ArtifactDialog::fillItem(QListWidgetItem & item, const ActionArtifact & act)
{
    const int type = MapActions::Artifact;
    item.setData(Qt::UserRole, QVariant::fromValue(TypeVariant(type, QVariant::fromValue(act))));
    item.setText(MapActions::transcribe(type).append(" - ").append(Artifact::transcribe(act.artifact)));
}

void Form::ArtifactDialog::artifactFormChanged(void)
{
    enableButtonOK();

    if(artifactGroup->result() == Artifact::SpellScroll)
    {
	spellGroup->setVisible(true);
	spellGroup->setValue(Spell::Random);
    }
    else
    {
	spellGroup->setVisible(false);
	spellGroup->setValue(Spell::None);
    }

    resize(minimumSizeHint());
}

Form::MapArtifactDialog::MapArtifactDialog(const MapArtifact & obj)
{
    setWindowTitle(QApplication::translate("MapArtifact", "Artifact Detail", 0));

    variantRandom = new QCheckBox("Random condition", this);

    variantCondition.push_back(new QRadioButton("no condition", this));
    variantCondition.push_back(new QRadioButton("cost: 2000 gold", this));
    variantCondition.push_back(new QRadioButton("cost: 2500 gold + 3 random resource", this));
    variantCondition.push_back(new QRadioButton("cost: 3000 gold + 5 random resource", this));
    variantCondition.push_back(new QRadioButton("need skill: Wizard", this));
    variantCondition.push_back(new QRadioButton("need skill: LeaderShip", this));
    variantCondition.push_back(new QRadioButton("fight: 50 Rogues", this));
    variantCondition.push_back(new QRadioButton("fight: 1 Genie", this));
    variantCondition.push_back(new QRadioButton("fight: 1 Paladin", this));
    variantCondition.push_back(new QRadioButton("fight: 1 Cyclops", this));
    variantCondition.push_back(new QRadioButton("fight: 1 Phoenix", this));
    variantCondition.push_back(new QRadioButton("fight: 1 Green Dragon", this));
    variantCondition.push_back(new QRadioButton("fight: 1 Giant", this));
    variantCondition.push_back(new QRadioButton("fight: 1 Bone Dragon", this));

    if(obj.condition < 0)
    {
        variantRandom->setChecked(true);
	setDefaultCondition(variantRandom->isChecked());
    }
    else
    if(obj.condition < variantCondition.size())
	variantCondition[obj.condition]->setChecked(true);

    labelSpell = new QLabel("spell:", this);
    spellGroup = new SpellGroup(this, Spell::Random);
    if(obj.artifact() == Artifact::SpellScroll)
    {
	labelSpell->setVisible(true);
	spellGroup->setVisible(true);
	spellGroup->setValue(Spell::None < obj.spell && Spell::Unknown > obj.spell ? obj.spell : Spell::Random);
    }
    else
    {
	labelSpell->setVisible(false);
	spellGroup->setVisible(false);
	spellGroup->setValue(Spell::None);
    }

    pushButtonOk = new QPushButton(this);
    pushButtonOk->setText(QApplication::translate("MapArtifact", "Ok", 0));
    pushButtonOk->setEnabled(false);

    horizontalSpacerButtons = new QSpacerItem(238, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    pushButtonCancel = new QPushButton(this);
    pushButtonCancel->setText(QApplication::translate("MapArtifact", "Cancel", 0));

    horizontalLayoutButtons = new QHBoxLayout();
    horizontalLayoutButtons->addWidget(pushButtonOk);
    horizontalLayoutButtons->addItem(horizontalSpacerButtons);
    horizontalLayoutButtons->addWidget(pushButtonCancel);

    verticalLayoutForm = new QVBoxLayout(this);
    verticalLayoutForm->addWidget(variantRandom);
    for(QVector<QRadioButton*>::iterator
	it = variantCondition.begin(); it != variantCondition.end(); ++it)
	verticalLayoutForm->addWidget(*it);
    verticalLayoutForm->addWidget(labelSpell);
    verticalLayoutForm->addWidget(spellGroup);
    verticalLayoutForm->addLayout(horizontalLayoutButtons);

    QSize minSize = minimumSizeHint();

    resize(minSize);
    setMinimumSize(minSize);

    connect(pushButtonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(pushButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(spellGroup, SIGNAL(formChanged()), this, SLOT(setEnableOKButton()));
    connect(variantRandom, SIGNAL(toggled(bool)), this, SLOT(setDefaultCondition(bool)));
    connect(variantRandom, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));
    for(QVector<QRadioButton*>::iterator
	it = variantCondition.begin(); it != variantCondition.end(); ++it)
	connect(*it, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));
}

QPair<int, int> Form::MapArtifactDialog::result(void) const
{
    QPair<int, int> res(-1, spellGroup->result());

    if(! variantRandom->isChecked())
    {
	QVector<QRadioButton*>::const_iterator it = std::find_if(variantCondition.begin(), variantCondition.end(), std::mem_fun(&QRadioButton::isChecked));
	if(it != variantCondition.end())
	    res.first = it - variantCondition.begin();
    }

    return res;
}

void Form::MapArtifactDialog::setEnableOKButton(void)
{
    pushButtonOk->setEnabled(true);
}

void Form::MapArtifactDialog::setDefaultCondition(bool f)
{
    std::for_each(variantCondition.begin(), variantCondition.end(), std::bind2nd(std::mem_fun(&QRadioButton::setDisabled), f));
    variantCondition.front()->setChecked(f);
}

Form::MapMonsterDialog::MapMonsterDialog(const MapMonster & obj)
{
    setWindowTitle(QApplication::translate("MapMonster", "Monster Detail", 0));

    joinDefault = new QCheckBox("Default", this);

    joinCondition.push_back(new QRadioButton("they do not join, fight only", this));
    joinCondition.push_back(new QRadioButton("may join only part of the army and only for money", this));
    joinCondition.push_back(new QRadioButton("they join, if the enemy's army will be stronger", this));
    joinCondition.push_back(new QRadioButton("they all joined, without conditions", this));

    if(obj.condition < 0)
    {
        joinDefault->setChecked(true);
	setDefaultCondition(joinDefault->isChecked());
    }
    else
    if(obj.condition < joinCondition.size())
	joinCondition[obj.condition]->setChecked(true);

    labelCount = new QLabel("count:", this);
    editCount = new QLineEdit(this);
    editCount->setText(QString::number(obj.count));
    editCount->setValidator(new QIntValidator(0, 65535, this));

    pushButtonOk = new QPushButton(this);
    pushButtonOk->setText(QApplication::translate("MapMonster", "Ok", 0));
    pushButtonOk->setEnabled(false);

    horizontalSpacerButtons = new QSpacerItem(238, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    pushButtonCancel = new QPushButton(this);
    pushButtonCancel->setText(QApplication::translate("MapMonster", "Cancel", 0));

    horizontalLayoutButtons = new QHBoxLayout();
    horizontalLayoutButtons->addWidget(pushButtonOk);
    horizontalLayoutButtons->addItem(horizontalSpacerButtons);
    horizontalLayoutButtons->addWidget(pushButtonCancel);

    verticalLayoutForm = new QVBoxLayout(this);
    verticalLayoutForm->addWidget(joinDefault);
    for(QVector<QRadioButton*>::iterator
	it = joinCondition.begin(); it != joinCondition.end(); ++it)
	verticalLayoutForm->addWidget(*it);
    verticalLayoutForm->addWidget(labelCount);
    verticalLayoutForm->addWidget(editCount);
    verticalLayoutForm->addLayout(horizontalLayoutButtons);

    QSize minSize = minimumSizeHint();

    resize(minSize);
    setMinimumSize(minSize);

    connect(pushButtonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(pushButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(editCount, SIGNAL(textChanged(QString)), this, SLOT(setEnableOKButton()));
    connect(joinDefault, SIGNAL(toggled(bool)), this, SLOT(setDefaultCondition(bool)));
    connect(joinDefault, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));
    for(QVector<QRadioButton*>::iterator
	it = joinCondition.begin(); it != joinCondition.end(); ++it)
	connect(*it, SIGNAL(toggled(bool)), this, SLOT(setEnableOKButton()));
}

QPair<int, int> Form::MapMonsterDialog::result(void) const
{
    QPair<int, int> res(-1, editCount->text().toInt());

    if(! joinDefault->isChecked())
    {
	QVector<QRadioButton*>::const_iterator it = std::find_if(joinCondition.begin(), joinCondition.end(), std::mem_fun(&QRadioButton::isChecked));
	if(it != joinCondition.end())
	    res.first = it - joinCondition.begin();
    }

    return res;
}

void Form::MapMonsterDialog::setEnableOKButton(void)
{
    pushButtonOk->setEnabled(true);
}

void Form::MapMonsterDialog::setDefaultCondition(bool f)
{
    std::for_each(joinCondition.begin(), joinCondition.end(), std::bind2nd(std::mem_fun(&QRadioButton::setDisabled), f));
    joinCondition.front()->setChecked(f);
}

Form::MapResourceDialog::MapResourceDialog(const MapResource & obj)
{
    setWindowTitle(QApplication::translate("MapResource", "Resource Detail", 0));

    labelCount = new QLabel("count:", this);
    editCount = new QLineEdit(this);
    editCount->setText(QString::number(obj.count));
    editCount->setValidator(new QIntValidator(0, 65535, this));

    pushButtonOk = new QPushButton(this);
    pushButtonOk->setText(QApplication::translate("MapResource", "Ok", 0));
    pushButtonOk->setEnabled(false);

    horizontalSpacerButtons = new QSpacerItem(238, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    pushButtonCancel = new QPushButton(this);
    pushButtonCancel->setText(QApplication::translate("MapResource", "Cancel", 0));

    horizontalLayoutButtons = new QHBoxLayout();
    horizontalLayoutButtons->addWidget(pushButtonOk);
    horizontalLayoutButtons->addItem(horizontalSpacerButtons);
    horizontalLayoutButtons->addWidget(pushButtonCancel);

    verticalLayoutForm = new QVBoxLayout(this);
    verticalLayoutForm->addWidget(labelCount);
    verticalLayoutForm->addWidget(editCount);
    verticalLayoutForm->addLayout(horizontalLayoutButtons);

    QSize minSize = minimumSizeHint();

    resize(minSize);
    setMinimumSize(minSize);

    connect(pushButtonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(pushButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(editCount, SIGNAL(textChanged(QString)), this, SLOT(setEnableOKButton()));
}

int Form::MapResourceDialog::result(void) const
{
    return editCount->text().toInt();
}

void Form::MapResourceDialog::setEnableOKButton(void)
{
    pushButtonOk->setEnabled(true);
}
