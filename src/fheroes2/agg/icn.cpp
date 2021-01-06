/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <cmath>
#include <cstring>

#include "agg.h"
#include "heroes.h"
#include "icn.h"
#include "objcrck.h"
#include "objdirt.h"
#include "objdsrt.h"
#include "objgras.h"
#include "objlava.h"
#include "objsnow.h"
#include "objswmp.h"
#include "race.h"
#include "settings.h"
#include "spell.h"

namespace ICN
{
    struct icnmap_t
    {
        int type;
        const char * string;
    };

    const icnmap_t icnmap[] = {{UNKNOWN, "UNKNOWN"},
                               {ADVBORDE, "ADVBORDE.ICN"},
                               {ADVBORD, "ADVBORD.ICN"},
                               {ADVBTNS, "ADVBTNS.ICN"},
                               {ADVEBTNS, "ADVEBTNS.ICN"},
                               {ADVMCO, "ADVMCO.ICN"},
                               {AELEM, "AELEM.ICN"},
                               {APANBKGE, "APANBKGE.ICN"},
                               {APANBKG, "APANBKG.ICN"},
                               {APANELE, "APANELE.ICN"},
                               {APANEL, "APANEL.ICN"},
                               {ARCHER2, "ARCHER2.ICN"},
                               {ARCHER, "ARCHER.ICN"},
                               {ARCH_MSL, "ARCH_MSL.ICN"},
                               {ART32, "ART32.ICN"},
                               {ARTFX, "ARTFX.ICN"},
                               {ARTIFACT, "ARTIFACT.ICN"},
                               {BARB32, "BARB32.ICN"},
                               {B_BFLG32, "B-BFLG32.ICN"},
                               {BERZERK, "BERZERK.ICN"},
                               {B_FLAG32, "B-FLAG32.ICN"},
                               {BIGBAR, "BIGBAR.ICN"},
                               {BLDGXTRA, "BLDGXTRA.ICN"},
                               {BLESS, "BLESS.ICN"},
                               {BLIND, "BLIND.ICN"},
                               {BLUEFIRE, "BLUEFIRE.ICN"},
                               {BOAR, "BOAR.ICN"},
                               {BOAT32, "BOAT32.ICN"},
                               {BOATSHAD, "BOATSHAD.ICN"},
                               {BOATWIND, "BOATWIND.ICN"},
                               {BOOK, "BOOK.ICN"},
                               {BORDEDIT, "BORDEDIT.ICN"},
                               {BOULDER, "BOULDER.ICN"},
                               {BRCREST, "BRCREST.ICN"},
                               {BROTHERS, "BROTHERS.ICN"},
                               {BTNBAUD, "BTNBAUD.ICN"},
                               {BTNCMPGN, "BTNCMPGN.ICN"},
                               {BTNCOM, "BTNCOM.ICN"},
                               {BTNDCCFG, "BTNDCCFG.ICN"},
                               {BTNDC, "BTNDC.ICN"},
                               {BTNEMAIN, "BTNEMAIN.ICN"},
                               {BTNENEW, "BTNENEW.ICN"},
                               {BTNESIZE, "BTNESIZE.ICN"},
                               {BTNHOTST, "BTNHOTST.ICN"},
                               {BTNMCFG, "BTNMCFG.ICN"},
                               {BTNMODEM, "BTNMODEM.ICN"},
                               {BTNMP, "BTNMP.ICN"},
                               {BTNNET2, "BTNNET2.ICN"},
                               {BTNNET, "BTNNET.ICN"},
                               {BTNNEWGM, "BTNNEWGM.ICN"},
                               {BTNSHNGL, "BTNSHNGL.ICN"},
                               {BUILDING, "BUILDING.ICN"},
                               {BUYBUILD, "BUYBUILD.ICN"},
                               {BUYBUILE, "BUYBUILE.ICN"},
                               {CAMPBKGE, "CAMPBKGE.ICN"},
                               {CAMPBKGG, "CAMPBKGG.ICN"},
                               {CAMPXTRE, "CAMPXTRE.ICN"},
                               {CAMPXTRG, "CAMPXTRG.ICN"},
                               {CAPTCOVR, "CAPTCOVR.ICN"},
                               {CASLBAR, "CASLBAR.ICN"},
                               {CASLWIND, "CASLWIND.ICN"},
                               {CASLXTRA, "CASLXTRA.ICN"},
                               {CASTBKGB, "CASTBKGB.ICN"},
                               {CASTBKGK, "CASTBKGK.ICN"},
                               {CASTBKGN, "CASTBKGN.ICN"},
                               {CASTBKGS, "CASTBKGS.ICN"},
                               {CASTBKGW, "CASTBKGW.ICN"},
                               {CASTBKGZ, "CASTBKGZ.ICN"},
                               {CASTLEB, "CASTLEB.ICN"},
                               {CASTLEK, "CASTLEK.ICN"},
                               {CASTLEN, "CASTLEN.ICN"},
                               {CASTLES, "CASTLES.ICN"},
                               {CASTLEW, "CASTLEW.ICN"},
                               {CASTLEZ, "CASTLEZ.ICN"},
                               {CATAPULT, "CATAPULT.ICN"},
                               {CAVALRYB, "CAVALRYB.ICN"},
                               {CAVALRYR, "CAVALRYR.ICN"},
                               {CBKGBEAC, "CBKGBEAC.ICN"},
                               {CBKGCRCK, "CBKGCRCK.ICN"},
                               {CBKGDIMT, "CBKGDIMT.ICN"},
                               {CBKGDITR, "CBKGDITR.ICN"},
                               {CBKGDSRT, "CBKGDSRT.ICN"},
                               {CBKGGRAV, "CBKGGRAV.ICN"},
                               {CBKGGRMT, "CBKGGRMT.ICN"},
                               {CBKGGRTR, "CBKGGRTR.ICN"},
                               {CBKGLAVA, "CBKGLAVA.ICN"},
                               {CBKGSNMT, "CBKGSNMT.ICN"},
                               {CBKGSNTR, "CBKGSNTR.ICN"},
                               {CBKGSWMP, "CBKGSWMP.ICN"},
                               {CBKGWATR, "CBKGWATR.ICN"},
                               {CELLWIN, "CELLWIN.ICN"},
                               {CENTAUR, "CENTAUR.ICN"},
                               {CFLGSMAL, "CFLGSMAL.ICN"},
                               {CLOP32, "CLOP32.ICN"},
                               {CLOUDLUK, "CLOUDLUK.ICN"},
                               {CMBTCAPB, "CMBTCAPB.ICN"},
                               {CMBTCAPK, "CMBTCAPK.ICN"},
                               {CMBTCAPN, "CMBTCAPN.ICN"},
                               {CMBTCAPS, "CMBTCAPS.ICN"},
                               {CMBTCAPW, "CMBTCAPW.ICN"},
                               {CMBTCAPZ, "CMBTCAPZ.ICN"},
                               {CMBTFLE1, "CMBTFLE1.ICN"},
                               {CMBTFLE2, "CMBTFLE2.ICN"},
                               {CMBTFLE3, "CMBTFLE3.ICN"},
                               {CMBTHROB, "CMBTHROB.ICN"},
                               {CMBTHROK, "CMBTHROK.ICN"},
                               {CMBTHRON, "CMBTHRON.ICN"},
                               {CMBTHROS, "CMBTHROS.ICN"},
                               {CMBTHROW, "CMBTHROW.ICN"},
                               {CMBTHROZ, "CMBTHROZ.ICN"},
                               {CMBTLOS1, "CMBTLOS1.ICN"},
                               {CMBTLOS2, "CMBTLOS2.ICN"},
                               {CMBTLOS3, "CMBTLOS3.ICN"},
                               {CMBTMISC, "CMBTMISC.ICN"},
                               {CMBTSURR, "CMBTSURR.ICN"},
                               {CMSECO, "CMSECO.ICN"},
                               {COBJ0000, "COBJ0000.ICN"},
                               {COBJ0001, "COBJ0001.ICN"},
                               {COBJ0002, "COBJ0002.ICN"},
                               {COBJ0003, "COBJ0003.ICN"},
                               {COBJ0004, "COBJ0004.ICN"},
                               {COBJ0005, "COBJ0005.ICN"},
                               {COBJ0006, "COBJ0006.ICN"},
                               {COBJ0007, "COBJ0007.ICN"},
                               {COBJ0008, "COBJ0008.ICN"},
                               {COBJ0009, "COBJ0009.ICN"},
                               {COBJ0010, "COBJ0010.ICN"},
                               {COBJ0011, "COBJ0011.ICN"},
                               {COBJ0012, "COBJ0012.ICN"},
                               {COBJ0013, "COBJ0013.ICN"},
                               {COBJ0014, "COBJ0014.ICN"},
                               {COBJ0015, "COBJ0015.ICN"},
                               {COBJ0016, "COBJ0016.ICN"},
                               {COBJ0017, "COBJ0017.ICN"},
                               {COBJ0018, "COBJ0018.ICN"},
                               {COBJ0019, "COBJ0019.ICN"},
                               {COBJ0020, "COBJ0020.ICN"},
                               {COBJ0021, "COBJ0021.ICN"},
                               {COBJ0022, "COBJ0022.ICN"},
                               {COBJ0023, "COBJ0023.ICN"},
                               {COBJ0024, "COBJ0024.ICN"},
                               {COBJ0025, "COBJ0025.ICN"},
                               {COBJ0026, "COBJ0026.ICN"},
                               {COBJ0027, "COBJ0027.ICN"},
                               {COBJ0028, "COBJ0028.ICN"},
                               {COBJ0029, "COBJ0029.ICN"},
                               {COBJ0030, "COBJ0030.ICN"},
                               {COBJ0031, "COBJ0031.ICN"},
                               {COLDRAY, "COLDRAY.ICN"},
                               {COLDRING, "COLDRING.ICN"},
                               {CONGRATS, "CONGRATS.ICN"},
                               {COVR0001, "COVR0001.ICN"},
                               {COVR0002, "COVR0002.ICN"},
                               {COVR0003, "COVR0003.ICN"},
                               {COVR0004, "COVR0004.ICN"},
                               {COVR0005, "COVR0005.ICN"},
                               {COVR0006, "COVR0006.ICN"},
                               {COVR0007, "COVR0007.ICN"},
                               {COVR0008, "COVR0008.ICN"},
                               {COVR0009, "COVR0009.ICN"},
                               {COVR0010, "COVR0010.ICN"},
                               {COVR0011, "COVR0011.ICN"},
                               {COVR0012, "COVR0012.ICN"},
                               {COVR0013, "COVR0013.ICN"},
                               {COVR0014, "COVR0014.ICN"},
                               {COVR0015, "COVR0015.ICN"},
                               {COVR0016, "COVR0016.ICN"},
                               {COVR0017, "COVR0017.ICN"},
                               {COVR0018, "COVR0018.ICN"},
                               {COVR0019, "COVR0019.ICN"},
                               {COVR0020, "COVR0020.ICN"},
                               {COVR0021, "COVR0021.ICN"},
                               {COVR0022, "COVR0022.ICN"},
                               {COVR0023, "COVR0023.ICN"},
                               {COVR0024, "COVR0024.ICN"},
                               {CPANBKGE, "CPANBKGE.ICN"},
                               {CPANBKG, "CPANBKG.ICN"},
                               {CPANELE, "CPANELE.ICN"},
                               {CPANEL, "CPANEL.ICN"},
                               {CREST, "CREST.ICN"},
                               {CSPANBKE, "CSPANBKE.ICN"},
                               {CSPANBKG, "CSPANBKG.ICN"},
                               {CSPANBTE, "CSPANBTE.ICN"},
                               {CSPANBTN, "CSPANBTN.ICN"},
                               {CSPANEL, "CSPANEL.ICN"},
                               {CSTLBARB, "CSTLBARB.ICN"},
                               {CSTLCAPB, "CSTLCAPB.ICN"},
                               {CSTLCAPK, "CSTLCAPK.ICN"},
                               {CSTLCAPN, "CSTLCAPN.ICN"},
                               {CSTLCAPS, "CSTLCAPS.ICN"},
                               {CSTLCAPW, "CSTLCAPW.ICN"},
                               {CSTLCAPZ, "CSTLCAPZ.ICN"},
                               {CSTLKNGT, "CSTLKNGT.ICN"},
                               {CSTLNECR, "CSTLNECR.ICN"},
                               {CSTLSORC, "CSTLSORC.ICN"},
                               {CSTLWRLK, "CSTLWRLK.ICN"},
                               {CSTLWZRD, "CSTLWZRD.ICN"},
                               {CTRACK00, "CTRACK00.ICN"},
                               {CTRACK01, "CTRACK01.ICN"},
                               {CTRACK02, "CTRACK02.ICN"},
                               {CTRACK03, "CTRACK03.ICN"},
                               {CTRACK04, "CTRACK04.ICN"},
                               {CTRACK05, "CTRACK05.ICN"},
                               {CTRACK06, "CTRACK06.ICN"},
                               {CURSE, "CURSE.ICN"},
                               {CYCLOPS, "CYCLOPS.ICN"},
                               {DISRRAY, "DISRRAY.ICN"},
                               {DRAGBLAK, "DRAGBLAK.ICN"},
                               {DRAGBONE, "DRAGBONE.ICN"},
                               {DRAGGREE, "DRAGGREE.ICN"},
                               {DRAGRED, "DRAGRED.ICN"},
                               {DRAGSLAY, "DRAGSLAY.ICN"},
                               {DROPLISL, "DROPLISL.ICN"},
                               {DROPLIST, "DROPLIST.ICN"},
                               {DRUID2, "DRUID2.ICN"},
                               {DRUID, "DRUID.ICN"},
                               {DRUIDMSL, "DRUIDMSL.ICN"},
                               {DUMMY, "DUMMY.ICN"},
                               {DWARF2, "DWARF2.ICN"},
                               {DWARF, "DWARF.ICN"},
                               {ECPANEL, "ECPANEL.ICN"},
                               {EDITBTNS, "EDITBTNS.ICN"},
                               {EDITOR, "EDITOR.ICN"},
                               {EDITPANL, "EDITPANL.ICN"},
                               {EELEM, "EELEM.ICN"},
                               {ELECTRIC, "ELECTRIC.ICN"},
                               {ELF2, "ELF2.ICN"},
                               {ELF, "ELF.ICN"},
                               {ELF__MSL, "ELF__MSL.ICN"},
                               {ESCROLL, "ESCROLL.ICN"},
                               {ESPANBKG, "ESPANBKG.ICN"},
                               {ESPANBTN, "ESPANBTN.ICN"},
                               {ESPANEL, "ESPANEL.ICN"},
                               {EVIW_ALL, "EVIW_ALL.ICN"},
                               {EVIWDDOR, "EVIWDDOR.ICN"},
                               {EVIWHROS, "EVIWHROS.ICN"},
                               {EVIWMINE, "EVIWMINE.ICN"},
                               {EVIWPUZL, "EVIWPUZL.ICN"},
                               {EVIWRSRC, "EVIWRSRC.ICN"},
                               {EVIWRTFX, "EVIWRTFX.ICN"},
                               {EVIWTWNS, "EVIWTWNS.ICN"},
                               {EVIWWRLD, "EVIWWRLD.ICN"},
                               {EXPMRL, "EXPMRL.ICN"},
                               {EXTRAOVR, "EXTRAOVR.ICN"},
                               {FELEM, "FELEM.ICN"},
                               {FIREBAL2, "FIREBAL2.ICN"},
                               {FIREBALL, "FIREBALL.ICN"},
                               {FLAG32, "FLAG32.ICN"},
                               {FONT, "FONT.ICN"},
                               {FRNG0001, "FRNG0001.ICN"},
                               {FRNG0002, "FRNG0002.ICN"},
                               {FRNG0003, "FRNG0003.ICN"},
                               {FRNG0004, "FRNG0004.ICN"},
                               {FRNG0005, "FRNG0005.ICN"},
                               {FRNG0006, "FRNG0006.ICN"},
                               {FRNG0007, "FRNG0007.ICN"},
                               {FRNG0008, "FRNG0008.ICN"},
                               {FRNG0009, "FRNG0009.ICN"},
                               {FRNG0010, "FRNG0010.ICN"},
                               {FRNG0011, "FRNG0011.ICN"},
                               {FRNG0012, "FRNG0012.ICN"},
                               {FRNG0013, "FRNG0013.ICN"},
                               {FROTH, "FROTH.ICN"},
                               {GARGOYLE, "GARGOYLE.ICN"},
                               {G_BFLG32, "G-BFLG32.ICN"},
                               {GENIE, "GENIE.ICN"},
                               {G_FLAG32, "G-FLAG32.ICN"},
                               {GHOST, "GHOST.ICN"},
                               {GOBLIN, "GOBLIN.ICN"},
                               {GOLEM2, "GOLEM2.ICN"},
                               {GOLEM, "GOLEM.ICN"},
                               {GRIFFIN, "GRIFFIN.ICN"},
                               {GROUND12, "GROUND12.ICN"},
                               {GROUND4, "GROUND4.ICN"},
                               {GROUND6, "GROUND6.ICN"},
                               {HALFLING, "HALFLING.ICN"},
                               {HALFLMSL, "HALFLMSL.ICN"},
                               {HASTE, "HASTE.ICN"},
                               {HEROBKG, "HEROBKG.ICN"},
                               {HEROES, "HEROES.ICN"},
                               {HEROEXTE, "HEROEXTE.ICN"},
                               {HEROEXTG, "HEROEXTG.ICN"},
                               {HEROFL00, "HEROFL00.ICN"},
                               {HEROFL01, "HEROFL01.ICN"},
                               {HEROFL02, "HEROFL02.ICN"},
                               {HEROFL03, "HEROFL03.ICN"},
                               {HEROFL04, "HEROFL04.ICN"},
                               {HEROFL05, "HEROFL05.ICN"},
                               {HEROFL06, "HEROFL06.ICN"},
                               {HEROLOGE, "HEROLOGE.ICN"},
                               {HEROLOGO, "HEROLOGO.ICN"},
                               {HISCORE, "HISCORE.ICN"},
                               {HOURGLAS, "HOURGLAS.ICN"},
                               {HSBKG, "HSBKG.ICN"},
                               {HSBTNS, "HSBTNS.ICN"},
                               {HSICONS, "HSICONS.ICN"},
                               {HYDRA, "HYDRA.ICN"},
                               {HYPNOTIZ, "HYPNOTIZ.ICN"},
                               {ICECLOUD, "ICECLOUD.ICN"},
                               {KEEP, "KEEP.ICN"},
                               {KNGT32, "KNGT32.ICN"},
                               {LETTER12, "LETTER12.ICN"},
                               {LETTER4, "LETTER4.ICN"},
                               {LETTER6, "LETTER6.ICN"},
                               {LGNDXTRA, "LGNDXTRA.ICN"},
                               {LGNDXTRE, "LGNDXTRE.ICN"},
                               {LICH2, "LICH2.ICN"},
                               {LICHCLOD, "LICHCLOD.ICN"},
                               {LICH, "LICH.ICN"},
                               {LICH_MSL, "LICH_MSL.ICN"},
                               {LISTBOX, "LISTBOX.ICN"},
                               {LISTBOXS, "LISTBOXS.ICN"},
                               {LOCATORE, "LOCATORE.ICN"},
                               {LOCATORS, "LOCATORS.ICN"},
                               {MAGE1, "MAGE1.ICN"},
                               {MAGE2, "MAGE2.ICN"},
                               {MAGEGLDB, "MAGEGLDB.ICN"},
                               {MAGEGLDK, "MAGEGLDK.ICN"},
                               {MAGEGLDN, "MAGEGLDN.ICN"},
                               {MAGEGLDS, "MAGEGLDS.ICN"},
                               {MAGEGLDW, "MAGEGLDW.ICN"},
                               {MAGEGLDZ, "MAGEGLDZ.ICN"},
                               {MAGIC01, "MAGIC01.ICN"},
                               {MAGIC02, "MAGIC02.ICN"},
                               {MAGIC03, "MAGIC03.ICN"},
                               {MAGIC04, "MAGIC04.ICN"},
                               {MAGIC06, "MAGIC06.ICN"},
                               {MAGIC07, "MAGIC07.ICN"},
                               {MAGIC08, "MAGIC08.ICN"},
                               {MANA, "MANA.ICN"},
                               {MEDUSA, "MEDUSA.ICN"},
                               {METEOR, "METEOR.ICN"},
                               {MINICAPT, "MINICAPT.ICN"},
                               {MINIHERO, "MINIHERO.ICN"},
                               {MINILKMR, "MINILKMR.ICN"},
                               {MINIMON, "MINIMON.ICN"},
                               {MINIPORT, "MINIPORT.ICN"},
                               {MINISS, "MINISS.ICN"},
                               {MINITOWN, "MINITOWN.ICN"},
                               {MINOTAU2, "MINOTAU2.ICN"},
                               {MINOTAUR, "MINOTAUR.ICN"},
                               {MISC12, "MISC12.ICN"},
                               {MISC4, "MISC4.ICN"},
                               {MISC6, "MISC6.ICN"},
                               {MOATPART, "MOATPART.ICN"},
                               {MOATWHOL, "MOATWHOL.ICN"},
                               {MOBILITY, "MOBILITY.ICN"},
                               {MONH0000, "MONH0000.ICN"},
                               {MONH0001, "MONH0001.ICN"},
                               {MONH0002, "MONH0002.ICN"},
                               {MONH0003, "MONH0003.ICN"},
                               {MONH0004, "MONH0004.ICN"},
                               {MONH0005, "MONH0005.ICN"},
                               {MONH0006, "MONH0006.ICN"},
                               {MONH0007, "MONH0007.ICN"},
                               {MONH0008, "MONH0008.ICN"},
                               {MONH0009, "MONH0009.ICN"},
                               {MONH0010, "MONH0010.ICN"},
                               {MONH0011, "MONH0011.ICN"},
                               {MONH0012, "MONH0012.ICN"},
                               {MONH0013, "MONH0013.ICN"},
                               {MONH0014, "MONH0014.ICN"},
                               {MONH0015, "MONH0015.ICN"},
                               {MONH0016, "MONH0016.ICN"},
                               {MONH0017, "MONH0017.ICN"},
                               {MONH0018, "MONH0018.ICN"},
                               {MONH0019, "MONH0019.ICN"},
                               {MONH0020, "MONH0020.ICN"},
                               {MONH0021, "MONH0021.ICN"},
                               {MONH0022, "MONH0022.ICN"},
                               {MONH0023, "MONH0023.ICN"},
                               {MONH0024, "MONH0024.ICN"},
                               {MONH0025, "MONH0025.ICN"},
                               {MONH0026, "MONH0026.ICN"},
                               {MONH0027, "MONH0027.ICN"},
                               {MONH0028, "MONH0028.ICN"},
                               {MONH0029, "MONH0029.ICN"},
                               {MONH0030, "MONH0030.ICN"},
                               {MONH0031, "MONH0031.ICN"},
                               {MONH0032, "MONH0032.ICN"},
                               {MONH0033, "MONH0033.ICN"},
                               {MONH0034, "MONH0034.ICN"},
                               {MONH0035, "MONH0035.ICN"},
                               {MONH0036, "MONH0036.ICN"},
                               {MONH0037, "MONH0037.ICN"},
                               {MONH0038, "MONH0038.ICN"},
                               {MONH0039, "MONH0039.ICN"},
                               {MONH0040, "MONH0040.ICN"},
                               {MONH0041, "MONH0041.ICN"},
                               {MONH0042, "MONH0042.ICN"},
                               {MONH0043, "MONH0043.ICN"},
                               {MONH0044, "MONH0044.ICN"},
                               {MONH0045, "MONH0045.ICN"},
                               {MONH0046, "MONH0046.ICN"},
                               {MONH0047, "MONH0047.ICN"},
                               {MONH0048, "MONH0048.ICN"},
                               {MONH0049, "MONH0049.ICN"},
                               {MONH0050, "MONH0050.ICN"},
                               {MONH0051, "MONH0051.ICN"},
                               {MONH0052, "MONH0052.ICN"},
                               {MONH0053, "MONH0053.ICN"},
                               {MONH0054, "MONH0054.ICN"},
                               {MONH0055, "MONH0055.ICN"},
                               {MONH0056, "MONH0056.ICN"},
                               {MONH0057, "MONH0057.ICN"},
                               {MONH0058, "MONH0058.ICN"},
                               {MONH0059, "MONH0059.ICN"},
                               {MONH0060, "MONH0060.ICN"},
                               {MONH0061, "MONH0061.ICN"},
                               {MONH0062, "MONH0062.ICN"},
                               {MONH0063, "MONH0063.ICN"},
                               {MONH0064, "MONH0064.ICN"},
                               {MONH0065, "MONH0065.ICN"},
                               {MONS32, "MONS32.ICN"},
                               {MORALEB, "MORALEB.ICN"},
                               {MORALEG, "MORALEG.ICN"},
                               {MTNCRCK, "MTNCRCK.ICN"},
                               {MTNDIRT, "MTNDIRT.ICN"},
                               {MTNDSRT, "MTNDSRT.ICN"},
                               {MTNGRAS, "MTNGRAS.ICN"},
                               {MTNLAVA, "MTNLAVA.ICN"},
                               {MTNMULT, "MTNMULT.ICN"},
                               {MTNSNOW, "MTNSNOW.ICN"},
                               {MTNSWMP, "MTNSWMP.ICN"},
                               {MUMMY2, "MUMMY2.ICN"},
                               {MUMMYW, "MUMMYW.ICN"},
                               {NECR32, "NECR32.ICN"},
                               {NETBOX, "NETBOX.ICN"},
                               {NGEXTRA, "NGEXTRA.ICN"},
                               {NGHSBKG, "NGHSBKG.ICN"},
                               {NGMPBKG, "NGMPBKG.ICN"},
                               {NGSPBKG, "NGSPBKG.ICN"},
                               {NOMAD, "NOMAD.ICN"},
                               {O_BFLG32, "O-BFLG32.ICN"},
                               {OBJNARTI, "OBJNARTI.ICN"},
                               {OBJNCRCK, "OBJNCRCK.ICN"},
                               {OBJNDIRT, "OBJNDIRT.ICN"},
                               {OBJNDSRT, "OBJNDSRT.ICN"},
                               {OBJNGRA2, "OBJNGRA2.ICN"},
                               {OBJNGRAS, "OBJNGRAS.ICN"},
                               {OBJNHAUN, "OBJNHAUN.ICN"},
                               {OBJNLAV2, "OBJNLAV2.ICN"},
                               {OBJNLAV3, "OBJNLAV3.ICN"},
                               {OBJNLAVA, "OBJNLAVA.ICN"},
                               {OBJNMUL2, "OBJNMUL2.ICN"},
                               {OBJNMULT, "OBJNMULT.ICN"},
                               {OBJNRSRC, "OBJNRSRC.ICN"},
                               {OBJNSNOW, "OBJNSNOW.ICN"},
                               {OBJNSWMP, "OBJNSWMP.ICN"},
                               {OBJNTOWN, "OBJNTOWN.ICN"},
                               {OBJNTWBA, "OBJNTWBA.ICN"},
                               {OBJNTWRD, "OBJNTWRD.ICN"},
                               {OBJNTWSH, "OBJNTWSH.ICN"},
                               {OBJNWAT2, "OBJNWAT2.ICN"},
                               {OBJNWATR, "OBJNWATR.ICN"},
                               {OBJNXTRA, "OBJNXTRA.ICN"},
                               {OBJPALET, "OBJPALET.ICN"},
                               {O_FLAG32, "O-FLAG32.ICN"},
                               {OGRE2, "OGRE2.ICN"},
                               {OGRE, "OGRE.ICN"},
                               {ORC2, "ORC2.ICN"},
                               {ORC, "ORC.ICN"},
                               {ORC__MSL, "ORC__MSL.ICN"},
                               {OVERBACK, "OVERBACK.ICN"},
                               {OVERLAY, "OVERLAY.ICN"},
                               {OVERVIEW, "OVERVIEW.ICN"},
                               {PALADIN2, "PALADIN2.ICN"},
                               {PALADIN, "PALADIN.ICN"},
                               {PARALYZE, "PARALYZE.ICN"},
                               {P_BFLG32, "P-BFLG32.ICN"},
                               {PEASANT, "PEASANT.ICN"},
                               {P_FLAG32, "P-FLAG32.ICN"},
                               {PHOENIX, "PHOENIX.ICN"},
                               {PHYSICAL, "PHYSICAL.ICN"},
                               {PIKEMAN2, "PIKEMAN2.ICN"},
                               {PIKEMAN, "PIKEMAN.ICN"},
                               {PORT0000, "PORT0000.ICN"},
                               {PORT0001, "PORT0001.ICN"},
                               {PORT0002, "PORT0002.ICN"},
                               {PORT0003, "PORT0003.ICN"},
                               {PORT0004, "PORT0004.ICN"},
                               {PORT0005, "PORT0005.ICN"},
                               {PORT0006, "PORT0006.ICN"},
                               {PORT0007, "PORT0007.ICN"},
                               {PORT0008, "PORT0008.ICN"},
                               {PORT0009, "PORT0009.ICN"},
                               {PORT0010, "PORT0010.ICN"},
                               {PORT0011, "PORT0011.ICN"},
                               {PORT0012, "PORT0012.ICN"},
                               {PORT0013, "PORT0013.ICN"},
                               {PORT0014, "PORT0014.ICN"},
                               {PORT0015, "PORT0015.ICN"},
                               {PORT0016, "PORT0016.ICN"},
                               {PORT0017, "PORT0017.ICN"},
                               {PORT0018, "PORT0018.ICN"},
                               {PORT0019, "PORT0019.ICN"},
                               {PORT0020, "PORT0020.ICN"},
                               {PORT0021, "PORT0021.ICN"},
                               {PORT0022, "PORT0022.ICN"},
                               {PORT0023, "PORT0023.ICN"},
                               {PORT0024, "PORT0024.ICN"},
                               {PORT0025, "PORT0025.ICN"},
                               {PORT0026, "PORT0026.ICN"},
                               {PORT0027, "PORT0027.ICN"},
                               {PORT0028, "PORT0028.ICN"},
                               {PORT0029, "PORT0029.ICN"},
                               {PORT0030, "PORT0030.ICN"},
                               {PORT0031, "PORT0031.ICN"},
                               {PORT0032, "PORT0032.ICN"},
                               {PORT0033, "PORT0033.ICN"},
                               {PORT0034, "PORT0034.ICN"},
                               {PORT0035, "PORT0035.ICN"},
                               {PORT0036, "PORT0036.ICN"},
                               {PORT0037, "PORT0037.ICN"},
                               {PORT0038, "PORT0038.ICN"},
                               {PORT0039, "PORT0039.ICN"},
                               {PORT0040, "PORT0040.ICN"},
                               {PORT0041, "PORT0041.ICN"},
                               {PORT0042, "PORT0042.ICN"},
                               {PORT0043, "PORT0043.ICN"},
                               {PORT0044, "PORT0044.ICN"},
                               {PORT0045, "PORT0045.ICN"},
                               {PORT0046, "PORT0046.ICN"},
                               {PORT0047, "PORT0047.ICN"},
                               {PORT0048, "PORT0048.ICN"},
                               {PORT0049, "PORT0049.ICN"},
                               {PORT0050, "PORT0050.ICN"},
                               {PORT0051, "PORT0051.ICN"},
                               {PORT0052, "PORT0052.ICN"},
                               {PORT0053, "PORT0053.ICN"},
                               {PORT0054, "PORT0054.ICN"},
                               {PORT0055, "PORT0055.ICN"},
                               {PORT0056, "PORT0056.ICN"},
                               {PORT0057, "PORT0057.ICN"},
                               {PORT0058, "PORT0058.ICN"},
                               {PORT0059, "PORT0059.ICN"},
                               {PORT0060, "PORT0060.ICN"},
                               {PORT0061, "PORT0061.ICN"},
                               {PORT0062, "PORT0062.ICN"},
                               {PORT0063, "PORT0063.ICN"},
                               {PORT0064, "PORT0064.ICN"},
                               {PORT0065, "PORT0065.ICN"},
                               {PORT0066, "PORT0066.ICN"},
                               {PORT0067, "PORT0067.ICN"},
                               {PORT0068, "PORT0068.ICN"},
                               {PORT0069, "PORT0069.ICN"},
                               {PORT0070, "PORT0070.ICN"},
                               {PORT0090, "PORT0090.ICN"},
                               {PORT0091, "PORT0091.ICN"},
                               {PORT0092, "PORT0092.ICN"},
                               {PORT0093, "PORT0093.ICN"},
                               {PORT0094, "PORT0094.ICN"},
                               {PORT0095, "PORT0095.ICN"},
                               {PORTCFLG, "PORTCFLG.ICN"},
                               {PORTMEDI, "PORTMEDI.ICN"},
                               {PORTXTRA, "PORTXTRA.ICN"},
                               {PRIMSKIL, "PRIMSKIL.ICN"},
                               {PUZZLE, "PUZZLE.ICN"},
                               {QWIKHERO, "QWIKHERO.ICN"},
                               {QWIKINFO, "QWIKINFO.ICN"},
                               {QWIKTOWN, "QWIKTOWN.ICN"},
                               {RADAR, "RADAR.ICN"},
                               {R_BFLG32, "R-BFLG32.ICN"},
                               {RECR2BKG, "RECR2BKG.ICN"},
                               {RECRBKG, "RECRBKG.ICN"},
                               {RECRUIT, "RECRUIT.ICN"},
                               {REDBACK, "REDBACK.ICN"},
                               {REDDEATH, "REDDEATH.ICN"},
                               {REDFIRE, "REDFIRE.ICN"},
                               {REQBKG, "REQBKG.ICN"},
                               {REQSBKG, "REQSBKG.ICN"},
                               {REQUEST, "REQUEST.ICN"},
                               {REQUESTS, "REQUESTS.ICN"},
                               {RESOURCE, "RESOURCE.ICN"},
                               {RESSMALL, "RESSMALL.ICN"},
                               {R_FLAG32, "R-FLAG32.ICN"},
                               {ROAD, "ROAD.ICN"},
                               {ROC, "ROC.ICN"},
                               {ROGUE, "ROGUE.ICN"},
                               {ROUTE, "ROUTE.ICN"},
                               {SCENIBKG, "SCENIBKG.ICN"},
                               {SCROLL2, "SCROLL2.ICN"},
                               {SCROLLCN, "SCROLLCN.ICN"},
                               {SCROLLE, "SCROLLE.ICN"},
                               {SCROLL, "SCROLL.ICN"},
                               {SECSKILL, "SECSKILL.ICN"},
                               {SHADOW32, "SHADOW32.ICN"},
                               {SHIELD, "SHIELD.ICN"},
                               {SHNGANIM, "SHNGANIM.ICN"},
                               {SKELETON, "SKELETON.ICN"},
                               {SMALCLOD, "SMALCLOD.ICN"},
                               {SMALFONT, "SMALFONT.ICN"},
                               {SMALLBAR, "SMALLBAR.ICN"},
                               {SORC32, "SORC32.ICN"},
                               {SPANBKGE, "SPANBKGE.ICN"},
                               {SPANBKG, "SPANBKG.ICN"},
                               {SPANBTNE, "SPANBTNE.ICN"},
                               {SPANBTN, "SPANBTN.ICN"},
                               {SPANEL, "SPANEL.ICN"},
                               {SPARKS, "SPARKS.ICN"},
                               {SPELCO, "SPELCO.ICN"},
                               {SPELLINF, "SPELLINF.ICN"},
                               {SPELLINL, "SPELLINL.ICN"},
                               {SPELLS, "SPELLS.ICN"},
                               {SPRITE, "SPRITE.ICN"},
                               {STELSKIN, "STELSKIN.ICN"},
                               {STONBACK, "STONBACK.ICN"},
                               {STONBAKE, "STONBAKE.ICN"},
                               {STONEBAK, "STONEBAK.ICN"},
                               {STONEBK2, "STONEBK2.ICN"},
                               {STONSKIN, "STONSKIN.ICN"},
                               {STORM, "STORM.ICN"},
                               {STREAM, "STREAM.ICN"},
                               {STRIP, "STRIP.ICN"},
                               {SUNMOONE, "SUNMOONE.ICN"},
                               {SUNMOON, "SUNMOON.ICN"},
                               {SURDRBKE, "SURDRBKE.ICN"},
                               {SURDRBKG, "SURDRBKG.ICN"},
                               {SURRENDE, "SURRENDE.ICN"},
                               {SURRENDR, "SURRENDR.ICN"},
                               {SWAPBTN, "SWAPBTN.ICN"},
                               {SWAPWIN, "SWAPWIN.ICN"},
                               {SWORDSM2, "SWORDSM2.ICN"},
                               {SWORDSMN, "SWORDSMN.ICN"},
                               {SYSTEME, "SYSTEME.ICN"},
                               {SYSTEM, "SYSTEM.ICN"},
                               {TAVWIN, "TAVWIN.ICN"},
                               {TENT, "TENT.ICN"},
                               {TERRAINS, "TERRAINS.ICN"},
                               {TEXTBACK, "TEXTBACK.ICN"},
                               {TEXTBAK2, "TEXTBAK2.ICN"},
                               {TEXTBAR, "TEXTBAR.ICN"},
                               {TITANBLA, "TITANBLA.ICN"},
                               {TITANBLU, "TITANBLU.ICN"},
                               {TITANMSL, "TITANMSL.ICN"},
                               {TOWNBKG0, "TOWNBKG0.ICN"},
                               {TOWNBKG1, "TOWNBKG1.ICN"},
                               {TOWNBKG2, "TOWNBKG2.ICN"},
                               {TOWNBKG3, "TOWNBKG3.ICN"},
                               {TOWNBKG4, "TOWNBKG4.ICN"},
                               {TOWNBKG5, "TOWNBKG5.ICN"},
                               {TOWNFIX, "TOWNFIX.ICN"},
                               {TOWNNAME, "TOWNNAME.ICN"},
                               {TOWNWIND, "TOWNWIND.ICN"},
                               {TRADPOSE, "TRADPOSE.ICN"},
                               {TRADPOST, "TRADPOST.ICN"},
                               {TREASURY, "TREASURY.ICN"},
                               {TREDECI, "TREDECI.ICN"},
                               {TREEVIL, "TREEVIL.ICN"},
                               {TREFALL, "TREFALL.ICN"},
                               {TREFIR, "TREFIR.ICN"},
                               {TREJNGL, "TREJNGL.ICN"},
                               {TRESNOW, "TRESNOW.ICN"},
                               {TROLL2, "TROLL2.ICN"},
                               {TROLL, "TROLL.ICN"},
                               {TROLLMSL, "TROLLMSL.ICN"},
                               {TWNBBOAT, "TWNBBOAT.ICN"},
                               {TWNBCAPT, "TWNBCAPT.ICN"},
                               {TWNBCSTL, "TWNBCSTL.ICN"},
                               {TWNBDOCK, "TWNBDOCK.ICN"},
                               {TWNBDW_0, "TWNBDW_0.ICN"},
                               {TWNBDW_1, "TWNBDW_1.ICN"},
                               {TWNBDW_2, "TWNBDW_2.ICN"},
                               {TWNBDW_3, "TWNBDW_3.ICN"},
                               {TWNBDW_4, "TWNBDW_4.ICN"},
                               {TWNBDW_5, "TWNBDW_5.ICN"},
                               {TWNBEXT0, "TWNBEXT0.ICN"},
                               {TWNBEXT1, "TWNBEXT1.ICN"},
                               {TWNBEXT2, "TWNBEXT2.ICN"},
                               {TWNBEXT3, "TWNBEXT3.ICN"},
                               {TWNBLTUR, "TWNBLTUR.ICN"},
                               {TWNBMAGE, "TWNBMAGE.ICN"},
                               {TWNBMARK, "TWNBMARK.ICN"},
                               {TWNBMOAT, "TWNBMOAT.ICN"},
                               {TWNBRTUR, "TWNBRTUR.ICN"},
                               {TWNBSPEC, "TWNBSPEC.ICN"},
                               {TWNBSTAT, "TWNBSTAT.ICN"},
                               {TWNBTENT, "TWNBTENT.ICN"},
                               {TWNBTHIE, "TWNBTHIE.ICN"},
                               {TWNBTVRN, "TWNBTVRN.ICN"},
                               {TWNBUP_1, "TWNBUP_1.ICN"},
                               {TWNBUP_3, "TWNBUP_3.ICN"},
                               {TWNBUP_4, "TWNBUP_4.ICN"},
                               {TWNBWEL2, "TWNBWEL2.ICN"},
                               {TWNBWELL, "TWNBWELL.ICN"},
                               {TWNKBOAT, "TWNKBOAT.ICN"},
                               {TWNKCAPT, "TWNKCAPT.ICN"},
                               {TWNKCSTL, "TWNKCSTL.ICN"},
                               {TWNKDOCK, "TWNKDOCK.ICN"},
                               {TWNKDW_0, "TWNKDW_0.ICN"},
                               {TWNKDW_1, "TWNKDW_1.ICN"},
                               {TWNKDW_2, "TWNKDW_2.ICN"},
                               {TWNKDW_3, "TWNKDW_3.ICN"},
                               {TWNKDW_4, "TWNKDW_4.ICN"},
                               {TWNKDW_5, "TWNKDW_5.ICN"},
                               {TWNKEXT0, "TWNKEXT0.ICN"},
                               {TWNKEXT1, "TWNKEXT1.ICN"},
                               {TWNKEXT2, "TWNKEXT2.ICN"},
                               {TWNKLTUR, "TWNKLTUR.ICN"},
                               {TWNKMAGE, "TWNKMAGE.ICN"},
                               {TWNKMARK, "TWNKMARK.ICN"},
                               {TWNKMOAT, "TWNKMOAT.ICN"},
                               {TWNKRTUR, "TWNKRTUR.ICN"},
                               {TWNKSPEC, "TWNKSPEC.ICN"},
                               {TWNKSTAT, "TWNKSTAT.ICN"},
                               {TWNKTENT, "TWNKTENT.ICN"},
                               {TWNKTHIE, "TWNKTHIE.ICN"},
                               {TWNKTVRN, "TWNKTVRN.ICN"},
                               {TWNKUP_1, "TWNKUP_1.ICN"},
                               {TWNKUP_2, "TWNKUP_2.ICN"},
                               {TWNKUP_3, "TWNKUP_3.ICN"},
                               {TWNKUP_4, "TWNKUP_4.ICN"},
                               {TWNKUP_5, "TWNKUP_5.ICN"},
                               {TWNKWEL2, "TWNKWEL2.ICN"},
                               {TWNKWELL, "TWNKWELL.ICN"},
                               {TWNNBOAT, "TWNNBOAT.ICN"},
                               {TWNNCAPT, "TWNNCAPT.ICN"},
                               {TWNNCSTL, "TWNNCSTL.ICN"},
                               {TWNNDOCK, "TWNNDOCK.ICN"},
                               {TWNNDW_0, "TWNNDW_0.ICN"},
                               {TWNNDW_1, "TWNNDW_1.ICN"},
                               {TWNNDW_2, "TWNNDW_2.ICN"},
                               {TWNNDW_3, "TWNNDW_3.ICN"},
                               {TWNNDW_4, "TWNNDW_4.ICN"},
                               {TWNNDW_5, "TWNNDW_5.ICN"},
                               {TWNNEXT0, "TWNNEXT0.ICN"},
                               {TWNNLTUR, "TWNNLTUR.ICN"},
                               {TWNNMAGE, "TWNNMAGE.ICN"},
                               {TWNNMARK, "TWNNMARK.ICN"},
                               {TWNNMOAT, "TWNNMOAT.ICN"},
                               {TWNNRTUR, "TWNNRTUR.ICN"},
                               {TWNNSPEC, "TWNNSPEC.ICN"},
                               {TWNNSTAT, "TWNNSTAT.ICN"},
                               {TWNNTENT, "TWNNTENT.ICN"},
                               {TWNNTHIE, "TWNNTHIE.ICN"},
                               {TWNNTVRN, "TWNNTVRN.ICN"},
                               {TWNNUP_1, "TWNNUP_1.ICN"},
                               {TWNNUP_2, "TWNNUP_2.ICN"},
                               {TWNNUP_3, "TWNNUP_3.ICN"},
                               {TWNNUP_4, "TWNNUP_4.ICN"},
                               {TWNNWEL2, "TWNNWEL2.ICN"},
                               {TWNNWELL, "TWNNWELL.ICN"},
                               {TWNSBOAT, "TWNSBOAT.ICN"},
                               {TWNSCAPT, "TWNSCAPT.ICN"},
                               {TWNSCSTL, "TWNSCSTL.ICN"},
                               {TWNSDOCK, "TWNSDOCK.ICN"},
                               {TWNSDW_0, "TWNSDW_0.ICN"},
                               {TWNSDW_1, "TWNSDW_1.ICN"},
                               {TWNSDW_2, "TWNSDW_2.ICN"},
                               {TWNSDW_3, "TWNSDW_3.ICN"},
                               {TWNSDW_4, "TWNSDW_4.ICN"},
                               {TWNSDW_5, "TWNSDW_5.ICN"},
                               {TWNSEXT0, "TWNSEXT0.ICN"},
                               {TWNSEXT1, "TWNSEXT1.ICN"},
                               {TWNSLTUR, "TWNSLTUR.ICN"},
                               {TWNSMAGE, "TWNSMAGE.ICN"},
                               {TWNSMARK, "TWNSMARK.ICN"},
                               {TWNSMOAT, "TWNSMOAT.ICN"},
                               {TWNSRTUR, "TWNSRTUR.ICN"},
                               {TWNSSPEC, "TWNSSPEC.ICN"},
                               {TWNSSTAT, "TWNSSTAT.ICN"},
                               {TWNSTENT, "TWNSTENT.ICN"},
                               {TWNSTHIE, "TWNSTHIE.ICN"},
                               {TWNSTVRN, "TWNSTVRN.ICN"},
                               {TWNSUP_1, "TWNSUP_1.ICN"},
                               {TWNSUP_2, "TWNSUP_2.ICN"},
                               {TWNSUP_3, "TWNSUP_3.ICN"},
                               {TWNSWEL2, "TWNSWEL2.ICN"},
                               {TWNSWELL, "TWNSWELL.ICN"},
                               {TWNWBOAT, "TWNWBOAT.ICN"},
                               {TWNWCAPT, "TWNWCAPT.ICN"},
                               {TWNWCSTL, "TWNWCSTL.ICN"},
                               {TWNWDOCK, "TWNWDOCK.ICN"},
                               {TWNWDW_0, "TWNWDW_0.ICN"},
                               {TWNWDW_1, "TWNWDW_1.ICN"},
                               {TWNWDW_2, "TWNWDW_2.ICN"},
                               {TWNWDW_3, "TWNWDW_3.ICN"},
                               {TWNWDW_4, "TWNWDW_4.ICN"},
                               {TWNWDW_5, "TWNWDW_5.ICN"},
                               {TWNWEXT0, "TWNWEXT0.ICN"},
                               {TWNWLTUR, "TWNWLTUR.ICN"},
                               {TWNWMAGE, "TWNWMAGE.ICN"},
                               {TWNWMARK, "TWNWMARK.ICN"},
                               {TWNWMOAT, "TWNWMOAT.ICN"},
                               {TWNWRTUR, "TWNWRTUR.ICN"},
                               {TWNWSPEC, "TWNWSPEC.ICN"},
                               {TWNWSTAT, "TWNWSTAT.ICN"},
                               {TWNWTENT, "TWNWTENT.ICN"},
                               {TWNWTHIE, "TWNWTHIE.ICN"},
                               {TWNWTVRN, "TWNWTVRN.ICN"},
                               {TWNWUP_3, "TWNWUP_3.ICN"},
                               {TWNWUP5B, "TWNWUP5B.ICN"},
                               {TWNWUP_5, "TWNWUP_5.ICN"},
                               {TWNWWEL2, "TWNWWEL2.ICN"},
                               {TWNWWELL, "TWNWWELL.ICN"},
                               {TWNZBOAT, "TWNZBOAT.ICN"},
                               {TWNZCAPT, "TWNZCAPT.ICN"},
                               {TWNZCSTL, "TWNZCSTL.ICN"},
                               {TWNZDOCK, "TWNZDOCK.ICN"},
                               {TWNZDW_0, "TWNZDW_0.ICN"},
                               {TWNZDW_1, "TWNZDW_1.ICN"},
                               {TWNZDW_2, "TWNZDW_2.ICN"},
                               {TWNZDW_3, "TWNZDW_3.ICN"},
                               {TWNZDW_4, "TWNZDW_4.ICN"},
                               {TWNZDW_5, "TWNZDW_5.ICN"},
                               {TWNZEXT0, "TWNZEXT0.ICN"},
                               {TWNZLTUR, "TWNZLTUR.ICN"},
                               {TWNZMAGE, "TWNZMAGE.ICN"},
                               {TWNZMARK, "TWNZMARK.ICN"},
                               {TWNZMOAT, "TWNZMOAT.ICN"},
                               {TWNZRTUR, "TWNZRTUR.ICN"},
                               {TWNZSPEC, "TWNZSPEC.ICN"},
                               {TWNZSTAT, "TWNZSTAT.ICN"},
                               {TWNZTENT, "TWNZTENT.ICN"},
                               {TWNZTHIE, "TWNZTHIE.ICN"},
                               {TWNZTVRN, "TWNZTVRN.ICN"},
                               {TWNZUP_2, "TWNZUP_2.ICN"},
                               {TWNZUP_4, "TWNZUP_4.ICN"},
                               {TWNZUP_5, "TWNZUP_5.ICN"},
                               {TWNZWEL2, "TWNZWEL2.ICN"},
                               {TWNZWELL, "TWNZWELL.ICN"},
                               {UNICORN, "UNICORN.ICN"},
                               {VAMPIRE2, "VAMPIRE2.ICN"},
                               {VAMPIRE, "VAMPIRE.ICN"},
                               {VGENBKGE, "VGENBKGE.ICN"},
                               {VGENBKG, "VGENBKG.ICN"},
                               {VIEW_ALL, "VIEW_ALL.ICN"},
                               {VIEWARME, "VIEWARME.ICN"},
                               {VIEWARMY, "VIEWARMY.ICN"},
                               {VIEWARSM, "VIEWARSM.ICN"},
                               {VIEWDDOR, "VIEWDDOR.ICN"},
                               {VIEWGEN, "VIEWGEN.ICN"},
                               {VIEWHROS, "VIEWHROS.ICN"},
                               {VIEWMINE, "VIEWMINE.ICN"},
                               {VIEWPUZL, "VIEWPUZL.ICN"},
                               {VIEWRSRC, "VIEWRSRC.ICN"},
                               {VIEWRTFX, "VIEWRTFX.ICN"},
                               {VIEWTWNS, "VIEWTWNS.ICN"},
                               {VIEWWRLD, "VIEWWRLD.ICN"},
                               {VWFLAG12, "VWFLAG12.ICN"},
                               {VWFLAG4, "VWFLAG4.ICN"},
                               {VWFLAG6, "VWFLAG6.ICN"},
                               {WELEM, "WELEM.ICN"},
                               {WELLBKG, "WELLBKG.ICN"},
                               {WELLXTRA, "WELLXTRA.ICN"},
                               {WINCMBBE, "WINCMBBE.ICN"},
                               {WINCMBTB, "WINCMBTB.ICN"},
                               {WINCMBT, "WINCMBT.ICN"},
                               {WINLOSEB, "WINLOSEB.ICN"},
                               {WINLOSEE, "WINLOSEE.ICN"},
                               {WINLOSE, "WINLOSE.ICN"},
                               {WOLF, "WOLF.ICN"},
                               {WRLK32, "WRLK32.ICN"},
                               {WZRD32, "WZRD32.ICN"},
                               {X_LOC1, "X_LOC1.ICN"},
                               {X_LOC2, "X_LOC2.ICN"},
                               {X_LOC3, "X_LOC3.ICN"},
                               {XPRIMARY, "XPRIMARY.ICN"},
                               {Y_BFLG32, "Y-BFLG32.ICN"},
                               {Y_FLAG32, "Y-FLAG32.ICN"},
                               {YINYANG, "YINYANG.ICN"},
                               {ZOMBIE2, "ZOMBIE2.ICN"},
                               {ZOMBIE, "ZOMBIE.ICN"},

                               {ROUTERED, "ROUTERED.ICN"},
                               {YELLOW_FONT, "YELLOWBF.ICN"},
                               {YELLOW_SMALFONT, "YELLOWSF.ICN"},
                               {BATTLESKIP, "BATTLESKIP.ICN"},
                               {BATTLEWAIT, "BATTLEWAIT.ICN"},
                               {BATTLEAUTO, "BATTLEAUTO.ICN"},
                               {BATTLESETS, "BATTLESETS.ICN"},
                               {BUYMAX, "BUYMAX.ICN"},
                               {BTNCONFIG, "BTNCONFIG.ICN"},
                               {BTNBATTLEONLY, "BTNBONLY.ICN"},
                               {BOAT12, "BOAT12.ICN"},
                               {BTNGIFT, "BTNGIFT.ICN"},
                               {BTNMIN, "BTNMIN.ICN"},
                               {CSLMARKER, "CSLMARKER.ICN"}};
}

const char * ICN::GetString( int icn )
{
    return UNKNOWN <= icn && LASTICN > icn ? icnmap[icn].string : "CUSTOM";
}

u32 ICN::AnimationFrame( int icn, u32 start, u32 ticket, bool quantity )
{
    switch ( icn ) {
    case TWNBBOAT:
    case TWNKBOAT:
    case TWNNBOAT:
    case TWNSBOAT:
    case TWNWBOAT:
    case TWNZBOAT:
        return 1 + ticket % 9;

    case CMBTCAPB:
    case CMBTCAPK:
    case CMBTCAPN:
    case CMBTCAPS:
    case CMBTCAPW:
    case CMBTCAPZ:
        return 1 + ticket % 10;

    case CMBTHROB:
        return 1 + ticket % 18;
    case CMBTHROK:
        return 1 + ticket % 19;
    case CMBTHRON:
        return 1 + ticket % 19;
    case CMBTHROS:
        return 1 + ticket % 16;
    case CMBTHROW:
        return 1 + ticket % 16;
    case CMBTHROZ:
        return 1 + ticket % 18;

    case HEROFL00:
    case HEROFL01:
    case HEROFL02:
    case HEROFL03:
    case HEROFL04:
    case HEROFL05:
    case HEROFL06:
        return ticket % 5;

    case TWNBDOCK:
    case TWNKDOCK:
    case TWNNDOCK:
    case TWNSDOCK:
    case TWNWDOCK:
    case TWNZDOCK:

    case TWNBEXT0:
    case TWNKEXT0:
    case TWNNEXT0:
    case TWNSEXT0:
    case TWNWEXT0:
    case TWNZEXT0:

    case TWNBCAPT:
    case TWNBDW_3:
    case TWNBDW_4:
    case TWNBDW_5:
    case TWNBEXT1:
    case TWNBMOAT:
    case TWNBUP_3:
    case TWNBUP_4:
    case TWNKCSTL:
    case TWNKDW_0:
    case TWNKLTUR:
    case TWNKRTUR:
    case TWNKTHIE:
    case TWNKTVRN:
    case TWNNCSTL:
    case TWNNDW_2:
    case TWNNUP_2:
    case TWNSCAPT:
    case TWNSCSTL:
    case TWNSDW_0:
    case TWNSDW_1:
    case TWNSEXT1:
    case TWNSTHIE:
    case TWNSTVRN:
    case TWNSUP_1:
    case TWNSWEL2:
    case TWNWCAPT:
    case TWNWCSTL:
    case TWNWMOAT:
    case TWNZCSTL:
    case TWNZDW_0:
    case TWNZDW_2:
    case TWNZTHIE:
    case TWNZUP_2:
        return 1 + ticket % 5;

    case TWNBCSTL:
    case TWNKDW_2:
    case TWNKUP_2:
    case TWNNDW_5:
    case TWNNWEL2:
    case TWNWDW_0:
    case TWNWWEL2:
    case TWNZTVRN:
        return 1 + ticket % 6;

    case TWNKDW_4:
    case TWNKUP_4:
        return 1 + ticket % 7;

    case TAVWIN:
        return 2 + ticket % 20;

    case CMBTLOS1:
        return 1 + ticket % 30;
    case CMBTLOS2:
        return 1 + ticket % 29;
    case CMBTLOS3:
        return 1 + ticket % 22;
    case CMBTFLE1:
        return 1 + ticket % 43;
    case CMBTFLE2:
        return 1 + ticket % 26;
    case CMBTFLE3:
        return 1 + ticket % 25;
    case CMBTSURR:
        return 1 + ticket % 20;

    case WINCMBT:
        return 1 + ticket % 20;

    case MINIMON:
        return start + 1 + ticket % 6;

    case TWNNMAGE:
        return start + 1 + ticket % 5;

    case TWNBMAGE:
        return 4 == start ? start + 1 + ticket % 8 : 0;

    case SHNGANIM:
        return 1 + ticket % 39;

    case BTNSHNGL:
        return start + ticket % 4;

    case OBJNHAUN:
        return ticket % 15;

    case OBJNWATR:

        switch ( start ) {
        // buttle
        case 0x00:
            return start + ( ticket % 11 ) + 1;

        // shadow
        case 0x0C:
        // chest
        case 0x13:
        // shadow
        case 0x26:
        // flotsam
        case 0x2D:
        // unkn
        case 0x37:
        // boat
        case 0x3E:
        // waves
        case 0x45:
        // seaweed
        case 0x4C:
        case 0x53:
        case 0x5A:
        case 0x61:
        case 0x68:
        // sailor-man
        case 0x6F:
        // shadow
        case 0xBC:
        // buoy
        case 0xC3:
        // broken ship (right)
        case 0xE2:
        case 0xE9:
        case 0xF1:
        case 0xF8:
            return start + ( ticket % 6 ) + 1;

        // seagull on stones
        case 0x76:
        case 0x86:
        case 0x96:
        case 0xA6:
            return start + ( ticket % 15 ) + 1;

        // whirlpool
        case 0xCA:
        case 0xCE:
        case 0xD2:
        case 0xD6:
        case 0xDA:
        case 0xDE:
            return start + ( ticket % 3 ) + 1;

        default:
            return 0;
        }

    case OBJNWAT2:

        switch ( start ) {
        // sail broken ship (left)
        case 0x03:
        case 0x0C:
            return start + ( ticket % 6 ) + 1;

        default:
            return 0;
        }

    case OBJNCRCK:

        switch ( start ) {
        // pool of oil
        case 0x50:
        case 0x5B:
        case 0x66:
        case 0x71:
        case 0x7C:
        case 0x89:
        case 0x94:
        case 0x9F:
        case 0xAA:
        // smoke from chimney
        case 0xBE:
        // shadow smoke
        case 0xCA:
            return start + ( ticket % 10 ) + 1;

        default:
            return 0;
        }

    case OBJNDIRT:

        switch ( start ) {
        // mill
        case 0x99:
        case 0x9D:
        case 0xA1:
        case 0xA5:
        case 0xA9:
        case 0xAD:
        case 0xB1:
        case 0xB5:
        case 0xB9:
        case 0xBD:
            return start + ( ticket % 3 ) + 1;

        default:
            return 0;
        }

    case OBJNDSRT:

        switch ( start ) {
        // campfire
        case 0x36:
        case 0x3D:
            return start + ( ticket % 6 ) + 1;

        default:
            return 0;
        }

    case OBJNGRA2:

        switch ( start ) {
        // mill
        case 0x17:
        case 0x1B:
        case 0x1F:
        case 0x23:
        case 0x27:
        case 0x2B:
        case 0x2F:
        case 0x33:
        case 0x37:
        case 0x3B:
            return start + ( ticket % 3 ) + 1;

        // smoke from chimney
        case 0x3F:
        case 0x46:
        case 0x4D:
        // archerhouse
        case 0x54:
        // smoke from chimney
        case 0x5D:
        case 0x64:
        // shadow smoke
        case 0x6B:
        // peasanthunt
        case 0x72:
            return start + ( ticket % 6 ) + 1;

        default:
            return 0;
        }

    case OBJNLAV2:

        switch ( start ) {
        // middle volcano
        case 0x00:
        // shadow
        case 0x07:
        case 0x0E:
        // lava
        case 0x15:
            return start + ( ticket % 6 ) + 1;

        // small volcano
        // shadow
        case 0x21:
        case 0x2C:
        // lava
        case 0x37:
        case 0x43:
            return start + ( ticket % 10 ) + 1;

        default:
            return 0;
        }

    case OBJNLAV3:

        // big volcano
        switch ( start ) {
        // smoke
        case 0x00:
        case 0x0F:
        case 0x1E:
        case 0x2D:
        case 0x3C:
        case 0x4B:
        case 0x5A:
        case 0x69:
        case 0x87:
        case 0x96:
        case 0xA5:
        // shadow
        case 0x78:
        case 0xB4:
        case 0xC3:
        case 0xD2:
        case 0xE1:
            return start + ( ticket % 14 ) + 1;

        default:
            return 0;
        }

    case OBJNLAVA:

        switch ( start ) {
        // shadow of lava
        case 0x4E:
        case 0x58:
        case 0x62:
            return start + ( ticket % 9 ) + 1;

        default:
            return 0;
        }

    case OBJNMUL2:

        switch ( start ) {
        // lighthouse
        case 0x3D:
            return start + ( ticket % 9 ) + 1;

        // alchemytower
        case 0x1B:
        // watermill
        case 0x53:
        case 0x5A:
        case 0x62:
        case 0x69:
        // fire in wagoncamp
        case 0x81:
        // smoke smithy (2 chimney)
        case 0xA6:
        // smoke smithy (1 chimney)
        case 0xAD:
        // shadow smoke
        case 0xB4:
            return start + ( ticket % 6 ) + 1;

        // magic garden
        case 0xBE:
            return quantity ? start + ( ticket % 6 ) + 1 : start + 7;

        default:
            return 0;
        }

    case OBJNMULT:

        switch ( start ) {
        // smoke
        case 0x05:
        // shadow
        case 0x0F:
        case 0x19:
            return start + ( ticket % 9 ) + 1;

        // smoke
        case 0x24:
        // shadow
        case 0x2D:
            return start + ( ticket % 8 ) + 1;

        // smoke
        case 0x5A:
        // shadow
        case 0x61:
        case 0x68:
        case 0x7C:
        // campfire
        case 0x83:
            return start + ( ticket % 6 ) + 1;

        default:
            return 0;
        }

    case OBJNSNOW:

        switch ( start ) {
        // firecamp
        case 0x04:
        // alchemytower
        case 0x97:
        // watermill
        case 0xA2:
        case 0xA9:
        case 0xB1:
        case 0xB8:
            return start + ( ticket % 6 ) + 1;

        // mill
        case 0x60:
        case 0x64:
        case 0x68:
        case 0x6C:
        case 0x70:
        case 0x74:
        case 0x78:
        case 0x7C:
        case 0x80:
        case 0x84:
            return start + ( ticket % 3 ) + 1;

        default:
            return 0;
        }

    case OBJNSWMP:

        switch ( start ) {
        // shadow
        case 0x00:
        case 0x0E:
        case 0x2B:
        // smoke
        case 0x07:
        case 0x22:
        case 0x33:
        // light in window
        case 0x16:
        case 0x3A:
        case 0x43:
        case 0x4A:
            return start + ( ticket % 6 ) + 1;

        default:
            return 0;
        }

    // extra objects for loyalty version
    case X_LOC1:

        if ( Settings::Get().PriceLoyaltyVersion() )
            switch ( start ) {
            // alchemist tower
            case 0x04:
            case 0x0D:
            case 0x16:
            // arena
            case 0x1F:
            case 0x28:
            case 0x32:
            case 0x3B:
            // earth altar
            case 0x55:
            case 0x5E:
            case 0x67:
                return start + ( ticket % 8 ) + 1;

            default:
                return 0;
            }
        break;

    // extra objects for loyalty version
    case X_LOC2:

        if ( Settings::Get().PriceLoyaltyVersion() )
            switch ( start ) {
            // mermaid
            case 0x0A:
            case 0x13:
            case 0x1C:
            case 0x25:
            // sirens
            case 0x2F:
            case 0x38:
            case 0x41:
            case 0x4A:
            case 0x53:
            case 0x5C:
            case 0x66:
                return start + ( ticket % 8 ) + 1;

            default:
                return 0;
            }
        break;

    // extra objects for loyalty version
    case X_LOC3:

        if ( Settings::Get().PriceLoyaltyVersion() )
            switch ( start ) {
            // hut magi
            case 0x00:
            case 0x0A:
            case 0x14:
            // eye magi
            case 0x20:
            case 0x29:
            case 0x32:
                return start + ( ticket % 8 ) + 1;

            // barrier
            case 0x3C:
            case 0x42:
            case 0x48:
            case 0x4E:
            case 0x54:
            case 0x5A:
            case 0x60:
            case 0x66:
                return start + ( ticket % 4 ) + 1;

            default:
                return 0;
            }
        break;

    default:
        break;
    }

    return 0;
}

int ICN::PORTxxxx( int heroId )
{
    switch ( heroId ) {
    case Heroes::LORDKILBURN:
        return ICN::PORT0000;
    case Heroes::SIRGALLANTH:
        return ICN::PORT0001;
    case Heroes::ECTOR:
        return ICN::PORT0002;
    case Heroes::GVENNETH:
        return ICN::PORT0003;
    case Heroes::TYRO:
        return ICN::PORT0004;
    case Heroes::AMBROSE:
        return ICN::PORT0005;
    case Heroes::RUBY:
        return ICN::PORT0006;
    case Heroes::MAXIMUS:
        return ICN::PORT0007;
    case Heroes::DIMITRY:
        return ICN::PORT0008;
    case Heroes::THUNDAX:
        return ICN::PORT0009;
    case Heroes::FINEOUS:
        return ICN::PORT0010;
    case Heroes::JOJOSH:
        return ICN::PORT0011;
    case Heroes::CRAGHACK:
        return ICN::PORT0012;
    case Heroes::JEZEBEL:
        return ICN::PORT0013;
    case Heroes::JACLYN:
        return ICN::PORT0014;
    case Heroes::ERGON:
        return ICN::PORT0015;
    case Heroes::TSABU:
        return ICN::PORT0016;
    case Heroes::ATLAS:
        return ICN::PORT0017;
    case Heroes::ASTRA:
        return ICN::PORT0018;
    case Heroes::NATASHA:
        return ICN::PORT0019;
    case Heroes::TROYAN:
        return ICN::PORT0020;
    case Heroes::VATAWNA:
        return ICN::PORT0021;
    case Heroes::REBECCA:
        return ICN::PORT0022;
    case Heroes::GEM:
        return ICN::PORT0023;
    case Heroes::ARIEL:
        return ICN::PORT0024;
    case Heroes::CARLAWN:
        return ICN::PORT0025;
    case Heroes::LUNA:
        return ICN::PORT0026;
    case Heroes::ARIE:
        return ICN::PORT0027;
    case Heroes::ALAMAR:
        return ICN::PORT0028;
    case Heroes::VESPER:
        return ICN::PORT0029;
    case Heroes::CRODO:
        return ICN::PORT0030;
    case Heroes::BAROK:
        return ICN::PORT0031;
    case Heroes::KASTORE:
        return ICN::PORT0032;
    case Heroes::AGAR:
        return ICN::PORT0033;
    case Heroes::FALAGAR:
        return ICN::PORT0034;
    case Heroes::WRATHMONT:
        return ICN::PORT0035;
    case Heroes::MYRA:
        return ICN::PORT0036;
    case Heroes::FLINT:
        return ICN::PORT0037;
    case Heroes::DAWN:
        return ICN::PORT0038;
    case Heroes::HALON:
        return ICN::PORT0039;
    case Heroes::MYRINI:
        return ICN::PORT0040;
    case Heroes::WILFREY:
        return ICN::PORT0041;
    case Heroes::SARAKIN:
        return ICN::PORT0042;
    case Heroes::KALINDRA:
        return ICN::PORT0043;
    case Heroes::MANDIGAL:
        return ICN::PORT0044;
    case Heroes::ZOM:
        return ICN::PORT0045;
    case Heroes::DARLANA:
        return ICN::PORT0046;
    case Heroes::ZAM:
        return ICN::PORT0047;
    case Heroes::RANLOO:
        return ICN::PORT0048;
    case Heroes::CHARITY:
        return ICN::PORT0049;
    case Heroes::RIALDO:
        return ICN::PORT0050;
    case Heroes::ROXANA:
        return ICN::PORT0051;
    case Heroes::SANDRO:
        return ICN::PORT0052;
    case Heroes::CELIA:
        return ICN::PORT0053;
    case Heroes::ROLAND:
        return ICN::PORT0054;
    case Heroes::CORLAGON:
        return ICN::PORT0055;
    case Heroes::ELIZA:
        return ICN::PORT0056;
    case Heroes::ARCHIBALD:
        return ICN::PORT0057;
    case Heroes::HALTON:
        return ICN::PORT0058;
    case Heroes::BAX:
        return ICN::PORT0059;

    case Heroes::SOLMYR:
        return ICN::PORT0060;
    case Heroes::DAINWIN:
        return ICN::PORT0061;
    case Heroes::MOG:
        return ICN::PORT0062;
    case Heroes::UNCLEIVAN:
        return ICN::PORT0063;
    case Heroes::JOSEPH:
        return ICN::PORT0064;
    case Heroes::GALLAVANT:
        return ICN::PORT0065;
    case Heroes::ELDERIAN:
        return ICN::PORT0066;
    case Heroes::CEALLACH:
        return ICN::PORT0067;
    case Heroes::DRAKONIA:
        return ICN::PORT0068;
    case Heroes::MARTINE:
        return ICN::PORT0069;
    case Heroes::JARKONAS:
        return ICN::PORT0070;

    case Heroes::SANDYSANDY:
        return ICN::PORT0059;

    default:
        break;
    }

    return ICN::UNKNOWN;
}

int ICN::FromString( const char * str )
{
    const icnmap_t * ptr = &icnmap[0];
    while ( ptr->type != ICN::UNKNOWN && str && 0 != std::strcmp( str, ptr->string ) )
        ++ptr;
    return ptr->type;
}

bool ICN::HighlyObjectSprite( int icn, u32 index )
{
    switch ( icn ) {
    case OBJNDIRT:
        // wind mill
        if ( 154 <= index && index <= 160 )
            return true;
        break;

    case OBJNGRA2:
        // wind mill
        if ( 24 <= index && index <= 30 )
            return true;
        break;

    case OBJNLAV2:
    case OBJNLAV3:
        // fog lava
        return true;

    case OBJNMUL2:
        // dragon city
        if ( 35 == index || 37 == index || 38 == index || 40 == index || 41 == index )
            return true;
        // ligth
        if ( 59 == index )
            return true;
        // water mill
        if ( 82 == index )
            return true;
        break;

    case OBJNMULT:
        // fort
        if ( 36 <= index && index <= 44 )
            return true;
        // tree
        if ( 117 == index || 118 == index )
            return true;
        break;

    case OBJNSNOW:
        // wind mill
        if ( 97 <= index && index <= 103 )
            return true;
        // water mill
        if ( 161 == index )
            return true;
        break;

    case OBJNSWMP:
        //
        if ( 35 <= index && index <= 42 )
            return true;
        break;

    case OBJNTOWN:
        if ( 1 <= index && index <= 5 )
            return true;
        if ( 32 <= index && index <= 37 )
            return true;
        if ( 64 <= index && index <= 69 )
            return true;
        if ( 96 <= index && index <= 101 )
            return true;
        if ( 128 <= index && index <= 133 )
            return true;
        if ( 160 <= index && index <= 165 )
            return true;
        break;

    case FLAG32:
        return true;

    default:
        break;
    }
    return false;
}

int ICN::Get4Captain( int race )
{
    switch ( race ) {
    case Race::BARB:
        return CSTLCAPB;
    case Race::KNGT:
        return CSTLCAPK;
    case Race::NECR:
        return CSTLCAPN;
    case Race::SORC:
        return CSTLCAPS;
    case Race::WRLK:
        return CSTLCAPW;
    case Race::WZRD:
        return CSTLCAPZ;
    default:
        break;
    }

    return UNKNOWN;
}

int ICN::Get4Building( int race )
{
    switch ( race ) {
    case Race::BARB:
        return CSTLBARB;
    case Race::KNGT:
        return CSTLKNGT;
    case Race::NECR:
        return CSTLNECR;
    case Race::SORC:
        return CSTLSORC;
    case Race::WRLK:
        return CSTLWRLK;
    case Race::WZRD:
        return CSTLWZRD;
    default:
        break;
    }

    return UNKNOWN;
}

int ICN::Get4Castle( int race )
{
    switch ( race ) {
    case Race::BARB:
        return CASTLEB;
    case Race::KNGT:
        return CASTLEK;
    case Race::NECR:
        return CASTLEN;
    case Race::SORC:
        return CASTLES;
    case Race::WRLK:
        return CASTLEW;
    case Race::WZRD:
        return CASTLEZ;
    default:
        break;
    }

    return UNKNOWN;
}

int ICN::GetFlagIcnId( int color )
{
    switch ( color ) {
    case Color::BLUE:
        return ICN::HEROFL00;
    case Color::GREEN:
        return ICN::HEROFL01;
    case Color::RED:
        return ICN::HEROFL02;
    case Color::YELLOW:
        return ICN::HEROFL03;
    case Color::ORANGE:
        return ICN::HEROFL04;
    case Color::PURPLE:
        return ICN::HEROFL05;
    default:
        return ICN::HEROFL06;
    }
}
