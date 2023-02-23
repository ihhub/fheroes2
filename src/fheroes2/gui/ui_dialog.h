/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2023                                             *
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

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "artifact.h"
#include "image.h"
#include "math_base.h"
#include "skill.h"
#include "spell.h"

class Funds;
class HeroBase;
class Heroes;

namespace fheroes2
{
    class DialogElement;
    class TextBase;

    int showMessage( const TextBase & header, const TextBase & body, const int buttons, const std::vector<const DialogElement *> & elements = {} );

    // This is a simplified version of UI window which is used to display a window with a text.
    // Header text has yellow normal font style and body text - white normal font style.
    int showStandardTextMessage( std::string headerText, std::string messageBody, const int buttons );

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
        const Size & area() const
        {
            return _area;
        }

        // Display a popup window with no buttons and standard description of the element. It is usually used for a right mouse click event.
        virtual void showPopup( const int buttons ) const = 0;

        // Update the content of UI elements. By default it does nothing.
        virtual bool update( Image & /*output*/, const Point & /*offset*/ ) const
        {
            return false;
        }

    protected:
        // This element must be cached to avoid heavy calculations.
        Size _area;
    };

    // IMPORTANT!
    // It is essential to store members by values rather than by references.
    // This leads to more memory consumption but at the same time prevents any memory related issues.

    class TextDialogElement : public DialogElement
    {
    public:
        explicit TextDialogElement( const std::shared_ptr<TextBase> & text );

        ~TextDialogElement() override = default;

        void draw( Image & output, const Point & offset ) const override;

        void processEvents( const Point & offset ) const override;

        // Never call this method as a custom image has nothing to popup.
        void showPopup( const int buttons ) const override;

    private:
        const std::shared_ptr<TextBase> _text;
    };

    class CustomImageDialogElement : public DialogElement
    {
    public:
        explicit CustomImageDialogElement( const Image & image );

        explicit CustomImageDialogElement( Image && image );

        ~CustomImageDialogElement() override = default;

        void draw( Image & output, const Point & offset ) const override;

        void processEvents( const Point & offset ) const override;

        // Never call this method as a custom image has nothing to popup.
        void showPopup( const int buttons ) const override;

    private:
        const Image _image;
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

        ResourceDialogElement( const int32_t resourceType, std::string && text );

        ~ResourceDialogElement() override = default;

        void draw( Image & output, const Point & offset ) const override;

        void processEvents( const Point & offset ) const override;

        void showPopup( const int buttons ) const override;

    private:
        const int32_t _resourceType = 0;
        const uint32_t _icnIndex = 0;
        const std::string _text;

        void init();
    };

    std::vector<ResourceDialogElement> getResourceDialogElements( const Funds & funds );

    int showResourceMessage( const TextBase & header, const TextBase & body, const int buttons, const Funds & funds );

    class SpellDialogElement : public DialogElement
    {
    public:
        SpellDialogElement( const Spell & spell, const HeroBase * hero );

        ~SpellDialogElement() override = default;

        void draw( Image & output, const Point & offset ) const override;

        void processEvents( const Point & offset ) const override;

        void showPopup( const int buttons ) const override;

    private:
        const Spell _spell;
        const HeroBase * _hero;
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
        PrimarySkillDialogElement( const int32_t skillType, const std::string & text );

        PrimarySkillDialogElement( const int32_t skillType, std::string && text );

        ~PrimarySkillDialogElement() override = default;

        void draw( Image & output, const Point & offset ) const override;

        void processEvents( const Point & offset ) const override;

        void showPopup( const int buttons ) const override;

    private:
        const int32_t _skillType;
        const std::string _text;

        void init();
    };

    class SecondarySkillDialogElement : public DialogElement
    {
    public:
        SecondarySkillDialogElement( const Skill::Secondary & skill, const Heroes & hero );

        ~SecondarySkillDialogElement() override = default;

        void draw( Image & output, const Point & offset ) const override;

        void processEvents( const Point & offset ) const override;

        void showPopup( const int buttons ) const override;

    private:
        const Skill::Secondary _skill;
        const Heroes & _hero;
    };

    class AnimationDialogElement : public DialogElement
    {
    public:
        explicit AnimationDialogElement( const int icnId, std::vector<uint32_t> backgroundIndices, const uint32_t animationIndexOffset, const uint64_t delay );

        ~AnimationDialogElement() override = default;

        void draw( Image & output, const Point & offset ) const override;

        void processEvents( const Point & offset ) const override;

        // Never call this method as a dynamic image has nothing to popup.
        void showPopup( const int buttons ) const override;

        bool update( Image & output, const Point & offset ) const override;

    private:
        const int _icnId;

        const std::vector<uint32_t> _backgroundIndices;

        const uint64_t _delay;

        mutable uint32_t _currentIndex;

        const uint32_t _animationIndexOffset;

        Point _internalOffset;
    };

    class CustomAnimationDialogElement : public DialogElement
    {
    public:
        explicit CustomAnimationDialogElement( Image staticImage, const int animationIcnId, const uint64_t delay, const uint32_t animationIndexOffset,
                                               const Point animationPositionOffset );

        ~CustomAnimationDialogElement() override = default;

        void draw( Image & output, const Point & offset ) const override;

        void processEvents( const Point & offset ) const override;

        // Never call this method as a dynamic image has nothing to popup.
        void showPopup( const int buttons ) const override;

        bool update( Image & output, const Point & offset ) const override;

    private:
        const Image _staticImage;

        const int _animationIcnId;

        const uint64_t _delay;

        mutable uint32_t _currentIndex;

        const Point _animationPosition;

        const uint32_t _animationIndexOffset;
    };
}
