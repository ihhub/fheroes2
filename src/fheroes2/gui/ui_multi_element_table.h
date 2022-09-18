/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2022                                             *
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

#include "image.h"
#include "math_base.h"

#include <cassert>
#include <vector>
#include <utility>

namespace fheroes2
{
    class PeripheralActionObject : public ActionObject
    {
    public:
        PeripheralActionObject() = default;

        ~PeripheralActionObject() override = default;

        // If a method below returns true it means that the object processed an event.
        // This is an indicator that all other UI elements must reset their states as only one element can be active at the moment.
        //
        // All mouse related events must be called after verification that the mouse cursor is within UI element's ROI. See isWithinRoi() method.
        // In this case none of these methods is required to verify mouse position.

        // Some methods require to have extra verification of states to avoid inter-element misbehavior like pressing left mouse button on one element,
        // moving mouse and releasing it over another element.
        // In order to support drag and drop capabilities a multi-element action object should be used.

        virtual bool onMouseHover( const Point & /*mousePosition*/ )
        {
            return false;
        }

        virtual bool onMouseOver()
        {
            return false;
        }

        bool onMouseLeftButtonPress( const Point & mousePosition )
        {
            reset();
            _mouseLeftButtonPressed = true;

            const bool returnValue = onMouseLeftButtonPressInternal( mousePosition );
            notifySubscriber();

            return returnValue;
        }

        bool onMouseLeftButtonRelease( const Point & mousePosition )
        {
            if ( !_mouseLeftButtonPressed ) {
                // It looks like mouse left button was pressed over another UI element.
                reset();
                notifySubscriber();
                return false;
            }

            reset();

            const bool returnValue = onMouseLeftButtonReleaseInternal( mousePosition );
            notifySubscriber();

            return returnValue;
        }

        bool onMouseRightButtonPress( const Point & mousePosition )
        {
            reset();
            _mouseRightButtonPressed = true;

            const bool returnValue = onMouseRightButtonPressInternal( mousePosition );
            notifySubscriber();

            return returnValue;
        }

        bool onMouseRightButtonRelease( const Point & mousePosition )
        {
            if ( !_mouseRightButtonPressed ) {
                // It looks like mouse right button was pressed over another UI element.
                reset();
                return false;
            }

            const bool returnValue = onMouseRightButtonReleaseInternal( mousePosition );
            notifySubscriber();

            return returnValue;
        }

        virtual bool onKeyboardButtonClick()
        {
            return false;
        }

        virtual const Rect getRoi() const = 0;

        bool isWithinRoi( const Point & mousePosition ) const
        {
            return ( getRoi() & mousePosition );
        }

        void reset()
        {
            _mouseLeftButtonPressed = false;
            _mouseRightButtonPressed = false;
        }

    protected:
        bool _mouseLeftButtonPressed{ false };
        bool _mouseRightButtonPressed{ false };

        virtual bool onMouseLeftButtonPressInternal( const Point & /*mousePosition*/ )
        {
            return false;
        }

        virtual bool onMouseLeftButtonReleaseInternal( const Point & /*mousePosition*/ )
        {
            return false;
        }

        virtual bool onMouseRightButtonPressInternal( const Point & /*mousePosition*/ )
        {
            return false;
        }

        virtual bool onMouseRightButtonReleaseInternal( const Point & /*mousePosition*/ )
        {
            return false;
        }
    };

    // Action manager should be used per one dialog / scene.
    class PeripheralActionObjectManager : public ActionObject
    {
    public:
        void addElement( PeripheralActionObject * element )
        {
            assert( element != nullptr );

            _elements.push_back( element );
        }

        void processEvents();

    private:
        std::vector<PeripheralActionObject *> _elements;

        void senderUpdate( const ActionObject * actionObject ) override
        {
            assert( actionObject != nullptr );

            auto iter = std::find( _elements.begin(), _elements.end(), actionObject );
            if ( iter == _elements.end() ) {
                // Something is really wrong with iter-object connections. Make sure that subcription is correct!
                assert( 0 );
                return;
            }
        }
    };
	
    // This UI element is not designed to be scrollable.
    template <class _TElement>
    class MultiElementTable
    {
    public:
        MultiElementTable() = default;

        virtual ~MultiElementTable() = default;

        void resize( Point renderingOffset, Size tableSize, Size elementAreaSize, Point offsetBetweenElements )
        {
            assert( elementAreaSize.width > 0 && elementAreaSize.height > 0 );
            assert( tableSize.width > 0 && tableSize.height > 0 );

            _singleElementAreaSize = std::move( elementAreaSize );
            _tableSize = std::move( tableSize );
            _offsetBetweenElements = std::move( offsetBetweenElements );

            _renderingRoi.x = renderingOffset.x;
            _renderingRoi.y = renderingOffset.y;
            _renderingRoi.width = _tableSize.width * _singleElementAreaSize.width + ( _tableSize.width - 1 ) * _offsetBetweenElements.width;
            _renderingRoi.height = _tableSize.height * _singleElementAreaSize.height + ( _tableSize.height - 1 ) * _offsetBetweenElements.height;
        }

        void setContent( std::vector<_TElement> & content )
        {
            _elements.clear();
            _elements.reserve( content.size() );

            for ( _TElement & element : content ) {
                _elements.push_back( &element );
            }
        }

        virtual void renderBackground( const Rect & /*elementRoi*/, Image & /*output*/ ) = 0;

        virtual void renderElement( const size_t /*index*/, _TElement & /*element*/, const Rect & /*elementRoi*/, Image & /*output*/ ) = 0;

        void render( Image & output )
        {
            if ( _tableSize.width <= 0 || _tableSize.height <= 0 ) {
                return;
            }

            // If this assertion blows up it means that the number of elements exceeds the number of UI spots for them.
            assert( _elements.size() <= static_cast<size_t>( _tableSize.height ) * static_cast<size_t>( _tableSize.width ) );

            Rect elementRoi{ _renderingRoi.x, _renderingRoi.y, _singleElementAreaSize.width, _singleElementAreaSize.height };
            size_t elementId = 0;

            for ( int32_t y = 0; y < _tableSize.height; ++y ) {
                for ( int32_t x = 0; x < _tableSize.width; ++x ) {
                    renderBackground( elementRoi, output );

                    if ( elementId < _elements.size() ) {
                        renderElement( elementId, _elements[elementId], elementRoi, output );
                    }

                    elementRoi.x += _offsetBetweenItems.width + _singleElementAreaSize.width;
                    ++elementId;
                }

                elementRoi.x = _renderingRoi.x;
                elementRoi.y += _offsetBetweenItems.height + _singleElementAreaSize.height;
            }
        }

    private:
        Size _tableSize;
        Size _singleElementAreaSize;
        Point _offsetBetweenElements;
        Rect _renderingRoi;
        std::vector<_TElement *> _elements;
    };
}
