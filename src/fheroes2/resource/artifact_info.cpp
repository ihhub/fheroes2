/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022 - 2024                                             *
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

#include "artifact_info.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <sstream>

#include "artifact.h"
#include "game_static.h"
#include "spell.h"
#include "tools.h"
#include "translations.h"

namespace
{
    // This container was chosen intentionally to support future modding.
    std::vector<fheroes2::ArtifactData> artifactData;

    void populateArtifactData()
    {
        assert( artifactData.empty() );

        artifactData = {
            { gettext_noop( "Invalid Artifact" ), gettext_noop( "Invalid Artifact" ), nullptr, {}, {} },
            // Ultimate artifacts do not have discovery event description as they are not supposed to be found on map.
            { gettext_noop( "Ultimate Book of Knowledge" ), gettext_noop( "The %{name} increases the hero's knowledge by %{count}." ), nullptr, {}, {} },
            { gettext_noop( "Ultimate Sword of Dominion" ), gettext_noop( "The %{name} increases the hero's attack skill by %{count}." ), nullptr, {}, {} },
            { gettext_noop( "Ultimate Cloak of Protection" ), gettext_noop( "The %{name} increases the hero's defense skill by %{count}." ), nullptr, {}, {} },
            { gettext_noop( "Ultimate Wand of Magic" ), gettext_noop( "The %{name} increases the hero's spell power by %{count}." ), nullptr, {}, {} },
            { gettext_noop( "Ultimate Shield" ), gettext_noop( "The %{name} increases the hero's attack and defense skills by %{count} each." ), nullptr, {}, {} },
            { gettext_noop( "Ultimate Staff" ), gettext_noop( "The %{name} increases the hero's spell power and knowledge by %{count} each." ), nullptr, {}, {} },
            { gettext_noop( "Ultimate Crown" ), gettext_noop( "The %{name} increases each of the hero's basic skills by %{count} points." ), nullptr, {}, {} },
            { gettext_noop( "Golden Goose" ), gettext_noop( "The %{name} brings in an income of %{count} gold per day." ), nullptr, {}, {} },

            { gettext_noop( "Arcane Necklace of Magic" ),
              gettext_noop( "The %{name} increases the hero's spell power by %{count}." ),
              gettext_noop( "After rescuing a Sorceress from a cursed tomb, she rewards your heroism with an exquisite jeweled necklace." ),
              {},
              {} },
            { gettext_noop( "Caster's Bracelet of Magic" ),
              gettext_noop( "The %{name} increases the hero's spell power by %{count}." ),
              gettext_noop(
                  "While searching through the rubble of a caved-in mine, you free a group of trapped Dwarves. Grateful, the leader gives you a golden bracelet." ),
              {},
              {} },
            { gettext_noop( "Mage's Ring of Power" ),
              gettext_noop( "The %{name} increases the hero's spell power by %{count}." ),
              gettext_noop(
                  "A cry of pain leads you to a Centaur, caught in a trap. Upon setting the creature free, he hands you a small pouch. Emptying the contents, you find a dazzling jeweled ring." ),
              {},
              {} },
            { gettext_noop( "Witch's Broach of Magic" ),
              gettext_noop( "The %{name} increases the hero's spell power by %{count}." ),
              gettext_noop(
                  "Alongside the remains of a burnt witch lies a beautiful broach, intricately designed. Approaching the corpse with caution, you add the broach to your inventory." ),
              {},
              {} },
            { gettext_noop( "Medal of Valor" ),
              gettext_noop( "The %{name} increases the morale of the hero's army by %{count}." ),
              gettext_noop( "Freeing a virtuous maiden from the clutches of an evil overlord, you are granted a Medal of Valor by the King's herald." ),
              {},
              {} },
            { gettext_noop( "Medal of Courage" ),
              gettext_noop( "The %{name} increases the morale of the hero's army by %{count}." ),
              gettext_noop(
                  "After saving a young boy from a vicious pack of Wolves, you return him to his father's manor. The grateful nobleman awards you with a Medal of Courage." ),
              {},
              {} },
            { gettext_noop( "Medal of Honor" ),
              gettext_noop( "The %{name} increases the morale of the hero's army by %{count}." ),
              gettext_noop( "After freeing a princess of a neighboring kingdom from the evil clutches of despicable slavers, she awards you with a Medal of Honor." ),
              {},
              {} },
            { gettext_noop( "Medal of Distinction" ),
              gettext_noop( "The %{name} increases the morale of the hero's army by %{count}." ),
              gettext_noop(
                  "Ridding the countryside of the hideous Minotaur who made a sport of eating noblemen's Knights, you are honored with the Medal of Distinction." ),
              {},
              {} },
            { gettext_noop( "Fizbin of Misfortune" ),
              gettext_noop( "The %{name} greatly decreases the morale of the hero's army by %{count}." ),
              gettext_noop(
                  "You stumble upon a medal lying alongside the empty road. Adding the medal to your inventory, you become aware that you have acquired the undesirable Fizbin of Misfortune, greatly decreasing your army's morale." ),
              {},
              {} },
            { gettext_noop( "Thunder Mace of Dominion" ),
              gettext_noop( "The %{name} increases the hero's attack skill by %{count}." ),
              gettext_noop( "During a sudden storm, a bolt of lightning strikes a tree, splitting it. Inside the tree you find a mysterious mace." ),
              {},
              {} },
            { gettext_noop( "Armored Gauntlets of Protection" ),
              gettext_noop( "The %{name} increase the hero's defense skill by %{count}." ),
              gettext_noop(
                  "You encounter the infamous Black Knight! After a grueling duel ending in a draw, the Knight, out of respect, offers you a pair of armored gauntlets." ),
              {},
              {} },
            { gettext_noop( "Defender Helm of Protection" ),
              gettext_noop( "The %{name} increases the hero's defense skill by %{count}." ),
              gettext_noop( "A glint of golden light catches your eye. Upon further investigation, you find a golden helm hidden under a bush." ),
              {},
              {} },
            { gettext_noop( "Giant Flail of Dominion" ),
              gettext_noop( "The %{name} increases the hero's attack skill by %{count}." ),
              gettext_noop(
                  "A clumsy Giant has killed himself with his own flail. Knowing your superior skill with this weapon, you confidently remove the spectacular flail from the fallen Giant." ),
              {},
              {} },
            { gettext_noop( "Ballista of Quickness" ),
              gettext_noop( "The %{name} gives the hero's catapult one extra shot per combat round." ),
              gettext_noop( "Walking through the ruins of an ancient walled city, you find the instrument of the city's destruction, an elaborately crafted ballista." ),
              {},
              {} },
            { gettext_noop( "Stealth Shield of Protection" ),
              gettext_noop( "The %{name} increases the hero's defense skill by %{count}." ),
              gettext_noop( "A stone statue of a warrior holds a silver shield. As you remove the shield, the statue crumbles into dust." ),
              {},
              {} },
            { gettext_noop( "Dragon Sword of Dominion" ),
              gettext_noop( "The %{name} increases the hero's attack skill by %{count}." ),
              gettext_noop(
                  "As you are walking along a narrow path, a nearby bush suddenly bursts into flames. Before your eyes the flames become the image of a beautiful woman. She holds out a magnificent sword to you." ),
              {},
              {} },
            { gettext_noop( "Power Axe of Dominion" ),
              gettext_noop( "The %{name} increases the hero's attack skill by %{count}." ),
              gettext_noop(
                  "You see a silver axe embedded deeply in the ground. After several unsuccessful attempts by your army to remove the axe, you tightly grip the handle of the axe and effortlessly pull it free." ),
              {},
              {} },
            { gettext_noop( "Divine Breastplate of Protection" ),
              gettext_noop( "The %{name} increases the hero's defense skill by %{count}." ),
              gettext_noop(
                  "A gang of Rogues is sifting through the possessions of dead warriors. Scaring off the scavengers, you note the Rogues had overlooked a beautiful breastplate." ),
              {},
              {} },
            { gettext_noop( "Minor Scroll of Knowledge" ),
              gettext_noop( "The %{name} increases the hero's knowledge by %{count}." ),
              gettext_noop(
                  "Before you appears a levitating glass case with a scroll, perched upon a bed of crimson velvet. At your touch, the lid opens and the scroll floats into your awaiting hands." ),
              {},
              {} },
            { gettext_noop( "Major Scroll of Knowledge" ),
              gettext_noop( "The %{name} increases the hero's knowledge by %{count}." ),
              gettext_noop(
                  "Visiting a local wiseman, you explain the intent of your journey. He reaches into a sack and withdraws a yellowed scroll and hands it to you." ),
              {},
              {} },
            { gettext_noop( "Superior Scroll of Knowledge" ),
              gettext_noop( "The %{name} increases the hero's knowledge by %{count}." ),
              gettext_noop(
                  "You come across the remains of an ancient Druid. Bones, yellowed with age, peer from the ragged folds of her robe. Searching the robe, you discover a scroll hidden in the folds." ),
              {},
              {} },
            { gettext_noop( "Foremost Scroll of Knowledge" ),
              gettext_noop( "The %{name} increases the hero's knowledge by %{count}." ),
              gettext_noop(
                  "Mangled bones, yellowed with age, peer from the ragged folds of a dead Druid's robe. Searching the robe, you discover a scroll hidden within." ),
              {},
              {} },
            { gettext_noop( "Endless Sack of Gold" ),
              gettext_noop( "The %{name} provides the hero with %{count} gold per day." ),
              gettext_noop(
                  "A little leprechaun dances gleefully around a magic sack. Seeing you approach, he stops in mid-stride. The little man screams and stamps his foot ferociously, vanishing into thin air. Remembering the old leprechaun saying 'Finders Keepers', you grab the sack and leave." ),
              {},
              {} },
            { gettext_noop( "Endless Bag of Gold" ),
              gettext_noop( "The %{name} provides the hero with %{count} gold per day." ),
              gettext_noop(
                  "A noblewoman, separated from her traveling companions, asks for your help. After escorting her home, she rewards you with a bag filled with gold." ),
              {},
              {} },
            { gettext_noop( "Endless Purse of Gold" ),
              gettext_noop( "The %{name} provides the hero with %{count} gold per day." ),
              gettext_noop(
                  "In your travels, you find a leather purse filled with gold that once belonged to a great warrior king who had the ability to transform any inanimate object into gold." ),
              {},
              {} },
            { gettext_noop( "Nomad Boots of Mobility" ),
              gettext_noop( "The %{name} increase the hero's movement on land." ),
              gettext_noop(
                  "A Nomad trader seeks protection from a tribe of Goblins. For your assistance, he gives you a finely crafted pair of boots made from the softest leather. Looking closely, you see fascinating ancient carvings engraved on the leather." ),
              {},
              {} },
            { gettext_noop( "Traveler's Boots of Mobility" ),
              gettext_noop( "The %{name} increase the hero's movement on land." ),
              gettext_noop(
                  "Discovering a pair of beautifully beaded boots made from the finest and softest leather, you thank the anonymous donor and add the boots to your inventory." ),
              {},
              {} },
            { gettext_noop( "Lucky Rabbit's Foot" ),
              gettext_noop( "The %{name} increases the luck of the hero's army by %{count}." ),
              gettext_noop(
                  "A traveling merchant offers you a rabbit's foot, made of gleaming silver fur, for safe passage. The merchant explains the charm will increase your luck in combat." ),
              {},
              {} },
            { gettext_noop( "Golden Horseshoe" ),
              gettext_noop( "The %{name} increases the luck of the hero's army by %{count}." ),
              gettext_noop(
                  "An ensnared Unicorn whinnies in fright. Murmuring soothing words, you set her free. Snorting and stamping her front hoof once, she gallops off. Looking down you see a golden horseshoe." ),
              {},
              {} },
            { gettext_noop( "Gambler's Lucky Coin" ),
              gettext_noop( "The %{name} increases the luck of the hero's army by %{count}." ),
              gettext_noop( "You have captured a mischievous imp who has been terrorizing the region. In exchange for his release, he rewards you with a magical coin." ),
              {},
              {} },
            { gettext_noop( "Four-Leaf Clover" ),
              gettext_noop( "The %{name} increases the luck of the hero's army by %{count}." ),
              gettext_noop( "In the middle of a patch of dead and dry vegetation, to your surprise you find a healthy green four-leaf clover." ),
              {},
              {} },
            { gettext_noop( "True Compass of Mobility" ),
              gettext_noop( "The %{name} increases the hero's movement on land and sea." ),
              gettext_noop( "An old man claiming to be an inventor asks you to try his latest invention. He then hands you a compass." ),
              {},
              {} },
            { gettext_noop( "Sailor's Astrolabe of Mobility" ),
              gettext_noop( "The %{name} increases the hero's movement on sea." ),
              gettext_noop(
                  "An old sea captain is being tortured by Ogres. You save him, and in return he rewards you with a wondrous instrument to measure the distance of a star." ),
              {},
              {} },
            { gettext_noop( "Evil Eye" ),
              gettext_noop( "The %{name} reduces the casting cost of curse spells by half." ),
              gettext_noop(
                  "While venturing into a decrepit hut you find the Skeleton of a long dead witch. Investigation of the remains reveals a glass eye rolling around inside an empty skull." ),
              {},
              {} },
            { gettext_noop( "Enchanted Hourglass" ),
              gettext_noop( "The %{name} extends the duration of all the hero's spells by %{count} turns." ),
              gettext_noop(
                  "A surprise turn in the landscape finds you in the midst of a grisly scene: Vultures picking at the aftermath of a terrible battle. Your cursory search of the remains turns up an enchanted hourglass." ),
              {},
              {} },
            { gettext_noop( "Gold Watch" ),
              gettext_noop( "The %{name} doubles the effectiveness of the hero's hypnotize spells." ),
              gettext_noop(
                  "In reward for helping his cart out of a ditch, a traveling potion salesman gives you a \"magic\" gold watch. Unbeknownst to him, the watch really is magical." ),
              {},
              {} },
            { gettext_noop( "Skullcap" ),
              gettext_noop( "The %{name} halves the casting cost of all mind influencing spells." ),
              gettext_noop(
                  "A brief stop at an improbable rural inn yields an exchange of money, tales, and accidentally, luggage. You find a magical skullcap in your new backpack." ),
              {},
              {} },
            { gettext_noop( "Ice Cloak" ),
              gettext_noop( "The %{name} halves all damage the hero's troops receive from cold spells." ),
              gettext_noop(
                  "Responding to the panicked cries of a damsel in distress, you discover a young woman fleeing from a hungry bear. You slay the beast in the nick of time, and the grateful Sorceress weaves a magic cloak from the bear's hide." ),
              {},
              {} },
            { gettext_noop( "Fire Cloak" ),
              gettext_noop( "The %{name} halves all damage the hero's troops receive from fire spells." ),
              gettext_noop(
                  "You've come upon a fight between a Necromancer and a Paladin. The Necromancer blasts the Paladin with a fire bolt, bringing him to his knees. Acting quickly, you slay the evil one before the final blow. The grateful Paladin gives you the fire cloak that saved him." ),
              {},
              {} },
            { gettext_noop( "Lightning Helm" ),
              gettext_noop( "The %{name} halves all damage the hero's troops receive from lightning spells." ),
              gettext_noop(
                  "A traveling tinker in need of supplies offers you a helm with a thunderbolt design on its top in exchange for food and water. Curious, you accept, and later find out that the helm is magical." ),
              {},
              {} },
            { gettext_noop( "Evercold Icicle" ),
              gettext_noop( "The %{name} causes the hero's cold spells to do %{count} percent more damage to enemy troops." ),
              gettext_noop(
                  "An icicle withstanding the full heat of the noonday sun attracts your attention. Intrigued, you break it off, and find that it does not melt in your hand." ),
              {},
              {} },
            { gettext_noop( "Everhot Lava Rock" ),
              gettext_noop( "The %{name} causes the hero's fire spells to do %{count} percent more damage to enemy troops." ),
              gettext_noop(
                  "Your wanderings bring you into contact with a tribe of ape-like beings using a magical lava rock that never cools to light their fires. You take pity on them and teach them to make fire with sticks. Believing you to be a god, the apes give you their rock." ),
              {},
              {} },
            { gettext_noop( "Lightning Rod" ),
              gettext_noop( "The %{name} causes the hero's lightning spells to do %{count} percent more damage to enemy troops." ),
              gettext_noop(
                  "While waiting out a storm, a lighting bolt strikes a nearby cottage's lightning rod, which melts and falls to the ground. The tip of the rod, however, survives intact and makes your hair stand on end when you touch it. Hmm..." ),
              {},
              {} },
            { gettext_noop( "Snake-Ring" ),
              gettext_noop( "The %{name} halves the casting cost of all of the hero's bless spells." ),
              gettext_noop( "You've found an oddly shaped ring on the finger of a long dead traveler. The ring looks like a snake biting its own tail." ),
              {},
              {} },
            { gettext_noop( "Ankh" ),
              gettext_noop( "The %{name} doubles the effectiveness of all of the hero's resurrect and animate spells." ),
              gettext_noop(
                  "A fierce windstorm reveals the entrance to a buried tomb. Your investigation reveals that the tomb has already been looted, but the thieves overlooked an ankh on a silver chain in the dark." ),
              {},
              {} },
            { gettext_noop( "Book of Elements" ),
              gettext_noop( "The %{name} doubles the effectiveness of all of the hero's summoning spells." ),
              gettext_noop(
                  "You come across a conjurer who begs to accompany you and your army awhile for safety. You agree, and he offers as payment a copy of the book of the elements." ),
              {},
              {} },
            { gettext_noop( "Elemental Ring" ),
              gettext_noop( "The %{name} halves the casting cost of all summoning spells." ),
              gettext_noop(
                  "While pausing to rest, you notice a bobcat climbing a short tree to get at a crow's nest. On impulse, you climb the tree yourself and scare off the cat. When you look in the nest, you find a collection of shiny stones and a ring." ),
              {},
              {} },
            { gettext_noop( "Holy Pendant" ),
              gettext_noop( "The %{name} makes all of the hero's troops immune to curse spells." ),
              gettext_noop(
                  "In your wanderings you come across a hermit living in a small, tidy hut. Impressed with your mission, he takes time out from his meditations to bless and give you a charm against curses." ),
              {},
              {} },
            { gettext_noop( "Pendant of Free Will" ),
              gettext_noop( "The %{name} makes all of the hero's troops immune to hypnotize spells." ),
              gettext_noop(
                  "Responding to cries for help, you find river Sprites making a sport of dunking an old man. Feeling vengeful, you rescue the man and drag a Sprite onto dry land for awhile. The Sprite, uncomfortable in the air, gives you a magic pendant to let him go." ),
              {},
              {} },
            { gettext_noop( "Pendant of Life" ),
              gettext_noop( "The %{name} makes all of the hero's troops immune to death spells." ),
              gettext_noop(
                  "A brief roadside encounter with a small caravan and a game of knucklebones wins a magic pendant. Its former owner says that it protects from Necromancers' death spells." ),
              {},
              {} },
            { gettext_noop( "Serenity Pendant" ),
              gettext_noop( "The %{name} makes all of the hero's troops immune to berserk spells." ),
              gettext_noop( "The sounds of combat draw you to the scene of a fight between an old Barbarian and an eight-headed Hydra. Your timely intervention swings the battle in favor of the man, and he rewards you with a pendant he used to use to calm his mind for battle." ),
              {},
              {} },
            { gettext_noop( "Seeing-eye Pendant" ),
              gettext_noop( "The %{name} makes all of the hero's troops immune to blindness spells." ),
              gettext_noop(
                  "You come upon a very old woman, long blind from cataracts and dying alone. You tend to her final needs and promise a proper burial. Grateful, she gives you a magic pendant emblazoned with a stylized eye. It lets you see with your eyes closed." ),
              {},
              {} },
            { gettext_noop( "Kinetic Pendant" ),
              gettext_noop( "The %{name} makes all of the hero's troops immune to paralyze spells." ),
              gettext_noop(
                  "You come across a golem wearing a glowing pendant and blocking your way. Acting on a hunch, you cut the pendant from its neck. Deprived of its power source, the golem breaks down, leaving you with the magical pendant." ),
              {},
              {} },
            { gettext_noop( "Pendant of Death" ),
              gettext_noop( "The %{name} makes all of the hero's troops immune to holy spells." ),
              gettext_noop(
                  "A quick and deadly battle with a Necromancer wins you his magical pendant. Later, a Wizard tells you that the pendant protects undead under your control from holy word spells." ),
              {},
              {} },
            { gettext_noop( "Wand of Negation" ),
              gettext_noop( "The %{name} makes all of the hero's troops immune to dispel magic spells." ),
              gettext_noop(
                  "You meet an old Wizard friend of yours traveling in the opposite direction. He presents  you with a gift: A wand that prevents the use of the dispel magic spell on your allies." ),
              {},
              {} },
            { gettext_noop( "Golden Bow" ),
              gettext_noop( "The %{name} eliminates the %{count} percent penalty for the hero's troops shooting past obstacles (e.g. castle walls)." ),
              gettext_noop( "A chance meeting with a famous Archer finds you in a game of knucklebones pitting his bow against your horse. You win." ),
              {},
              {} },
            { gettext_noop( "Telescope" ),
              gettext_noop( "The %{name} increases the amount of terrain the hero reveals when adventuring by %{count} extra square." ),
              gettext_noop(
                  "A merchant from far away lands trades you a new invention of his people for traveling supplies. It makes distant objects appear closer, and he calls it...\n\na telescope." ),
              {},
              {} },
            { gettext_noop( "Statesman's Quill" ),
              gettext_noop( "The %{name} reduces the cost of surrender to %{count} percent of the total cost of troops the hero has in their army." ),
              gettext_noop(
                  "You pause to help a diplomat with a broken axle fix his problem. In gratitude, he gives you a writing quill with magical properties which he says will \"help people see things your way\"." ),
              {},
              {} },
            { gettext_noop( "Wizard's Hat" ),
              gettext_noop( "The %{name} increases the duration of the hero's spells by %{count} turns." ),
              gettext_noop(
                  "You see a Wizard fleeing from a Griffin and riding like the wind. The Wizard opens a portal and rides through, getting his hat knocked off by the edge of the gate. The Griffin follows; the gate closes. You pick the hat up, dust it off, and put it on." ),
              {},
              {} },
            { gettext_noop( "Power Ring" ),
              gettext_noop( "The %{name} returns %{count} extra spell points per day to the hero." ),
              gettext_noop(
                  "You find a small tree that closely resembles the great Warlock Carnauth with a ring around one of its twigs. Scraps of clothing and rotting leather lead you to suspect that it IS Carnauth, transformed. Since you can't help him, you take the magic ring." ),
              {},
              {} },
            { gettext_noop( "Ammo Cart" ),
              gettext_noop( "The %{name} provides endless ammunition for all of the hero's troops that shoot." ),
              gettext_noop(
                  "An ammunition cart in the middle of an old battlefield catches your eye. Inspection shows it to be in good working order, so  you take it along." ),
              {},
              {} },
            { gettext_noop( "Tax Lien" ),
              gettext_noop( "The %{name} costs the hero %{count} gold pieces per day." ),
              gettext_noop(
                  "Your big spending habits have earned you a massive tax bill that you can't hope to pay. The tax man takes pity and agrees to only take 250 gold a day from your account for life. Check here if you want one dollar to go to the presidential campaign election fund." ),
              {},
              {} },
            { gettext_noop( "Hideous Mask" ),
              gettext_noop( "The %{name} prevents all 'wandering' armies from joining the hero." ),
              gettext_noop(
                  "Your looting of the grave of Sinfilas Gardolad, the famous shapeshifting Warlock, unearths his fabled mask. Trembling, you put it on and it twists your visage into an awful grimace! Oh no! It's actually the hideous mask of Gromluck Greene, and you are stuck with it." ),
              {},
              {} },
            { gettext_noop( "Endless Pouch of Sulfur" ),
              gettext_noop( "The %{name} provides %{count} unit of sulfur per day." ),
              gettext_noop(
                  "You visit an alchemist who, upon seeing your army, is swayed by the righteousness of your cause. The newly loyal subject gives you his endless pouch of sulfur to help with the war effort." ),
              {},
              {} },
            { gettext_noop( "Endless Vial of Mercury" ),
              gettext_noop( "The %{name} provides %{count} unit of mercury per day." ),
              gettext_noop(
                  "A brief stop at a hastily abandoned Wizard's tower turns up a magical vial of mercury that always has a little left on the bottom. Recognizing a treasure when you see one, you cap it and slip it in your pocket." ),
              {},
              {} },
            { gettext_noop( "Endless Pouch of Gems" ),
              gettext_noop( "The %{name} provides %{count} unit of gems per day." ),
              gettext_noop(
                  "A short rainstorm brings forth a rainbow...and you can see the end of it. Riding quickly, you seize the pot of gold you find there. The leprechaun who owns it, unable to stop you from taking it, offers an endless pouch of gems for the return of his gold. You accept." ),
              {},
              {} },
            { gettext_noop( "Endless Cord of Wood" ),
              gettext_noop( "The %{name} provides %{count} unit of wood per day." ),
              gettext_noop(
                  "Pausing to rest and light a cook fire, you pull wood out of a nearby pile of dead wood. As you keep pulling wood from the pile, you notice that it doesn't shrink. You realize to your delight that the wood is enchanted, so you take it along." ),
              {},
              {} },
            { gettext_noop( "Endless Cart of Ore" ),
              gettext_noop( "The %{name} provides %{count} unit of ore per day." ),
              gettext_noop(
                  "You've found a Goblin weapon smithy making weapons for use against humans. With a tremendous yell you and your army descend upon their camp and drive them away. A search finds a magic ore cart that never runs out of iron." ),
              {},
              {} },
            { gettext_noop( "Endless Pouch of Crystal" ),
              gettext_noop( "The %{name} provides %{count} unit of crystal per day." ),
              gettext_noop(
                  "Taking shelter from a storm in a small cave, you notice a small patch of crystal in one corner. Curious, you break a piece off and notice that the original crystal grows the lost piece back. You decide to stuff the entire patch into a pouch and take it with you." ),
              {},
              {} },
            { gettext_noop( "Spiked Helm" ),
              gettext_noop( "The %{name} increases the hero's attack and defense skills by %{count} each." ),
              gettext_noop(
                  "Your army is ambushed by a small tribe of wild (and none too bright) Orcs. You fend them off easily and the survivors flee in all directions. One of the Orcs was wearing a polished spiked helm. Figuring it will make a good souvenir, you take it." ),
              {},
              {} },
            { gettext_noop( "Spiked Shield" ),
              gettext_noop( "The %{name} increases the hero's attack and defense skills by %{count} each." ),
              gettext_noop( "You come upon a bridge spanning a dry gully. Before you can cross, a Troll steps out from under the bridge and demands payment before it will permit you to pass. You refuse, and the Troll charges, forcing you to slay it. You take its spiked shield as a trophy." ),
              {},
              {} },
            { gettext_noop( "White Pearl" ),
              gettext_noop( "The %{name} increases the hero's spell power and knowledge by %{count} each." ),
              gettext_noop( "A walk across a dry saltwater lake bed yields an unlikely prize: A white pearl amidst shattered shells and debris." ),
              {},
              {} },
            { gettext_noop( "Black Pearl" ),
              gettext_noop( "The %{name} increases the hero's spell power and knowledge by %{count} each." ),
              gettext_noop(
                  "Rumors of a Griffin of unusual size preying upon the countryside lead you to its cave lair. A quick, brutal fight dispatches the beast, and a search of its foul nest turns up a huge black pearl." ),
              {},
              {} },

            // Magic Book cannot be located on the original map. The Resurrection map format allows to place it on the map.
            { gettext_noop( "Magic Book" ),
              gettext_noop( "The %{name} enables the hero to cast spells." ),
              gettext_noop(
                  "A young man approaches you: \"My Lord, allow me to show you my latest invention for spreading knowledge!\" You follow the man into his workshop and immediately observe a large apparatus with levers and cranks. \"This here is it!\" he says eagerly, \"The Printing Press.\" And before you get to say a word, he hands you a Magic Book." ),
              {},
              {} },

            // This artifact is only used in Editor for the special victory conditions.
            { gettext_noop( "Ultimate Artifact" ), gettext_noop( "Victory can be achieved by finding any Ultimate Artifact." ), nullptr, {}, {} },

            // These are artifacts used only for original map editor (?).
            { gettext_noop( "Dummy 2" ), gettext_noop( "The reserved artifact." ), nullptr, {}, {} },
            { gettext_noop( "Dummy 3" ), gettext_noop( "The reserved artifact." ), nullptr, {}, {} },
            { gettext_noop( "Dummy 4" ), gettext_noop( "The reserved artifact." ), nullptr, {}, {} },

            { gettext_noop( "Spell Scroll" ),
              gettext_noop( "This %{name} gives the hero the ability to cast the %{spell} spell if the hero has a Magic Book." ),
              gettext_noop(
                  "You find an elaborate container which houses an old vellum scroll. The runes on the container are very old, and the artistry with which it was put together is stunning. As you pull the scroll out, you feel imbued with magical power." ),
              {},
              {} },
            { gettext_noop( "Arm of the Martyr" ),
              gettext_noop( "The %{name} increases the hero's spell power by %{count} but adds the undead morale penalty." ),
              gettext_noop(
                  "One of the less intelligent members of your party picks up an arm off of the ground. Despite its missing a body, it is still moving. Your troops find the dismembered arm repulsive, but you cannot bring yourself to drop it: it seems to hold some sort of magical power that influences your decision making." ),
              {},
              {} },
            { gettext_noop( "Breastplate of Anduran" ),
              gettext_noop( "The %{name} increases the hero's defense by %{count}." ),
              gettext_noop(
                  "You come upon a sign. It reads: \"Here lies the body of Anduran. Bow and swear fealty, and you shall be rewarded.\" You decide to do as it says. As you stand up, you feel a coldness against your skin. Looking down, you find that you are suddenly wearing a gleaming, ornate breastplate." ),
              {},
              {} },
            { gettext_noop( "Broach of Shielding" ),
              gettext_noop( "The %{name} provides %{count} percent protection from Armageddon and Elemental Storm, but decreases spell power by 2." ),
              gettext_noop(
                  "A kindly Sorceress thinks that your army's defenses could use a magical boost. She offers to enchant the Broach that you wear on your cloak, and you accept." ),
              {},
              {} },
            { gettext_noop( "Battle Garb of Anduran" ),
              gettext_noop(
                  "The %{name} combines the powers of the three Anduran artifacts. It provides maximum luck and morale for the hero's troops and gives the hero the Town Portal spell." ),
              gettext_noop(
                  "Out of pity for a poor peasant, you purchase a chest of old junk they are hawking for too much gold. Later, as you search through it, you find it contains the 3 pieces of the legendary battle garb of Anduran!" ),
              {},
              {} },
            { gettext_noop( "Crystal Ball" ),
              gettext_noop( "The %{name} lets the hero get more specific information about monsters, enemy heroes, and castles nearby the hero." ),
              gettext_noop(
                  "You come upon a caravan of gypsies who are feasting and fortifying their bodies with mead. They call you forward and say \"If you prove that you can dance the Rama-Buta, we will reward you.\" You don't know it, but try anyway. They laugh hysterically, but admire your bravery, giving you a Crystal Ball." ),
              {},
              {} },
            { gettext_noop( "Heart of Fire" ),
              gettext_noop( "The %{name} provides %{count} percent protection from fire, but doubles the damage taken from cold." ),
              gettext_noop(
                  "You enter a recently burned glade and come upon a Fire Elemental sitting atop a rock. It looks up, its flaming face contorted in a look of severe pain. It then tosses a glowing object at you. You put up your hands to block it, but it passes right through them and sears itself into your chest." ),
              {},
              {} },
            { gettext_noop( "Heart of Ice" ),
              gettext_noop( "The %{name} provides %{count} percent protection from cold, but doubles the damage taken from fire." ),
              gettext_noop(
                  "Suddenly, a biting coldness engulfs your body. You seize up, falling from your horse. The pain subsides, but you still feel as if your chest is frozen. As you pick yourself up off of the ground, you hear hearty laughter. You turn around just in time to see a Frost Giant run off into the woods and disappear." ),
              {},
              {} },
            { gettext_noop( "Helmet of Anduran" ),
              gettext_noop( "The %{name} increases the hero's spell power by %{count}." ),
              gettext_noop(
                  "You spy a gleaming object poking up out of the ground. You send a member of your party over to investigate. He comes back with a golden helmet in his hands. You realize that it must be the helmet of the legendary Anduran, the only man who was known to wear solid gold armor." ),
              {},
              {} },
            { gettext_noop( "Holy Hammer" ), gettext_noop( "The %{name} increases the hero's attack skill by %{count}." ), gettext_noop( "You come upon a battle where a Paladin has been mortally wounded by a group of Zombies. He asks you to take his hammer and finish what he started. As you pick it up, it begins to hum, and then everything becomes a blur. The Zombies lie dead, the hammer dripping with blood. You strap it to your belt." ), {}, {} },
            { gettext_noop( "Legendary Scepter" ),
              gettext_noop( "The %{name} adds %{count} points to all attributes." ),
              gettext_noop(
                  "Upon cresting a small hill, you come upon a ridiculous looking sight. A Sprite is attempting to carry a Scepter that is almost as big as it is. Trying not to laugh, you ask, \"Need help?\" The Sprite glares at you and answers: \"You think this is funny? Fine. You can carry it. I much prefer flying anyway.\"" ),
              {},
              {} },
            { gettext_noop( "Masthead" ),
              gettext_noop( "The %{name} boosts the hero's troops' luck and morale by %{count} each in sea combat." ),
              gettext_noop(
                  "An old seaman tells you a tale of an enchanted masthead that he used in his youth to rally his crew during times of trouble. He then hands you a faded map that shows where he hid it. After much exploring, you find it stashed underneath a nearby dock." ),
              {},
              {} },
            { gettext_noop( "Sphere of Negation" ),
              gettext_noop( "The %{name} disables all spell casting, for both sides, in combat." ),
              gettext_noop(
                  "You stop to help a Peasant catch a runaway mare. To show his gratitude, he hands you a tiny sphere. As soon as you grasp it, you feel the magical energy drain from your limbs..." ),
              {},
              {} },
            { gettext_noop( "Staff of Wizardry" ),
              gettext_noop( "The %{name} boosts the hero's spell power by %{count}." ),
              gettext_noop(
                  "While out scaring up game, your troops find a mysterious staff levitating about three feet off of the ground. They hand it to you, and you notice an inscription. It reads: \"Brains best brawn and magic beats might. Heed my words, and you'll win every fight.\"" ),
              {},
              {} },
            { gettext_noop( "Sword Breaker" ),
              gettext_noop( "The %{name} increases the hero's defense by %{count} and attack by 1." ),
              gettext_noop( "A former Captain of the Guard admires your quest and gives you the enchanted Sword Breaker that he relied on during his tour of duty." ),
              {},
              {} },
            { gettext_noop( "Sword of Anduran" ),
              gettext_noop( "The %{name} increases the hero's attack skill by %{count}." ),
              gettext_noop(
                  "A Troll stops you and says: \"Pay me 5,000 gold, or the Sword of Anduran will slay you where you stand.\" You refuse. The troll grabs the sword hanging from its belt, screams in pain, and runs away. Picking up the fabled sword, you give thanks that half-witted Trolls tend to grab the wrong end of sharp objects." ),
              {},
              {} },
            { gettext_noop( "Spade of Necromancy" ),
              gettext_noop( "The %{name} gives the hero increased necromancy skill." ),
              gettext_noop(
                  "A dirty shovel has been thrust into a dirt mound nearby. Upon investigation, you discover it to be the enchanted shovel of the Gravediggers, long thought lost by mortals." ),
              {},
              {} },
        };

        assert( artifactData.size() == ( Artifact::ARTIFACT_COUNT ) );

        // Artifact bonus and curse 'value' is signed integer. However, it should not be negative.

        artifactData[Artifact::UNKNOWN].bonuses.emplace_back( fheroes2::ArtifactBonusType::NONE );

        artifactData[Artifact::ULTIMATE_BOOK].bonuses.emplace_back( fheroes2::ArtifactBonusType::KNOWLEDGE_SKILL, 12 );

        artifactData[Artifact::ULTIMATE_SWORD].bonuses.emplace_back( fheroes2::ArtifactBonusType::ATTACK_SKILL, 12 );

        artifactData[Artifact::ULTIMATE_CLOAK].bonuses.emplace_back( fheroes2::ArtifactBonusType::DEFENCE_SKILL, 12 );

        artifactData[Artifact::ULTIMATE_WAND].bonuses.emplace_back( fheroes2::ArtifactBonusType::SPELL_POWER_SKILL, 12 );

        artifactData[Artifact::ULTIMATE_SHIELD].bonuses.emplace_back( fheroes2::ArtifactBonusType::ATTACK_SKILL, 6 );
        artifactData[Artifact::ULTIMATE_SHIELD].bonuses.emplace_back( fheroes2::ArtifactBonusType::DEFENCE_SKILL, 6 );

        artifactData[Artifact::ULTIMATE_STAFF].bonuses.emplace_back( fheroes2::ArtifactBonusType::SPELL_POWER_SKILL, 6 );
        artifactData[Artifact::ULTIMATE_STAFF].bonuses.emplace_back( fheroes2::ArtifactBonusType::KNOWLEDGE_SKILL, 6 );

        artifactData[Artifact::ULTIMATE_CROWN].bonuses.emplace_back( fheroes2::ArtifactBonusType::ATTACK_SKILL, 4 );
        artifactData[Artifact::ULTIMATE_CROWN].bonuses.emplace_back( fheroes2::ArtifactBonusType::DEFENCE_SKILL, 4 );
        artifactData[Artifact::ULTIMATE_CROWN].bonuses.emplace_back( fheroes2::ArtifactBonusType::SPELL_POWER_SKILL, 4 );
        artifactData[Artifact::ULTIMATE_CROWN].bonuses.emplace_back( fheroes2::ArtifactBonusType::KNOWLEDGE_SKILL, 4 );

        artifactData[Artifact::GOLDEN_GOOSE].bonuses.emplace_back( fheroes2::ArtifactBonusType::GOLD_INCOME, 10000 );

        artifactData[Artifact::ARCANE_NECKLACE].bonuses.emplace_back( fheroes2::ArtifactBonusType::SPELL_POWER_SKILL, 4 );

        artifactData[Artifact::CASTER_BRACELET].bonuses.emplace_back( fheroes2::ArtifactBonusType::SPELL_POWER_SKILL, 2 );

        artifactData[Artifact::MAGE_RING].bonuses.emplace_back( fheroes2::ArtifactBonusType::SPELL_POWER_SKILL, 2 );

        artifactData[Artifact::WITCHES_BROACH].bonuses.emplace_back( fheroes2::ArtifactBonusType::SPELL_POWER_SKILL, 3 );

        artifactData[Artifact::MEDAL_VALOR].bonuses.emplace_back( fheroes2::ArtifactBonusType::MORALE, 1 );

        artifactData[Artifact::MEDAL_COURAGE].bonuses.emplace_back( fheroes2::ArtifactBonusType::MORALE, 1 );

        artifactData[Artifact::MEDAL_HONOR].bonuses.emplace_back( fheroes2::ArtifactBonusType::MORALE, 1 );

        artifactData[Artifact::MEDAL_DISTINCTION].bonuses.emplace_back( fheroes2::ArtifactBonusType::MORALE, 1 );

        artifactData[Artifact::FIZBIN_MISFORTUNE].curses.emplace_back( fheroes2::ArtifactCurseType::MORALE, 2 );

        artifactData[Artifact::THUNDER_MACE].bonuses.emplace_back( fheroes2::ArtifactBonusType::ATTACK_SKILL, 1 );

        artifactData[Artifact::ARMORED_GAUNTLETS].bonuses.emplace_back( fheroes2::ArtifactBonusType::DEFENCE_SKILL, 1 );

        artifactData[Artifact::DEFENDER_HELM].bonuses.emplace_back( fheroes2::ArtifactBonusType::DEFENCE_SKILL, 1 );

        artifactData[Artifact::GIANT_FLAIL].bonuses.emplace_back( fheroes2::ArtifactBonusType::ATTACK_SKILL, 1 );

        artifactData[Artifact::BALLISTA].bonuses.emplace_back( fheroes2::ArtifactBonusType::EXTRA_CATAPULT_SHOTS, 1 );

        artifactData[Artifact::STEALTH_SHIELD].bonuses.emplace_back( fheroes2::ArtifactBonusType::DEFENCE_SKILL, 2 );

        artifactData[Artifact::DRAGON_SWORD].bonuses.emplace_back( fheroes2::ArtifactBonusType::ATTACK_SKILL, 3 );

        artifactData[Artifact::POWER_AXE].bonuses.emplace_back( fheroes2::ArtifactBonusType::ATTACK_SKILL, 2 );

        artifactData[Artifact::DIVINE_BREASTPLATE].bonuses.emplace_back( fheroes2::ArtifactBonusType::DEFENCE_SKILL, 3 );

        artifactData[Artifact::MINOR_SCROLL].bonuses.emplace_back( fheroes2::ArtifactBonusType::KNOWLEDGE_SKILL, 2 );

        artifactData[Artifact::MAJOR_SCROLL].bonuses.emplace_back( fheroes2::ArtifactBonusType::KNOWLEDGE_SKILL, 3 );

        artifactData[Artifact::SUPERIOR_SCROLL].bonuses.emplace_back( fheroes2::ArtifactBonusType::KNOWLEDGE_SKILL, 4 );

        artifactData[Artifact::FOREMOST_SCROLL].bonuses.emplace_back( fheroes2::ArtifactBonusType::KNOWLEDGE_SKILL, 5 );

        artifactData[Artifact::ENDLESS_SACK_GOLD].bonuses.emplace_back( fheroes2::ArtifactBonusType::GOLD_INCOME, 1000 );

        artifactData[Artifact::ENDLESS_BAG_GOLD].bonuses.emplace_back( fheroes2::ArtifactBonusType::GOLD_INCOME, 750 );

        artifactData[Artifact::ENDLESS_PURSE_GOLD].bonuses.emplace_back( fheroes2::ArtifactBonusType::GOLD_INCOME, 500 );

        artifactData[Artifact::NOMAD_BOOTS_MOBILITY].bonuses.emplace_back( fheroes2::ArtifactBonusType::LAND_MOBILITY, 600 );

        artifactData[Artifact::TRAVELER_BOOTS_MOBILITY].bonuses.emplace_back( fheroes2::ArtifactBonusType::LAND_MOBILITY, 300 );

        artifactData[Artifact::RABBIT_FOOT].bonuses.emplace_back( fheroes2::ArtifactBonusType::LUCK, 1 );

        artifactData[Artifact::GOLDEN_HORSESHOE].bonuses.emplace_back( fheroes2::ArtifactBonusType::LUCK, 1 );

        artifactData[Artifact::GAMBLER_LUCKY_COIN].bonuses.emplace_back( fheroes2::ArtifactBonusType::LUCK, 1 );

        artifactData[Artifact::FOUR_LEAF_CLOVER].bonuses.emplace_back( fheroes2::ArtifactBonusType::LUCK, 1 );

        artifactData[Artifact::TRUE_COMPASS_MOBILITY].bonuses.emplace_back( fheroes2::ArtifactBonusType::LAND_MOBILITY, 500 );
        artifactData[Artifact::TRUE_COMPASS_MOBILITY].bonuses.emplace_back( fheroes2::ArtifactBonusType::SEA_MOBILITY, 500 );

        artifactData[Artifact::SAILORS_ASTROLABE_MOBILITY].bonuses.emplace_back( fheroes2::ArtifactBonusType::SEA_MOBILITY, 1000 );

        artifactData[Artifact::EVIL_EYE].bonuses.emplace_back( fheroes2::ArtifactBonusType::CURSE_SPELL_COST_REDUCTION_PERCENT, 50 );

        artifactData[Artifact::ENCHANTED_HOURGLASS].bonuses.emplace_back( fheroes2::ArtifactBonusType::EVERY_COMBAT_SPELL_DURATION, 2 );

        artifactData[Artifact::GOLD_WATCH].bonuses.emplace_back( fheroes2::ArtifactBonusType::HYPNOTIZE_SPELL_EXTRA_EFFECTIVENESS_PERCENT, 100 );

        artifactData[Artifact::SKULLCAP].bonuses.emplace_back( fheroes2::ArtifactBonusType::MIND_INFLUENCE_SPELL_COST_REDUCTION_PERCENT, 50 );

        artifactData[Artifact::ICE_CLOAK].bonuses.emplace_back( fheroes2::ArtifactBonusType::COLD_SPELL_DAMAGE_REDUCTION_PERCENT, 50 );

        artifactData[Artifact::FIRE_CLOAK].bonuses.emplace_back( fheroes2::ArtifactBonusType::FIRE_SPELL_DAMAGE_REDUCTION_PERCENT, 50 );

        artifactData[Artifact::LIGHTNING_HELM].bonuses.emplace_back( fheroes2::ArtifactBonusType::LIGHTNING_SPELL_DAMAGE_REDUCTION_PERCENT, 50 );

        artifactData[Artifact::EVERCOLD_ICICLE].bonuses.emplace_back( fheroes2::ArtifactBonusType::COLD_SPELL_EXTRA_EFFECTIVENESS_PERCENT, 50 );

        artifactData[Artifact::EVERHOT_LAVA_ROCK].bonuses.emplace_back( fheroes2::ArtifactBonusType::FIRE_SPELL_EXTRA_EFFECTIVENESS_PERCENT, 50 );

        artifactData[Artifact::LIGHTNING_ROD].bonuses.emplace_back( fheroes2::ArtifactBonusType::LIGHTNING_SPELL_EXTRA_EFFECTIVENESS_PERCENT, 50 );

        artifactData[Artifact::SNAKE_RING].bonuses.emplace_back( fheroes2::ArtifactBonusType::BLESS_SPELL_COST_REDUCTION_PERCENT, 50 );

        artifactData[Artifact::ANKH].bonuses.emplace_back( fheroes2::ArtifactBonusType::RESURRECT_SPELL_EXTRA_EFFECTIVENESS_PERCENT, 100 );

        artifactData[Artifact::BOOK_ELEMENTS].bonuses.emplace_back( fheroes2::ArtifactBonusType::SUMMONING_SPELL_EXTRA_EFFECTIVENESS_PERCENT, 100 );

        artifactData[Artifact::ELEMENTAL_RING].bonuses.emplace_back( fheroes2::ArtifactBonusType::SUMMONING_SPELL_COST_REDUCTION_PERCENT, 50 );

        artifactData[Artifact::HOLY_PENDANT].bonuses.emplace_back( fheroes2::ArtifactBonusType::CURSE_SPELL_IMMUNITY );

        artifactData[Artifact::PENDANT_FREE_WILL].bonuses.emplace_back( fheroes2::ArtifactBonusType::HYPNOTIZE_SPELL_IMMUNITY );

        artifactData[Artifact::PENDANT_LIFE].bonuses.emplace_back( fheroes2::ArtifactBonusType::DEATH_SPELL_IMMUNITY );

        artifactData[Artifact::SERENITY_PENDANT].bonuses.emplace_back( fheroes2::ArtifactBonusType::BERSERK_SPELL_IMMUNITY );

        artifactData[Artifact::SEEING_EYE_PENDANT].bonuses.emplace_back( fheroes2::ArtifactBonusType::BLIND_SPELL_IMMUNITY );

        artifactData[Artifact::KINETIC_PENDANT].bonuses.emplace_back( fheroes2::ArtifactBonusType::PARALYZE_SPELL_IMMUNITY );

        artifactData[Artifact::PENDANT_DEATH].bonuses.emplace_back( fheroes2::ArtifactBonusType::HOLY_SPELL_IMMUNITY );

        artifactData[Artifact::WAND_NEGATION].bonuses.emplace_back( fheroes2::ArtifactBonusType::DISPEL_SPELL_IMMUNITY );

        artifactData[Artifact::GOLDEN_BOW].bonuses.emplace_back( fheroes2::ArtifactBonusType::NO_SHOOTING_PENALTY, GameStatic::getCastleWallRangedPenalty() );

        artifactData[Artifact::TELESCOPE].bonuses.emplace_back( fheroes2::ArtifactBonusType::AREA_REVEAL_DISTANCE, 1 );

        artifactData[Artifact::STATESMAN_QUILL].bonuses.emplace_back( fheroes2::ArtifactBonusType::SURRENDER_COST_REDUCTION_PERCENT, 10 );

        artifactData[Artifact::WIZARD_HAT].bonuses.emplace_back( fheroes2::ArtifactBonusType::EVERY_COMBAT_SPELL_DURATION, 10 );

        artifactData[Artifact::POWER_RING].bonuses.emplace_back( fheroes2::ArtifactBonusType::SPELL_POINTS_DAILY_GENERATION, 2 );

        artifactData[Artifact::AMMO_CART].bonuses.emplace_back( fheroes2::ArtifactBonusType::ENDLESS_AMMUNITION );

        artifactData[Artifact::TAX_LIEN].curses.emplace_back( fheroes2::ArtifactCurseType::GOLD_PENALTY, 250 );

        artifactData[Artifact::HIDEOUS_MASK].curses.emplace_back( fheroes2::ArtifactCurseType::NO_JOINING_ARMIES );

        artifactData[Artifact::ENDLESS_POUCH_SULFUR].bonuses.emplace_back( fheroes2::ArtifactBonusType::SULFUR_INCOME, 1 );

        artifactData[Artifact::ENDLESS_VIAL_MERCURY].bonuses.emplace_back( fheroes2::ArtifactBonusType::MERCURY_INCOME, 1 );

        artifactData[Artifact::ENDLESS_POUCH_GEMS].bonuses.emplace_back( fheroes2::ArtifactBonusType::GEMS_INCOME, 1 );

        artifactData[Artifact::ENDLESS_CORD_WOOD].bonuses.emplace_back( fheroes2::ArtifactBonusType::WOOD_INCOME, 1 );

        artifactData[Artifact::ENDLESS_CART_ORE].bonuses.emplace_back( fheroes2::ArtifactBonusType::ORE_INCOME, 1 );

        artifactData[Artifact::ENDLESS_POUCH_CRYSTAL].bonuses.emplace_back( fheroes2::ArtifactBonusType::CRYSTAL_INCOME, 1 );

        artifactData[Artifact::SPIKED_HELM].bonuses.emplace_back( fheroes2::ArtifactBonusType::ATTACK_SKILL, 1 );
        artifactData[Artifact::SPIKED_HELM].bonuses.emplace_back( fheroes2::ArtifactBonusType::DEFENCE_SKILL, 1 );

        artifactData[Artifact::SPIKED_SHIELD].bonuses.emplace_back( fheroes2::ArtifactBonusType::ATTACK_SKILL, 2 );
        artifactData[Artifact::SPIKED_SHIELD].bonuses.emplace_back( fheroes2::ArtifactBonusType::DEFENCE_SKILL, 2 );

        artifactData[Artifact::WHITE_PEARL].bonuses.emplace_back( fheroes2::ArtifactBonusType::KNOWLEDGE_SKILL, 1 );
        artifactData[Artifact::WHITE_PEARL].bonuses.emplace_back( fheroes2::ArtifactBonusType::SPELL_POWER_SKILL, 1 );

        artifactData[Artifact::BLACK_PEARL].bonuses.emplace_back( fheroes2::ArtifactBonusType::KNOWLEDGE_SKILL, 2 );
        artifactData[Artifact::BLACK_PEARL].bonuses.emplace_back( fheroes2::ArtifactBonusType::SPELL_POWER_SKILL, 2 );

        artifactData[Artifact::MAGIC_BOOK].bonuses.emplace_back( fheroes2::ArtifactBonusType::NONE );
        artifactData[Artifact::EDITOR_ANY_ULTIMATE_ARTIFACT].bonuses.emplace_back( fheroes2::ArtifactBonusType::NONE );
        artifactData[Artifact::UNUSED_84].bonuses.emplace_back( fheroes2::ArtifactBonusType::NONE );
        artifactData[Artifact::UNUSED_85].bonuses.emplace_back( fheroes2::ArtifactBonusType::NONE );
        artifactData[Artifact::UNUSED_86].bonuses.emplace_back( fheroes2::ArtifactBonusType::NONE );

        artifactData[Artifact::SPELL_SCROLL].bonuses.emplace_back( fheroes2::ArtifactBonusType::ADD_SPELL, Spell::NONE );

        artifactData[Artifact::ARM_MARTYR].bonuses.emplace_back( fheroes2::ArtifactBonusType::SPELL_POWER_SKILL, 3 );
        artifactData[Artifact::ARM_MARTYR].curses.emplace_back( fheroes2::ArtifactCurseType::UNDEAD_MORALE_PENALTY );

        artifactData[Artifact::BREASTPLATE_ANDURAN].bonuses.emplace_back( fheroes2::ArtifactBonusType::DEFENCE_SKILL, 5 );

        artifactData[Artifact::BROACH_SHIELDING].bonuses.emplace_back( fheroes2::ArtifactBonusType::ELEMENTAL_SPELL_DAMAGE_REDUCTION_PERCENT, 50 );
        artifactData[Artifact::BROACH_SHIELDING].curses.emplace_back( fheroes2::ArtifactCurseType::SPELL_POWER_SKILL, 2 );

        artifactData[Artifact::BATTLE_GARB].bonuses.emplace_back( fheroes2::ArtifactBonusType::ATTACK_SKILL, 5 );
        artifactData[Artifact::BATTLE_GARB].bonuses.emplace_back( fheroes2::ArtifactBonusType::DEFENCE_SKILL, 5 );
        artifactData[Artifact::BATTLE_GARB].bonuses.emplace_back( fheroes2::ArtifactBonusType::SPELL_POWER_SKILL, 5 );
        artifactData[Artifact::BATTLE_GARB].bonuses.emplace_back( fheroes2::ArtifactBonusType::MAXIMUM_MORALE );
        artifactData[Artifact::BATTLE_GARB].bonuses.emplace_back( fheroes2::ArtifactBonusType::MAXIMUM_LUCK );
        artifactData[Artifact::BATTLE_GARB].bonuses.emplace_back( fheroes2::ArtifactBonusType::ADD_SPELL, Spell::TOWNPORTAL );

        artifactData[Artifact::CRYSTAL_BALL].bonuses.emplace_back( fheroes2::ArtifactBonusType::VIEW_MONSTER_INFORMATION );

        artifactData[Artifact::HEART_FIRE].bonuses.emplace_back( fheroes2::ArtifactBonusType::FIRE_SPELL_DAMAGE_REDUCTION_PERCENT, 50 );
        artifactData[Artifact::HEART_FIRE].curses.emplace_back( fheroes2::ArtifactCurseType::COLD_SPELL_EXTRA_DAMAGE_PERCENT, 100 );

        artifactData[Artifact::HEART_ICE].bonuses.emplace_back( fheroes2::ArtifactBonusType::COLD_SPELL_DAMAGE_REDUCTION_PERCENT, 50 );
        artifactData[Artifact::HEART_ICE].curses.emplace_back( fheroes2::ArtifactCurseType::FIRE_SPELL_EXTRA_DAMAGE_PERCENT, 100 );

        artifactData[Artifact::HELMET_ANDURAN].bonuses.emplace_back( fheroes2::ArtifactBonusType::SPELL_POWER_SKILL, 5 );

        artifactData[Artifact::HOLY_HAMMER].bonuses.emplace_back( fheroes2::ArtifactBonusType::ATTACK_SKILL, 5 );

        artifactData[Artifact::LEGENDARY_SCEPTER].bonuses.emplace_back( fheroes2::ArtifactBonusType::ATTACK_SKILL, 2 );
        artifactData[Artifact::LEGENDARY_SCEPTER].bonuses.emplace_back( fheroes2::ArtifactBonusType::DEFENCE_SKILL, 2 );
        artifactData[Artifact::LEGENDARY_SCEPTER].bonuses.emplace_back( fheroes2::ArtifactBonusType::KNOWLEDGE_SKILL, 2 );
        artifactData[Artifact::LEGENDARY_SCEPTER].bonuses.emplace_back( fheroes2::ArtifactBonusType::SPELL_POWER_SKILL, 2 );

        artifactData[Artifact::MASTHEAD].bonuses.emplace_back( fheroes2::ArtifactBonusType::SEA_BATTLE_MORALE_BOOST, 1 );
        artifactData[Artifact::MASTHEAD].bonuses.emplace_back( fheroes2::ArtifactBonusType::SEA_BATTLE_LUCK_BOOST, 1 );

        artifactData[Artifact::SPHERE_NEGATION].bonuses.emplace_back( fheroes2::ArtifactBonusType::DISABLE_ALL_SPELL_COMBAT_CASTING );

        artifactData[Artifact::STAFF_WIZARDRY].bonuses.emplace_back( fheroes2::ArtifactBonusType::SPELL_POWER_SKILL, 5 );

        artifactData[Artifact::SWORD_BREAKER].bonuses.emplace_back( fheroes2::ArtifactBonusType::DEFENCE_SKILL, 4 );
        artifactData[Artifact::SWORD_BREAKER].bonuses.emplace_back( fheroes2::ArtifactBonusType::ATTACK_SKILL, 1 );

        artifactData[Artifact::SWORD_ANDURAN].bonuses.emplace_back( fheroes2::ArtifactBonusType::ATTACK_SKILL, 5 );

        artifactData[Artifact::SPADE_NECROMANCY].bonuses.emplace_back( fheroes2::ArtifactBonusType::NECROMANCY_SKILL, 10 );

        for ( const fheroes2::ArtifactData & artifact : artifactData ) {
            if ( artifact.bonuses.empty() && artifact.curses.empty() ) {
                // Artifact info is not populated properly. An artifact with no effects cannot exist.
                assert( 0 );
            }

            for ( const fheroes2::ArtifactBonus & bonus : artifact.bonuses ) {
                if ( bonus.value < 0 ) {
                    assert( 0 );
                }
            }

            for ( const fheroes2::ArtifactCurse & curse : artifact.curses ) {
                if ( curse.value < 0 ) {
                    assert( 0 );
                }
            }
        }
    }
}

namespace fheroes2
{
    bool isBonusCumulative( const ArtifactBonusType bonus )
    {
        switch ( bonus ) {
        case ArtifactBonusType::KNOWLEDGE_SKILL:
        case ArtifactBonusType::SPELL_POWER_SKILL:
        case ArtifactBonusType::ATTACK_SKILL:
        case ArtifactBonusType::DEFENCE_SKILL:
        case ArtifactBonusType::GOLD_INCOME:
        case ArtifactBonusType::WOOD_INCOME:
        case ArtifactBonusType::MERCURY_INCOME:
        case ArtifactBonusType::ORE_INCOME:
        case ArtifactBonusType::SULFUR_INCOME:
        case ArtifactBonusType::CRYSTAL_INCOME:
        case ArtifactBonusType::GEMS_INCOME:
            return true;
        default:
            break;
        }

        return false;
    }

    bool isBonusMultiplied( const ArtifactBonusType bonus )
    {
        switch ( bonus ) {
        case ArtifactBonusType::SURRENDER_COST_REDUCTION_PERCENT:
        case ArtifactBonusType::CURSE_SPELL_COST_REDUCTION_PERCENT:
        case ArtifactBonusType::BLESS_SPELL_COST_REDUCTION_PERCENT:
        case ArtifactBonusType::SUMMONING_SPELL_COST_REDUCTION_PERCENT:
        case ArtifactBonusType::MIND_INFLUENCE_SPELL_COST_REDUCTION_PERCENT:
        case ArtifactBonusType::COLD_SPELL_DAMAGE_REDUCTION_PERCENT:
        case ArtifactBonusType::FIRE_SPELL_DAMAGE_REDUCTION_PERCENT:
        case ArtifactBonusType::LIGHTNING_SPELL_DAMAGE_REDUCTION_PERCENT:
        case ArtifactBonusType::ELEMENTAL_SPELL_DAMAGE_REDUCTION_PERCENT:
        case ArtifactBonusType::HYPNOTIZE_SPELL_EXTRA_EFFECTIVENESS_PERCENT:
        case ArtifactBonusType::COLD_SPELL_EXTRA_EFFECTIVENESS_PERCENT:
        case ArtifactBonusType::FIRE_SPELL_EXTRA_EFFECTIVENESS_PERCENT:
        case ArtifactBonusType::LIGHTNING_SPELL_EXTRA_EFFECTIVENESS_PERCENT:
        case ArtifactBonusType::RESURRECT_SPELL_EXTRA_EFFECTIVENESS_PERCENT:
        case ArtifactBonusType::SUMMONING_SPELL_EXTRA_EFFECTIVENESS_PERCENT:
            return true;
        default:
            break;
        }

        return false;
    }

    bool isBonusUnique( const ArtifactBonusType bonus )
    {
        switch ( bonus ) {
        case ArtifactBonusType::CURSE_SPELL_IMMUNITY:
        case ArtifactBonusType::HYPNOTIZE_SPELL_IMMUNITY:
        case ArtifactBonusType::DEATH_SPELL_IMMUNITY:
        case ArtifactBonusType::BERSERK_SPELL_IMMUNITY:
        case ArtifactBonusType::BLIND_SPELL_IMMUNITY:
        case ArtifactBonusType::PARALYZE_SPELL_IMMUNITY:
        case ArtifactBonusType::HOLY_SPELL_IMMUNITY:
        case ArtifactBonusType::DISPEL_SPELL_IMMUNITY:
        case ArtifactBonusType::ENDLESS_AMMUNITION:
        case ArtifactBonusType::NO_SHOOTING_PENALTY:
        case ArtifactBonusType::VIEW_MONSTER_INFORMATION:
        case ArtifactBonusType::DISABLE_ALL_SPELL_COMBAT_CASTING:
            return true;
        default:
            break;
        }

        return false;
    }

    bool isCurseCumulative( const ArtifactCurseType curse )
    {
        switch ( curse ) {
        case ArtifactCurseType::SPELL_POWER_SKILL:
        case ArtifactCurseType::GOLD_PENALTY:
            return true;
        default:
            break;
        }

        return false;
    }

    bool isCurseMultiplied( const ArtifactCurseType curse )
    {
        switch ( curse ) {
        case ArtifactCurseType::FIRE_SPELL_EXTRA_DAMAGE_PERCENT:
        case ArtifactCurseType::COLD_SPELL_EXTRA_DAMAGE_PERCENT:
            return true;
        default:
            break;
        }

        return false;
    }

    bool isCurseUnique( const ArtifactCurseType curse )
    {
        switch ( curse ) {
        case ArtifactCurseType::NO_JOINING_ARMIES:
        case ArtifactCurseType::UNDEAD_MORALE_PENALTY:
            return true;
        default:
            break;
        }

        return false;
    }

    std::string ArtifactData::getDescription( const int extraParameter ) const
    {
        std::string description( _( baseDescription ) );

        StringReplace( description, "%{name}", _( name ) );

        std::vector<ArtifactBonus>::const_iterator foundBonus = std::find( bonuses.begin(), bonuses.end(), ArtifactBonus( ArtifactBonusType::ADD_SPELL ) );
        if ( foundBonus != bonuses.end() ) {
            if ( foundBonus->value == Spell::NONE ) {
                if ( extraParameter == Spell::NONE ) {
                    // This is a case when artifact description is viewed from list in Battle Only mode or in Editor.
                    StringReplace( description, "%{spell}", _( "spellBonus|selected by user" ) );
                }
                else {
                    StringReplace( description, "%{spell}", Spell( extraParameter ).GetName() );
                }
            }
            else {
                StringReplace( description, "%{spell}", Spell( foundBonus->value ).GetName() );
            }
        }
        else if ( !bonuses.empty() ) {
            StringReplace( description, "%{count}", bonuses.front().value );
        }
        else if ( !curses.empty() ) {
            StringReplace( description, "%{count}", curses.front().value );
        }
        else {
            // An artifact that does nothing? Check your logic!
            assert( 0 );
        }

        return description;
    }

    const ArtifactData & getArtifactData( const int artifactId )
    {
        if ( artifactData.empty() ) {
            populateArtifactData();
        }

        if ( artifactId < 0 || static_cast<size_t>( artifactId ) >= artifactData.size() ) {
            // Invalid artifact ID!
            assert( 0 );
            return artifactData.front();
        }

        return artifactData[artifactId];
    }

    std::string getArtifactDescription( const int artifactId )
    {
        const ArtifactData & data = getArtifactData( artifactId );

        std::ostringstream os;
        os << "----------" << std::endl;
        os << "Name: " << data.name << std::endl;
        os << "Description: " << data.getDescription( Spell::RANDOM ) << std::endl;

        if ( data.discoveryEventDescription != nullptr ) {
            os << "Discovery event description: " << data.discoveryEventDescription << std::endl;
        }
        else {
            os << "No discovery event description" << std::endl;
        }

        if ( !data.bonuses.empty() ) {
            os << "Bonuses:" << std::endl;
            for ( const ArtifactBonus & bonus : data.bonuses ) {
                os << "   ";
                if ( isBonusCumulative( bonus.type ) ) {
                    os << "[ cumulative ] ";
                }
                else if ( isBonusMultiplied( bonus.type ) ) {
                    os << "[ multiplied ] ";
                }
                else if ( isBonusUnique( bonus.type ) ) {
                    os << "[ unique ] ";
                }
                else {
                    os << "[ cumulative per artifact type ] ";
                }

                switch ( bonus.type ) {
                case ArtifactBonusType::NONE:
                    os << "None" << std::endl;
                    break;
                case ArtifactBonusType::KNOWLEDGE_SKILL:
                    os << "Add " << bonus.value << " to hero's Knowledge Skill" << std::endl;
                    break;
                case ArtifactBonusType::ATTACK_SKILL:
                    os << "Add " << bonus.value << " to hero's Attack Skill" << std::endl;
                    break;
                case ArtifactBonusType::DEFENCE_SKILL:
                    os << "Add " << bonus.value << " to hero's Defence Skill" << std::endl;
                    break;
                case ArtifactBonusType::SPELL_POWER_SKILL:
                    os << "Add " << bonus.value << " to hero's Spell Power Skill" << std::endl;
                    break;
                case ArtifactBonusType::GOLD_INCOME:
                    os << "Increase daily kingdom's income by " << bonus.value << " gold" << std::endl;
                    break;
                case ArtifactBonusType::WOOD_INCOME:
                    os << "Increase daily kingdom's income by " << bonus.value << " wood" << std::endl;
                    break;
                case ArtifactBonusType::MERCURY_INCOME:
                    os << "Increase daily kingdom's income by " << bonus.value << " mercury" << std::endl;
                    break;
                case ArtifactBonusType::ORE_INCOME:
                    os << "Increase daily kingdom's income by " << bonus.value << " ore" << std::endl;
                    break;
                case ArtifactBonusType::SULFUR_INCOME:
                    os << "Increase daily kingdom's income by " << bonus.value << " sulfur" << std::endl;
                    break;
                case ArtifactBonusType::CRYSTAL_INCOME:
                    os << "Increase daily kingdom's income by " << bonus.value << " crystal" << std::endl;
                    break;
                case ArtifactBonusType::GEMS_INCOME:
                    os << "Increase daily kingdom's income by " << bonus.value << " gem" << std::endl;
                    break;
                case ArtifactBonusType::SPELL_POINTS_DAILY_GENERATION:
                    os << "Restore " << bonus.value << " spell points for a hero" << std::endl;
                    break;
                case ArtifactBonusType::LAND_MOBILITY:
                    os << "Add " << bonus.value << " move points to a hero on Land" << std::endl;
                    break;
                case ArtifactBonusType::SEA_MOBILITY:
                    os << "Add " << bonus.value << " move points to a hero on Sea" << std::endl;
                    break;
                case ArtifactBonusType::MORALE:
                    os << "Increase army Morale by " << bonus.value << std::endl;
                    break;
                case ArtifactBonusType::LUCK:
                    os << "Increase army Luck by " << bonus.value << std::endl;
                    break;
                case ArtifactBonusType::EVERY_COMBAT_SPELL_DURATION:
                    os << "Increase every combat spell duration by " << bonus.value << " rounds" << std::endl;
                    break;
                case ArtifactBonusType::SURRENDER_COST_REDUCTION_PERCENT:
                    os << "Reduce Surrender cost to " << bonus.value << " percent" << std::endl;
                    break;
                case ArtifactBonusType::CURSE_SPELL_COST_REDUCTION_PERCENT:
                    os << "Reduce Curse spell cost by " << bonus.value << " percent" << std::endl;
                    break;
                case ArtifactBonusType::BLESS_SPELL_COST_REDUCTION_PERCENT:
                    os << "Reduce Bless spell cost by " << bonus.value << " percent" << std::endl;
                    break;
                case ArtifactBonusType::SUMMONING_SPELL_COST_REDUCTION_PERCENT:
                    os << "Reduce Summoning spell cost by " << bonus.value << " percent" << std::endl;
                    break;
                case ArtifactBonusType::MIND_INFLUENCE_SPELL_COST_REDUCTION_PERCENT:
                    os << "Reduce Mind Influence related spell cost by " << bonus.value << " percent" << std::endl;
                    break;
                case ArtifactBonusType::COLD_SPELL_DAMAGE_REDUCTION_PERCENT:
                    os << "Reduce Cold spell cost by " << bonus.value << " percent" << std::endl;
                    break;
                case ArtifactBonusType::FIRE_SPELL_DAMAGE_REDUCTION_PERCENT:
                    os << "Reduce Fire spell cost by " << bonus.value << " percent" << std::endl;
                    break;
                case ArtifactBonusType::LIGHTNING_SPELL_DAMAGE_REDUCTION_PERCENT:
                    os << "Reduce Lightning spell cost by " << bonus.value << " percent" << std::endl;
                    break;
                case ArtifactBonusType::ELEMENTAL_SPELL_DAMAGE_REDUCTION_PERCENT:
                    os << "Reduce Elemental Storm and Armageddon spell cost by " << bonus.value << " percent" << std::endl;
                    break;
                case ArtifactBonusType::HYPNOTIZE_SPELL_EXTRA_EFFECTIVENESS_PERCENT:
                    os << "Improve Hypnotize spell effectiveness by " << bonus.value << " percent" << std::endl;
                    break;
                case ArtifactBonusType::COLD_SPELL_EXTRA_EFFECTIVENESS_PERCENT:
                    os << "Improve Cold spell effectiveness by " << bonus.value << " percent" << std::endl;
                    break;
                case ArtifactBonusType::FIRE_SPELL_EXTRA_EFFECTIVENESS_PERCENT:
                    os << "Improve Fire spell effectiveness by " << bonus.value << " percent" << std::endl;
                    break;
                case ArtifactBonusType::LIGHTNING_SPELL_EXTRA_EFFECTIVENESS_PERCENT:
                    os << "Improve Lightning spell effectiveness by " << bonus.value << " percent" << std::endl;
                    break;
                case ArtifactBonusType::RESURRECT_SPELL_EXTRA_EFFECTIVENESS_PERCENT:
                    os << "Improve Resurrection spell effectiveness by " << bonus.value << " percent" << std::endl;
                    break;
                case ArtifactBonusType::SUMMONING_SPELL_EXTRA_EFFECTIVENESS_PERCENT:
                    os << "Improve Summoning spell effectiveness by " << bonus.value << " percent" << std::endl;
                    break;
                case ArtifactBonusType::CURSE_SPELL_IMMUNITY:
                    os << "Add immunity to Curse spells" << std::endl;
                    break;
                case ArtifactBonusType::HYPNOTIZE_SPELL_IMMUNITY:
                    os << "Add immunity to Hypnotize spells" << std::endl;
                    break;
                case ArtifactBonusType::DEATH_SPELL_IMMUNITY:
                    os << "Add immunity to Death spells" << std::endl;
                    break;
                case ArtifactBonusType::BERSERK_SPELL_IMMUNITY:
                    os << "Add immunity to Berserk spells" << std::endl;
                    break;
                case ArtifactBonusType::BLIND_SPELL_IMMUNITY:
                    os << "Add immunity to Blind spells" << std::endl;
                    break;
                case ArtifactBonusType::PARALYZE_SPELL_IMMUNITY:
                    os << "Add immunity to Paralyze spells" << std::endl;
                    break;
                case ArtifactBonusType::HOLY_SPELL_IMMUNITY:
                    os << "Add immunity to Holy spells" << std::endl;
                    break;
                case ArtifactBonusType::DISPEL_SPELL_IMMUNITY:
                    os << "Add immunity to Dispel spells" << std::endl;
                    break;
                case ArtifactBonusType::ENDLESS_AMMUNITION:
                    os << "Shooters have unlimited number of shots" << std::endl;
                    break;
                case ArtifactBonusType::NO_SHOOTING_PENALTY:
                    os << "Shooters have no penalty over obstacles" << std::endl;
                    break;
                case ArtifactBonusType::EXTRA_CATAPULT_SHOTS:
                    os << "Add " << bonus.value << " shots for catapult" << std::endl;
                    break;
                case ArtifactBonusType::AREA_REVEAL_DISTANCE:
                    os << "Increase fog reveal area by " << bonus.value << std::endl;
                    break;
                case ArtifactBonusType::ADD_SPELL:
                    os << "Add '" << Spell( bonus.value ).GetName() << "' spell to hero's list of spells" << std::endl;
                    break;
                case ArtifactBonusType::VIEW_MONSTER_INFORMATION:
                    os << "Add ability to view monster information" << std::endl;
                    break;
                case ArtifactBonusType::SEA_BATTLE_MORALE_BOOST:
                    os << "Increase army's Morale by " << bonus.value << " when in Sea" << std::endl;
                    break;
                case ArtifactBonusType::SEA_BATTLE_LUCK_BOOST:
                    os << "Increase army's Luck by " << bonus.value << " when in Sea" << std::endl;
                    break;
                case ArtifactBonusType::DISABLE_ALL_SPELL_COMBAT_CASTING:
                    os << "Disable all spell casting during battle" << std::endl;
                    break;
                case ArtifactBonusType::NECROMANCY_SKILL:
                    os << "Increase Necromancy Skill by " << bonus.value << " percent" << std::endl;
                    break;
                case ArtifactBonusType::MAXIMUM_MORALE:
                    os << "Gives the army maximum Morale." << std::endl;
                    break;
                case ArtifactBonusType::MAXIMUM_LUCK:
                    os << "Gives the army maximum Luck." << std::endl;
                    break;
                default:
                    // Did you add a new bonus type? Add the logic above!
                    assert( 0 );
                    break;
                }
            }
        }

        if ( !data.curses.empty() ) {
            os << "Curses:" << std::endl;
            for ( const ArtifactCurse & curse : data.curses ) {
                os << "   ";
                if ( isCurseCumulative( curse.type ) ) {
                    os << "[ cumulative ] ";
                }
                else if ( isCurseMultiplied( curse.type ) ) {
                    os << "[ multiplied ] ";
                }
                else if ( isCurseUnique( curse.type ) ) {
                    os << "[ unique ] ";
                }
                else {
                    os << "[ cumulative per artifact type ] ";
                }

                switch ( curse.type ) {
                case ArtifactCurseType::NO_JOINING_ARMIES:
                    os << "No army can join hero" << std::endl;
                    break;
                case ArtifactCurseType::MORALE:
                    os << "Decreases army's Morale by " << curse.value << std::endl;
                    break;
                case ArtifactCurseType::UNDEAD_MORALE_PENALTY:
                    os << "Add Undead Penalty to army's Morale" << std::endl;
                    break;
                case ArtifactCurseType::GOLD_PENALTY:
                    os << "Deducts " << curse.value << " gold from kingdom daily" << std::endl;
                    break;
                case ArtifactCurseType::SPELL_POWER_SKILL:
                    os << "Reduces Spell Power by " << curse.value << std::endl;
                    break;
                case ArtifactCurseType::FIRE_SPELL_EXTRA_DAMAGE_PERCENT:
                    os << "Increases Damage from Fire spells by " << curse.value << " percent" << std::endl;
                    break;
                case ArtifactCurseType::COLD_SPELL_EXTRA_DAMAGE_PERCENT:
                    os << "Increases Damage from Cold spells by " << curse.value << " percent" << std::endl;
                    break;
                default:
                    // Did you add a new curse type? Add the logic above!
                    assert( 0 );
                    break;
                }
            }
        }

        return os.str();
    }
}
