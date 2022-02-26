/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2021                                                    *
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

#pragma once

#include <string>
#include <vector>

#include "artifact.h"
#include "math_base.h"
#include "skill.h"
#include "spell.h"

class Funds;
class Heroes;

namespace fheroes2
{
    class DialogElement;
    class Image;
    class TextBase;

    int showMessage( const TextBase & header, const TextBase & body, const int buttons, const std::vector<const DialogElement *> & elements = {} );

    // An interactive UI element within a dialog.
    class DialogElement
    {
    public:
        virtual ~DialogElement() = default;

        // Draw the element on a given image.
        virtual void draw( Image & output, const Point & offset ) const = 0;

        // Process events internally. In most cases it is a right mouse click event only.
        virtual void processEvents( const Point & offset ) const = 0;

        // Return the size of the element.
        Size area() const
        {
            return _area;
        }

        // Display a popup window with no buttons and standard description of the element. It is usually used for a right mouse click event.
        virtual void showPopup( const int buttons ) const = 0;

    protected:
        // This element must be cached to avoid heavy calculations.
        Size _area;
    };

    class ArtifactDialogElement : public DialogElement
    {
    public:
        explicit ArtifactDialogElement( const Artifact & artifact );

        ~ArtifactDialogElement() override = default;

        void draw( Image & output, const Point & offset ) const override;

        void processEvents( const Point & offset ) const override;

        void showPopup( const int buttons ) const override;

    private:
        const Artifact _artifact;
    };

    class ResourceDialogElement : public DialogElement
    {
    public:
        ResourceDialogElement( const int32_t resourceType, const std::string & text );

        ~ResourceDialogElement() override = default;

        void draw( Image & output, const Point & offset ) const override;

        void processEvents( const Point & offset ) const override;

        void showPopup( const int buttons ) const override;

    private:
        const int32_t _resourceType = 0;
        const uint32_t _icnIndex = 0;
        const std::string _text;
    };

    std::vector<ResourceDialogElement> getResourceDialogElements( const Funds & funds );

    int showResourceMessage( const TextBase & header, const TextBase & body, const int buttons, const Funds & funds );

    class SpellDialogElement : public DialogElement
    {
    public:
        explicit SpellDialogElement( const Spell & spell );

        ~SpellDialogElement() override = default;

        void draw( Image & output, const Point & offset ) const override;

        void processEvents( const Point & offset ) const override;

        void showPopup( const int buttons ) const override;

    private:
        const Spell _spell;
    };

    class LuckDialogElement : public DialogElement
    {
    public:
        explicit LuckDialogElement( const bool goodLuck );

        ~LuckDialogElement() override = default;

        void draw( Image & output, const Point & offset ) const override;

        void processEvents( const Point & offset ) const override;

        void showPopup( const int buttons ) const override;

    private:
        const bool _goodLuck;
    };

    class MoraleDialogElement : public DialogElement
    {
    public:
        explicit MoraleDialogElement( const bool goodMorale );

        ~MoraleDialogElement() override = default;

        void draw( Image & output, const Point & offset ) const override;

        void processEvents( const Point & offset ) const override;

        void showPopup( const int buttons ) const override;

    private:
        const bool _goodMorale;
    };

    class ExperienceDialogElement : public DialogElement
    {
    public:
        explicit ExperienceDialogElement( const int32_t experience );

        ~ExperienceDialogElement() override = default;

        void draw( Image & output, const Point & offset ) const override;

        void processEvents( const Point & offset ) const override;

        void showPopup( const int buttons ) const override;

    private:
        const int32_t _experience;
    };

    class PrimarySkillDialogElement : public DialogElement
    {
    public:
        explicit PrimarySkillDialogElement( const int32_t skillType, const std::string & text );

        ~PrimarySkillDialogElement() override = default;

        void draw( Image & output, const Point & offset ) const override;

        void processEvents( const Point & offset ) const override;

        void showPopup( const int buttons ) const override;

    private:
        const int32_t _skillType;
        const std::string _text;
    };

    class SecondarySkillDialogElement : public DialogElement
    {
    public:
        explicit SecondarySkillDialogElement( const Heroes & hero, const Skill::Secondary & skill );

        ~SecondarySkillDialogElement() override = default;

        void draw( Image & output, const Point & offset ) const override;

        void processEvents( const Point & offset ) const override;

        void showPopup( const int buttons ) const override;

    private:
        const Skill::Secondary _skill;
        const Heroes & _hero;
    };
}
