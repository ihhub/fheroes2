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

#include <vector>

#include "math_base.h"

class Artifact;
class Funds;
class Spell;

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
        virtual void draw( Image & output, const Point & offset ) const = 0;

        virtual void processEvents( const Point & offset ) const = 0;

        Size area() const
        {
            return _area;
        }

    protected:
        // This element must be cached to avoid heavy calculations.
        Size _area;
    };

    class ArtifactDialogElement : public DialogElement
    {
    public:
        explicit ArtifactDialogElement( const Artifact & artifact );

        void draw( Image & output, const Point & offset ) const override;

        void processEvents( const Point & offset ) const override;

    private:
        const Artifact & _artifact;
    };

    class ResourceDialogElement : public DialogElement
    {
    public:
        ResourceDialogElement( const int32_t resourceType, const int32_t quantity );

        void draw( Image & output, const Point & offset ) const override;

        void processEvents( const Point & offset ) const override;

    private:
        enum
        {
            TEXT_OFFSET = 2
        };

        const int32_t _resourceType = 0;
        const int32_t _quantity = 0;

        uint32_t _icnIndex = 0;
    };

    std::vector<ResourceDialogElement> getResourceDialogElements( const Funds & funds );

    class SpellDialogElement : public DialogElement
    {
    public:
        explicit SpellDialogElement( const Spell & spell );

        void draw( Image & output, const Point & offset ) const override;

        void processEvents( const Point & offset ) const override;

    private:
        enum
        {
            TEXT_OFFSET = 2
        };

        const Spell & _spell;
    };

    class LuckDialogElement : public DialogElement
    {
    public:
        explicit LuckDialogElement( const bool goodLuck );

        void draw( Image & output, const Point & offset ) const override;

        void processEvents( const Point & offset ) const override;
    private:
        const bool _goodLuck;
    };

    class MoraleDialogElement : public DialogElement
    {
    public:
        explicit MoraleDialogElement( const bool goodMorale );

        void draw( Image & output, const Point & offset ) const override;

        void processEvents( const Point & offset ) const override;
    private:
        const bool _goodMorale;
    };

    class ExperienceDialogElement : public DialogElement
    {
    public:
        explicit ExperienceDialogElement( const int32_t experience );

        void draw( Image & output, const Point & offset ) const override;

        void processEvents( const Point & offset ) const override;

    private:
        enum
        {
            TEXT_OFFSET = 2
        };

        const int32_t _experience;
    };
}
