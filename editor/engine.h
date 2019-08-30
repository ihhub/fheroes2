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

#ifndef _EDITOR_ENGINE_H_
#define _EDITOR_ENGINE_H_

#include <QFile>
#include <QMap>
#include <QString>
#include <QPixmap>
#include <QByteArray>
#include <QPair>
#include <QVector>
#include <QMap>
#include <QDomElement>
#include <QVariant>
#include <QMenu>

namespace Ground
{
    enum { Unknown = 0, Desert = 0x01, Snow = 0x02, Swamp = 0x04, Wasteland = 0x08, Beach = 0x10, Lava = 0x20, Dirt = 0x40, Grass = 0x80, Water = 0x100,
	    All = Desert | Snow | Swamp | Wasteland | Beach | Lava | Dirt | Grass | Water };
}

namespace Direction
{
    enum { Unknown = 0x0000, TopLeft = 0x0001, Top = 0x0002, TopRight = 0x0004, Right = 0x0008, BottomRight = 0x0010, Bottom = 0x0020, BottomLeft = 0x0040, Left = 0x0080, Center = 0x0100,
	    TopRow = TopLeft | Top | TopRight, BottomRow = BottomLeft | Bottom | BottomRight, CenterRow = Left | Center | Right, All = TopRow | CenterRow | BottomRow };
}

namespace ICN
{
    enum { UNKNOWN = 0,
	    OBJNARTI = 0x2C, MONS32 = 0x30, FLAG32 = 0x38, MINIHERO = 0x54, MTNSNOW = 0x58, MTNSWMP = 0x5C, MTNLAVA = 0x60, MTNDSRT = 0x64, MTNDIRT = 0x68, MTNMULT = 0x6C,
	    EXTRAOVR = 0x74, ROAD = 0x78, MTNCRCK = 0x7C, MTNGRAS = 0x80, TREJNGL = 0x84, TREEVIL = 0x88, OBJNTOWN = 0x8C, OBJNTWBA = 0x90, OBJNTWSH = 0x94, OBJNTWRD = 0x98, OBJNWAT2 = 0xA0,
	    OBJNMUL2 = 0xA4, TRESNOW = 0xA8, TREFIR = 0xAC, TREFALL = 0xB0, STREAM = 0xB4, OBJNRSRC = 0xB8, OBJNGRA2 = 0xC0, TREDECI = 0xC4, OBJNWATR = 0xC8, OBJNGRAS = 0xCC, OBJNSNOW = 0xD0,
	    OBJNSWMP = 0xD4, OBJNLAVA = 0xD8, OBJNDSRT = 0xDC, OBJNDIRT = 0xE0, OBJNCRCK = 0xE4, OBJNLAV3 = 0xE8, OBJNMULT = 0xEC, OBJNLAV2 = 0xF0, X_LOC1 = 0xF4, X_LOC2 = 0xF8, X_LOC3 = 0xFC };
    QString	transcribe(int);
}

namespace Color
{
    enum { None = 0, Blue = 0x01, Green = 0x02, Red = 0x04, Yellow = 0x08, Orange = 0x10, Purple = 0x20,
	    All = Blue | Red | Green | Yellow | Orange | Purple };

    int			count(int);
    QColor		convert(int);
    QVector<int>	colors(int);
    QString		transcribe(int);
    int			index(int);
}

namespace Race
{
    enum { Unknown = 0, Knight = 0x01, Barbarian = 0x02, Sorceress = 0x04, Warlock = 0x08, Wizard = 0x10, Necromancer = 0x20,
	    Random = 0x80, Multi = 0x40, All = Knight | Barbarian | Sorceress | Warlock | Wizard | Necromancer };

    QString		transcribe(int);
    int			index(int);
}

namespace Speed
{
    enum { Standing = 0, Crawling, VerySlow, Slow, Average, Fast, VeryFast, UltraFast, Blazing, Instant };

    QString		transcribe(int);
}

namespace Building
{
    enum { Unknown = 0, ThievesGuild = 0x00000001, Tavern = 0x00000002, Shipyard = 0x00000004, Well = 0x00000008, Statue = 0x00000010, LeftTurret = 0x00000020,
	    RightTurret = 0x00000040, Marketplace = 0x00000080, ExtraWel2 = 0x00000100 /* Farm, Garbage He, Crystal Gar, Waterfall, Orchard, Skull Pile */,
	    Moat = 0x00000200, ExtraSpec = 0x00000400 /* Fortification, Coliseum, Rainbow, Dungeon, Library, Storm */,
	    Castle = 0x00000800, Captain = 0x00001000, Shrine = 0x00002000,
	    MageGuild1 = 0x00004000, MageGuild2 = 0x00008000, MageGuild3 = 0x00010000, MageGuild4 = 0x00020000, MageGuild5 = 0x00040000,
	    Dwelling1 = 0x00100000, Dwelling2 = 0x00200000, Dwelling3 = 0x00400000, Dwelling4 = 0x00800000, Dwelling5 = 0x01000000, Dwelling6 = 0x02000000,
	    Upgrade2 = 0x04000000, Upgrade3 = 0x08000000, Upgrade4 = 0x10000000, Upgrade5 = 0x20000000, Upgrade6 = 0x40000000, Upgrade7 = 0x80000000,
	    MageGuild = MageGuild1 | MageGuild2 | MageGuild3 | MageGuild4 | MageGuild5,
	    Dwellings = Dwelling1 | Dwelling2 | Dwelling3 | Dwelling4 | Dwelling5 | Dwelling6,
	    Upgrades = Upgrade2 | Upgrade3 | Upgrade4 | Upgrade5 | Upgrade6 | Upgrade7 };

    int		dwellingMap(int race);
    QString	extraWel2(int race);
    QString	extraSpec(int race);
    QString	description(int type, int race);
}

namespace SpriteLevel
{
    enum { Unknown, Bottom, Action, Top };

    int fromString(const QString &);
}

namespace Difficulty
{
    enum { Easy, Normal, Tough, Expert };
}

namespace Artifact
{
    enum { None, UltimateBook, UltimateSword, UltimateCloak, UltimateWand, UltimateShield, UltimateStaff, UltimateCrown, GoldenGoose,
	    ArcaneNecklace, CasterBracelet, MageRing, WitchesBroach, MedalValor, MedalCourage, MedalHonor, MedalDistinction, FizbinMisfortune,
	    ThunderMace, ArmoredGauntlets, DefenderHelm, GiantFlail, Ballista, StealthShield, DragonSword, PowerAxe, DivineBreastplate,
	    MinorScroll, MajorScroll, SuperiorScroll, ForemostScroll, EndlessSackGold, EndlessBagGold, EndlessPurseGold, NomadBootsMobility,
	    TravelerBootsMobility, RabbitFoot, GoldenHorseShoe, GamblerLuckyCoin, FourLeafClover, TrueCompassMobility, SailorsAstrolabeMobility,
	    EvilEye, EnchantedHourglass, GoldWatch, SkullCap, IceCloak, FireCloak, LightningHelm, EvercoldIcicle, EverhotLavaRock, LightningRod,
	    SnakeRing, Ankh, BookElements, ElementalRing, HolyPendant, PendantFreeWill, PendantLife, SerenityPendant, SeeingEyePendant, KineticPendant,
	    PendantDeath, WandNegation, GoldenBow, Telescope, StatesmanQuill, WizardHat, PowerRing, AmmoCart, TaxLien, HideousMask, EndlessPouchSulfur,
	    EndlessVialMercury, EndlessPouchGems, EndlessCordWood, EndlessCartOre, EndlessPouchCrystal, SpikedHelm, SpikedShield, WhitePearl, BlackPearl,
	    Random, UltimateRandom, Random1, Random2, Random3, SpellScroll, ArmMartyr, BreastplateAnduran, BroachShielding, BattleGarb, CrystalBall, HeartFire,
	    HeartIce, HelmetAnduran, HolyHammer, LegendaryScepter, Masthead, SphereNegation, StaffWizardry, SwordBreaker, SwordAnduran, SpadeNecromancy,
	    Unknown };

    bool	isValid(int);
    QString	transcribe(int);
    QString	description(int);
}

namespace Resource
{
    enum { Unknown = 0, Wood = 0x01, Mercury = 0x02, Ore = 0x04, Sulfur = 0x08, Crystal = 0x10, Gems = 0x20, Gold = 0x40,
	    Random = 0x80, All = Wood | Mercury | Ore | Sulfur | Crystal | Gems | Gold };

    QString transcribe(int);
}

namespace Portrait
{
    enum { Unknown,
	    LordKilburn, SirGallanth, Ector, Gwenneth, Tyro, Ambrose, Ruby, Maximus, Dimitry,
	    Thundax, Fineous, Jojosh, CragHack, Jezebel, Jaclyn, Ergon, Tsabu, Atlas,
	    Astra, Natasha, Troyan, Vatawna, Rebecca, Gem, Ariel, Carlawn, Luna,
	    Arie, Alamar, Vesper, Crodo, Barok, Kastore, Agar, Falagar, Wrathmont,
	    Myra, Flint, Dawn, Halon, Myrini, Wilfrey, Sarakin, Kalindra, Mandigal,
	    Zom, Darlana, Zam, Ranloo, Charity, Rialdo, Roxana, Sandro, Celia,
	    Roland, LordCorlagon, SisterEliza, Archibald, LordHalton, BrotherBax,
	    Solmyr, Dainwin, Mog, UncleIvan, Joseph, Gallavant, Elderian, Ceallach, Drakonia, Martine, Jarkonas,
	    Random };

    QString transcribe(int);
}

namespace SkillType
{
    enum { None, PathFinding, Archery, Logistics, Scouting, Diplomacy, Navigation, LeaderShip, Wisdom, Mysticism, Luck, Ballistics, EagleEye, Necromancy, Estates, Unknown };

    QString transcribe(int);
}

namespace SkillLevel
{
    enum { Unknown = 0, Basic = 1, Advanced = 2, Expert = 3 };

    QString transcribe(int);
}

namespace Spell
{
    enum { None,
	    FireBall, FireBlast, LightningBolt, ChainLightning, Teleport, Cure, MassCure, Resurrect, ResurrectTrue, Haste, MassHaste, Slow, MassSlow,
	    Blind, Bless, MassBless, StoneSkin, SteelSkin, Curse, MassCurse, HolyWord, HolyShout, AntiMagic, Dispel, MassDispel, Arrow, Berserker,
	    Armageddon, ElementalStorm, MeteorShower, Paralyze, Hypnotize, ColdRay, ColdRing, DisruptingRay, DeathRipple, DeathWave, DragonSlayer,
	    BloodLust, AnimateDead, MirrorImage, Shield, MassShield, SummonEElement, SummonAElement, SummonFElement, SummonWElement, EarthQuake,
	    ViewMines, ViewResources, ViewArtifacts, ViewTowns, ViewHeroes, ViewAll, IdentifyHero, SummonBoat, DimensionDoor, TownGate, TownPortal,
	    Visions, Haunt, SetEGuardian, SetAGuardian, SetFGuardian, SetWGuardian,
	    Random, Random1, Random2, Random3, Random4, Random5, Unknown };

    int		level(int);
    bool	isBattle(int);
    QString	transcribe(int);
    QString	tips(int);
    QPixmap	pixmap(int);
}

namespace Monster
{
    enum { None,
	Peasant, Archer, Ranger, Pikeman, VeteranPikeman, Swordsman, MasterSwordsman, Cavalry, Champion, Paladin, Crusader,
	Goblin, Orc, OrcChief, Wolf, Ogre, OgreLord, Troll, WarTroll, Cyclops,
	Sprite, Dwarf, BattleDwarf, Elf, GrandElf, Druid, GreaterDruid, Unicorn, Phoenix,
	Centaur, Gargoyle, Griffin, Minotaur, MinotaurKing, Hydra, GreenDragon, RedDragon, BlackDragon,
	Halfling, Boar, IronGolem, SteelGolem, Roc, Mage, Archmage, Giant, Titan,
	Skeleton, Zombie, MutantZombie, Mummy, RoyalMummy, Vampire, VampireLord, Lich, PowerLich, BoneDragon,
	Rogue, Nomad, Ghost, Genie, Medusa, EarthElement, AirElement, FireElement, WaterElement,
	Random, Random1, Random2, Random3, Random4, Unknown };

    bool	isValid(int);
    QString	transcribe(int);
    QString	tips(int);
}

struct mp2icn_t
{
    mp2icn_t(const char*);

    quint16	offsetX;
    quint16	offsetY;
    quint16	width;
    quint16	height;
    quint8	type;	/* animation: 0x01, */
    quint32	offsetData;

    static int	sizeOf(void) { return 13; };
};

struct mp2lev_t
{
    mp2lev_t();

    quint8	object;
    quint8	index;
    quint32	uniq;
};

struct mp2ext_t
{
    mp2ext_t();

    quint16     indexExt;
    quint8      quantity;
    mp2lev_t	level1;
    mp2lev_t	level2;
};

struct mp2til_t
{
    mp2til_t();

    quint16     tileSprite;
    quint8      quantity1;
    quint8      quantity2;
    quint8      tileShape;
    quint8      objectID;
    mp2ext_t	ext;
};


struct mp2pos_t
{
    quint8	posx;
    quint8	posy;
    quint8	type;
};

struct mp2town_t
{
    quint8	color;
    quint8	customBuilding;
    quint32	building;
    quint8	magicTower;
    quint8	customTroops;
    quint8	troopId[5];
    quint16	troopCount[5];
    quint8	captainPresent;
    quint8	customName;
    QString	name; /* string: 13 byte */
    quint8	race;
    quint8	isCastle;
    quint8	forceTown;
    quint8      unknown1[29];
};

struct mp2hero_t
{
    quint8	unknown1;
    quint8	customTroops;
    quint8	troopId[5];
    quint16	troopCount[5];
    quint8	customPortrate;
    quint8	portrateType;
    quint8	artifacts[3];
    quint8	unknown2;
    quint32	experience;
    quint8	customSkills;
    quint8	skillId[8];
    quint8	skillLevel[8];
    quint8	unknown3;
    quint8	customName;
    QString	name; /* string: 13 byte */
    quint8	patrol;
    quint8	patrolSquare;
    quint8      unknown4[15];
};

struct mp2sign_t
{
    quint8	id; /* 0x01 */
    quint8	zero[8];
    QString	text;
};

struct mp2mapevent_t
{
    quint8	id; /* 0x01 */
    qint32	resources[7]; /* wood, mercury, ore, sulfur, crystal, gems, golds */
    quint16	artifact; /* 0xffff - none */
    quint8	allowComputer;
    quint8	cancelAfterFirstVisit;
    quint8	zero[10];
    quint8	colors[6]; /* blue, green, red, yellow, orange, purple */
    QString	text;
};

struct mp2sphinx_t
{
    quint8	id; /* 0x00 */
    qint32	resources[7]; /* wood, mercury, ore, sulfur, crystal, gems, golds */
    quint16	artifact; /* 0xffff - none */
    QStringList answers; /* 8 blocks, 13 byte string */
    QString	text;
};

struct mp2dayevent_t
{
    quint8	id; /* 0 */
    qint32	resources[7]; /* wood, mercury, ore, sulfur, crystal, gems, golds */
    quint16	artifact; /* always 0xffff - none */
    quint16	allowComputer;
    quint16	dayFirstOccurent;
    quint16	subsequentOccurrences;
    quint8	zero[6];
    quint8	colors[6]; /* blue, green, red, yellow, orange, purple */
    QString	text;
};

struct mp2rumor_t
{
    quint8	id; /* 0 */
    quint8	zero[7];
    QString	text;
};

QDataStream & operator>> (QDataStream &, mp2icn_t &);
QDataStream & operator>> (QDataStream &, mp2til_t &);
QDataStream & operator>> (QDataStream &, mp2ext_t &);
QDataStream & operator>> (QDataStream &, mp2town_t &);
QDataStream & operator>> (QDataStream &, mp2hero_t &);
QDataStream & operator>> (QDataStream &, mp2sign_t &);
QDataStream & operator>> (QDataStream &, mp2mapevent_t &);
QDataStream & operator>> (QDataStream &, mp2dayevent_t &);
QDataStream & operator>> (QDataStream &, mp2rumor_t &);
QDataStream & operator>> (QDataStream &, mp2sphinx_t &);

namespace MapObj
{
    enum { None = 0, AlchemyLab = 0x01, Sign = 0x02, Buoy = 0x03, Skeleton = 0x04, DaemonCave = 0x05, TreasureChest = 0x06, FaerieRing = 0x07,
	    CampFire = 0x08, Fountain = 0x09, Gazebo = 0x0A, AncientLamp = 0x0B, Graveyard = 0x0C, ArcherHouse = 0x0D, GoblinHut = 0x0E, DwarfCott = 0x0F,
	    PeasantHut = 0x10, Unused17 = 0x11, Unused18 = 0x12, Event = 0x13, DragonCity = 0x14, LightHouse = 0x15, WaterWheel = 0x16, Mines = 0x17,
	    Monster = 0x18, Obelisk = 0x19, Oasis = 0x1A, Resource = 0x1B, Coast = 0x1C, SawMill = 0x1D, Oracle = 0x1E, Shrine1 = 0x1F, ShipWreck = 0x20,
	    Unused33 = 0x21, DesertTent = 0x22, Castle = 0x23, StoneLights = 0x24, WagonCamp = 0x25, WaterChest = 0x26, WhirlPool = 0x27, WindMill = 0x28,
	    Artifact = 0x29, Reefs = 0x2A, Boat = 0x2B, RndUltimateArtifact = 0x2C, RndArtifact = 0x2D, RndResource = 0x2E, RndMonster = 0x2F, 
	    RndTown = 0x30, RndCastle = 0x31, Mermaid = 0x32, RndMonster1 = 0x33, RndMonster2 = 0x34, RndMonster3 = 0x35, RndMonster4 = 0x36, Heroes = 0x37,
	    Sirens = 0x38, HutMagi = 0x39, WatchTower = 0x3A, TreeHouse = 0x3B, TreeCity = 0x3C, Ruins = 0x3D, Fort = 0x3E, TradingPost = 0x3F,
	    AbandonedMine = 0x40, ThatchedHut = 0x41, StandingStones = 0x42, Idol = 0x43, TreeKnowledge = 0x44, DoctorHut = 0x45, Temple = 0x46,
	    HillFort = 0x47, HalflingHole = 0x48, MercenaryCamp = 0x49, Shrine2 = 0x4A, Shrine3 = 0x4B, Pyramid = 0x4C, CityDead = 0x4D, Excavation = 0x4E,
	    Sphinx = 0x4F, Wagon = 0x50, Tarpit = 0x51, ArtesianSpring = 0x52, TrollBridge = 0x53, WateringHole = 0x54, WitchsHut = 0x55, Xanadu = 0x56,
	    Cave = 0x57, Leanto = 0x58, MagellanMaps = 0x59, FlotSam = 0x5A, DerelictShip = 0x5B, ShipwreckSurviror = 0x5C, Bottle = 0x5D, MagicWell = 0x5E,
	    MagicGarden = 0x5F, ObservationTower = 0x60, FreemanFoundry = 0x61, EyeMagi = 0x62, Trees = 0x63, Mounts = 0x64, Volcano = 0x65, Flowers = 0x66,
	    Stones = 0x67, WaterLake = 0x68, Mandrake = 0x69, DeadTree = 0x6A, Stump = 0x6B, Crater = 0x6C, Cactus = 0x6D, Mound = 0x6E, Dune = 0x6F,
	    LavaPool = 0x70, Shrub = 0x71, Arena = 0x72, BarrowMounds = 0x73, RndArtifact1 = 0x74, RndArtifact2 = 0x75, RndArtifact3 = 0x76, Barrier = 0x77,
	    TravellerTent = 0x78, AlchemyTower = 0x79, Stables = 0x7A, Jail = 0x7B, FireAltar = 0x7C, AirAltar = 0x7D, EarthAltar = 0x7E, WaterAltar = 0x7F,
	    IsAction = 0x80 };

    bool	IsPickup(int);
    QString	transcribe(int);
}

struct CompositeSprite
{
    int		spriteICN;
    int		spriteIndex;
    QPoint	spritePos;
    int		spriteLevel;
    int		spritePassable;
    int		spriteAnimation;

    CompositeSprite(){}
    CompositeSprite(const QString &, const QDomElement &);
};

struct CompositeObject : public QVector<CompositeSprite>
{
    QString	name;
    QSize	size;
    int		classId;
    QString	icn;

    CompositeObject() : classId(MapObj::None) {}
    CompositeObject(const QDomElement &);

    bool	isValid(void) const;
};

Q_DECLARE_METATYPE(CompositeObject);

struct AccessResult
{
    int		allowPlayers;
    bool	allowComputer;
    bool	cancelAfterFirstVisit;

    AccessResult() : allowPlayers(0), allowComputer(false), cancelAfterFirstVisit(false) {}
    AccessResult(int c, bool a, bool f) : allowPlayers(c), allowComputer(a), cancelAfterFirstVisit(f) {}

    QString	transcribe(void) const;
};

Q_DECLARE_METATYPE(AccessResult);

namespace Editor
{
    quint32 Rand(quint32 max);
    quint32 Rand(quint32 min, quint32 max);
    float   RandF(float max);
    float   RandF(float min, float max);
    QPixmap pixmapBorder(const QSize &, const QColor &, const QColor &);
    QPixmap pixmapBorderPassable(int passable);

    QStringList townNames(void);

    class MyXML : public QDomElement
    {
    public:
	MyXML(const QString &, const QString &, bool debug = true);
    };

    class MyObjectsXML : public QList<QDomElement>
    {
    public:
	MyObjectsXML(const QString &, bool debug = true);
    };
}

namespace H2
{
    class File : public QFile
    {
    public:
	File();
	File(const QString &);

	quint32	readByte(void);
	QString	readString(size_t);
	QByteArray
		readBlock(size_t, int = -1);

	quint32	readLE16(void);
	quint32	readLE32(void);
    };

    class ICNSprite : public QImage
    {
    public:
	ICNSprite(const mp2icn_t &, const char*, quint32, const QVector<QRgb> &);

    private:
	void DrawVariant1(const quint8*, const quint8*, const QVector<QRgb> &);
	void DrawVariant2(const quint8*, const quint8*, const QVector<QRgb> &);
    };

    int          MP2ICN(int, bool);
    QString      icnString(int);
    int          isAnimationICN(int, int, int);

    struct TownPos : QPair<mp2town_t, QPoint>
    {
	TownPos() {}
	TownPos(const mp2town_t & t, const QPoint & p) : QPair<mp2town_t, QPoint>(t, p) {}

	const mp2town_t & town(void) const { return first; }
	const QPoint & pos(void) const { return second; }
    };

    struct HeroPos : QPair<mp2hero_t, QPoint>
    {
	HeroPos() {}
	HeroPos(const mp2hero_t & t, const QPoint & p) : QPair<mp2hero_t, QPoint>(t, p) {}

	const mp2hero_t & hero(void) const { return first; }
	const QPoint & pos(void) const { return second; }
    };

    struct SignPos : QPair<mp2sign_t, QPoint>
    {
	SignPos() {}
	SignPos(const mp2sign_t & t, const QPoint & p) : QPair<mp2sign_t, QPoint>(t, p) {}

	const mp2sign_t & sign(void) const { return first; }
	const QPoint & pos(void) const { return second; }
    };

    struct EventPos : QPair<mp2mapevent_t, QPoint>
    {
	EventPos() {}
	EventPos(const mp2mapevent_t & t, const QPoint & p) : QPair<mp2mapevent_t, QPoint>(t, p) {}

	const mp2mapevent_t & event(void) const { return first; }
	const QPoint & pos(void) const { return second; }
    };

    struct SphinxPos : QPair<mp2sphinx_t, QPoint>
    {
	SphinxPos() {}
	SphinxPos(const mp2sphinx_t & t, const QPoint & p) : QPair<mp2sphinx_t, QPoint>(t, p) {}

	const mp2sphinx_t & sphinx(void) const { return first; }
	const QPoint & pos(void) const { return second; }
    };
}

namespace AGG
{
    struct Item
    {
	quint32 crc;
	quint32 offset;
	quint32 size;
    };

    class File : public H2::File
    {
    protected:
	QMap<QString, Item>	items;

    public:
	File(){}

	int			seekToData(const QString &);
	QByteArray		readRawData(const QString &);

	bool			loadFile(const QString &);
	bool			exists(const QString &) const;

	QPixmap			getImageTIL(const QString &, int, QVector<QRgb> &);
	QPair<QPixmap, QPoint>	getImageICN(const QString &, int, QVector<QRgb> &);
    };

    class Spool
    {
	AGG::File		first;
	AGG::File		second; /* first: heroes2.agg, second: heroes2x.agg */
	QVector<QRgb>           colors;
	QMap<QString, QPoint>	icnOffsetCache;

	void			fixAGGImagesBugs(const QString &, int index, QPair<QPixmap, QPoint> &);

    public:
	Spool(){}

	bool			setData(const QString &);

	QPixmap			getImageTIL(const QString &, int);
	QPair<QPixmap, QPoint>	getImageICN(const QString &, int);
	QPixmap			getImage(const CompositeObject &, const QSize &);

	bool			isHeroes2XMode(void) const;
	QString			dirName(void) const;
    };
}

class MapData;
class MapTile;
class MapTiles;

class AroundGrounds: public QVector<int>
{
public:
    AroundGrounds() : QVector<int>(9, Ground::Unknown){} /* ground: top left, top, top right, right, bottom right, bottom, bottom left, left, center */
    AroundGrounds(const MapTiles &, const QPoint &);

    int operator() (void) const;
    int groundsDirects(int) const;
    int directionsAroundGround(int) const;
    int preferablyGround(void) const;
};

namespace EditorTheme
{
    bool			load(const QString &);
    int          		mapICN(const QString &);

    QPixmap			getImageTIL(const QString &, int);
    QPair<QPixmap, QPoint>	getImageICN(int, int);
    QPair<QPixmap, QPoint>	getImageICN(const QString &, int);
    QPixmap			getImage(const CompositeObject &);
    int				getSpriteID(int, int);
    int				getSpriteLevel(int, int);
    int				getSpritePassable(int, int);

    const QSize &		tileSize(void);

    int				startFilledTile(int);
    int				startGroundTile(int);
    int				startFilledOriginalTile(int);
    int				startGroundOriginalTile(int);

    int				ground(int);
    QPair<int, int>		groundBoundariesFix(const MapTile &, const MapTiles &);
    int				groundOneTileFix(const MapTile &, const MapTiles &);

    QString			resourceFile(const QString & dir, const QString & file);
    QStringList			resourceFiles(const QString & dir, const QString & file);
}

struct resources_t
{
    int		wood;
    int		mercury;
    int		ore;
    int		sulfur;
    int		crystal;
    int		gems;
    int		gold;
};

struct Resources : public resources_t
{
    Resources(){ wood = 0; mercury = 0; ore = 0; sulfur = 0; crystal = 0; gems = 0; gold = 0; }

    QString	describe(void) const;
};

struct MonsterStat
{
    int		attack;
    int		defense;
    int		damageMin;
    int		damageMax;
    int		hp;
    int		speed;
    int		grown;
    int		shots;
    resources_t	cost;
};

Q_DECLARE_METATYPE(Resources);

struct TypeVariant
{
    int		type;
    QVariant	variant;

    TypeVariant() : type(-1), variant() {}
    TypeVariant(const int & t, const QVariant & v) : type(t), variant(v) {}
};

Q_DECLARE_METATYPE(TypeVariant);

class MapObject : public QPoint
{
    quint32	objUid;
    int		objType;

    friend 	QDomElement & operator<< (QDomElement &, const MapObject &);
    friend	QDomElement & operator>> (QDomElement &, MapObject &);

public:
    MapObject(const QPoint & pos, int uid, int type = MapObj::None) : QPoint(pos), objUid(uid), objType(type) {}
    virtual ~MapObject() {}

    quint32 		uid(void) const { return objUid; }
    int 		type(void) const { return objType; }
    const QPoint &	pos(void) const { return *this; }
    virtual QString	name(void) const { return MapObj::transcribe(objType); }
    virtual QString	object(void) const { return "object"; }
    virtual int		color(void) const { return Color::None; }
    virtual MapObject*	copy(void) const = 0;

    void 		setUID(quint32 uid) { objUid = uid; }
    void 		setPos(const QPoint & pos) { setX(pos.x()); setY(pos.y()); }
};

struct Skill : public QPair<int, int>
{
    Skill() : QPair<int, int>(SkillType::None, SkillLevel::Unknown) {}
    Skill(int type, int level) : QPair<int, int>(type, level) {}

    bool	isValid(void) const { return first && second; }
    const int &	skill(void) const { return first; }
    const int &	level(void) const { return second; }

    QString	name(void) const;
    QString	description(void) const;
    QPixmap	pixmap(void) const;
};

Q_DECLARE_METATYPE(Skill);

struct Skills : public QVector<Skill>
{
    Skills() { reserve(10); }
};

struct Troop : public QPair<int, int>
{
    Troop() : QPair<int, int>(0, 0) {}
    Troop(int type, int count) : QPair<int, int>(type, count) {}

    bool	isValid(void) const { return first && second; }
    const int &	type(void) const { return first; }
    const int &	count(void) const { return second; }
};

struct Troops : public QVector<Troop>
{
    Troops() : QVector<Troop>(5) { /* five slots */ }

    int validCount(void) const;
};

struct MapTown : public MapObject
{
    int		col;
    int		race;
    uint	buildings;
    uint	dwellings;
    QString     nameTown;
    Troops	troops;
    bool	isCastle;
    bool	forceTown;
    bool	captainPresent;
    bool	customTroops;
    bool	customBuildings;
    bool	customDwellings;

    MapTown(const QPoint &, quint32, const mp2town_t &);
    MapTown(const QPoint & pos = QPoint(-1, -1), quint32 uid = -1);

    QString	name(void) const { return nameTown; }
    QString	object(void) const { return "town"; }

    int		color(void) const { return col; }
    MapObject*	copy(void) const { return new MapTown(*this); }
    void	updateInfo(int, bool);
};

struct MapHero : public MapObject
{
    int		attack;
    int		defence;
    int		power;
    int		knowledge;
    int		col;
    int		race;
    Troops	troops;
    int		portrait;
    QVector<int> artifacts;
    QVector<int> spells;
    Skills	skills;
    quint32	experience;
    bool	patrolMode;
    int		patrolSquare;
    bool	jailMode;
    bool	magicBook;
    QString     nameHero;

    MapHero(const QPoint &, quint32, const mp2hero_t &, int, bool);
    MapHero(const QPoint & pos = QPoint(-1, -1), quint32 uid = -1);

    QString	name(void) const;
    QString	object(void) const { return "hero"; }

    int		color(void) const { return col; }
    MapObject*	copy(void) const { return new MapHero(*this); }
    void	updateInfo(int);
    bool	haveMagicBook(void) const;
};

struct MapSign : public MapObject
{
    QString	message;

    MapSign(const QPoint &, quint32, const mp2sign_t &);
    MapSign(const QPoint & pos = QPoint(-1, -1), quint32 uid = -1);

    QString	object(void) const { return "sign"; }

    MapObject*	copy(void) const { return new MapSign(*this); }
};

struct MapEvent : public MapObject
{
    Resources	resources;
    int		artifact;
    bool	allowComputer;
    bool	cancelAfterFirstVisit;
    int		colors;
    QString	message;

    MapEvent(const QPoint &, quint32, const mp2mapevent_t &);
    MapEvent(const QPoint & pos = QPoint(-1, -1), quint32 uid = -1);

    QString	object(void) const { return "event"; }

    MapObject*	copy(void) const { return new MapEvent(*this); }
};

struct MapSphinx : public MapObject
{
    Resources	resources;
    int		artifact;
    QStringList answers;
    QString	message;

    MapSphinx(const QPoint &, quint32, const mp2sphinx_t &);
    MapSphinx(const QPoint & pos = QPoint(-1, -1), quint32 uid = -1);

    QString	object(void) const { return "sphinx"; }
    MapObject*	copy(void) const { return new MapSphinx(*this); }
};

struct MapResource : public MapObject
{
    int		type;
    int		count;

    MapResource(const QPoint & pos = QPoint(-1, -1), quint32 uid = -1, int res = Resource::Unknown) :
	MapObject(pos, uid, MapObj::Resource), type(res), count(0) {}

    QString	object(void) const { return "resource"; }
    MapObject*	copy(void) const { return new MapResource(*this); }
};

struct MapMonster : public MapObject
{
    int		type;
    int		count;
    int		condition;

    MapMonster(const QPoint & pos = QPoint(-1, -1), quint32 uid = -1, int mons = Monster::None) :
	MapObject(pos, uid, MapObj::Monster), type(mons), count(0), condition(-1) {}

    QString	object(void) const { return "monster"; }
    MapObject*	copy(void) const { return new MapMonster(*this); }
    void	updateInfo(const mp2til_t &);
    int		monster(void) const { return type; }
};

struct MapArtifact : public MapObject
{
    int		type;
    int		spell;
    int		condition;

    MapArtifact(const QPoint & pos = QPoint(-1, -1), quint32 uid = -1, int art = Artifact::None) :
	MapObject(pos, uid, MapObj::Artifact), type(art), spell(Spell::None), condition(-1) {}

    QString	object(void) const { return "artifact"; }
    MapObject*	copy(void) const { return new MapArtifact(*this); }
    void	updateInfo(const mp2til_t &);
    int		artifact(void) const { return type; }
};

struct ActionSimple;

class SharedActionSimple : public QSharedPointer<ActionSimple>
{
public:
    SharedActionSimple(ActionSimple* ptr) : QSharedPointer<ActionSimple>(ptr) {}

    int	type(void) const;
};

class MapActionList : public QList<SharedActionSimple>
{
};

struct MapActions : public MapObject
{
    enum { DefaultAction = 0, Access, Message, Resources, Artifact, Troops, Morale, Luck, Experience, Skill, Unknown };
    static QString transcribe(int);

    MapActions(const QPoint & pos = QPoint(-1, -1), quint32 uid = -1);
    QString	object(void) const { return "actions"; }
    MapObject*	copy(void) const { return new MapActions(*this); }
    bool	isDefault(void) const;

    MapActionList list;
};

struct ActionSimple
{
    int type;

    ActionSimple(int v = MapActions::DefaultAction) : type(v) {}
    virtual ~ActionSimple() {}
};

struct ActionMessage : public ActionSimple
{
    QString message;

    ActionMessage() : ActionSimple(MapActions::Message) {}
    ActionMessage(const QString & m) : ActionSimple(MapActions::Message), message(m) {}
};

Q_DECLARE_METATYPE(ActionMessage);

struct ActionDefault : public ActionSimple
{
    ActionMessage msg;
    bool result;

    ActionDefault() : ActionSimple(MapActions::DefaultAction), result(true) {}
    ActionDefault(const QString & m, bool v) : ActionSimple(MapActions::DefaultAction), msg(m), result(v) {}
};

Q_DECLARE_METATYPE(ActionDefault);

struct ActionAccess : public ActionSimple
{
    ActionMessage msg;
    AccessResult access;

    ActionAccess() : ActionSimple(MapActions::Access) {}
    ActionAccess(const QString & m, const AccessResult & v) : ActionSimple(MapActions::Access), msg(m), access(v) {}
};

Q_DECLARE_METATYPE(ActionAccess);

struct ActionResources : public ActionSimple
{
    ActionMessage msg;
    Resources resources;

    ActionResources() : ActionSimple(MapActions::Resources) {}
    ActionResources(const QString & m, const Resources & v) : ActionSimple(MapActions::Resources), msg(m), resources(v) {}
};

Q_DECLARE_METATYPE(ActionResources);

struct ActionArtifact : public ActionSimple
{
    ActionMessage msg;
    int artifact;
    int spell;

    ActionArtifact() : ActionSimple(MapActions::Artifact), artifact(Artifact::None), spell(Spell::None) {}
    ActionArtifact(const QString & m, int v) : ActionSimple(MapActions::Artifact), msg(m), artifact(v), spell(Spell::None) {}
};

Q_DECLARE_METATYPE(ActionArtifact);

struct DayEvent
{
    Resources	resources;
    bool	allowComputer;
    int		dayFirstOccurent;
    int		daySubsequentOccurrences;
    int		colors;
    QString	message;

    DayEvent() : allowComputer(false), dayFirstOccurent(0), daySubsequentOccurrences(0), colors(0) {}
    DayEvent(const mp2dayevent_t &);

    QString	header(void) const;
};

Q_DECLARE_METATYPE(DayEvent);

struct Rumor : public QString
{
    Rumor(const mp2rumor_t & mp2) : QString(mp2.text) {}
};

class SharedMapObject : public QSharedPointer<MapObject>
{
public:
    SharedMapObject(MapObject* ptr) : QSharedPointer<MapObject>(ptr) {}

    bool		operator== (const QPoint & pt) const { return data() && pt == data()->pos(); }
    bool		operator== (quint32 uid) const { return data() && uid == data()->uid(); }
};

class MapObjects : public QList<SharedMapObject>
{
public:
    MapObjects();

    SharedMapObject		find(const QPoint &, bool last = false) const;
    QList<SharedMapObject>	list(int types) const;
    QMap<quint32, quint32>	importObjects(const MapObjects &, const QRect &, const QPoint &, quint32);

    void			remove(const QPoint &);
    void			remove(int uid);
};

class DayEvents : public QList<DayEvent>
{
public:
    DayEvents();
};

class TavernRumors : public QStringList
{
public:
    TavernRumors(){}
};

struct CompositeObjectCursor : public CompositeObject
{
    QPoint		scenePos;
    QPoint		centerOffset;
    QPixmap		objectArea;
    QPixmap		passableMap;
    bool		valid;

    CompositeObjectCursor() : valid(false) {}
    CompositeObjectCursor(const CompositeObject &);

    void		move(const MapTile &);
    void		reset(void);
    bool		isValid(void) const;
    void		paint(QPainter &, const QPoint &, bool allow);
    QRect		area(void) const;
    QPoint		center(void) const;
};

namespace Conditions
{
    enum { Wins = 0x1000, CaptureTown = 0x1001, DefeatHero = 0x1002, FindArtifact = 0x1003, SideWins = 0x1004, AccumulateGold = 0x1005,
	    CompAlsoWins = 0x0100, AllowNormalVictory = 0x0200,
	    Loss = 0x2000, LoseTown = 0x2001, LoseHero = 0x2002, OutTime = 0x2003 };
}

struct GameCondition : public QPair<int, QVariant>
{
    GameCondition(int cond, const QVariant & val = QVariant()) : QPair<int, QVariant>(cond, val) {}

    void		set(int cond, const QVariant & val = QVariant()) { first = cond; second = val; }
    int			index(void) const { return 0x000000FF & first; }
    const QVariant &	variant(void) const { return second; }
    QString		variantString(void) const;
};

struct CondWins : public GameCondition
{
    CondWins() : GameCondition(Conditions::Wins){}

    bool		allowNormalVictory(void) const { return Conditions::AllowNormalVictory & first; }
    bool		compAlsoWins(void) const { return Conditions::CompAlsoWins & first; }
    int			condition(void) const { return Conditions::Wins | index(); }

    void		setAllowNormalVictory(bool f) { if(f) first |= Conditions::AllowNormalVictory; else first &= ~Conditions::AllowNormalVictory; }
    void		setCompAlsoWins(bool f) { if(f) first |= Conditions::CompAlsoWins; else first &= ~Conditions::CompAlsoWins; }
};

struct CondLoss : public GameCondition
{
    CondLoss() : GameCondition(Conditions::Loss){}

    int			condition(void) const { return Conditions::Loss | index(); }
};

struct ListStringPos : public QList< QPair<QString, QPoint> >
{
    ListStringPos() {}
};

QDomElement & operator<< (QDomElement &, const QSize &);
QDomElement & operator>> (QDomElement &, QSize &);

QDomElement & operator<< (QDomElement &, const QPoint &);
QDomElement & operator>> (QDomElement &, QPoint &);

QDomElement & operator<< (QDomElement &, const Resources &);
QDomElement & operator>> (QDomElement &, Resources &);

QDomElement & operator<< (QDomElement &, const DayEvents &);
QDomElement & operator>> (QDomElement &, DayEvents &);

QDomElement & operator<< (QDomElement &, const DayEvent &);
QDomElement & operator>> (QDomElement &, DayEvent &);

QDomElement & operator<< (QDomElement &, const TavernRumors &);
QDomElement & operator>> (QDomElement &, TavernRumors &);

QDomElement & operator<< (QDomElement &, const MapObjects &);
QDomElement & operator>> (QDomElement &, MapObjects &);

QDomElement & operator<< (QDomElement &, const MapSign &);
QDomElement & operator>> (QDomElement &, MapSign &);

QDomElement & operator<< (QDomElement &, const MapEvent &);
QDomElement & operator>> (QDomElement &, MapEvent &);

QDomElement & operator<< (QDomElement &, const MapSphinx &);
QDomElement & operator>> (QDomElement &, MapSphinx &);

QDomElement & operator<< (QDomElement &, const MapActions &);
QDomElement & operator>> (QDomElement &, MapActions &);

QDomElement & operator<< (QDomElement &, const MapHero &);
QDomElement & operator>> (QDomElement &, MapHero &);

QDomElement & operator<< (QDomElement &, const MapTown &);
QDomElement & operator>> (QDomElement &, MapTown &);

QDomElement & operator<< (QDomElement &, const MapResource &);
QDomElement & operator>> (QDomElement &, MapResource &);

QDomElement & operator<< (QDomElement &, const MapArtifact &);
QDomElement & operator>> (QDomElement &, MapArtifact &);

QDomElement & operator<< (QDomElement &, const MapMonster &);
QDomElement & operator>> (QDomElement &, MapMonster &);

QDomElement & operator<< (QDomElement &, const Troops &);
QDomElement & operator>> (QDomElement &, Troops &);

QDomElement & operator<< (QDomElement &, const Skills &);
QDomElement & operator>> (QDomElement &, Skills &);

QDataStream & operator<< (QDataStream &, const GameCondition &);
QDataStream & operator>> (QDataStream &, GameCondition &);

QDomElement & operator<< (QDomElement &, const ActionMessage &);
QDomElement & operator>> (QDomElement &, ActionMessage &);

QDomElement & operator<< (QDomElement &, const ActionDefault &);
QDomElement & operator>> (QDomElement &, ActionDefault &);

QDomElement & operator<< (QDomElement &, const ActionAccess &);
QDomElement & operator>> (QDomElement &, ActionAccess &);

QDomElement & operator<< (QDomElement &, const ActionResources &);
QDomElement & operator>> (QDomElement &, ActionResources &);

QDomElement & operator<< (QDomElement &, const ActionArtifact &);
QDomElement & operator>> (QDomElement &, ActionArtifact &);

#endif
