/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "icn.h"

#include <array>
#include <cassert>

#include "color.h"
#include "heroes.h"
#include "race.h"
#include "settings.h"

namespace
{
    struct ICNMapItem
    {
        int type;
        const char * string;
    };
}

namespace ICN
{
    // Make sure that each item ID has exactly the same location ID!
    const std::array<ICNMapItem, LAST_VALID_FILE_ICN> icnMap
        = { { { UNKNOWN, "UNKNOWN" },       { ADVBORDE, "ADVBORDE.ICN" }, { ADVBORD, "ADVBORD.ICN" },   { ADVBTNS, "ADVBTNS.ICN" },   { ADVEBTNS, "ADVEBTNS.ICN" },
              { ADVMCO, "ADVMCO.ICN" },     { AELEM, "AELEM.ICN" },       { APANBKGE, "APANBKGE.ICN" }, { APANBKG, "APANBKG.ICN" },   { APANELE, "APANELE.ICN" },
              { APANEL, "APANEL.ICN" },     { ARCHER2, "ARCHER2.ICN" },   { ARCHER, "ARCHER.ICN" },     { ARCH_MSL, "ARCH_MSL.ICN" }, { ART32, "ART32.ICN" },
              { ARTFX, "ARTFX.ICN" },       { ARTIFACT, "ARTIFACT.ICN" }, { BARB32, "BARB32.ICN" },     { B_BFLG32, "B-BFLG32.ICN" }, { BERZERK, "BERZERK.ICN" },
              { B_FLAG32, "B-FLAG32.ICN" }, { BIGBAR, "BIGBAR.ICN" },     { BLDGXTRA, "BLDGXTRA.ICN" }, { BLESS, "BLESS.ICN" },       { BLIND, "BLIND.ICN" },
              { BLUEFIRE, "BLUEFIRE.ICN" }, { BOAR, "BOAR.ICN" },         { BOAT32, "BOAT32.ICN" },     { BOATSHAD, "BOATSHAD.ICN" }, { BOATWIND, "BOATWIND.ICN" },
              { BOOK, "BOOK.ICN" },         { BORDEDIT, "BORDEDIT.ICN" }, { BOULDER, "BOULDER.ICN" },   { BRCREST, "BRCREST.ICN" },   { BROTHERS, "BROTHERS.ICN" },
              { BTNBAUD, "BTNBAUD.ICN" },   { BTNCMPGN, "BTNCMPGN.ICN" }, { BTNCOM, "BTNCOM.ICN" },     { BTNDCCFG, "BTNDCCFG.ICN" }, { BTNDC, "BTNDC.ICN" },
              { BTNEMAIN, "BTNEMAIN.ICN" }, { BTNENEW, "BTNENEW.ICN" },   { BTNESIZE, "BTNESIZE.ICN" }, { BTNHOTST, "BTNHOTST.ICN" }, { BTNMCFG, "BTNMCFG.ICN" },
              { BTNMODEM, "BTNMODEM.ICN" }, { BTNMP, "BTNMP.ICN" },       { BTNNET2, "BTNNET2.ICN" },   { BTNNET, "BTNNET.ICN" },     { BTNNEWGM, "BTNNEWGM.ICN" },
              { BTNSHNGL, "BTNSHNGL.ICN" }, { BUILDING, "BUILDING.ICN" }, { BUYBUILD, "BUYBUILD.ICN" }, { BUYBUILE, "BUYBUILE.ICN" }, { CAMPBKGE, "CAMPBKGE.ICN" },
              { CAMPBKGG, "CAMPBKGG.ICN" }, { CAMPXTRE, "CAMPXTRE.ICN" }, { CAMPXTRG, "CAMPXTRG.ICN" }, { CAPTCOVR, "CAPTCOVR.ICN" }, { CASLBAR, "CASLBAR.ICN" },
              { CASLWIND, "CASLWIND.ICN" }, { CASLXTRA, "CASLXTRA.ICN" }, { CASTBKGB, "CASTBKGB.ICN" }, { CASTBKGK, "CASTBKGK.ICN" }, { CASTBKGN, "CASTBKGN.ICN" },
              { CASTBKGS, "CASTBKGS.ICN" }, { CASTBKGW, "CASTBKGW.ICN" }, { CASTBKGZ, "CASTBKGZ.ICN" }, { CASTLEB, "CASTLEB.ICN" },   { CASTLEK, "CASTLEK.ICN" },
              { CASTLEN, "CASTLEN.ICN" },   { CASTLES, "CASTLES.ICN" },   { CASTLEW, "CASTLEW.ICN" },   { CASTLEZ, "CASTLEZ.ICN" },   { CATAPULT, "CATAPULT.ICN" },
              { CAVALRYB, "CAVALRYB.ICN" }, { CAVALRYR, "CAVALRYR.ICN" }, { CBKGBEAC, "CBKGBEAC.ICN" }, { CBKGCRCK, "CBKGCRCK.ICN" }, { CBKGDIMT, "CBKGDIMT.ICN" },
              { CBKGDITR, "CBKGDITR.ICN" }, { CBKGDSRT, "CBKGDSRT.ICN" }, { CBKGGRAV, "CBKGGRAV.ICN" }, { CBKGGRMT, "CBKGGRMT.ICN" }, { CBKGGRTR, "CBKGGRTR.ICN" },
              { CBKGLAVA, "CBKGLAVA.ICN" }, { CBKGSNMT, "CBKGSNMT.ICN" }, { CBKGSNTR, "CBKGSNTR.ICN" }, { CBKGSWMP, "CBKGSWMP.ICN" }, { CBKGWATR, "CBKGWATR.ICN" },
              { CELLWIN, "CELLWIN.ICN" },   { CENTAUR, "CENTAUR.ICN" },   { CFLGSMAL, "CFLGSMAL.ICN" }, { CLOP32, "CLOP32.ICN" },     { CLOUDLUK, "CLOUDLUK.ICN" },
              { CMBTCAPB, "CMBTCAPB.ICN" }, { CMBTCAPK, "CMBTCAPK.ICN" }, { CMBTCAPN, "CMBTCAPN.ICN" }, { CMBTCAPS, "CMBTCAPS.ICN" }, { CMBTCAPW, "CMBTCAPW.ICN" },
              { CMBTCAPZ, "CMBTCAPZ.ICN" }, { CMBTFLE1, "CMBTFLE1.ICN" }, { CMBTFLE2, "CMBTFLE2.ICN" }, { CMBTFLE3, "CMBTFLE3.ICN" }, { CMBTHROB, "CMBTHROB.ICN" },
              { CMBTHROK, "CMBTHROK.ICN" }, { CMBTHRON, "CMBTHRON.ICN" }, { CMBTHROS, "CMBTHROS.ICN" }, { CMBTHROW, "CMBTHROW.ICN" }, { CMBTHROZ, "CMBTHROZ.ICN" },
              { CMBTLOS1, "CMBTLOS1.ICN" }, { CMBTLOS2, "CMBTLOS2.ICN" }, { CMBTLOS3, "CMBTLOS3.ICN" }, { CMBTMISC, "CMBTMISC.ICN" }, { CMBTSURR, "CMBTSURR.ICN" },
              { CMSECO, "CMSECO.ICN" },     { COBJ0000, "COBJ0000.ICN" }, { COBJ0001, "COBJ0001.ICN" }, { COBJ0002, "COBJ0002.ICN" }, { COBJ0003, "COBJ0003.ICN" },
              { COBJ0004, "COBJ0004.ICN" }, { COBJ0005, "COBJ0005.ICN" }, { COBJ0006, "COBJ0006.ICN" }, { COBJ0007, "COBJ0007.ICN" }, { COBJ0008, "COBJ0008.ICN" },
              { COBJ0009, "COBJ0009.ICN" }, { COBJ0010, "COBJ0010.ICN" }, { COBJ0011, "COBJ0011.ICN" }, { COBJ0012, "COBJ0012.ICN" }, { COBJ0013, "COBJ0013.ICN" },
              { COBJ0014, "COBJ0014.ICN" }, { COBJ0015, "COBJ0015.ICN" }, { COBJ0016, "COBJ0016.ICN" }, { COBJ0017, "COBJ0017.ICN" }, { COBJ0018, "COBJ0018.ICN" },
              { COBJ0019, "COBJ0019.ICN" }, { COBJ0020, "COBJ0020.ICN" }, { COBJ0021, "COBJ0021.ICN" }, { COBJ0022, "COBJ0022.ICN" }, { COBJ0023, "COBJ0023.ICN" },
              { COBJ0024, "COBJ0024.ICN" }, { COBJ0025, "COBJ0025.ICN" }, { COBJ0026, "COBJ0026.ICN" }, { COBJ0027, "COBJ0027.ICN" }, { COBJ0028, "COBJ0028.ICN" },
              { COBJ0029, "COBJ0029.ICN" }, { COBJ0030, "COBJ0030.ICN" }, { COBJ0031, "COBJ0031.ICN" }, { COLDRAY, "COLDRAY.ICN" },   { COLDRING, "COLDRING.ICN" },
              { CONGRATS, "CONGRATS.ICN" }, { COVR0001, "COVR0001.ICN" }, { COVR0002, "COVR0002.ICN" }, { COVR0003, "COVR0003.ICN" }, { COVR0004, "COVR0004.ICN" },
              { COVR0005, "COVR0005.ICN" }, { COVR0006, "COVR0006.ICN" }, { COVR0007, "COVR0007.ICN" }, { COVR0008, "COVR0008.ICN" }, { COVR0009, "COVR0009.ICN" },
              { COVR0010, "COVR0010.ICN" }, { COVR0011, "COVR0011.ICN" }, { COVR0012, "COVR0012.ICN" }, { COVR0013, "COVR0013.ICN" }, { COVR0014, "COVR0014.ICN" },
              { COVR0015, "COVR0015.ICN" }, { COVR0016, "COVR0016.ICN" }, { COVR0017, "COVR0017.ICN" }, { COVR0018, "COVR0018.ICN" }, { COVR0019, "COVR0019.ICN" },
              { COVR0020, "COVR0020.ICN" }, { COVR0021, "COVR0021.ICN" }, { COVR0022, "COVR0022.ICN" }, { COVR0023, "COVR0023.ICN" }, { COVR0024, "COVR0024.ICN" },
              { CPANBKGE, "CPANBKGE.ICN" }, { CPANBKG, "CPANBKG.ICN" },   { CPANELE, "CPANELE.ICN" },   { CPANEL, "CPANEL.ICN" },     { CREST, "CREST.ICN" },
              { CSPANBKE, "CSPANBKE.ICN" }, { CSPANBKG, "CSPANBKG.ICN" }, { CSPANBTE, "CSPANBTE.ICN" }, { CSPANBTN, "CSPANBTN.ICN" }, { CSPANEL, "CSPANEL.ICN" },
              { CSTLBARB, "CSTLBARB.ICN" }, { CSTLCAPB, "CSTLCAPB.ICN" }, { CSTLCAPK, "CSTLCAPK.ICN" }, { CSTLCAPN, "CSTLCAPN.ICN" }, { CSTLCAPS, "CSTLCAPS.ICN" },
              { CSTLCAPW, "CSTLCAPW.ICN" }, { CSTLCAPZ, "CSTLCAPZ.ICN" }, { CSTLKNGT, "CSTLKNGT.ICN" }, { CSTLNECR, "CSTLNECR.ICN" }, { CSTLSORC, "CSTLSORC.ICN" },
              { CSTLWRLK, "CSTLWRLK.ICN" }, { CSTLWZRD, "CSTLWZRD.ICN" }, { CTRACK00, "CTRACK00.ICN" }, { CTRACK01, "CTRACK01.ICN" }, { CTRACK02, "CTRACK02.ICN" },
              { CTRACK03, "CTRACK03.ICN" }, { CTRACK04, "CTRACK04.ICN" }, { CTRACK05, "CTRACK05.ICN" }, { CTRACK06, "CTRACK06.ICN" }, { CURSE, "CURSE.ICN" },
              { CYCLOPS, "CYCLOPS.ICN" },   { DISRRAY, "DISRRAY.ICN" },   { DRAGBLAK, "DRAGBLAK.ICN" }, { DRAGBONE, "DRAGBONE.ICN" }, { DRAGGREE, "DRAGGREE.ICN" },
              { DRAGRED, "DRAGRED.ICN" },   { DRAGSLAY, "DRAGSLAY.ICN" }, { DROPLISL, "DROPLISL.ICN" }, { DROPLIST, "DROPLIST.ICN" }, { DRUID2, "DRUID2.ICN" },
              { DRUID, "DRUID.ICN" },       { DRUIDMSL, "DRUIDMSL.ICN" }, { DUMMY, "DUMMY.ICN" },       { DWARF2, "DWARF2.ICN" },     { DWARF, "DWARF.ICN" },
              { ECPANEL, "ECPANEL.ICN" },   { EDITBTNS, "EDITBTNS.ICN" }, { EDITOR, "EDITOR.ICN" },     { EDITPANL, "EDITPANL.ICN" }, { EELEM, "EELEM.ICN" },
              { ELECTRIC, "ELECTRIC.ICN" }, { ELF2, "ELF2.ICN" },         { ELF, "ELF.ICN" },           { ELF__MSL, "ELF__MSL.ICN" }, { ESCROLL, "ESCROLL.ICN" },
              { ESPANBKG, "ESPANBKG.ICN" }, { ESPANBTN, "ESPANBTN.ICN" }, { ESPANEL, "ESPANEL.ICN" },   { EVIW_ALL, "EVIW_ALL.ICN" }, { EVIWDDOR, "EVIWDDOR.ICN" },
              { EVIWHROS, "EVIWHROS.ICN" }, { EVIWMINE, "EVIWMINE.ICN" }, { EVIWPUZL, "EVIWPUZL.ICN" }, { EVIWRSRC, "EVIWRSRC.ICN" }, { EVIWRTFX, "EVIWRTFX.ICN" },
              { EVIWTWNS, "EVIWTWNS.ICN" }, { EVIWWRLD, "EVIWWRLD.ICN" }, { EXPMRL, "EXPMRL.ICN" },     { EXTRAOVR, "EXTRAOVR.ICN" }, { FELEM, "FELEM.ICN" },
              { FIREBAL2, "FIREBAL2.ICN" }, { FIREBALL, "FIREBALL.ICN" }, { FLAG32, "FLAG32.ICN" },     { FONT, "FONT.ICN" },         { FRNG0001, "FRNG0001.ICN" },
              { FRNG0002, "FRNG0002.ICN" }, { FRNG0003, "FRNG0003.ICN" }, { FRNG0004, "FRNG0004.ICN" }, { FRNG0005, "FRNG0005.ICN" }, { FRNG0006, "FRNG0006.ICN" },
              { FRNG0007, "FRNG0007.ICN" }, { FRNG0008, "FRNG0008.ICN" }, { FRNG0009, "FRNG0009.ICN" }, { FRNG0010, "FRNG0010.ICN" }, { FRNG0011, "FRNG0011.ICN" },
              { FRNG0012, "FRNG0012.ICN" }, { FRNG0013, "FRNG0013.ICN" }, { FROTH, "FROTH.ICN" },       { GARGOYLE, "GARGOYLE.ICN" }, { G_BFLG32, "G-BFLG32.ICN" },
              { GENIE, "GENIE.ICN" },       { G_FLAG32, "G-FLAG32.ICN" }, { GHOST, "GHOST.ICN" },       { GOBLIN, "GOBLIN.ICN" },     { GOLEM2, "GOLEM2.ICN" },
              { GOLEM, "GOLEM.ICN" },       { GRIFFIN, "GRIFFIN.ICN" },   { GROUND12, "GROUND12.ICN" }, { GROUND4, "GROUND4.ICN" },   { GROUND6, "GROUND6.ICN" },
              { HALFLING, "HALFLING.ICN" }, { HALFLMSL, "HALFLMSL.ICN" }, { HASTE, "HASTE.ICN" },       { HEROBKG, "HEROBKG.ICN" },   { HEROES, "HEROES.ICN" },
              { HEROEXTE, "HEROEXTE.ICN" }, { HEROEXTG, "HEROEXTG.ICN" }, { HEROFL00, "HEROFL00.ICN" }, { HEROFL01, "HEROFL01.ICN" }, { HEROFL02, "HEROFL02.ICN" },
              { HEROFL03, "HEROFL03.ICN" }, { HEROFL04, "HEROFL04.ICN" }, { HEROFL05, "HEROFL05.ICN" }, { HEROFL06, "HEROFL06.ICN" }, { HEROLOGE, "HEROLOGE.ICN" },
              { HEROLOGO, "HEROLOGO.ICN" }, { HISCORE, "HISCORE.ICN" },   { HOURGLAS, "HOURGLAS.ICN" }, { HSBKG, "HSBKG.ICN" },       { HSBTNS, "HSBTNS.ICN" },
              { HSICONS, "HSICONS.ICN" },   { HYDRA, "HYDRA.ICN" },       { HYPNOTIZ, "HYPNOTIZ.ICN" }, { ICECLOUD, "ICECLOUD.ICN" }, { KEEP, "KEEP.ICN" },
              { KNGT32, "KNGT32.ICN" },     { LETTER12, "LETTER12.ICN" }, { LETTER4, "LETTER4.ICN" },   { LETTER6, "LETTER6.ICN" },   { LGNDXTRA, "LGNDXTRA.ICN" },
              { LGNDXTRE, "LGNDXTRE.ICN" }, { LICH2, "LICH2.ICN" },       { LICHCLOD, "LICHCLOD.ICN" }, { LICH, "LICH.ICN" },         { LICH_MSL, "LICH_MSL.ICN" },
              { LISTBOX, "LISTBOX.ICN" },   { LISTBOXS, "LISTBOXS.ICN" }, { LOCATORE, "LOCATORE.ICN" }, { LOCATORS, "LOCATORS.ICN" }, { MAGE1, "MAGE1.ICN" },
              { MAGE2, "MAGE2.ICN" },       { MAGEGLDB, "MAGEGLDB.ICN" }, { MAGEGLDK, "MAGEGLDK.ICN" }, { MAGEGLDN, "MAGEGLDN.ICN" }, { MAGEGLDS, "MAGEGLDS.ICN" },
              { MAGEGLDW, "MAGEGLDW.ICN" }, { MAGEGLDZ, "MAGEGLDZ.ICN" }, { MAGIC01, "MAGIC01.ICN" },   { MAGIC02, "MAGIC02.ICN" },   { MAGIC03, "MAGIC03.ICN" },
              { MAGIC04, "MAGIC04.ICN" },   { MAGIC06, "MAGIC06.ICN" },   { MAGIC07, "MAGIC07.ICN" },   { MAGIC08, "MAGIC08.ICN" },   { MANA, "MANA.ICN" },
              { MEDUSA, "MEDUSA.ICN" },     { METEOR, "METEOR.ICN" },     { MINICAPT, "MINICAPT.ICN" }, { MINIHERO, "MINIHERO.ICN" }, { MINILKMR, "MINILKMR.ICN" },
              { MINIMON, "MINIMON.ICN" },   { MINIPORT, "MINIPORT.ICN" }, { MINISS, "MINISS.ICN" },     { MINITOWN, "MINITOWN.ICN" }, { MINOTAU2, "MINOTAU2.ICN" },
              { MINOTAUR, "MINOTAUR.ICN" }, { MISC12, "MISC12.ICN" },     { MISC4, "MISC4.ICN" },       { MISC6, "MISC6.ICN" },       { MOATPART, "MOATPART.ICN" },
              { MOATWHOL, "MOATWHOL.ICN" }, { MOBILITY, "MOBILITY.ICN" }, { MONH0000, "MONH0000.ICN" }, { MONH0001, "MONH0001.ICN" }, { MONH0002, "MONH0002.ICN" },
              { MONH0003, "MONH0003.ICN" }, { MONH0004, "MONH0004.ICN" }, { MONH0005, "MONH0005.ICN" }, { MONH0006, "MONH0006.ICN" }, { MONH0007, "MONH0007.ICN" },
              { MONH0008, "MONH0008.ICN" }, { MONH0009, "MONH0009.ICN" }, { MONH0010, "MONH0010.ICN" }, { MONH0011, "MONH0011.ICN" }, { MONH0012, "MONH0012.ICN" },
              { MONH0013, "MONH0013.ICN" }, { MONH0014, "MONH0014.ICN" }, { MONH0015, "MONH0015.ICN" }, { MONH0016, "MONH0016.ICN" }, { MONH0017, "MONH0017.ICN" },
              { MONH0018, "MONH0018.ICN" }, { MONH0019, "MONH0019.ICN" }, { MONH0020, "MONH0020.ICN" }, { MONH0021, "MONH0021.ICN" }, { MONH0022, "MONH0022.ICN" },
              { MONH0023, "MONH0023.ICN" }, { MONH0024, "MONH0024.ICN" }, { MONH0025, "MONH0025.ICN" }, { MONH0026, "MONH0026.ICN" }, { MONH0027, "MONH0027.ICN" },
              { MONH0028, "MONH0028.ICN" }, { MONH0029, "MONH0029.ICN" }, { MONH0030, "MONH0030.ICN" }, { MONH0031, "MONH0031.ICN" }, { MONH0032, "MONH0032.ICN" },
              { MONH0033, "MONH0033.ICN" }, { MONH0034, "MONH0034.ICN" }, { MONH0035, "MONH0035.ICN" }, { MONH0036, "MONH0036.ICN" }, { MONH0037, "MONH0037.ICN" },
              { MONH0038, "MONH0038.ICN" }, { MONH0039, "MONH0039.ICN" }, { MONH0040, "MONH0040.ICN" }, { MONH0041, "MONH0041.ICN" }, { MONH0042, "MONH0042.ICN" },
              { MONH0043, "MONH0043.ICN" }, { MONH0044, "MONH0044.ICN" }, { MONH0045, "MONH0045.ICN" }, { MONH0046, "MONH0046.ICN" }, { MONH0047, "MONH0047.ICN" },
              { MONH0048, "MONH0048.ICN" }, { MONH0049, "MONH0049.ICN" }, { MONH0050, "MONH0050.ICN" }, { MONH0051, "MONH0051.ICN" }, { MONH0052, "MONH0052.ICN" },
              { MONH0053, "MONH0053.ICN" }, { MONH0054, "MONH0054.ICN" }, { MONH0055, "MONH0055.ICN" }, { MONH0056, "MONH0056.ICN" }, { MONH0057, "MONH0057.ICN" },
              { MONH0058, "MONH0058.ICN" }, { MONH0059, "MONH0059.ICN" }, { MONH0060, "MONH0060.ICN" }, { MONH0061, "MONH0061.ICN" }, { MONH0062, "MONH0062.ICN" },
              { MONH0063, "MONH0063.ICN" }, { MONH0064, "MONH0064.ICN" }, { MONH0065, "MONH0065.ICN" }, { MONS32, "MONS32.ICN" },     { MORALEB, "MORALEB.ICN" },
              { MORALEG, "MORALEG.ICN" },   { MTNCRCK, "MTNCRCK.ICN" },   { MTNDIRT, "MTNDIRT.ICN" },   { MTNDSRT, "MTNDSRT.ICN" },   { MTNGRAS, "MTNGRAS.ICN" },
              { MTNLAVA, "MTNLAVA.ICN" },   { MTNMULT, "MTNMULT.ICN" },   { MTNSNOW, "MTNSNOW.ICN" },   { MTNSWMP, "MTNSWMP.ICN" },   { MUMMY2, "MUMMY2.ICN" },
              { MUMMYW, "MUMMYW.ICN" },     { NECR32, "NECR32.ICN" },     { NETBOX, "NETBOX.ICN" },     { NGEXTRA, "NGEXTRA.ICN" },   { NGHSBKG, "NGHSBKG.ICN" },
              { NGMPBKG, "NGMPBKG.ICN" },   { NGSPBKG, "NGSPBKG.ICN" },   { NOMAD, "NOMAD.ICN" },       { O_BFLG32, "O-BFLG32.ICN" }, { OBJNARTI, "OBJNARTI.ICN" },
              { OBJNCRCK, "OBJNCRCK.ICN" }, { OBJNDIRT, "OBJNDIRT.ICN" }, { OBJNDSRT, "OBJNDSRT.ICN" }, { OBJNGRA2, "OBJNGRA2.ICN" }, { OBJNGRAS, "OBJNGRAS.ICN" },
              { OBJNHAUN, "OBJNHAUN.ICN" }, { OBJNLAV2, "OBJNLAV2.ICN" }, { OBJNLAV3, "OBJNLAV3.ICN" }, { OBJNLAVA, "OBJNLAVA.ICN" }, { OBJNMUL2, "OBJNMUL2.ICN" },
              { OBJNMULT, "OBJNMULT.ICN" }, { OBJNRSRC, "OBJNRSRC.ICN" }, { OBJNSNOW, "OBJNSNOW.ICN" }, { OBJNSWMP, "OBJNSWMP.ICN" }, { OBJNTOWN, "OBJNTOWN.ICN" },
              { OBJNTWBA, "OBJNTWBA.ICN" }, { OBJNTWRD, "OBJNTWRD.ICN" }, { OBJNTWSH, "OBJNTWSH.ICN" }, { OBJNWAT2, "OBJNWAT2.ICN" }, { OBJNWATR, "OBJNWATR.ICN" },
              { OBJNXTRA, "OBJNXTRA.ICN" }, { OBJPALET, "OBJPALET.ICN" }, { O_FLAG32, "O-FLAG32.ICN" }, { OGRE2, "OGRE2.ICN" },       { OGRE, "OGRE.ICN" },
              { ORC2, "ORC2.ICN" },         { ORC, "ORC.ICN" },           { ORC__MSL, "ORC__MSL.ICN" }, { OVERBACK, "OVERBACK.ICN" }, { OVERLAY, "OVERLAY.ICN" },
              { OVERVIEW, "OVERVIEW.ICN" }, { PALADIN2, "PALADIN2.ICN" }, { PALADIN, "PALADIN.ICN" },   { PARALYZE, "PARALYZE.ICN" }, { P_BFLG32, "P-BFLG32.ICN" },
              { PEASANT, "PEASANT.ICN" },   { P_FLAG32, "P-FLAG32.ICN" }, { PHOENIX, "PHOENIX.ICN" },   { PHYSICAL, "PHYSICAL.ICN" }, { PIKEMAN2, "PIKEMAN2.ICN" },
              { PIKEMAN, "PIKEMAN.ICN" },   { PORT0000, "PORT0000.ICN" }, { PORT0001, "PORT0001.ICN" }, { PORT0002, "PORT0002.ICN" }, { PORT0003, "PORT0003.ICN" },
              { PORT0004, "PORT0004.ICN" }, { PORT0005, "PORT0005.ICN" }, { PORT0006, "PORT0006.ICN" }, { PORT0007, "PORT0007.ICN" }, { PORT0008, "PORT0008.ICN" },
              { PORT0009, "PORT0009.ICN" }, { PORT0010, "PORT0010.ICN" }, { PORT0011, "PORT0011.ICN" }, { PORT0012, "PORT0012.ICN" }, { PORT0013, "PORT0013.ICN" },
              { PORT0014, "PORT0014.ICN" }, { PORT0015, "PORT0015.ICN" }, { PORT0016, "PORT0016.ICN" }, { PORT0017, "PORT0017.ICN" }, { PORT0018, "PORT0018.ICN" },
              { PORT0019, "PORT0019.ICN" }, { PORT0020, "PORT0020.ICN" }, { PORT0021, "PORT0021.ICN" }, { PORT0022, "PORT0022.ICN" }, { PORT0023, "PORT0023.ICN" },
              { PORT0024, "PORT0024.ICN" }, { PORT0025, "PORT0025.ICN" }, { PORT0026, "PORT0026.ICN" }, { PORT0027, "PORT0027.ICN" }, { PORT0028, "PORT0028.ICN" },
              { PORT0029, "PORT0029.ICN" }, { PORT0030, "PORT0030.ICN" }, { PORT0031, "PORT0031.ICN" }, { PORT0032, "PORT0032.ICN" }, { PORT0033, "PORT0033.ICN" },
              { PORT0034, "PORT0034.ICN" }, { PORT0035, "PORT0035.ICN" }, { PORT0036, "PORT0036.ICN" }, { PORT0037, "PORT0037.ICN" }, { PORT0038, "PORT0038.ICN" },
              { PORT0039, "PORT0039.ICN" }, { PORT0040, "PORT0040.ICN" }, { PORT0041, "PORT0041.ICN" }, { PORT0042, "PORT0042.ICN" }, { PORT0043, "PORT0043.ICN" },
              { PORT0044, "PORT0044.ICN" }, { PORT0045, "PORT0045.ICN" }, { PORT0046, "PORT0046.ICN" }, { PORT0047, "PORT0047.ICN" }, { PORT0048, "PORT0048.ICN" },
              { PORT0049, "PORT0049.ICN" }, { PORT0050, "PORT0050.ICN" }, { PORT0051, "PORT0051.ICN" }, { PORT0052, "PORT0052.ICN" }, { PORT0053, "PORT0053.ICN" },
              { PORT0054, "PORT0054.ICN" }, { PORT0055, "PORT0055.ICN" }, { PORT0056, "PORT0056.ICN" }, { PORT0057, "PORT0057.ICN" }, { PORT0058, "PORT0058.ICN" },
              { PORT0059, "PORT0059.ICN" }, { PORT0060, "PORT0060.ICN" }, { PORT0061, "PORT0061.ICN" }, { PORT0062, "PORT0062.ICN" }, { PORT0063, "PORT0063.ICN" },
              { PORT0064, "PORT0064.ICN" }, { PORT0065, "PORT0065.ICN" }, { PORT0066, "PORT0066.ICN" }, { PORT0067, "PORT0067.ICN" }, { PORT0068, "PORT0068.ICN" },
              { PORT0069, "PORT0069.ICN" }, { PORT0070, "PORT0070.ICN" }, { PORT0090, "PORT0090.ICN" }, { PORT0091, "PORT0091.ICN" }, { PORT0092, "PORT0092.ICN" },
              { PORT0093, "PORT0093.ICN" }, { PORT0094, "PORT0094.ICN" }, { PORT0095, "PORT0095.ICN" }, { PORTCFLG, "PORTCFLG.ICN" }, { PORTMEDI, "PORTMEDI.ICN" },
              { PORTXTRA, "PORTXTRA.ICN" }, { PRIMSKIL, "PRIMSKIL.ICN" }, { PUZZLE, "PUZZLE.ICN" },     { QWIKHERO, "QWIKHERO.ICN" }, { QWIKINFO, "QWIKINFO.ICN" },
              { QWIKTOWN, "QWIKTOWN.ICN" }, { RADAR, "RADAR.ICN" },       { R_BFLG32, "R-BFLG32.ICN" }, { RECR2BKG, "RECR2BKG.ICN" }, { RECRBKG, "RECRBKG.ICN" },
              { RECRUIT, "RECRUIT.ICN" },   { REDBACK, "REDBACK.ICN" },   { REDDEATH, "REDDEATH.ICN" }, { REDFIRE, "REDFIRE.ICN" },   { REQBKG, "REQBKG.ICN" },
              { REQSBKG, "REQSBKG.ICN" },   { REQUEST, "REQUEST.ICN" },   { REQUESTS, "REQUESTS.ICN" }, { RESOURCE, "RESOURCE.ICN" }, { RESSMALL, "RESSMALL.ICN" },
              { R_FLAG32, "R-FLAG32.ICN" }, { ROAD, "ROAD.ICN" },         { ROC, "ROC.ICN" },           { ROGUE, "ROGUE.ICN" },       { ROUTE, "ROUTE.ICN" },
              { SCENIBKG, "SCENIBKG.ICN" }, { SCROLL2, "SCROLL2.ICN" },   { SCROLLCN, "SCROLLCN.ICN" }, { SCROLLE, "SCROLLE.ICN" },   { SCROLL, "SCROLL.ICN" },
              { SECSKILL, "SECSKILL.ICN" }, { SHADOW32, "SHADOW32.ICN" }, { SHIELD, "SHIELD.ICN" },     { SHNGANIM, "SHNGANIM.ICN" }, { SKELETON, "SKELETON.ICN" },
              { SMALCLOD, "SMALCLOD.ICN" }, { SMALFONT, "SMALFONT.ICN" }, { SMALLBAR, "SMALLBAR.ICN" }, { SORC32, "SORC32.ICN" },     { SPANBKGE, "SPANBKGE.ICN" },
              { SPANBKG, "SPANBKG.ICN" },   { SPANBTNE, "SPANBTNE.ICN" }, { SPANBTN, "SPANBTN.ICN" },   { SPANEL, "SPANEL.ICN" },     { SPARKS, "SPARKS.ICN" },
              { SPELCO, "SPELCO.ICN" },     { SPELLINF, "SPELLINF.ICN" }, { SPELLINL, "SPELLINL.ICN" }, { SPELLS, "SPELLS.ICN" },     { SPRITE, "SPRITE.ICN" },
              { STELSKIN, "STELSKIN.ICN" }, { STONBACK, "STONBACK.ICN" }, { STONBAKE, "STONBAKE.ICN" }, { STONEBAK, "STONEBAK.ICN" }, { STONEBK2, "STONEBK2.ICN" },
              { STONSKIN, "STONSKIN.ICN" }, { STORM, "STORM.ICN" },       { STREAM, "STREAM.ICN" },     { STRIP, "STRIP.ICN" },       { SUNMOONE, "SUNMOONE.ICN" },
              { SUNMOON, "SUNMOON.ICN" },   { SURDRBKE, "SURDRBKE.ICN" }, { SURDRBKG, "SURDRBKG.ICN" }, { SURRENDE, "SURRENDE.ICN" }, { SURRENDR, "SURRENDR.ICN" },
              { SWAPBTN, "SWAPBTN.ICN" },   { SWAPWIN, "SWAPWIN.ICN" },   { SWORDSM2, "SWORDSM2.ICN" }, { SWORDSMN, "SWORDSMN.ICN" }, { SYSTEME, "SYSTEME.ICN" },
              { SYSTEM, "SYSTEM.ICN" },     { TAVWIN, "TAVWIN.ICN" },     { TENT, "TENT.ICN" },         { TERRAINS, "TERRAINS.ICN" }, { TEXTBACK, "TEXTBACK.ICN" },
              { TEXTBAK2, "TEXTBAK2.ICN" }, { TEXTBAR, "TEXTBAR.ICN" },   { TITANBLA, "TITANBLA.ICN" }, { TITANBLU, "TITANBLU.ICN" }, { TITANMSL, "TITANMSL.ICN" },
              { TOWNBKG0, "TOWNBKG0.ICN" }, { TOWNBKG1, "TOWNBKG1.ICN" }, { TOWNBKG2, "TOWNBKG2.ICN" }, { TOWNBKG3, "TOWNBKG3.ICN" }, { TOWNBKG4, "TOWNBKG4.ICN" },
              { TOWNBKG5, "TOWNBKG5.ICN" }, { TOWNFIX, "TOWNFIX.ICN" },   { TOWNNAME, "TOWNNAME.ICN" }, { TOWNWIND, "TOWNWIND.ICN" }, { TRADPOSE, "TRADPOSE.ICN" },
              { TRADPOST, "TRADPOST.ICN" }, { TREASURY, "TREASURY.ICN" }, { TREDECI, "TREDECI.ICN" },   { TREEVIL, "TREEVIL.ICN" },   { TREFALL, "TREFALL.ICN" },
              { TREFIR, "TREFIR.ICN" },     { TREJNGL, "TREJNGL.ICN" },   { TRESNOW, "TRESNOW.ICN" },   { TROLL2, "TROLL2.ICN" },     { TROLL, "TROLL.ICN" },
              { TROLLMSL, "TROLLMSL.ICN" }, { TWNBBOAT, "TWNBBOAT.ICN" }, { TWNBCAPT, "TWNBCAPT.ICN" }, { TWNBCSTL, "TWNBCSTL.ICN" }, { TWNBDOCK, "TWNBDOCK.ICN" },
              { TWNBDW_0, "TWNBDW_0.ICN" }, { TWNBDW_1, "TWNBDW_1.ICN" }, { TWNBDW_2, "TWNBDW_2.ICN" }, { TWNBDW_3, "TWNBDW_3.ICN" }, { TWNBDW_4, "TWNBDW_4.ICN" },
              { TWNBDW_5, "TWNBDW_5.ICN" }, { TWNBEXT0, "TWNBEXT0.ICN" }, { TWNBEXT1, "TWNBEXT1.ICN" }, { TWNBEXT2, "TWNBEXT2.ICN" }, { TWNBEXT3, "TWNBEXT3.ICN" },
              { TWNBLTUR, "TWNBLTUR.ICN" }, { TWNBMAGE, "TWNBMAGE.ICN" }, { TWNBMARK, "TWNBMARK.ICN" }, { TWNBMOAT, "TWNBMOAT.ICN" }, { TWNBRTUR, "TWNBRTUR.ICN" },
              { TWNBSPEC, "TWNBSPEC.ICN" }, { TWNBSTAT, "TWNBSTAT.ICN" }, { TWNBTENT, "TWNBTENT.ICN" }, { TWNBTHIE, "TWNBTHIE.ICN" }, { TWNBTVRN, "TWNBTVRN.ICN" },
              { TWNBUP_1, "TWNBUP_1.ICN" }, { TWNBUP_3, "TWNBUP_3.ICN" }, { TWNBUP_4, "TWNBUP_4.ICN" }, { TWNBWEL2, "TWNBWEL2.ICN" }, { TWNBWELL, "TWNBWELL.ICN" },
              { TWNKBOAT, "TWNKBOAT.ICN" }, { TWNKCAPT, "TWNKCAPT.ICN" }, { TWNKCSTL, "TWNKCSTL.ICN" }, { TWNKDOCK, "TWNKDOCK.ICN" }, { TWNKDW_0, "TWNKDW_0.ICN" },
              { TWNKDW_1, "TWNKDW_1.ICN" }, { TWNKDW_2, "TWNKDW_2.ICN" }, { TWNKDW_3, "TWNKDW_3.ICN" }, { TWNKDW_4, "TWNKDW_4.ICN" }, { TWNKDW_5, "TWNKDW_5.ICN" },
              { TWNKEXT0, "TWNKEXT0.ICN" }, { TWNKEXT1, "TWNKEXT1.ICN" }, { TWNKEXT2, "TWNKEXT2.ICN" }, { TWNKLTUR, "TWNKLTUR.ICN" }, { TWNKMAGE, "TWNKMAGE.ICN" },
              { TWNKMARK, "TWNKMARK.ICN" }, { TWNKMOAT, "TWNKMOAT.ICN" }, { TWNKRTUR, "TWNKRTUR.ICN" }, { TWNKSPEC, "TWNKSPEC.ICN" }, { TWNKSTAT, "TWNKSTAT.ICN" },
              { TWNKTENT, "TWNKTENT.ICN" }, { TWNKTHIE, "TWNKTHIE.ICN" }, { TWNKTVRN, "TWNKTVRN.ICN" }, { TWNKUP_1, "TWNKUP_1.ICN" }, { TWNKUP_2, "TWNKUP_2.ICN" },
              { TWNKUP_3, "TWNKUP_3.ICN" }, { TWNKUP_4, "TWNKUP_4.ICN" }, { TWNKUP_5, "TWNKUP_5.ICN" }, { TWNKWEL2, "TWNKWEL2.ICN" }, { TWNKWELL, "TWNKWELL.ICN" },
              { TWNNBOAT, "TWNNBOAT.ICN" }, { TWNNCAPT, "TWNNCAPT.ICN" }, { TWNNCSTL, "TWNNCSTL.ICN" }, { TWNNDOCK, "TWNNDOCK.ICN" }, { TWNNDW_0, "TWNNDW_0.ICN" },
              { TWNNDW_1, "TWNNDW_1.ICN" }, { TWNNDW_2, "TWNNDW_2.ICN" }, { TWNNDW_3, "TWNNDW_3.ICN" }, { TWNNDW_4, "TWNNDW_4.ICN" }, { TWNNDW_5, "TWNNDW_5.ICN" },
              { TWNNEXT0, "TWNNEXT0.ICN" }, { TWNNLTUR, "TWNNLTUR.ICN" }, { TWNNMAGE, "TWNNMAGE.ICN" }, { TWNNMARK, "TWNNMARK.ICN" }, { TWNNMOAT, "TWNNMOAT.ICN" },
              { TWNNRTUR, "TWNNRTUR.ICN" }, { TWNNSPEC, "TWNNSPEC.ICN" }, { TWNNSTAT, "TWNNSTAT.ICN" }, { TWNNTENT, "TWNNTENT.ICN" }, { TWNNTHIE, "TWNNTHIE.ICN" },
              { TWNNTVRN, "TWNNTVRN.ICN" }, { TWNNUP_1, "TWNNUP_1.ICN" }, { TWNNUP_2, "TWNNUP_2.ICN" }, { TWNNUP_3, "TWNNUP_3.ICN" }, { TWNNUP_4, "TWNNUP_4.ICN" },
              { TWNNWEL2, "TWNNWEL2.ICN" }, { TWNNWELL, "TWNNWELL.ICN" }, { TWNSBOAT, "TWNSBOAT.ICN" }, { TWNSCAPT, "TWNSCAPT.ICN" }, { TWNSCSTL, "TWNSCSTL.ICN" },
              { TWNSDOCK, "TWNSDOCK.ICN" }, { TWNSDW_0, "TWNSDW_0.ICN" }, { TWNSDW_1, "TWNSDW_1.ICN" }, { TWNSDW_2, "TWNSDW_2.ICN" }, { TWNSDW_3, "TWNSDW_3.ICN" },
              { TWNSDW_4, "TWNSDW_4.ICN" }, { TWNSDW_5, "TWNSDW_5.ICN" }, { TWNSEXT0, "TWNSEXT0.ICN" }, { TWNSEXT1, "TWNSEXT1.ICN" }, { TWNSLTUR, "TWNSLTUR.ICN" },
              { TWNSMAGE, "TWNSMAGE.ICN" }, { TWNSMARK, "TWNSMARK.ICN" }, { TWNSMOAT, "TWNSMOAT.ICN" }, { TWNSRTUR, "TWNSRTUR.ICN" }, { TWNSSPEC, "TWNSSPEC.ICN" },
              { TWNSSTAT, "TWNSSTAT.ICN" }, { TWNSTENT, "TWNSTENT.ICN" }, { TWNSTHIE, "TWNSTHIE.ICN" }, { TWNSTVRN, "TWNSTVRN.ICN" }, { TWNSUP_1, "TWNSUP_1.ICN" },
              { TWNSUP_2, "TWNSUP_2.ICN" }, { TWNSUP_3, "TWNSUP_3.ICN" }, { TWNSWEL2, "TWNSWEL2.ICN" }, { TWNSWELL, "TWNSWELL.ICN" }, { TWNWBOAT, "TWNWBOAT.ICN" },
              { TWNWCAPT, "TWNWCAPT.ICN" }, { TWNWCSTL, "TWNWCSTL.ICN" }, { TWNWDOCK, "TWNWDOCK.ICN" }, { TWNWDW_0, "TWNWDW_0.ICN" }, { TWNWDW_1, "TWNWDW_1.ICN" },
              { TWNWDW_2, "TWNWDW_2.ICN" }, { TWNWDW_3, "TWNWDW_3.ICN" }, { TWNWDW_4, "TWNWDW_4.ICN" }, { TWNWDW_5, "TWNWDW_5.ICN" }, { TWNWEXT0, "TWNWEXT0.ICN" },
              { TWNWLTUR, "TWNWLTUR.ICN" }, { TWNWMAGE, "TWNWMAGE.ICN" }, { TWNWMARK, "TWNWMARK.ICN" }, { TWNWMOAT, "TWNWMOAT.ICN" }, { TWNWRTUR, "TWNWRTUR.ICN" },
              { TWNWSPEC, "TWNWSPEC.ICN" }, { TWNWSTAT, "TWNWSTAT.ICN" }, { TWNWTENT, "TWNWTENT.ICN" }, { TWNWTHIE, "TWNWTHIE.ICN" }, { TWNWTVRN, "TWNWTVRN.ICN" },
              { TWNWUP_3, "TWNWUP_3.ICN" }, { TWNWUP5B, "TWNWUP5B.ICN" }, { TWNWUP_5, "TWNWUP_5.ICN" }, { TWNWWEL2, "TWNWWEL2.ICN" }, { TWNWWELL, "TWNWWELL.ICN" },
              { TWNZBOAT, "TWNZBOAT.ICN" }, { TWNZCAPT, "TWNZCAPT.ICN" }, { TWNZCSTL, "TWNZCSTL.ICN" }, { TWNZDOCK, "TWNZDOCK.ICN" }, { TWNZDW_0, "TWNZDW_0.ICN" },
              { TWNZDW_1, "TWNZDW_1.ICN" }, { TWNZDW_2, "TWNZDW_2.ICN" }, { TWNZDW_3, "TWNZDW_3.ICN" }, { TWNZDW_4, "TWNZDW_4.ICN" }, { TWNZDW_5, "TWNZDW_5.ICN" },
              { TWNZEXT0, "TWNZEXT0.ICN" }, { TWNZLTUR, "TWNZLTUR.ICN" }, { TWNZMAGE, "TWNZMAGE.ICN" }, { TWNZMARK, "TWNZMARK.ICN" }, { TWNZMOAT, "TWNZMOAT.ICN" },
              { TWNZRTUR, "TWNZRTUR.ICN" }, { TWNZSPEC, "TWNZSPEC.ICN" }, { TWNZSTAT, "TWNZSTAT.ICN" }, { TWNZTENT, "TWNZTENT.ICN" }, { TWNZTHIE, "TWNZTHIE.ICN" },
              { TWNZTVRN, "TWNZTVRN.ICN" }, { TWNZUP_2, "TWNZUP_2.ICN" }, { TWNZUP_4, "TWNZUP_4.ICN" }, { TWNZUP_5, "TWNZUP_5.ICN" }, { TWNZWEL2, "TWNZWEL2.ICN" },
              { TWNZWELL, "TWNZWELL.ICN" }, { UNICORN, "UNICORN.ICN" },   { VAMPIRE2, "VAMPIRE2.ICN" }, { VAMPIRE, "VAMPIRE.ICN" },   { VGENBKGE, "VGENBKGE.ICN" },
              { VGENBKG, "VGENBKG.ICN" },   { VIEW_ALL, "VIEW_ALL.ICN" }, { VIEWARME, "VIEWARME.ICN" }, { VIEWARMY, "VIEWARMY.ICN" }, { VIEWARSM, "VIEWARSM.ICN" },
              { VIEWDDOR, "VIEWDDOR.ICN" }, { VIEWGEN, "VIEWGEN.ICN" },   { VIEWHROS, "VIEWHROS.ICN" }, { VIEWMINE, "VIEWMINE.ICN" }, { VIEWPUZL, "VIEWPUZL.ICN" },
              { VIEWRSRC, "VIEWRSRC.ICN" }, { VIEWRTFX, "VIEWRTFX.ICN" }, { VIEWTWNS, "VIEWTWNS.ICN" }, { VIEWWRLD, "VIEWWRLD.ICN" }, { VWFLAG12, "VWFLAG12.ICN" },
              { VWFLAG4, "VWFLAG4.ICN" },   { VWFLAG6, "VWFLAG6.ICN" },   { WELEM, "WELEM.ICN" },       { WELLBKG, "WELLBKG.ICN" },   { WELLXTRA, "WELLXTRA.ICN" },
              { WINCMBBE, "WINCMBBE.ICN" }, { WINCMBTB, "WINCMBTB.ICN" }, { WINCMBT, "WINCMBT.ICN" },   { WINLOSEB, "WINLOSEB.ICN" }, { WINLOSEE, "WINLOSEE.ICN" },
              { WINLOSE, "WINLOSE.ICN" },   { WOLF, "WOLF.ICN" },         { WRLK32, "WRLK32.ICN" },     { WZRD32, "WZRD32.ICN" },     { X_IVY, "X_IVY.ICN" },
              { X_LOADCM, "X_LOADCM.ICN" }, { X_CMPBKG, "X_CMPBKG.ICN" }, { X_CMPBTN, "X_CMPBTN.ICN" }, { X_CMPEXT, "X_CMPEXT.ICN" }, { X_TRACK1, "X_TRACK1.ICN" },
              { X_TRACK2, "X_TRACK2.ICN" }, { X_TRACK3, "X_TRACK3.ICN" }, { X_TRACK4, "X_TRACK4.ICN" }, { X_LOC1, "X_LOC1.ICN" },     { X_LOC2, "X_LOC2.ICN" },
              { X_LOC3, "X_LOC3.ICN" },     { XPRIMARY, "XPRIMARY.ICN" }, { Y_BFLG32, "Y-BFLG32.ICN" }, { Y_FLAG32, "Y-FLAG32.ICN" }, { YINYANG, "YINYANG.ICN" },
              { ZOMBIE2, "ZOMBIE2.ICN" },   { ZOMBIE, "ZOMBIE.ICN" } } };
}

const char * ICN::getIcnFileName( const int icnId )
{
    return UNKNOWN <= icnId && icnId < LAST_VALID_FILE_ICN ? icnMap[icnId].string : "CUSTOM";
}

uint32_t ICN::getAnimatedIcnIndex( const int icnId, const uint32_t startIndex, const uint32_t currentFrameNumber, const bool quantity /* = false */ )
{
    switch ( icnId ) {
    case TWNBBOAT:
    case TWNKBOAT:
    case TWNNBOAT:
    case TWNSBOAT:
    case TWNWBOAT:
    case TWNZBOAT:
        return 1 + currentFrameNumber % 9;
    case CMBTCAPB:
    case CMBTCAPK:
    case CMBTCAPN:
    case CMBTCAPS:
    case CMBTCAPW:
    case CMBTCAPZ:
        return 1 + currentFrameNumber % 10;
    case CMBTHROB:
        return 1 + currentFrameNumber % 18;
    case CMBTHROK:
    case CMBTHRON:
        return 1 + currentFrameNumber % 19;
    case CMBTHROS:
    case CMBTHROW:
        return 1 + currentFrameNumber % 16;
    case CMBTHROZ:
        return 1 + currentFrameNumber % 18;
    case HEROFL00:
    case HEROFL01:
    case HEROFL02:
    case HEROFL03:
    case HEROFL04:
    case HEROFL05:
    case HEROFL06:
        return currentFrameNumber % 5;
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
        return 1 + currentFrameNumber % 5;
    case TWNBCSTL:
    case TWNKDW_2:
    case TWNKUP_2:
    case TWNNDW_5:
    case TWNNWEL2:
    case TWNWDW_0:
    case TWNWWEL2:
    case TWNZTVRN:
        return 1 + currentFrameNumber % 6;
    case TWNKDW_4:
    case TWNKUP_4:
        return 1 + currentFrameNumber % 7;
    case TAVWIN:
        return 2 + currentFrameNumber % 20;
    case CMBTLOS1:
        return 1 + currentFrameNumber % 30;
    case CMBTLOS2:
        return 1 + currentFrameNumber % 29;
    case CMBTLOS3:
        return 1 + currentFrameNumber % 22;
    case CMBTFLE1:
        return 1 + currentFrameNumber % 43;
    case CMBTFLE2:
        return 1 + currentFrameNumber % 26;
    case CMBTFLE3:
        return 1 + currentFrameNumber % 25;
    case CMBTSURR:
    case WINCMBT:
        return 1 + currentFrameNumber % 20;
    case MINIMON:
        return startIndex + 1 + currentFrameNumber % 6;
    case TWNNMAGE:
        return startIndex + 1 + currentFrameNumber % 5;
    case TWNBMAGE:
        return 4 == startIndex ? startIndex + 1 + currentFrameNumber % 8 : 0;
    case SHNGANIM:
        return 1 + currentFrameNumber % 39;
    case BTNSHNGL:
        return startIndex + currentFrameNumber % 4;
    case OBJNHAUN:
        return currentFrameNumber % 15;
    case OBJNWATR:
        switch ( startIndex ) {
        // bottle
        case 0x00:
            // 'startIndex' is equal to 0 here so it is not used in the expression.
            return ( currentFrameNumber % 11 ) + 1;
        // shadow
        case 0x0C:
        // chest
        case 0x13:
        // shadow
        case 0x26:
        // flotsam
        case 0x2D:
        // Magellan's Maps building shadow.
        case 0x37:
        // Magellan's Maps building boat.
        case 0x3E:
        // Magellan's Maps building waves.
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
            return startIndex + ( currentFrameNumber % 6 ) + 1;
        // seagull on stones
        case 0x76:
        case 0x86:
        case 0x96:
        case 0xA6:
            return startIndex + ( currentFrameNumber % 15 ) + 1;
        // whirlpool
        case 0xCA:
        case 0xCE:
        case 0xD2:
        case 0xD6:
        case 0xDA:
        case 0xDE:
            return startIndex + ( currentFrameNumber % 3 ) + 1;
        default:
            return 0;
        }
    case OBJNWAT2:
        switch ( startIndex ) {
        // sail broken ship (left)
        case 0x03:
        case 0x0C:
            return startIndex + ( currentFrameNumber % 6 ) + 1;
        default:
            return 0;
        }
    case OBJNCRCK:
        switch ( startIndex ) {
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
            return startIndex + ( currentFrameNumber % 10 ) + 1;
        default:
            return 0;
        }
    case OBJNDIRT:
        switch ( startIndex ) {
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
            return startIndex + ( currentFrameNumber % 3 ) + 1;
        default:
            return 0;
        }
    case OBJNDSRT:
        switch ( startIndex ) {
        // campfire
        case 0x36:
        case 0x3D:
            return startIndex + ( currentFrameNumber % 6 ) + 1;
        default:
            return 0;
        }
    case OBJNGRA2:
        switch ( startIndex ) {
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
            return startIndex + ( currentFrameNumber % 3 ) + 1;
        // smoke from chimney
        case 0x3F:
        case 0x46:
        case 0x4D:
        // Archer House
        case 0x54:
        // smoke from chimney
        case 0x5D:
        case 0x64:
        // shadow smoke
        case 0x6B:
        // Peasant Hut
        case 0x72:
            return startIndex + ( currentFrameNumber % 6 ) + 1;
        default:
            return 0;
        }
    case OBJNLAV2:
        switch ( startIndex ) {
        // middle volcano
        case 0x00:
        // shadow
        case 0x07:
        case 0x0E:
        // lava
        case 0x15:
            return startIndex + ( currentFrameNumber % 6 ) + 1;
        // small volcano
        // shadow
        case 0x21:
        case 0x2C:
        // lava
        case 0x37:
        case 0x43:
            return startIndex + ( currentFrameNumber % 10 ) + 1;
        default:
            return 0;
        }
    case OBJNLAV3:
        // big volcano
        switch ( startIndex ) {
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
            return startIndex + ( currentFrameNumber % 14 ) + 1;
        default:
            return 0;
        }
    case OBJNLAVA:
        switch ( startIndex ) {
        // shadow of lava
        case 0x4E:
        case 0x58:
        case 0x62:
            return startIndex + ( currentFrameNumber % 9 ) + 1;
        default:
            return 0;
        }
    case OBJNMUL2:
        switch ( startIndex ) {
        // lighthouse
        case 0x3D:
            return startIndex + ( currentFrameNumber % 9 ) + 1;
        // Alchemist Tower
        case 0x1B:
        // watermill
        case 0x53:
        case 0x5A:
        case 0x62:
        case 0x69:
        // fire in Wagon Camp
        case 0x81:
        // smoke smithy (2 chimney)
        case 0xA6:
        // smoke smithy (1 chimney)
        case 0xAD:
        // shadow smoke
        case 0xB4:
            return startIndex + ( currentFrameNumber % 6 ) + 1;
        // magic garden
        case 0xBE:
            return quantity ? startIndex + ( currentFrameNumber % 6 ) + 1 : startIndex + 7;
        default:
            return 0;
        }
    case OBJNMULT:
        switch ( startIndex ) {
        // smoke
        case 0x05:
        // shadow
        case 0x0F:
        case 0x19:
            return startIndex + ( currentFrameNumber % 9 ) + 1;
        // smoke
        case 0x24:
        // shadow
        case 0x2D:
            return startIndex + ( currentFrameNumber % 8 ) + 1;
        // smoke
        case 0x5A:
        // shadow
        case 0x61:
        case 0x68:
        case 0x7C:
        // campfire
        case 0x83:
            return startIndex + ( currentFrameNumber % 6 ) + 1;
        default:
            return 0;
        }
    case OBJNSNOW:
        switch ( startIndex ) {
        // fire camp
        case 0x04:
        // Alchemist Tower
        case 0x97:
        // watermill
        case 0xA2:
        case 0xA9:
        case 0xB1:
        case 0xB8:
            return startIndex + ( currentFrameNumber % 6 ) + 1;
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
            return startIndex + ( currentFrameNumber % 3 ) + 1;
        default:
            return 0;
        }
    case OBJNSWMP:
        switch ( startIndex ) {
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
            return startIndex + ( currentFrameNumber % 6 ) + 1;
        default:
            return 0;
        }
    // extra objects for loyalty version
    case X_LOC1:
        if ( Settings::Get().isPriceOfLoyaltySupported() )
            switch ( startIndex ) {
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
                return startIndex + ( currentFrameNumber % 8 ) + 1;
            default:
                return 0;
            }
        break;
    // extra objects for loyalty version
    case X_LOC2:
        if ( Settings::Get().isPriceOfLoyaltySupported() )
            switch ( startIndex ) {
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
                return startIndex + ( currentFrameNumber % 8 ) + 1;
            default:
                return 0;
            }
        break;

    // extra objects for loyalty version
    case X_LOC3:
        if ( Settings::Get().isPriceOfLoyaltySupported() )
            switch ( startIndex ) {
            // hut magi
            case 0x00:
            case 0x0A:
            case 0x14:
            // eye magi
            case 0x20:
            case 0x29:
            case 0x32:
                return startIndex + ( currentFrameNumber % 8 ) + 1;
            // barrier
            case 0x3C:
            case 0x42:
            case 0x48:
            case 0x4E:
            case 0x54:
            case 0x5A:
            case 0x60:
            case 0x66:
                return startIndex + ( currentFrameNumber % 4 ) + 1;
            default:
                return 0;
            }
        break;

    default:
        break;
    }
    return 0;
}

int ICN::getHeroPortraitIcnId( const int heroId )
{
    switch ( heroId ) {
    case Heroes::UNKNOWN:
        return ICN::UNKNOWN;
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
    case Heroes::BRAX:
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
    case Heroes::DEBUG_HERO:
        return ICN::PORT0059;
    default:
        // Did you add a new hero? Add the logic above!
        assert( 0 );
        break;
    }

    return ICN::UNKNOWN;
}

int ICN::getCaptainIcnId( const int race )
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

int ICN::getBuildingIcnId( const int race )
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

int ICN::getCastleIcnId( const int race )
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

int ICN::getFlagIcnId( const int color )
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


int ICN::getIcnFromStr(const std::string& id){
    if(id == "ADVBORDE"){ return ICN::ADVBORDE; }else
    if(id == "ADVBORD"){ return ICN::ADVBORD; }else
    if(id == "ADVBTNS"){ return ICN::ADVBTNS; }else
    if(id == "ADVEBTNS"){ return ICN::ADVEBTNS; }else
    if(id == "ADVMCO"){ return ICN::ADVMCO; }else
    if(id == "AELEM"){ return ICN::AELEM; }else
    if(id == "APANBKGE"){ return ICN::APANBKGE; }else
    if(id == "APANBKG"){ return ICN::APANBKG; }else
    if(id == "APANELE"){ return ICN::APANELE; }else
    if(id == "APANEL"){ return ICN::APANEL; }else
    if(id == "ARCHER2"){ return ICN::ARCHER2; }else
    if(id == "ARCHER"){ return ICN::ARCHER; }else
    if(id == "ARCH_MSL"){ return ICN::ARCH_MSL; }else
    if(id == "ART32"){ return ICN::ART32; }else
    if(id == "ARTFX"){ return ICN::ARTFX; }else
    if(id == "ARTIFACT"){ return ICN::ARTIFACT; }else
    if(id == "BARB32"){ return ICN::BARB32; }else
    if(id == "B_BFLG32"){ return ICN::B_BFLG32; }else
    if(id == "BERZERK"){ return ICN::BERZERK; }else
    if(id == "B_FLAG32"){ return ICN::B_FLAG32; }else
    if(id == "BIGBAR"){ return ICN::BIGBAR; }else
    if(id == "BLDGXTRA"){ return ICN::BLDGXTRA; }else
    if(id == "BLESS"){ return ICN::BLESS; }else
    if(id == "BLIND"){ return ICN::BLIND; }else
    if(id == "BLUEFIRE"){ return ICN::BLUEFIRE; }else
    if(id == "BOAR"){ return ICN::BOAR; }else
    if(id == "BOAT32"){ return ICN::BOAT32; }else
    if(id == "BOATSHAD"){ return ICN::BOATSHAD; }else
    if(id == "BOATWIND"){ return ICN::BOATWIND; }else
    if(id == "BOOK"){ return ICN::BOOK; }else
    if(id == "BORDEDIT"){ return ICN::BORDEDIT; }else
    if(id == "BOULDER"){ return ICN::BOULDER; }else
    if(id == "BRCREST"){ return ICN::BRCREST; }else
    if(id == "BROTHERS"){ return ICN::BROTHERS; }else
    if(id == "BTNBAUD"){ return ICN::BTNBAUD; }else
    if(id == "BTNCMPGN"){ return ICN::BTNCMPGN; }else
    if(id == "BTNCOM"){ return ICN::BTNCOM; }else
    if(id == "BTNDCCFG"){ return ICN::BTNDCCFG; }else
    if(id == "BTNDC"){ return ICN::BTNDC; }else
    if(id == "BTNEMAIN"){ return ICN::BTNEMAIN; }else
    if(id == "BTNENEW"){ return ICN::BTNENEW; }else
    if(id == "BTNESIZE"){ return ICN::BTNESIZE; }else
    if(id == "BTNHOTST"){ return ICN::BTNHOTST; }else
    if(id == "BTNMCFG"){ return ICN::BTNMCFG; }else
    if(id == "BTNMODEM"){ return ICN::BTNMODEM; }else
    if(id == "BTNMP"){ return ICN::BTNMP; }else
    if(id == "BTNNET2"){ return ICN::BTNNET2; }else
    if(id == "BTNNET"){ return ICN::BTNNET; }else
    if(id == "BTNNEWGM"){ return ICN::BTNNEWGM; }else
    if(id == "BTNSHNGL"){ return ICN::BTNSHNGL; }else
    if(id == "BUILDING"){ return ICN::BUILDING; }else
    if(id == "BUYBUILD"){ return ICN::BUYBUILD; }else
    if(id == "BUYBUILE"){ return ICN::BUYBUILE; }else
    if(id == "CAMPBKGE"){ return ICN::CAMPBKGE; }else
    if(id == "CAMPBKGG"){ return ICN::CAMPBKGG; }else
    if(id == "CAMPXTRE"){ return ICN::CAMPXTRE; }else
    if(id == "CAMPXTRG"){ return ICN::CAMPXTRG; }else
    if(id == "CAPTCOVR"){ return ICN::CAPTCOVR; }else
    if(id == "CASLBAR"){ return ICN::CASLBAR; }else
    if(id == "CASLWIND"){ return ICN::CASLWIND; }else
    if(id == "CASLXTRA"){ return ICN::CASLXTRA; }else
    if(id == "CASTBKGB"){ return ICN::CASTBKGB; }else
    if(id == "CASTBKGK"){ return ICN::CASTBKGK; }else
    if(id == "CASTBKGN"){ return ICN::CASTBKGN; }else
    if(id == "CASTBKGS"){ return ICN::CASTBKGS; }else
    if(id == "CASTBKGW"){ return ICN::CASTBKGW; }else
    if(id == "CASTBKGZ"){ return ICN::CASTBKGZ; }else
    if(id == "CASTLEB"){ return ICN::CASTLEB; }else
    if(id == "CASTLEK"){ return ICN::CASTLEK; }else
    if(id == "CASTLEN"){ return ICN::CASTLEN; }else
    if(id == "CASTLES"){ return ICN::CASTLES; }else
    if(id == "CASTLEW"){ return ICN::CASTLEW; }else
    if(id == "CASTLEZ"){ return ICN::CASTLEZ; }else
    if(id == "CATAPULT"){ return ICN::CATAPULT; }else
    if(id == "CAVALRYB"){ return ICN::CAVALRYB; }else
    if(id == "CAVALRYR"){ return ICN::CAVALRYR; }else
    if(id == "CBKGBEAC"){ return ICN::CBKGBEAC; }else
    if(id == "CBKGCRCK"){ return ICN::CBKGCRCK; }else
    if(id == "CBKGDIMT"){ return ICN::CBKGDIMT; }else
    if(id == "CBKGDITR"){ return ICN::CBKGDITR; }else
    if(id == "CBKGDSRT"){ return ICN::CBKGDSRT; }else
    if(id == "CBKGGRAV"){ return ICN::CBKGGRAV; }else
    if(id == "CBKGGRMT"){ return ICN::CBKGGRMT; }else
    if(id == "CBKGGRTR"){ return ICN::CBKGGRTR; }else
    if(id == "CBKGLAVA"){ return ICN::CBKGLAVA; }else
    if(id == "CBKGSNMT"){ return ICN::CBKGSNMT; }else
    if(id == "CBKGSNTR"){ return ICN::CBKGSNTR; }else
    if(id == "CBKGSWMP"){ return ICN::CBKGSWMP; }else
    if(id == "CBKGWATR"){ return ICN::CBKGWATR; }else
    if(id == "CELLWIN"){ return ICN::CELLWIN; }else
    if(id == "CENTAUR"){ return ICN::CENTAUR; }else
    if(id == "CFLGSMAL"){ return ICN::CFLGSMAL; }else
    if(id == "CLOP32"){ return ICN::CLOP32; }else
    if(id == "CLOUDLUK"){ return ICN::CLOUDLUK; }else
    if(id == "CMBTCAPB"){ return ICN::CMBTCAPB; }else
    if(id == "CMBTCAPK"){ return ICN::CMBTCAPK; }else
    if(id == "CMBTCAPN"){ return ICN::CMBTCAPN; }else
    if(id == "CMBTCAPS"){ return ICN::CMBTCAPS; }else
    if(id == "CMBTCAPW"){ return ICN::CMBTCAPW; }else
    if(id == "CMBTCAPZ"){ return ICN::CMBTCAPZ; }else
    if(id == "CMBTFLE1"){ return ICN::CMBTFLE1; }else
    if(id == "CMBTFLE2"){ return ICN::CMBTFLE2; }else
    if(id == "CMBTFLE3"){ return ICN::CMBTFLE3; }else
    if(id == "CMBTHROB"){ return ICN::CMBTHROB; }else
    if(id == "CMBTHROK"){ return ICN::CMBTHROK; }else
    if(id == "CMBTHRON"){ return ICN::CMBTHRON; }else
    if(id == "CMBTHROS"){ return ICN::CMBTHROS; }else
    if(id == "CMBTHROW"){ return ICN::CMBTHROW; }else
    if(id == "CMBTHROZ"){ return ICN::CMBTHROZ; }else
    if(id == "CMBTLOS1"){ return ICN::CMBTLOS1; }else
    if(id == "CMBTLOS2"){ return ICN::CMBTLOS2; }else
    if(id == "CMBTLOS3"){ return ICN::CMBTLOS3; }else
    if(id == "CMBTMISC"){ return ICN::CMBTMISC; }else
    if(id == "CMBTSURR"){ return ICN::CMBTSURR; }else
    if(id == "CMSECO"){ return ICN::CMSECO; }else
    if(id == "COBJ0000"){ return ICN::COBJ0000; }else
    if(id == "COBJ0001"){ return ICN::COBJ0001; }else
    if(id == "COBJ0002"){ return ICN::COBJ0002; }else
    if(id == "COBJ0003"){ return ICN::COBJ0003; }else
    if(id == "COBJ0004"){ return ICN::COBJ0004; }else
    if(id == "COBJ0005"){ return ICN::COBJ0005; }else
    if(id == "COBJ0006"){ return ICN::COBJ0006; }else
    if(id == "COBJ0007"){ return ICN::COBJ0007; }else
    if(id == "COBJ0008"){ return ICN::COBJ0008; }else
    if(id == "COBJ0009"){ return ICN::COBJ0009; }else
    if(id == "COBJ0010"){ return ICN::COBJ0010; }else
    if(id == "COBJ0011"){ return ICN::COBJ0011; }else
    if(id == "COBJ0012"){ return ICN::COBJ0012; }else
    if(id == "COBJ0013"){ return ICN::COBJ0013; }else
    if(id == "COBJ0014"){ return ICN::COBJ0014; }else
    if(id == "COBJ0015"){ return ICN::COBJ0015; }else
    if(id == "COBJ0016"){ return ICN::COBJ0016; }else
    if(id == "COBJ0017"){ return ICN::COBJ0017; }else
    if(id == "COBJ0018"){ return ICN::COBJ0018; }else
    if(id == "COBJ0019"){ return ICN::COBJ0019; }else
    if(id == "COBJ0020"){ return ICN::COBJ0020; }else
    if(id == "COBJ0021"){ return ICN::COBJ0021; }else
    if(id == "COBJ0022"){ return ICN::COBJ0022; }else
    if(id == "COBJ0023"){ return ICN::COBJ0023; }else
    if(id == "COBJ0024"){ return ICN::COBJ0024; }else
    if(id == "COBJ0025"){ return ICN::COBJ0025; }else
    if(id == "COBJ0026"){ return ICN::COBJ0026; }else
    if(id == "COBJ0027"){ return ICN::COBJ0027; }else
    if(id == "COBJ0028"){ return ICN::COBJ0028; }else
    if(id == "COBJ0029"){ return ICN::COBJ0029; }else
    if(id == "COBJ0030"){ return ICN::COBJ0030; }else
    if(id == "COBJ0031"){ return ICN::COBJ0031; }else
    if(id == "COLDRAY"){ return ICN::COLDRAY; }else
    if(id == "COLDRING"){ return ICN::COLDRING; }else
    if(id == "CONGRATS"){ return ICN::CONGRATS; }else
    if(id == "COVR0001"){ return ICN::COVR0001; }else
    if(id == "COVR0002"){ return ICN::COVR0002; }else
    if(id == "COVR0003"){ return ICN::COVR0003; }else
    if(id == "COVR0004"){ return ICN::COVR0004; }else
    if(id == "COVR0005"){ return ICN::COVR0005; }else
    if(id == "COVR0006"){ return ICN::COVR0006; }else
    if(id == "COVR0007"){ return ICN::COVR0007; }else
    if(id == "COVR0008"){ return ICN::COVR0008; }else
    if(id == "COVR0009"){ return ICN::COVR0009; }else
    if(id == "COVR0010"){ return ICN::COVR0010; }else
    if(id == "COVR0011"){ return ICN::COVR0011; }else
    if(id == "COVR0012"){ return ICN::COVR0012; }else
    if(id == "COVR0013"){ return ICN::COVR0013; }else
    if(id == "COVR0014"){ return ICN::COVR0014; }else
    if(id == "COVR0015"){ return ICN::COVR0015; }else
    if(id == "COVR0016"){ return ICN::COVR0016; }else
    if(id == "COVR0017"){ return ICN::COVR0017; }else
    if(id == "COVR0018"){ return ICN::COVR0018; }else
    if(id == "COVR0019"){ return ICN::COVR0019; }else
    if(id == "COVR0020"){ return ICN::COVR0020; }else
    if(id == "COVR0021"){ return ICN::COVR0021; }else
    if(id == "COVR0022"){ return ICN::COVR0022; }else
    if(id == "COVR0023"){ return ICN::COVR0023; }else
    if(id == "COVR0024"){ return ICN::COVR0024; }else
    if(id == "CPANBKGE"){ return ICN::CPANBKGE; }else
    if(id == "CPANBKG"){ return ICN::CPANBKG; }else
    if(id == "CPANELE"){ return ICN::CPANELE; }else
    if(id == "CPANEL"){ return ICN::CPANEL; }else
    if(id == "CREST"){ return ICN::CREST; }else
    if(id == "CSPANBKE"){ return ICN::CSPANBKE; }else
    if(id == "CSPANBKG"){ return ICN::CSPANBKG; }else
    if(id == "CSPANBTE"){ return ICN::CSPANBTE; }else
    if(id == "CSPANBTN"){ return ICN::CSPANBTN; }else
    if(id == "CSPANEL"){ return ICN::CSPANEL; }else
    if(id == "CSTLBARB"){ return ICN::CSTLBARB; }else
    if(id == "CSTLCAPB"){ return ICN::CSTLCAPB; }else
    if(id == "CSTLCAPK"){ return ICN::CSTLCAPK; }else
    if(id == "CSTLCAPN"){ return ICN::CSTLCAPN; }else
    if(id == "CSTLCAPS"){ return ICN::CSTLCAPS; }else
    if(id == "CSTLCAPW"){ return ICN::CSTLCAPW; }else
    if(id == "CSTLCAPZ"){ return ICN::CSTLCAPZ; }else
    if(id == "CSTLKNGT"){ return ICN::CSTLKNGT; }else
    if(id == "CSTLNECR"){ return ICN::CSTLNECR; }else
    if(id == "CSTLSORC"){ return ICN::CSTLSORC; }else
    if(id == "CSTLWRLK"){ return ICN::CSTLWRLK; }else
    if(id == "CSTLWZRD"){ return ICN::CSTLWZRD; }else
    if(id == "CTRACK00"){ return ICN::CTRACK00; }else
    if(id == "CTRACK01"){ return ICN::CTRACK01; }else
    if(id == "CTRACK02"){ return ICN::CTRACK02; }else
    if(id == "CTRACK03"){ return ICN::CTRACK03; }else
    if(id == "CTRACK04"){ return ICN::CTRACK04; }else
    if(id == "CTRACK05"){ return ICN::CTRACK05; }else
    if(id == "CTRACK06"){ return ICN::CTRACK06; }else
    if(id == "CURSE"){ return ICN::CURSE; }else
    if(id == "CYCLOPS"){ return ICN::CYCLOPS; }else
    if(id == "DISRRAY"){ return ICN::DISRRAY; }else
    if(id == "DRAGBLAK"){ return ICN::DRAGBLAK; }else
    if(id == "DRAGBONE"){ return ICN::DRAGBONE; }else
    if(id == "DRAGGREE"){ return ICN::DRAGGREE; }else
    if(id == "DRAGRED"){ return ICN::DRAGRED; }else
    if(id == "DRAGSLAY"){ return ICN::DRAGSLAY; }else
    if(id == "DROPLISL"){ return ICN::DROPLISL; }else
    if(id == "DROPLIST"){ return ICN::DROPLIST; }else
    if(id == "DRUID2"){ return ICN::DRUID2; }else
    if(id == "DRUID"){ return ICN::DRUID; }else
    if(id == "DRUIDMSL"){ return ICN::DRUIDMSL; }else
    if(id == "DUMMY"){ return ICN::DUMMY; }else
    if(id == "DWARF2"){ return ICN::DWARF2; }else
    if(id == "DWARF"){ return ICN::DWARF; }else
    if(id == "ECPANEL"){ return ICN::ECPANEL; }else
    if(id == "EDITBTNS"){ return ICN::EDITBTNS; }else
    if(id == "EDITOR"){ return ICN::EDITOR; }else
    if(id == "EDITPANL"){ return ICN::EDITPANL; }else
    if(id == "EELEM"){ return ICN::EELEM; }else
    if(id == "ELECTRIC"){ return ICN::ELECTRIC; }else
    if(id == "ELF2"){ return ICN::ELF2; }else
    if(id == "ELF"){ return ICN::ELF; }else
    if(id == "ELF__MSL"){ return ICN::ELF__MSL; }else
    if(id == "ESCROLL"){ return ICN::ESCROLL; }else
    if(id == "ESPANBKG"){ return ICN::ESPANBKG; }else
    if(id == "ESPANBTN"){ return ICN::ESPANBTN; }else
    if(id == "ESPANEL"){ return ICN::ESPANEL; }else
    if(id == "EVIW_ALL"){ return ICN::EVIW_ALL; }else
    if(id == "EVIWDDOR"){ return ICN::EVIWDDOR; }else
    if(id == "EVIWHROS"){ return ICN::EVIWHROS; }else
    if(id == "EVIWMINE"){ return ICN::EVIWMINE; }else
    if(id == "EVIWPUZL"){ return ICN::EVIWPUZL; }else
    if(id == "EVIWRSRC"){ return ICN::EVIWRSRC; }else
    if(id == "EVIWRTFX"){ return ICN::EVIWRTFX; }else
    if(id == "EVIWTWNS"){ return ICN::EVIWTWNS; }else
    if(id == "EVIWWRLD"){ return ICN::EVIWWRLD; }else
    if(id == "EXPMRL"){ return ICN::EXPMRL; }else
    if(id == "EXTRAOVR"){ return ICN::EXTRAOVR; }else
    if(id == "FELEM"){ return ICN::FELEM; }else
    if(id == "FIREBAL2"){ return ICN::FIREBAL2; }else
    if(id == "FIREBALL"){ return ICN::FIREBALL; }else
    if(id == "FLAG32"){ return ICN::FLAG32; }else
    if(id == "FONT"){ return ICN::FONT; }else
    if(id == "FRNG0001"){ return ICN::FRNG0001; }else
    if(id == "FRNG0002"){ return ICN::FRNG0002; }else
    if(id == "FRNG0003"){ return ICN::FRNG0003; }else
    if(id == "FRNG0004"){ return ICN::FRNG0004; }else
    if(id == "FRNG0005"){ return ICN::FRNG0005; }else
    if(id == "FRNG0006"){ return ICN::FRNG0006; }else
    if(id == "FRNG0007"){ return ICN::FRNG0007; }else
    if(id == "FRNG0008"){ return ICN::FRNG0008; }else
    if(id == "FRNG0009"){ return ICN::FRNG0009; }else
    if(id == "FRNG0010"){ return ICN::FRNG0010; }else
    if(id == "FRNG0011"){ return ICN::FRNG0011; }else
    if(id == "FRNG0012"){ return ICN::FRNG0012; }else
    if(id == "FRNG0013"){ return ICN::FRNG0013; }else
    if(id == "FROTH"){ return ICN::FROTH; }else
    if(id == "GARGOYLE"){ return ICN::GARGOYLE; }else
    if(id == "G_BFLG32"){ return ICN::G_BFLG32; }else
    if(id == "GENIE"){ return ICN::GENIE; }else
    if(id == "G_FLAG32"){ return ICN::G_FLAG32; }else
    if(id == "GHOST"){ return ICN::GHOST; }else
    if(id == "GOBLIN"){ return ICN::GOBLIN; }else
    if(id == "GOLEM2"){ return ICN::GOLEM2; }else
    if(id == "GOLEM"){ return ICN::GOLEM; }else
    if(id == "GRIFFIN"){ return ICN::GRIFFIN; }else
    if(id == "GROUND12"){ return ICN::GROUND12; }else
    if(id == "GROUND4"){ return ICN::GROUND4; }else
    if(id == "GROUND6"){ return ICN::GROUND6; }else
    if(id == "HALFLING"){ return ICN::HALFLING; }else
    if(id == "HALFLMSL"){ return ICN::HALFLMSL; }else
    if(id == "HASTE"){ return ICN::HASTE; }else
    if(id == "HEROBKG"){ return ICN::HEROBKG; }else
    if(id == "HEROES"){ return ICN::HEROES; }else
    if(id == "HEROEXTE"){ return ICN::HEROEXTE; }else
    if(id == "HEROEXTG"){ return ICN::HEROEXTG; }else
    if(id == "HEROFL00"){ return ICN::HEROFL00; }else
    if(id == "HEROFL01"){ return ICN::HEROFL01; }else
    if(id == "HEROFL02"){ return ICN::HEROFL02; }else
    if(id == "HEROFL03"){ return ICN::HEROFL03; }else
    if(id == "HEROFL04"){ return ICN::HEROFL04; }else
    if(id == "HEROFL05"){ return ICN::HEROFL05; }else
    if(id == "HEROFL06"){ return ICN::HEROFL06; }else
    if(id == "HEROLOGE"){ return ICN::HEROLOGE; }else
    if(id == "HEROLOGO"){ return ICN::HEROLOGO; }else
    if(id == "HISCORE"){ return ICN::HISCORE; }else
    if(id == "HOURGLAS"){ return ICN::HOURGLAS; }else
    if(id == "HSBKG"){ return ICN::HSBKG; }else
    if(id == "HSBTNS"){ return ICN::HSBTNS; }else
    if(id == "HSICONS"){ return ICN::HSICONS; }else
    if(id == "HYDRA"){ return ICN::HYDRA; }else
    if(id == "HYPNOTIZ"){ return ICN::HYPNOTIZ; }else
    if(id == "ICECLOUD"){ return ICN::ICECLOUD; }else
    if(id == "KEEP"){ return ICN::KEEP; }else
    if(id == "KNGT32"){ return ICN::KNGT32; }else
    if(id == "LETTER12"){ return ICN::LETTER12; }else
    if(id == "LETTER4"){ return ICN::LETTER4; }else
    if(id == "LETTER6"){ return ICN::LETTER6; }else
    if(id == "LGNDXTRA"){ return ICN::LGNDXTRA; }else
    if(id == "LGNDXTRE"){ return ICN::LGNDXTRE; }else
    if(id == "LICH2"){ return ICN::LICH2; }else
    if(id == "LICHCLOD"){ return ICN::LICHCLOD; }else
    if(id == "LICH"){ return ICN::LICH; }else
    if(id == "LICH_MSL"){ return ICN::LICH_MSL; }else
    if(id == "LISTBOX"){ return ICN::LISTBOX; }else
    if(id == "LISTBOXS"){ return ICN::LISTBOXS; }else
    if(id == "LOCATORE"){ return ICN::LOCATORE; }else
    if(id == "LOCATORS"){ return ICN::LOCATORS; }else
    if(id == "MAGE1"){ return ICN::MAGE1; }else
    if(id == "MAGE2"){ return ICN::MAGE2; }else
    if(id == "MAGEGLDB"){ return ICN::MAGEGLDB; }else
    if(id == "MAGEGLDK"){ return ICN::MAGEGLDK; }else
    if(id == "MAGEGLDN"){ return ICN::MAGEGLDN; }else
    if(id == "MAGEGLDS"){ return ICN::MAGEGLDS; }else
    if(id == "MAGEGLDW"){ return ICN::MAGEGLDW; }else
    if(id == "MAGEGLDZ"){ return ICN::MAGEGLDZ; }else
    if(id == "MAGIC01"){ return ICN::MAGIC01; }else
    if(id == "MAGIC02"){ return ICN::MAGIC02; }else
    if(id == "MAGIC03"){ return ICN::MAGIC03; }else
    if(id == "MAGIC04"){ return ICN::MAGIC04; }else
    if(id == "MAGIC06"){ return ICN::MAGIC06; }else
    if(id == "MAGIC07"){ return ICN::MAGIC07; }else
    if(id == "MAGIC08"){ return ICN::MAGIC08; }else
    if(id == "MANA"){ return ICN::MANA; }else
    if(id == "MEDUSA"){ return ICN::MEDUSA; }else
    if(id == "METEOR"){ return ICN::METEOR; }else
    if(id == "MINICAPT"){ return ICN::MINICAPT; }else
    if(id == "MINIHERO"){ return ICN::MINIHERO; }else
    if(id == "MINILKMR"){ return ICN::MINILKMR; }else
    if(id == "MINIMON"){ return ICN::MINIMON; }else
    if(id == "MINIPORT"){ return ICN::MINIPORT; }else
    if(id == "MINISS"){ return ICN::MINISS; }else
    if(id == "MINITOWN"){ return ICN::MINITOWN; }else
    if(id == "MINOTAU2"){ return ICN::MINOTAU2; }else
    if(id == "MINOTAUR"){ return ICN::MINOTAUR; }else
    if(id == "MISC12"){ return ICN::MISC12; }else
    if(id == "MISC4"){ return ICN::MISC4; }else
    if(id == "MISC6"){ return ICN::MISC6; }else
    if(id == "MOATPART"){ return ICN::MOATPART; }else
    if(id == "MOATWHOL"){ return ICN::MOATWHOL; }else
    if(id == "MOBILITY"){ return ICN::MOBILITY; }else
    if(id == "MONH0000"){ return ICN::MONH0000; }else
    if(id == "MONH0001"){ return ICN::MONH0001; }else
    if(id == "MONH0002"){ return ICN::MONH0002; }else
    if(id == "MONH0003"){ return ICN::MONH0003; }else
    if(id == "MONH0004"){ return ICN::MONH0004; }else
    if(id == "MONH0005"){ return ICN::MONH0005; }else
    if(id == "MONH0006"){ return ICN::MONH0006; }else
    if(id == "MONH0007"){ return ICN::MONH0007; }else
    if(id == "MONH0008"){ return ICN::MONH0008; }else
    if(id == "MONH0009"){ return ICN::MONH0009; }else
    if(id == "MONH0010"){ return ICN::MONH0010; }else
    if(id == "MONH0011"){ return ICN::MONH0011; }else
    if(id == "MONH0012"){ return ICN::MONH0012; }else
    if(id == "MONH0013"){ return ICN::MONH0013; }else
    if(id == "MONH0014"){ return ICN::MONH0014; }else
    if(id == "MONH0015"){ return ICN::MONH0015; }else
    if(id == "MONH0016"){ return ICN::MONH0016; }else
    if(id == "MONH0017"){ return ICN::MONH0017; }else
    if(id == "MONH0018"){ return ICN::MONH0018; }else
    if(id == "MONH0019"){ return ICN::MONH0019; }else
    if(id == "MONH0020"){ return ICN::MONH0020; }else
    if(id == "MONH0021"){ return ICN::MONH0021; }else
    if(id == "MONH0022"){ return ICN::MONH0022; }else
    if(id == "MONH0023"){ return ICN::MONH0023; }else
    if(id == "MONH0024"){ return ICN::MONH0024; }else
    if(id == "MONH0025"){ return ICN::MONH0025; }else
    if(id == "MONH0026"){ return ICN::MONH0026; }else
    if(id == "MONH0027"){ return ICN::MONH0027; }else
    if(id == "MONH0028"){ return ICN::MONH0028; }else
    if(id == "MONH0029"){ return ICN::MONH0029; }else
    if(id == "MONH0030"){ return ICN::MONH0030; }else
    if(id == "MONH0031"){ return ICN::MONH0031; }else
    if(id == "MONH0032"){ return ICN::MONH0032; }else
    if(id == "MONH0033"){ return ICN::MONH0033; }else
    if(id == "MONH0034"){ return ICN::MONH0034; }else
    if(id == "MONH0035"){ return ICN::MONH0035; }else
    if(id == "MONH0036"){ return ICN::MONH0036; }else
    if(id == "MONH0037"){ return ICN::MONH0037; }else
    if(id == "MONH0038"){ return ICN::MONH0038; }else
    if(id == "MONH0039"){ return ICN::MONH0039; }else
    if(id == "MONH0040"){ return ICN::MONH0040; }else
    if(id == "MONH0041"){ return ICN::MONH0041; }else
    if(id == "MONH0042"){ return ICN::MONH0042; }else
    if(id == "MONH0043"){ return ICN::MONH0043; }else
    if(id == "MONH0044"){ return ICN::MONH0044; }else
    if(id == "MONH0045"){ return ICN::MONH0045; }else
    if(id == "MONH0046"){ return ICN::MONH0046; }else
    if(id == "MONH0047"){ return ICN::MONH0047; }else
    if(id == "MONH0048"){ return ICN::MONH0048; }else
    if(id == "MONH0049"){ return ICN::MONH0049; }else
    if(id == "MONH0050"){ return ICN::MONH0050; }else
    if(id == "MONH0051"){ return ICN::MONH0051; }else
    if(id == "MONH0052"){ return ICN::MONH0052; }else
    if(id == "MONH0053"){ return ICN::MONH0053; }else
    if(id == "MONH0054"){ return ICN::MONH0054; }else
    if(id == "MONH0055"){ return ICN::MONH0055; }else
    if(id == "MONH0056"){ return ICN::MONH0056; }else
    if(id == "MONH0057"){ return ICN::MONH0057; }else
    if(id == "MONH0058"){ return ICN::MONH0058; }else
    if(id == "MONH0059"){ return ICN::MONH0059; }else
    if(id == "MONH0060"){ return ICN::MONH0060; }else
    if(id == "MONH0061"){ return ICN::MONH0061; }else
    if(id == "MONH0062"){ return ICN::MONH0062; }else
    if(id == "MONH0063"){ return ICN::MONH0063; }else
    if(id == "MONH0064"){ return ICN::MONH0064; }else
    if(id == "MONH0065"){ return ICN::MONH0065; }else
    if(id == "MONS32"){ return ICN::MONS32; }else
    if(id == "MORALEB"){ return ICN::MORALEB; }else
    if(id == "MORALEG"){ return ICN::MORALEG; }else
    if(id == "MTNCRCK"){ return ICN::MTNCRCK; }else
    if(id == "MTNDIRT"){ return ICN::MTNDIRT; }else
    if(id == "MTNDSRT"){ return ICN::MTNDSRT; }else
    if(id == "MTNGRAS"){ return ICN::MTNGRAS; }else
    if(id == "MTNLAVA"){ return ICN::MTNLAVA; }else
    if(id == "MTNMULT"){ return ICN::MTNMULT; }else
    if(id == "MTNSNOW"){ return ICN::MTNSNOW; }else
    if(id == "MTNSWMP"){ return ICN::MTNSWMP; }else
    if(id == "MUMMY2"){ return ICN::MUMMY2; }else
    if(id == "MUMMYW"){ return ICN::MUMMYW; }else
    if(id == "NECR32"){ return ICN::NECR32; }else
    if(id == "NETBOX"){ return ICN::NETBOX; }else
    if(id == "NGEXTRA"){ return ICN::NGEXTRA; }else
    if(id == "NGHSBKG"){ return ICN::NGHSBKG; }else
    if(id == "NGMPBKG"){ return ICN::NGMPBKG; }else
    if(id == "NGSPBKG"){ return ICN::NGSPBKG; }else
    if(id == "NOMAD"){ return ICN::NOMAD; }else
    if(id == "O_BFLG32"){ return ICN::O_BFLG32; }else
    if(id == "OBJNARTI"){ return ICN::OBJNARTI; }else
    if(id == "OBJNCRCK"){ return ICN::OBJNCRCK; }else
    if(id == "OBJNDIRT"){ return ICN::OBJNDIRT; }else
    if(id == "OBJNDSRT"){ return ICN::OBJNDSRT; }else
    if(id == "OBJNGRA2"){ return ICN::OBJNGRA2; }else
    if(id == "OBJNGRAS"){ return ICN::OBJNGRAS; }else
    if(id == "OBJNHAUN"){ return ICN::OBJNHAUN; }else
    if(id == "OBJNLAV2"){ return ICN::OBJNLAV2; }else
    if(id == "OBJNLAV3"){ return ICN::OBJNLAV3; }else
    if(id == "OBJNLAVA"){ return ICN::OBJNLAVA; }else
    if(id == "OBJNMUL2"){ return ICN::OBJNMUL2; }else
    if(id == "OBJNMULT"){ return ICN::OBJNMULT; }else
    if(id == "OBJNRSRC"){ return ICN::OBJNRSRC; }else
    if(id == "OBJNSNOW"){ return ICN::OBJNSNOW; }else
    if(id == "OBJNSWMP"){ return ICN::OBJNSWMP; }else
    if(id == "OBJNTOWN"){ return ICN::OBJNTOWN; }else
    if(id == "OBJNTWBA"){ return ICN::OBJNTWBA; }else
    if(id == "OBJNTWRD"){ return ICN::OBJNTWRD; }else
    if(id == "OBJNTWSH"){ return ICN::OBJNTWSH; }else
    if(id == "OBJNWAT2"){ return ICN::OBJNWAT2; }else
    if(id == "OBJNWATR"){ return ICN::OBJNWATR; }else
    if(id == "OBJNXTRA"){ return ICN::OBJNXTRA; }else
    if(id == "OBJPALET"){ return ICN::OBJPALET; }else
    if(id == "O_FLAG32"){ return ICN::O_FLAG32; }else
    if(id == "OGRE2"){ return ICN::OGRE2; }else
    if(id == "OGRE"){ return ICN::OGRE; }else
    if(id == "ORC2"){ return ICN::ORC2; }else
    if(id == "ORC"){ return ICN::ORC; }else
    if(id == "ORC__MSL"){ return ICN::ORC__MSL; }else
    if(id == "OVERBACK"){ return ICN::OVERBACK; }else
    if(id == "OVERLAY"){ return ICN::OVERLAY; }else
    if(id == "OVERVIEW"){ return ICN::OVERVIEW; }else
    if(id == "PALADIN2"){ return ICN::PALADIN2; }else
    if(id == "PALADIN"){ return ICN::PALADIN; }else
    if(id == "PARALYZE"){ return ICN::PARALYZE; }else
    if(id == "P_BFLG32"){ return ICN::P_BFLG32; }else
    if(id == "PEASANT"){ return ICN::PEASANT; }else
    if(id == "P_FLAG32"){ return ICN::P_FLAG32; }else
    if(id == "PHOENIX"){ return ICN::PHOENIX; }else
    if(id == "PHYSICAL"){ return ICN::PHYSICAL; }else
    if(id == "PIKEMAN2"){ return ICN::PIKEMAN2; }else
    if(id == "PIKEMAN"){ return ICN::PIKEMAN; }else
    if(id == "PORT0000"){ return ICN::PORT0000; }else
    if(id == "PORT0001"){ return ICN::PORT0001; }else
    if(id == "PORT0002"){ return ICN::PORT0002; }else
    if(id == "PORT0003"){ return ICN::PORT0003; }else
    if(id == "PORT0004"){ return ICN::PORT0004; }else
    if(id == "PORT0005"){ return ICN::PORT0005; }else
    if(id == "PORT0006"){ return ICN::PORT0006; }else
    if(id == "PORT0007"){ return ICN::PORT0007; }else
    if(id == "PORT0008"){ return ICN::PORT0008; }else
    if(id == "PORT0009"){ return ICN::PORT0009; }else
    if(id == "PORT0010"){ return ICN::PORT0010; }else
    if(id == "PORT0011"){ return ICN::PORT0011; }else
    if(id == "PORT0012"){ return ICN::PORT0012; }else
    if(id == "PORT0013"){ return ICN::PORT0013; }else
    if(id == "PORT0014"){ return ICN::PORT0014; }else
    if(id == "PORT0015"){ return ICN::PORT0015; }else
    if(id == "PORT0016"){ return ICN::PORT0016; }else
    if(id == "PORT0017"){ return ICN::PORT0017; }else
    if(id == "PORT0018"){ return ICN::PORT0018; }else
    if(id == "PORT0019"){ return ICN::PORT0019; }else
    if(id == "PORT0020"){ return ICN::PORT0020; }else
    if(id == "PORT0021"){ return ICN::PORT0021; }else
    if(id == "PORT0022"){ return ICN::PORT0022; }else
    if(id == "PORT0023"){ return ICN::PORT0023; }else
    if(id == "PORT0024"){ return ICN::PORT0024; }else
    if(id == "PORT0025"){ return ICN::PORT0025; }else
    if(id == "PORT0026"){ return ICN::PORT0026; }else
    if(id == "PORT0027"){ return ICN::PORT0027; }else
    if(id == "PORT0028"){ return ICN::PORT0028; }else
    if(id == "PORT0029"){ return ICN::PORT0029; }else
    if(id == "PORT0030"){ return ICN::PORT0030; }else
    if(id == "PORT0031"){ return ICN::PORT0031; }else
    if(id == "PORT0032"){ return ICN::PORT0032; }else
    if(id == "PORT0033"){ return ICN::PORT0033; }else
    if(id == "PORT0034"){ return ICN::PORT0034; }else
    if(id == "PORT0035"){ return ICN::PORT0035; }else
    if(id == "PORT0036"){ return ICN::PORT0036; }else
    if(id == "PORT0037"){ return ICN::PORT0037; }else
    if(id == "PORT0038"){ return ICN::PORT0038; }else
    if(id == "PORT0039"){ return ICN::PORT0039; }else
    if(id == "PORT0040"){ return ICN::PORT0040; }else
    if(id == "PORT0041"){ return ICN::PORT0041; }else
    if(id == "PORT0042"){ return ICN::PORT0042; }else
    if(id == "PORT0043"){ return ICN::PORT0043; }else
    if(id == "PORT0044"){ return ICN::PORT0044; }else
    if(id == "PORT0045"){ return ICN::PORT0045; }else
    if(id == "PORT0046"){ return ICN::PORT0046; }else
    if(id == "PORT0047"){ return ICN::PORT0047; }else
    if(id == "PORT0048"){ return ICN::PORT0048; }else
    if(id == "PORT0049"){ return ICN::PORT0049; }else
    if(id == "PORT0050"){ return ICN::PORT0050; }else
    if(id == "PORT0051"){ return ICN::PORT0051; }else
    if(id == "PORT0052"){ return ICN::PORT0052; }else
    if(id == "PORT0053"){ return ICN::PORT0053; }else
    if(id == "PORT0054"){ return ICN::PORT0054; }else
    if(id == "PORT0055"){ return ICN::PORT0055; }else
    if(id == "PORT0056"){ return ICN::PORT0056; }else
    if(id == "PORT0057"){ return ICN::PORT0057; }else
    if(id == "PORT0058"){ return ICN::PORT0058; }else
    if(id == "PORT0059"){ return ICN::PORT0059; }else
    if(id == "PORT0060"){ return ICN::PORT0060; }else
    if(id == "PORT0061"){ return ICN::PORT0061; }else
    if(id == "PORT0062"){ return ICN::PORT0062; }else
    if(id == "PORT0063"){ return ICN::PORT0063; }else
    if(id == "PORT0064"){ return ICN::PORT0064; }else
    if(id == "PORT0065"){ return ICN::PORT0065; }else
    if(id == "PORT0066"){ return ICN::PORT0066; }else
    if(id == "PORT0067"){ return ICN::PORT0067; }else
    if(id == "PORT0068"){ return ICN::PORT0068; }else
    if(id == "PORT0069"){ return ICN::PORT0069; }else
    if(id == "PORT0070"){ return ICN::PORT0070; }else
    if(id == "PORT0090"){ return ICN::PORT0090; }else
    if(id == "PORT0091"){ return ICN::PORT0091; }else
    if(id == "PORT0092"){ return ICN::PORT0092; }else
    if(id == "PORT0093"){ return ICN::PORT0093; }else
    if(id == "PORT0094"){ return ICN::PORT0094; }else
    if(id == "PORT0095"){ return ICN::PORT0095; }else
    if(id == "PORTCFLG"){ return ICN::PORTCFLG; }else
    if(id == "PORTMEDI"){ return ICN::PORTMEDI; }else
    if(id == "PORTXTRA"){ return ICN::PORTXTRA; }else
    if(id == "PRIMSKIL"){ return ICN::PRIMSKIL; }else
    if(id == "PUZZLE"){ return ICN::PUZZLE; }else
    if(id == "QWIKHERO"){ return ICN::QWIKHERO; }else
    if(id == "QWIKINFO"){ return ICN::QWIKINFO; }else
    if(id == "QWIKTOWN"){ return ICN::QWIKTOWN; }else
    if(id == "RADAR"){ return ICN::RADAR; }else
    if(id == "R_BFLG32"){ return ICN::R_BFLG32; }else
    if(id == "RECR2BKG"){ return ICN::RECR2BKG; }else
    if(id == "RECRBKG"){ return ICN::RECRBKG; }else
    if(id == "RECRUIT"){ return ICN::RECRUIT; }else
    if(id == "REDBACK"){ return ICN::REDBACK; }else
    if(id == "REDDEATH"){ return ICN::REDDEATH; }else
    if(id == "REDFIRE"){ return ICN::REDFIRE; }else
    if(id == "REQBKG"){ return ICN::REQBKG; }else
    if(id == "REQSBKG"){ return ICN::REQSBKG; }else
    if(id == "REQUEST"){ return ICN::REQUEST; }else
    if(id == "REQUESTS"){ return ICN::REQUESTS; }else
    if(id == "RESOURCE"){ return ICN::RESOURCE; }else
    if(id == "RESSMALL"){ return ICN::RESSMALL; }else
    if(id == "R_FLAG32"){ return ICN::R_FLAG32; }else
    if(id == "ROAD"){ return ICN::ROAD; }else
    if(id == "ROC"){ return ICN::ROC; }else
    if(id == "ROGUE"){ return ICN::ROGUE; }else
    if(id == "ROUTE"){ return ICN::ROUTE; }else
    if(id == "SCENIBKG"){ return ICN::SCENIBKG; }else
    if(id == "SCROLL2"){ return ICN::SCROLL2; }else
    if(id == "SCROLLCN"){ return ICN::SCROLLCN; }else
    if(id == "SCROLLE"){ return ICN::SCROLLE; }else
    if(id == "SCROLL"){ return ICN::SCROLL; }else
    if(id == "SECSKILL"){ return ICN::SECSKILL; }else
    if(id == "SHADOW32"){ return ICN::SHADOW32; }else
    if(id == "SHIELD"){ return ICN::SHIELD; }else
    if(id == "SHNGANIM"){ return ICN::SHNGANIM; }else
    if(id == "SKELETON"){ return ICN::SKELETON; }else
    if(id == "SMALCLOD"){ return ICN::SMALCLOD; }else
    if(id == "SMALFONT"){ return ICN::SMALFONT; }else
    if(id == "SMALLBAR"){ return ICN::SMALLBAR; }else
    if(id == "SORC32"){ return ICN::SORC32; }else
    if(id == "SPANBKGE"){ return ICN::SPANBKGE; }else
    if(id == "SPANBKG"){ return ICN::SPANBKG; }else
    if(id == "SPANBTNE"){ return ICN::SPANBTNE; }else
    if(id == "SPANBTN"){ return ICN::SPANBTN; }else
    if(id == "SPANEL"){ return ICN::SPANEL; }else
    if(id == "SPARKS"){ return ICN::SPARKS; }else
    if(id == "SPELCO"){ return ICN::SPELCO; }else
    if(id == "SPELLINF"){ return ICN::SPELLINF; }else
    if(id == "SPELLINL"){ return ICN::SPELLINL; }else
    if(id == "SPELLS"){ return ICN::SPELLS; }else
    if(id == "SPRITE"){ return ICN::SPRITE; }else
    if(id == "STELSKIN"){ return ICN::STELSKIN; }else
    if(id == "STONBACK"){ return ICN::STONBACK; }else
    if(id == "STONBAKE"){ return ICN::STONBAKE; }else
    if(id == "STONEBAK"){ return ICN::STONEBAK; }else
    if(id == "STONEBK2"){ return ICN::STONEBK2; }else
    if(id == "STONSKIN"){ return ICN::STONSKIN; }else
    if(id == "STORM"){ return ICN::STORM; }else
    if(id == "STREAM"){ return ICN::STREAM; }else
    if(id == "STRIP"){ return ICN::STRIP; }else
    if(id == "SUNMOONE"){ return ICN::SUNMOONE; }else
    if(id == "SUNMOON"){ return ICN::SUNMOON; }else
    if(id == "SURDRBKE"){ return ICN::SURDRBKE; }else
    if(id == "SURDRBKG"){ return ICN::SURDRBKG; }else
    if(id == "SURRENDE"){ return ICN::SURRENDE; }else
    if(id == "SURRENDR"){ return ICN::SURRENDR; }else
    if(id == "SWAPBTN"){ return ICN::SWAPBTN; }else
    if(id == "SWAPWIN"){ return ICN::SWAPWIN; }else
    if(id == "SWORDSM2"){ return ICN::SWORDSM2; }else
    if(id == "SWORDSMN"){ return ICN::SWORDSMN; }else
    if(id == "SYSTEME"){ return ICN::SYSTEME; }else
    if(id == "SYSTEM"){ return ICN::SYSTEM; }else
    if(id == "TAVWIN"){ return ICN::TAVWIN; }else
    if(id == "TENT"){ return ICN::TENT; }else
    if(id == "TERRAINS"){ return ICN::TERRAINS; }else
    if(id == "TEXTBACK"){ return ICN::TEXTBACK; }else
    if(id == "TEXTBAK2"){ return ICN::TEXTBAK2; }else
    if(id == "TEXTBAR"){ return ICN::TEXTBAR; }else
    if(id == "TITANBLA"){ return ICN::TITANBLA; }else
    if(id == "TITANBLU"){ return ICN::TITANBLU; }else
    if(id == "TITANMSL"){ return ICN::TITANMSL; }else
    if(id == "TOWNBKG0"){ return ICN::TOWNBKG0; }else
    if(id == "TOWNBKG1"){ return ICN::TOWNBKG1; }else
    if(id == "TOWNBKG2"){ return ICN::TOWNBKG2; }else
    if(id == "TOWNBKG3"){ return ICN::TOWNBKG3; }else
    if(id == "TOWNBKG4"){ return ICN::TOWNBKG4; }else
    if(id == "TOWNBKG5"){ return ICN::TOWNBKG5; }else
    if(id == "TOWNFIX"){ return ICN::TOWNFIX; }else
    if(id == "TOWNNAME"){ return ICN::TOWNNAME; }else
    if(id == "TOWNWIND"){ return ICN::TOWNWIND; }else
    if(id == "TRADPOSE"){ return ICN::TRADPOSE; }else
    if(id == "TRADPOST"){ return ICN::TRADPOST; }else
    if(id == "TREASURY"){ return ICN::TREASURY; }else
    if(id == "TREDECI"){ return ICN::TREDECI; }else
    if(id == "TREEVIL"){ return ICN::TREEVIL; }else
    if(id == "TREFALL"){ return ICN::TREFALL; }else
    if(id == "TREFIR"){ return ICN::TREFIR; }else
    if(id == "TREJNGL"){ return ICN::TREJNGL; }else
    if(id == "TRESNOW"){ return ICN::TRESNOW; }else
    if(id == "TROLL2"){ return ICN::TROLL2; }else
    if(id == "TROLL"){ return ICN::TROLL; }else
    if(id == "TROLLMSL"){ return ICN::TROLLMSL; }else
    if(id == "TWNBBOAT"){ return ICN::TWNBBOAT; }else
    if(id == "TWNBCAPT"){ return ICN::TWNBCAPT; }else
    if(id == "TWNBCSTL"){ return ICN::TWNBCSTL; }else
    if(id == "TWNBDOCK"){ return ICN::TWNBDOCK; }else
    if(id == "TWNBDW_0"){ return ICN::TWNBDW_0; }else
    if(id == "TWNBDW_1"){ return ICN::TWNBDW_1; }else
    if(id == "TWNBDW_2"){ return ICN::TWNBDW_2; }else
    if(id == "TWNBDW_3"){ return ICN::TWNBDW_3; }else
    if(id == "TWNBDW_4"){ return ICN::TWNBDW_4; }else
    if(id == "TWNBDW_5"){ return ICN::TWNBDW_5; }else
    if(id == "TWNBEXT0"){ return ICN::TWNBEXT0; }else
    if(id == "TWNBEXT1"){ return ICN::TWNBEXT1; }else
    if(id == "TWNBEXT2"){ return ICN::TWNBEXT2; }else
    if(id == "TWNBEXT3"){ return ICN::TWNBEXT3; }else
    if(id == "TWNBLTUR"){ return ICN::TWNBLTUR; }else
    if(id == "TWNBMAGE"){ return ICN::TWNBMAGE; }else
    if(id == "TWNBMARK"){ return ICN::TWNBMARK; }else
    if(id == "TWNBMOAT"){ return ICN::TWNBMOAT; }else
    if(id == "TWNBRTUR"){ return ICN::TWNBRTUR; }else
    if(id == "TWNBSPEC"){ return ICN::TWNBSPEC; }else
    if(id == "TWNBSTAT"){ return ICN::TWNBSTAT; }else
    if(id == "TWNBTENT"){ return ICN::TWNBTENT; }else
    if(id == "TWNBTHIE"){ return ICN::TWNBTHIE; }else
    if(id == "TWNBTVRN"){ return ICN::TWNBTVRN; }else
    if(id == "TWNBUP_1"){ return ICN::TWNBUP_1; }else
    if(id == "TWNBUP_3"){ return ICN::TWNBUP_3; }else
    if(id == "TWNBUP_4"){ return ICN::TWNBUP_4; }else
    if(id == "TWNBWEL2"){ return ICN::TWNBWEL2; }else
    if(id == "TWNBWELL"){ return ICN::TWNBWELL; }else
    if(id == "TWNKBOAT"){ return ICN::TWNKBOAT; }else
    if(id == "TWNKCAPT"){ return ICN::TWNKCAPT; }else
    if(id == "TWNKCSTL"){ return ICN::TWNKCSTL; }else
    if(id == "TWNKDOCK"){ return ICN::TWNKDOCK; }else
    if(id == "TWNKDW_0"){ return ICN::TWNKDW_0; }else
    if(id == "TWNKDW_1"){ return ICN::TWNKDW_1; }else
    if(id == "TWNKDW_2"){ return ICN::TWNKDW_2; }else
    if(id == "TWNKDW_3"){ return ICN::TWNKDW_3; }else
    if(id == "TWNKDW_4"){ return ICN::TWNKDW_4; }else
    if(id == "TWNKDW_5"){ return ICN::TWNKDW_5; }else
    if(id == "TWNKEXT0"){ return ICN::TWNKEXT0; }else
    if(id == "TWNKEXT1"){ return ICN::TWNKEXT1; }else
    if(id == "TWNKEXT2"){ return ICN::TWNKEXT2; }else
    if(id == "TWNKLTUR"){ return ICN::TWNKLTUR; }else
    if(id == "TWNKMAGE"){ return ICN::TWNKMAGE; }else
    if(id == "TWNKMARK"){ return ICN::TWNKMARK; }else
    if(id == "TWNKMOAT"){ return ICN::TWNKMOAT; }else
    if(id == "TWNKRTUR"){ return ICN::TWNKRTUR; }else
    if(id == "TWNKSPEC"){ return ICN::TWNKSPEC; }else
    if(id == "TWNKSTAT"){ return ICN::TWNKSTAT; }else
    if(id == "TWNKTENT"){ return ICN::TWNKTENT; }else
    if(id == "TWNKTHIE"){ return ICN::TWNKTHIE; }else
    if(id == "TWNKTVRN"){ return ICN::TWNKTVRN; }else
    if(id == "TWNKUP_1"){ return ICN::TWNKUP_1; }else
    if(id == "TWNKUP_2"){ return ICN::TWNKUP_2; }else
    if(id == "TWNKUP_3"){ return ICN::TWNKUP_3; }else
    if(id == "TWNKUP_4"){ return ICN::TWNKUP_4; }else
    if(id == "TWNKUP_5"){ return ICN::TWNKUP_5; }else
    if(id == "TWNKWEL2"){ return ICN::TWNKWEL2; }else
    if(id == "TWNKWELL"){ return ICN::TWNKWELL; }else
    if(id == "TWNNBOAT"){ return ICN::TWNNBOAT; }else
    if(id == "TWNNCAPT"){ return ICN::TWNNCAPT; }else
    if(id == "TWNNCSTL"){ return ICN::TWNNCSTL; }else
    if(id == "TWNNDOCK"){ return ICN::TWNNDOCK; }else
    if(id == "TWNNDW_0"){ return ICN::TWNNDW_0; }else
    if(id == "TWNNDW_1"){ return ICN::TWNNDW_1; }else
    if(id == "TWNNDW_2"){ return ICN::TWNNDW_2; }else
    if(id == "TWNNDW_3"){ return ICN::TWNNDW_3; }else
    if(id == "TWNNDW_4"){ return ICN::TWNNDW_4; }else
    if(id == "TWNNDW_5"){ return ICN::TWNNDW_5; }else
    if(id == "TWNNEXT0"){ return ICN::TWNNEXT0; }else
    if(id == "TWNNLTUR"){ return ICN::TWNNLTUR; }else
    if(id == "TWNNMAGE"){ return ICN::TWNNMAGE; }else
    if(id == "TWNNMARK"){ return ICN::TWNNMARK; }else
    if(id == "TWNNMOAT"){ return ICN::TWNNMOAT; }else
    if(id == "TWNNRTUR"){ return ICN::TWNNRTUR; }else
    if(id == "TWNNSPEC"){ return ICN::TWNNSPEC; }else
    if(id == "TWNNSTAT"){ return ICN::TWNNSTAT; }else
    if(id == "TWNNTENT"){ return ICN::TWNNTENT; }else
    if(id == "TWNNTHIE"){ return ICN::TWNNTHIE; }else
    if(id == "TWNNTVRN"){ return ICN::TWNNTVRN; }else
    if(id == "TWNNUP_1"){ return ICN::TWNNUP_1; }else
    if(id == "TWNNUP_2"){ return ICN::TWNNUP_2; }else
    if(id == "TWNNUP_3"){ return ICN::TWNNUP_3; }else
    if(id == "TWNNUP_4"){ return ICN::TWNNUP_4; }else
    if(id == "TWNNWEL2"){ return ICN::TWNNWEL2; }else
    if(id == "TWNNWELL"){ return ICN::TWNNWELL; }else
    if(id == "TWNSBOAT"){ return ICN::TWNSBOAT; }else
    if(id == "TWNSCAPT"){ return ICN::TWNSCAPT; }else
    if(id == "TWNSCSTL"){ return ICN::TWNSCSTL; }else
    if(id == "TWNSDOCK"){ return ICN::TWNSDOCK; }else
    if(id == "TWNSDW_0"){ return ICN::TWNSDW_0; }else
    if(id == "TWNSDW_1"){ return ICN::TWNSDW_1; }else
    if(id == "TWNSDW_2"){ return ICN::TWNSDW_2; }else
    if(id == "TWNSDW_3"){ return ICN::TWNSDW_3; }else
    if(id == "TWNSDW_4"){ return ICN::TWNSDW_4; }else
    if(id == "TWNSDW_5"){ return ICN::TWNSDW_5; }else
    if(id == "TWNSEXT0"){ return ICN::TWNSEXT0; }else
    if(id == "TWNSEXT1"){ return ICN::TWNSEXT1; }else
    if(id == "TWNSLTUR"){ return ICN::TWNSLTUR; }else
    if(id == "TWNSMAGE"){ return ICN::TWNSMAGE; }else
    if(id == "TWNSMARK"){ return ICN::TWNSMARK; }else
    if(id == "TWNSMOAT"){ return ICN::TWNSMOAT; }else
    if(id == "TWNSRTUR"){ return ICN::TWNSRTUR; }else
    if(id == "TWNSSPEC"){ return ICN::TWNSSPEC; }else
    if(id == "TWNSSTAT"){ return ICN::TWNSSTAT; }else
    if(id == "TWNSTENT"){ return ICN::TWNSTENT; }else
    if(id == "TWNSTHIE"){ return ICN::TWNSTHIE; }else
    if(id == "TWNSTVRN"){ return ICN::TWNSTVRN; }else
    if(id == "TWNSUP_1"){ return ICN::TWNSUP_1; }else
    if(id == "TWNSUP_2"){ return ICN::TWNSUP_2; }else
    if(id == "TWNSUP_3"){ return ICN::TWNSUP_3; }else
    if(id == "TWNSWEL2"){ return ICN::TWNSWEL2; }else
    if(id == "TWNSWELL"){ return ICN::TWNSWELL; }else
    if(id == "TWNWBOAT"){ return ICN::TWNWBOAT; }else
    if(id == "TWNWCAPT"){ return ICN::TWNWCAPT; }else
    if(id == "TWNWCSTL"){ return ICN::TWNWCSTL; }else
    if(id == "TWNWDOCK"){ return ICN::TWNWDOCK; }else
    if(id == "TWNWDW_0"){ return ICN::TWNWDW_0; }else
    if(id == "TWNWDW_1"){ return ICN::TWNWDW_1; }else
    if(id == "TWNWDW_2"){ return ICN::TWNWDW_2; }else
    if(id == "TWNWDW_3"){ return ICN::TWNWDW_3; }else
    if(id == "TWNWDW_4"){ return ICN::TWNWDW_4; }else
    if(id == "TWNWDW_5"){ return ICN::TWNWDW_5; }else
    if(id == "TWNWEXT0"){ return ICN::TWNWEXT0; }else
    if(id == "TWNWLTUR"){ return ICN::TWNWLTUR; }else
    if(id == "TWNWMAGE"){ return ICN::TWNWMAGE; }else
    if(id == "TWNWMARK"){ return ICN::TWNWMARK; }else
    if(id == "TWNWMOAT"){ return ICN::TWNWMOAT; }else
    if(id == "TWNWRTUR"){ return ICN::TWNWRTUR; }else
    if(id == "TWNWSPEC"){ return ICN::TWNWSPEC; }else
    if(id == "TWNWSTAT"){ return ICN::TWNWSTAT; }else
    if(id == "TWNWTENT"){ return ICN::TWNWTENT; }else
    if(id == "TWNWTHIE"){ return ICN::TWNWTHIE; }else
    if(id == "TWNWTVRN"){ return ICN::TWNWTVRN; }else
    if(id == "TWNWUP_3"){ return ICN::TWNWUP_3; }else
    if(id == "TWNWUP5B"){ return ICN::TWNWUP5B; }else
    if(id == "TWNWUP_5"){ return ICN::TWNWUP_5; }else
    if(id == "TWNWWEL2"){ return ICN::TWNWWEL2; }else
    if(id == "TWNWWELL"){ return ICN::TWNWWELL; }else
    if(id == "TWNZBOAT"){ return ICN::TWNZBOAT; }else
    if(id == "TWNZCAPT"){ return ICN::TWNZCAPT; }else
    if(id == "TWNZCSTL"){ return ICN::TWNZCSTL; }else
    if(id == "TWNZDOCK"){ return ICN::TWNZDOCK; }else
    if(id == "TWNZDW_0"){ return ICN::TWNZDW_0; }else
    if(id == "TWNZDW_1"){ return ICN::TWNZDW_1; }else
    if(id == "TWNZDW_2"){ return ICN::TWNZDW_2; }else
    if(id == "TWNZDW_3"){ return ICN::TWNZDW_3; }else
    if(id == "TWNZDW_4"){ return ICN::TWNZDW_4; }else
    if(id == "TWNZDW_5"){ return ICN::TWNZDW_5; }else
    if(id == "TWNZEXT0"){ return ICN::TWNZEXT0; }else
    if(id == "TWNZLTUR"){ return ICN::TWNZLTUR; }else
    if(id == "TWNZMAGE"){ return ICN::TWNZMAGE; }else
    if(id == "TWNZMARK"){ return ICN::TWNZMARK; }else
    if(id == "TWNZMOAT"){ return ICN::TWNZMOAT; }else
    if(id == "TWNZRTUR"){ return ICN::TWNZRTUR; }else
    if(id == "TWNZSPEC"){ return ICN::TWNZSPEC; }else
    if(id == "TWNZSTAT"){ return ICN::TWNZSTAT; }else
    if(id == "TWNZTENT"){ return ICN::TWNZTENT; }else
    if(id == "TWNZTHIE"){ return ICN::TWNZTHIE; }else
    if(id == "TWNZTVRN"){ return ICN::TWNZTVRN; }else
    if(id == "TWNZUP_2"){ return ICN::TWNZUP_2; }else
    if(id == "TWNZUP_4"){ return ICN::TWNZUP_4; }else
    if(id == "TWNZUP_5"){ return ICN::TWNZUP_5; }else
    if(id == "TWNZWEL2"){ return ICN::TWNZWEL2; }else
    if(id == "TWNZWELL"){ return ICN::TWNZWELL; }else
    if(id == "UNICORN"){ return ICN::UNICORN; }else
    if(id == "VAMPIRE2"){ return ICN::VAMPIRE2; }else
    if(id == "VAMPIRE"){ return ICN::VAMPIRE; }else
    if(id == "VGENBKGE"){ return ICN::VGENBKGE; }else
    if(id == "VGENBKG"){ return ICN::VGENBKG; }else
    if(id == "VIEW_ALL"){ return ICN::VIEW_ALL; }else
    if(id == "VIEWARME"){ return ICN::VIEWARME; }else
    if(id == "VIEWARMY"){ return ICN::VIEWARMY; }else
    if(id == "VIEWARSM"){ return ICN::VIEWARSM; }else
    if(id == "VIEWDDOR"){ return ICN::VIEWDDOR; }else
    if(id == "VIEWGEN"){ return ICN::VIEWGEN; }else
    if(id == "VIEWHROS"){ return ICN::VIEWHROS; }else
    if(id == "VIEWMINE"){ return ICN::VIEWMINE; }else
    if(id == "VIEWPUZL"){ return ICN::VIEWPUZL; }else
    if(id == "VIEWRSRC"){ return ICN::VIEWRSRC; }else
    if(id == "VIEWRTFX"){ return ICN::VIEWRTFX; }else
    if(id == "VIEWTWNS"){ return ICN::VIEWTWNS; }else
    if(id == "VIEWWRLD"){ return ICN::VIEWWRLD; }else
    if(id == "VWFLAG12"){ return ICN::VWFLAG12; }else
    if(id == "VWFLAG4"){ return ICN::VWFLAG4; }else
    if(id == "VWFLAG6"){ return ICN::VWFLAG6; }else
    if(id == "WELEM"){ return ICN::WELEM; }else
    if(id == "WELLBKG"){ return ICN::WELLBKG; }else
    if(id == "WELLXTRA"){ return ICN::WELLXTRA; }else
    if(id == "WINCMBBE"){ return ICN::WINCMBBE; }else
    if(id == "WINCMBTB"){ return ICN::WINCMBTB; }else
    if(id == "WINCMBT"){ return ICN::WINCMBT; }else
    if(id == "WINLOSEB"){ return ICN::WINLOSEB; }else
    if(id == "WINLOSEE"){ return ICN::WINLOSEE; }else
    if(id == "WINLOSE"){ return ICN::WINLOSE; }else
    if(id == "WOLF"){ return ICN::WOLF; }else
    if(id == "WRLK32"){ return ICN::WRLK32; }else
    if(id == "WZRD32"){ return ICN::WZRD32; }else
    if(id == "X_IVY"){ return ICN::X_IVY; }else
    if(id == "X_LOADCM"){ return ICN::X_LOADCM; }else
    if(id == "X_CMPBKG"){ return ICN::X_CMPBKG; }else
    if(id == "X_CMPBTN"){ return ICN::X_CMPBTN; }else
    if(id == "X_CMPEXT"){ return ICN::X_CMPEXT; }else
    if(id == "X_TRACK1"){ return ICN::X_TRACK1; }else
    if(id == "X_TRACK2"){ return ICN::X_TRACK2; }else
    if(id == "X_TRACK3"){ return ICN::X_TRACK3; }else
    if(id == "X_TRACK4"){ return ICN::X_TRACK4; }else
    if(id == "X_LOC1"){ return ICN::X_LOC1; }else
    if(id == "X_LOC2"){ return ICN::X_LOC2; }else
    if(id == "X_LOC3"){ return ICN::X_LOC3; }else
    if(id == "XPRIMARY"){ return ICN::XPRIMARY; }else
    if(id == "Y_BFLG32"){ return ICN::Y_BFLG32; }else
    if(id == "Y_FLAG32"){ return ICN::Y_FLAG32; }else
    if(id == "YINYANG"){ return ICN::YINYANG; }else
    if(id == "ZOMBIE2"){ return ICN::ZOMBIE2; }else
    if(id == "ZOMBIE"){ return ICN::ZOMBIE; }else
    if(id == "LAST_VALID_FILE_ICN"){ return ICN::LAST_VALID_FILE_ICN; }else
    if(id == "ROUTERED"){ return ICN::ROUTERED; }else
    if(id == "YELLOW_FONT"){ return ICN::YELLOW_FONT; }else
    if(id == "YELLOW_SMALLFONT"){ return ICN::YELLOW_SMALLFONT; }else
    if(id == "BUYMAX"){ return ICN::BUYMAX; }else
    if(id == "BTNBATTLEONLY"){ return ICN::BTNBATTLEONLY; }else
    if(id == "BTNGIFT_GOOD"){ return ICN::BTNGIFT_GOOD; }else
    if(id == "BTNGIFT_EVIL"){ return ICN::BTNGIFT_EVIL; }else
    if(id == "CSLMARKER"){ return ICN::CSLMARKER; }else
    if(id == "GRAY_FONT"){ return ICN::GRAY_FONT; }else
    if(id == "GRAY_SMALL_FONT"){ return ICN::GRAY_SMALL_FONT; }else
    if(id == "TROLL2MSL"){ return ICN::TROLL2MSL; }else
    if(id == "MONSTER_SWITCH_LEFT_ARROW"){ return ICN::MONSTER_SWITCH_LEFT_ARROW; }else
    if(id == "MONSTER_SWITCH_RIGHT_ARROW"){ return ICN::MONSTER_SWITCH_RIGHT_ARROW; }else
    if(id == "NON_UNIFORM_GOOD_RESTART_BUTTON"){ return ICN::NON_UNIFORM_GOOD_RESTART_BUTTON; }else
    if(id == "NON_UNIFORM_EVIL_RESTART_BUTTON"){ return ICN::NON_UNIFORM_EVIL_RESTART_BUTTON; }else
    if(id == "UNIFORM_GOOD_MAX_BUTTON"){ return ICN::UNIFORM_GOOD_MAX_BUTTON; }else
    if(id == "UNIFORM_GOOD_MIN_BUTTON"){ return ICN::UNIFORM_GOOD_MIN_BUTTON; }else
    if(id == "UNIFORM_EVIL_MAX_BUTTON"){ return ICN::UNIFORM_EVIL_MAX_BUTTON; }else
    if(id == "UNIFORM_EVIL_MIN_BUTTON"){ return ICN::UNIFORM_EVIL_MIN_BUTTON; }else
    if(id == "UNIFORM_GOOD_OKAY_BUTTON"){ return ICN::UNIFORM_GOOD_OKAY_BUTTON; }else
    if(id == "UNIFORM_EVIL_OKAY_BUTTON"){ return ICN::UNIFORM_EVIL_OKAY_BUTTON; }else
    if(id == "UNIFORM_GOOD_CANCEL_BUTTON"){ return ICN::UNIFORM_GOOD_CANCEL_BUTTON; }else
    if(id == "UNIFORM_EVIL_CANCEL_BUTTON"){ return ICN::UNIFORM_EVIL_CANCEL_BUTTON; }else
    if(id == "UNIFORM_GOOD_EXIT_BUTTON"){ return ICN::UNIFORM_GOOD_EXIT_BUTTON; }else
    if(id == "UNIFORM_EVIL_EXIT_BUTTON"){ return ICN::UNIFORM_EVIL_EXIT_BUTTON; }else
    if(id == "GOLDEN_GRADIENT_FONT"){ return ICN::GOLDEN_GRADIENT_FONT; }else
    if(id == "SILVER_GRADIENT_FONT"){ return ICN::SILVER_GRADIENT_FONT; }else
    if(id == "WHITE_LARGE_FONT"){ return ICN::WHITE_LARGE_FONT; }else
    if(id == "GOLDEN_GRADIENT_LARGE_FONT"){ return ICN::GOLDEN_GRADIENT_LARGE_FONT; }else
    if(id == "SILVER_GRADIENT_LARGE_FONT"){ return ICN::SILVER_GRADIENT_LARGE_FONT; }else
    if(id == "SWAP_ARROW_LEFT_TO_RIGHT"){ return ICN::SWAP_ARROW_LEFT_TO_RIGHT; }else
    if(id == "SWAP_ARROW_RIGHT_TO_LEFT"){ return ICN::SWAP_ARROW_RIGHT_TO_LEFT; }else
    if(id == "COLOR_CURSOR_ADVENTURE_MAP"){ return ICN::COLOR_CURSOR_ADVENTURE_MAP; }else
    if(id == "MONO_CURSOR_ADVENTURE_MAP"){ return ICN::MONO_CURSOR_ADVENTURE_MAP; }else
    if(id == "DISMISS_HERO_DISABLED_BUTTON"){ return ICN::DISMISS_HERO_DISABLED_BUTTON; }else
    if(id == "NEW_CAMPAIGN_DISABLED_BUTTON"){ return ICN::NEW_CAMPAIGN_DISABLED_BUTTON; }else
    if(id == "KNIGHT_CASTLE_RIGHT_FARM"){ return ICN::KNIGHT_CASTLE_RIGHT_FARM; }else
    if(id == "KNIGHT_CASTLE_LEFT_FARM"){ return ICN::KNIGHT_CASTLE_LEFT_FARM; }else
    if(id == "NECROMANCER_CASTLE_STANDALONE_CAPTAIN_QUARTERS"){ return ICN::NECROMANCER_CASTLE_STANDALONE_CAPTAIN_QUARTERS; }else
    if(id == "NECROMANCER_CASTLE_CAPTAIN_QUARTERS_BRIDGE"){ return ICN::NECROMANCER_CASTLE_CAPTAIN_QUARTERS_BRIDGE; }else
    if(id == "MAP_TYPE_ICON"){ return ICN::MAP_TYPE_ICON; }else
    if(id == "BARBARIAN_CASTLE_CAPTAIN_QUARTERS_LEFT_SIDE"){ return ICN::BARBARIAN_CASTLE_CAPTAIN_QUARTERS_LEFT_SIDE; }else
    if(id == "SORCERESS_CASTLE_CAPTAIN_QUARTERS_LEFT_SIDE"){ return ICN::SORCERESS_CASTLE_CAPTAIN_QUARTERS_LEFT_SIDE; }else
    if(id == "GOOD_ARMY_BUTTON"){ return ICN::GOOD_ARMY_BUTTON; }else
    if(id == "GOOD_MARKET_BUTTON"){ return ICN::GOOD_MARKET_BUTTON; }else
    if(id == "EVIL_ARMY_BUTTON"){ return ICN::EVIL_ARMY_BUTTON; }else
    if(id == "EVIL_MARKET_BUTTON"){ return ICN::EVIL_MARKET_BUTTON; }else
    if(id == "MONO_CURSOR_ADVMBW"){ return ICN::MONO_CURSOR_ADVMBW; }else
    if(id == "MONO_CURSOR_SPELBW"){ return ICN::MONO_CURSOR_SPELBW; }else
    if(id == "MONO_CURSOR_CMSSBW"){ return ICN::MONO_CURSOR_CMSSBW; }else
    if(id == "ESPANBKG_EVIL"){ return ICN::ESPANBKG_EVIL; }else
    if(id == "STONEBAK_EVIL"){ return ICN::STONEBAK_EVIL; }else
    if(id == "STONEBAK_SMALL_POL"){ return ICN::STONEBAK_SMALL_POL; }else
    if(id == "UNIFORMBAK_GOOD"){ return ICN::UNIFORMBAK_GOOD; }else
    if(id == "UNIFORMBAK_EVIL"){ return ICN::UNIFORMBAK_EVIL; }else
    if(id == "REDBAK_SMALL_VERTICAL"){ return ICN::REDBAK_SMALL_VERTICAL; }else
    if(id == "WELLBKG_EVIL"){ return ICN::WELLBKG_EVIL; }else
    if(id == "CASLWIND_EVIL"){ return ICN::CASLWIND_EVIL; }else
    if(id == "CASLXTRA_EVIL"){ return ICN::CASLXTRA_EVIL; }else
    if(id == "STRIP_BACKGROUND_EVIL"){ return ICN::STRIP_BACKGROUND_EVIL; }else
    if(id == "EDITBTNS_EVIL"){ return ICN::EDITBTNS_EVIL; }else
    if(id == "DROPLISL_EVIL"){ return ICN::DROPLISL_EVIL; }else
    if(id == "CELLWIN_EVIL"){ return ICN::CELLWIN_EVIL; }else
    if(id == "GOOD_CAMPAIGN_BUTTONS"){ return ICN::GOOD_CAMPAIGN_BUTTONS; }else
    if(id == "EVIL_CAMPAIGN_BUTTONS"){ return ICN::EVIL_CAMPAIGN_BUTTONS; }else
    if(id == "POL_CAMPAIGN_BUTTONS"){ return ICN::POL_CAMPAIGN_BUTTONS; }else
    if(id == "MINI_MONSTER_IMAGE"){ return ICN::MINI_MONSTER_IMAGE; }else
    if(id == "MINI_MONSTER_SHADOW"){ return ICN::MINI_MONSTER_SHADOW; }else
    if(id == "BUTTON_GOOD_FONT_RELEASED"){ return ICN::BUTTON_GOOD_FONT_RELEASED; }else
    if(id == "BUTTON_GOOD_FONT_PRESSED"){ return ICN::BUTTON_GOOD_FONT_PRESSED; }else
    if(id == "BUTTON_EVIL_FONT_RELEASED"){ return ICN::BUTTON_EVIL_FONT_RELEASED; }else
    if(id == "BUTTON_EVIL_FONT_PRESSED"){ return ICN::BUTTON_EVIL_FONT_PRESSED; }else
    if(id == "EMPTY_GOOD_BUTTON"){ return ICN::EMPTY_GOOD_BUTTON; }else
    if(id == "EMPTY_EVIL_BUTTON"){ return ICN::EMPTY_EVIL_BUTTON; }else
    if(id == "EMPTY_GOOD_MEDIUM_BUTTON"){ return ICN::EMPTY_GOOD_MEDIUM_BUTTON; }else
    if(id == "EMPTY_EVIL_MEDIUM_BUTTON"){ return ICN::EMPTY_EVIL_MEDIUM_BUTTON; }else
    if(id == "EMPTY_POL_BUTTON"){ return ICN::EMPTY_POL_BUTTON; }else
    if(id == "EMPTY_GUILDWELL_BUTTON"){ return ICN::EMPTY_GUILDWELL_BUTTON; }else
    if(id == "EMPTY_VERTICAL_GOOD_BUTTON"){ return ICN::EMPTY_VERTICAL_GOOD_BUTTON; }else
    if(id == "EMPTY_MAP_SELECT_BUTTON"){ return ICN::EMPTY_MAP_SELECT_BUTTON; }else
    if(id == "BUTTON_STANDARD_GAME"){ return ICN::BUTTON_STANDARD_GAME; }else
    if(id == "BUTTON_CAMPAIGN_GAME"){ return ICN::BUTTON_CAMPAIGN_GAME; }else
    if(id == "BUTTON_MULTIPLAYER_GAME"){ return ICN::BUTTON_MULTIPLAYER_GAME; }else
    if(id == "BUTTON_LARGE_CANCEL"){ return ICN::BUTTON_LARGE_CANCEL; }else
    if(id == "BUTTON_LARGE_CONFIG"){ return ICN::BUTTON_LARGE_CONFIG; }else
    if(id == "BUTTON_ORIGINAL_CAMPAIGN"){ return ICN::BUTTON_ORIGINAL_CAMPAIGN; }else
    if(id == "BUTTON_EXPANSION_CAMPAIGN"){ return ICN::BUTTON_EXPANSION_CAMPAIGN; }else
    if(id == "BUTTON_HOT_SEAT"){ return ICN::BUTTON_HOT_SEAT; }else
    if(id == "BUTTON_2_PLAYERS"){ return ICN::BUTTON_2_PLAYERS; }else
    if(id == "BUTTON_3_PLAYERS"){ return ICN::BUTTON_3_PLAYERS; }else
    if(id == "BUTTON_4_PLAYERS"){ return ICN::BUTTON_4_PLAYERS; }else
    if(id == "BUTTON_5_PLAYERS"){ return ICN::BUTTON_5_PLAYERS; }else
    if(id == "BUTTON_6_PLAYERS"){ return ICN::BUTTON_6_PLAYERS; }else
    if(id == "BUTTON_NEW_GAME_GOOD"){ return ICN::BUTTON_NEW_GAME_GOOD; }else
    if(id == "BUTTON_NEW_GAME_EVIL"){ return ICN::BUTTON_NEW_GAME_EVIL; }else
    if(id == "BUTTON_SAVE_GAME_GOOD"){ return ICN::BUTTON_SAVE_GAME_GOOD; }else
    if(id == "BUTTON_SAVE_GAME_EVIL"){ return ICN::BUTTON_SAVE_GAME_EVIL; }else
    if(id == "BUTTON_LOAD_GAME_GOOD"){ return ICN::BUTTON_LOAD_GAME_GOOD; }else
    if(id == "BUTTON_LOAD_GAME_EVIL"){ return ICN::BUTTON_LOAD_GAME_EVIL; }else
    if(id == "BUTTON_INFO_GOOD"){ return ICN::BUTTON_INFO_GOOD; }else
    if(id == "BUTTON_INFO_EVIL"){ return ICN::BUTTON_INFO_EVIL; }else
    if(id == "BUTTON_QUIT_GOOD"){ return ICN::BUTTON_QUIT_GOOD; }else
    if(id == "BUTTON_QUIT_EVIL"){ return ICN::BUTTON_QUIT_EVIL; }else
    if(id == "BUTTON_SMALL_CANCEL_GOOD"){ return ICN::BUTTON_SMALL_CANCEL_GOOD; }else
    if(id == "BUTTON_SMALL_CANCEL_EVIL"){ return ICN::BUTTON_SMALL_CANCEL_EVIL; }else
    if(id == "BUTTON_SMALL_OKAY_GOOD"){ return ICN::BUTTON_SMALL_OKAY_GOOD; }else
    if(id == "BUTTON_SMALL_OKAY_EVIL"){ return ICN::BUTTON_SMALL_OKAY_EVIL; }else
    if(id == "BUTTON_SMALLER_OKAY_GOOD"){ return ICN::BUTTON_SMALLER_OKAY_GOOD; }else
    if(id == "BUTTON_SMALLER_OKAY_EVIL"){ return ICN::BUTTON_SMALLER_OKAY_EVIL; }else
    if(id == "BUTTON_SMALL_ACCEPT_GOOD"){ return ICN::BUTTON_SMALL_ACCEPT_GOOD; }else
    if(id == "BUTTON_SMALL_ACCEPT_EVIL"){ return ICN::BUTTON_SMALL_ACCEPT_EVIL; }else
    if(id == "BUTTON_SMALL_DECLINE_GOOD"){ return ICN::BUTTON_SMALL_DECLINE_GOOD; }else
    if(id == "BUTTON_SMALL_DECLINE_EVIL"){ return ICN::BUTTON_SMALL_DECLINE_EVIL; }else
    if(id == "BUTTON_SMALL_LEARN_GOOD"){ return ICN::BUTTON_SMALL_LEARN_GOOD; }else
    if(id == "BUTTON_SMALL_LEARN_EVIL"){ return ICN::BUTTON_SMALL_LEARN_EVIL; }else
    if(id == "BUTTON_SMALL_TRADE_GOOD"){ return ICN::BUTTON_SMALL_TRADE_GOOD; }else
    if(id == "BUTTON_SMALL_TRADE_EVIL"){ return ICN::BUTTON_SMALL_TRADE_EVIL; }else
    if(id == "BUTTON_SMALL_YES_GOOD"){ return ICN::BUTTON_SMALL_YES_GOOD; }else
    if(id == "BUTTON_SMALL_YES_EVIL"){ return ICN::BUTTON_SMALL_YES_EVIL; }else
    if(id == "BUTTON_SMALL_NO_GOOD"){ return ICN::BUTTON_SMALL_NO_GOOD; }else
    if(id == "BUTTON_SMALL_NO_EVIL"){ return ICN::BUTTON_SMALL_NO_EVIL; }else
    if(id == "BUTTON_SMALL_EXIT_GOOD"){ return ICN::BUTTON_SMALL_EXIT_GOOD; }else
    if(id == "BUTTON_SMALL_EXIT_EVIL"){ return ICN::BUTTON_SMALL_EXIT_EVIL; }else
    if(id == "BUTTON_EXIT_HEROES_MEETING"){ return ICN::BUTTON_EXIT_HEROES_MEETING; }else
    if(id == "BUTTON_EXIT_TOWN"){ return ICN::BUTTON_EXIT_TOWN; }else
    if(id == "BUTTON_EXIT_PUZZLE_DIM_DOOR_GOOD"){ return ICN::BUTTON_EXIT_PUZZLE_DIM_DOOR_GOOD; }else
    if(id == "BUTTON_EXIT_PUZZLE_DIM_DOOR_EVIL"){ return ICN::BUTTON_EXIT_PUZZLE_DIM_DOOR_EVIL; }else
    if(id == "BUTTON_SMALL_DISMISS_GOOD"){ return ICN::BUTTON_SMALL_DISMISS_GOOD; }else
    if(id == "BUTTON_SMALL_DISMISS_EVIL"){ return ICN::BUTTON_SMALL_DISMISS_EVIL; }else
    if(id == "BUTTON_SMALL_UPGRADE_GOOD"){ return ICN::BUTTON_SMALL_UPGRADE_GOOD; }else
    if(id == "BUTTON_SMALL_UPGRADE_EVIL"){ return ICN::BUTTON_SMALL_UPGRADE_EVIL; }else
    if(id == "BUTTON_SMALL_RESTART_GOOD"){ return ICN::BUTTON_SMALL_RESTART_GOOD; }else
    if(id == "BUTTON_SMALL_RESTART_EVIL"){ return ICN::BUTTON_SMALL_RESTART_EVIL; }else
    if(id == "BUTTON_SMALL_MIN_GOOD"){ return ICN::BUTTON_SMALL_MIN_GOOD; }else
    if(id == "BUTTON_SMALL_MIN_EVIL"){ return ICN::BUTTON_SMALL_MIN_EVIL; }else
    if(id == "BUTTON_SMALL_MAX_GOOD"){ return ICN::BUTTON_SMALL_MAX_GOOD; }else
    if(id == "BUTTON_SMALL_MAX_EVIL"){ return ICN::BUTTON_SMALL_MAX_EVIL; }else
    if(id == "BUTTON_EXIT_GOOD"){ return ICN::BUTTON_EXIT_GOOD; }else
    if(id == "BUTTON_RESET_GOOD"){ return ICN::BUTTON_RESET_GOOD; }else
    if(id == "BUTTON_START_GOOD"){ return ICN::BUTTON_START_GOOD; }else
    if(id == "BUTTON_CASTLE_GOOD"){ return ICN::BUTTON_CASTLE_GOOD; }else
    if(id == "BUTTON_CASTLE_EVIL"){ return ICN::BUTTON_CASTLE_EVIL; }else
    if(id == "BUTTON_TOWN_GOOD"){ return ICN::BUTTON_TOWN_GOOD; }else
    if(id == "BUTTON_TOWN_EVIL"){ return ICN::BUTTON_TOWN_EVIL; }else
    if(id == "BUTTON_RESTRICT_GOOD"){ return ICN::BUTTON_RESTRICT_GOOD; }else
    if(id == "BUTTON_RESTRICT_EVIL"){ return ICN::BUTTON_RESTRICT_EVIL; }else
    if(id == "BUTTON_KINGDOM_EXIT"){ return ICN::BUTTON_KINGDOM_EXIT; }else
    if(id == "BUTTON_KINGDOM_HEROES"){ return ICN::BUTTON_KINGDOM_HEROES; }else
    if(id == "BUTTON_KINGDOM_TOWNS"){ return ICN::BUTTON_KINGDOM_TOWNS; }else
    if(id == "BUTTON_MAPSIZE_SMALL"){ return ICN::BUTTON_MAPSIZE_SMALL; }else
    if(id == "BUTTON_MAPSIZE_MEDIUM"){ return ICN::BUTTON_MAPSIZE_MEDIUM; }else
    if(id == "BUTTON_MAPSIZE_LARGE"){ return ICN::BUTTON_MAPSIZE_LARGE; }else
    if(id == "BUTTON_MAPSIZE_XLARGE"){ return ICN::BUTTON_MAPSIZE_XLARGE; }else
    if(id == "BUTTON_MAPSIZE_ALL"){ return ICN::BUTTON_MAPSIZE_ALL; }else
    if(id == "BUTTON_MAP_SELECT_GOOD"){ return ICN::BUTTON_MAP_SELECT_GOOD; }else
    if(id == "BUTTON_MAP_SELECT_EVIL"){ return ICN::BUTTON_MAP_SELECT_EVIL; }else
    if(id == "BUTTON_GUILDWELL_EXIT"){ return ICN::BUTTON_GUILDWELL_EXIT; }else
    if(id == "BUTTON_VIEWWORLD_EXIT_GOOD"){ return ICN::BUTTON_VIEWWORLD_EXIT_GOOD; }else
    if(id == "BUTTON_VIEWWORLD_EXIT_EVIL"){ return ICN::BUTTON_VIEWWORLD_EXIT_EVIL; }else
    if(id == "BUTTON_VERTICAL_DISMISS"){ return ICN::BUTTON_VERTICAL_DISMISS; }else
    if(id == "BUTTON_VERTICAL_EXIT"){ return ICN::BUTTON_VERTICAL_EXIT; }else
    if(id == "BUTTON_VERTICAL_PATROL"){ return ICN::BUTTON_VERTICAL_PATROL; }else
    if(id == "BUTTON_HSCORES_VERTICAL_CAMPAIGN"){ return ICN::BUTTON_HSCORES_VERTICAL_CAMPAIGN; }else
    if(id == "BUTTON_HSCORES_VERTICAL_EXIT"){ return ICN::BUTTON_HSCORES_VERTICAL_EXIT; }else
    if(id == "BUTTON_HSCORES_VERTICAL_STANDARD"){ return ICN::BUTTON_HSCORES_VERTICAL_STANDARD; }else
    if(id == "GAME_OPTION_ICON"){ return ICN::GAME_OPTION_ICON; }else
    if(id == "DIFFICULTY_ICON_EASY"){ return ICN::DIFFICULTY_ICON_EASY; }else
    if(id == "DIFFICULTY_ICON_NORMAL"){ return ICN::DIFFICULTY_ICON_NORMAL; }else
    if(id == "DIFFICULTY_ICON_HARD"){ return ICN::DIFFICULTY_ICON_HARD; }else
    if(id == "DIFFICULTY_ICON_EXPERT"){ return ICN::DIFFICULTY_ICON_EXPERT; }else
    if(id == "DIFFICULTY_ICON_IMPOSSIBLE"){ return ICN::DIFFICULTY_ICON_IMPOSSIBLE; }else
    if(id == "METALLIC_BORDERED_TEXTBOX_GOOD"){ return ICN::METALLIC_BORDERED_TEXTBOX_GOOD; }else
    if(id == "METALLIC_BORDERED_TEXTBOX_EVIL"){ return ICN::METALLIC_BORDERED_TEXTBOX_EVIL; }else
    if(id == "BUTTON_NEW_MAP_EVIL"){ return ICN::BUTTON_NEW_MAP_EVIL; }else
    if(id == "BUTTON_NEW_MAP_GOOD"){ return ICN::BUTTON_NEW_MAP_GOOD; }else
    if(id == "BUTTON_SAVE_MAP_EVIL"){ return ICN::BUTTON_SAVE_MAP_EVIL; }else
    if(id == "BUTTON_SAVE_MAP_GOOD"){ return ICN::BUTTON_SAVE_MAP_GOOD; }else
    if(id == "BUTTON_LOAD_MAP_EVIL"){ return ICN::BUTTON_LOAD_MAP_EVIL; }else
    if(id == "BUTTON_LOAD_MAP_GOOD"){ return ICN::BUTTON_LOAD_MAP_GOOD; }else
    if(id == "BUTTON_RUMORS_GOOD"){ return ICN::BUTTON_RUMORS_GOOD; }else
    if(id == "BUTTON_RUMORS_EVIL"){ return ICN::BUTTON_RUMORS_EVIL; }else
    if(id == "BUTTON_EVENTS_GOOD"){ return ICN::BUTTON_EVENTS_GOOD; }else
    if(id == "BUTTON_EVENTS_EVIL"){ return ICN::BUTTON_EVENTS_EVIL; }else
    if(id == "BUTTON_LANGUAGE_GOOD"){ return ICN::BUTTON_LANGUAGE_GOOD; }else
    if(id == "BUTTON_LANGUAGE_EVIL"){ return ICN::BUTTON_LANGUAGE_EVIL; }else
    if(id == "SCENIBKG_EVIL"){ return ICN::SCENIBKG_EVIL; }
    return -1;
}
