/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2INTERFACE_LIST_H
#define H2INTERFACE_LIST_H

#include <algorithm>

#include "localevent.h"
#include "ui_button.h"
#include "ui_scrollbar.h"

namespace Interface
{
    class ListBasic
    {
    public:
        ListBasic()
            : _currentId( -1 )
            , _topId( -1 )
        {
            // Do nothing.
        }

        virtual ~ListBasic() = default;
        virtual bool IsNeedRedraw() const = 0;
        virtual void Redraw() = 0;
        virtual bool QueueEventProcessing() = 0;

        int getTopId() const
        {
            return _topId;
        }

    protected:
        int _currentId;

        int _topId;
    };

    template <class Item>
    class ListBox : public ListBasic
    {
    public:
        explicit ListBox( const fheroes2::Point & pt = fheroes2::Point() )
            : needRedraw( false )
            , content( nullptr )
            , maxItems( 0 )
            , ptRedraw( pt )
            , useHotkeys( true )
            , _updateScrollbar( false )
            , _timedButtonPgUp( [this]() { return buttonPgUp.isPressed(); } )
            , _timedButtonPgDn( [this]() { return buttonPgDn.isPressed(); } )
        {
            buttonPgUp.subscribe( &_timedButtonPgUp );
            buttonPgDn.subscribe( &_timedButtonPgDn );
        }
        ~ListBox() override = default;

        virtual void RedrawItem( const Item &, int32_t ox, int32_t oy, bool current ) = 0;
        virtual void RedrawBackground( const fheroes2::Point & ) = 0;

        virtual void ActionCurrentUp() = 0;
        virtual void ActionCurrentDn() = 0;

        virtual void ActionListDoubleClick( Item & ) = 0;
        virtual void ActionListSingleClick( Item & ) = 0;
        virtual void ActionListPressRight( Item & ) = 0;

        virtual void ActionListDoubleClick( Item & item, const fheroes2::Point & /*mousePos*/, int32_t /*itemOffsetX*/, int32_t /*itemOffsetY*/ )
        {
            ActionListDoubleClick( item );
        }

        virtual void ActionListSingleClick( Item & item, const fheroes2::Point & /*mousePos*/, int32_t /*itemOffsetX*/, int32_t /*itemOffsetY*/ )
        {
            ActionListSingleClick( item );
        }

        virtual void ActionListPressRight( Item & item, const fheroes2::Point & /*mousePos*/, int32_t /*itemOffsetX*/, int32_t /*itemOffsetY*/ )
        {
            ActionListPressRight( item );
        }

        virtual bool ActionListCursor( Item &, const fheroes2::Point & )
        {
            return false;
        }

        void SetTopLeft( const fheroes2::Point & tl )
        {
            ptRedraw = tl;
        }

        void SetScrollButtonUp( int icn, uint32_t index1, uint32_t index2, const fheroes2::Point & pos )
        {
            buttonPgUp.setICNInfo( icn, index1, index2 );
            buttonPgUp.setPosition( pos.x, pos.y );
        }

        void SetScrollButtonDn( int icn, uint32_t index1, uint32_t index2, const fheroes2::Point & pos )
        {
            buttonPgDn.setICNInfo( icn, index1, index2 );
            buttonPgDn.setPosition( pos.x, pos.y );
        }

        void setScrollBarArea( const fheroes2::Rect & area )
        {
            _scrollbar.setArea( area );
        }

        void setScrollBarImage( const fheroes2::Image & image )
        {
            _scrollbar.setImage( image );
        }

        fheroes2::Scrollbar & GetScrollbar()
        {
            return _scrollbar;
        }

        void SetAreaMaxItems( int maxValue )
        {
            maxItems = maxValue;
            Reset();
        }

        void SetAreaItems( const fheroes2::Rect & rt )
        {
            rtAreaItems = rt;
        }

        void SetListContent( std::vector<Item> & list )
        {
            content = &list;
            Reset();

            if ( IsValid() )
                _currentId = 0;
        }

        void Reset()
        {
            if ( content == nullptr || content->empty() ) { // empty content. Must be non-initialized array
                _currentId = -1;
                _topId = -1;
                _scrollbar.setRange( 0, 0 );
            }
            else {
                _currentId = -1; // no selection
                _topId = 0;

                if ( maxItems < _size() ) {
                    _scrollbar.setRange( 0, _size() - maxItems );
                }
                else {
                    _scrollbar.setRange( 0, 0 );
                }
            }
        }

        void DisableHotkeys( bool f )
        {
            useHotkeys = !f;
        }

        void Redraw() override
        {
            needRedraw = false;

            RedrawBackground( ptRedraw );

            buttonPgUp.draw();
            buttonPgDn.draw();
            _scrollbar.redraw();

            Verify(); // reset values if they are wrong

            if ( IsValid() ) { // we have something to display
                int id = _topId;
                const int end = ( _topId + maxItems ) > _size() ? _size() - _topId : _topId + maxItems;
                for ( ; id < end; ++id )
                    RedrawItem( ( *content )[id], rtAreaItems.x, rtAreaItems.y + ( id - _topId ) * rtAreaItems.height / maxItems, id == _currentId );
            }
        }

        bool IsValid() const
        {
            return content != nullptr && !content->empty() && _topId >= 0 && _topId < _size() && _currentId < _size() && maxItems > 0;
        }

        bool IsNeedRedraw() const override
        {
            return needRedraw;
        }

        Item & GetCurrent() // always call this function only after IsValid()!
        {
            return ( *content )[_currentId];
        }

        Item * GetFromPosition( const fheroes2::Point & mp )
        {
            Verify();
            if ( !IsValid() )
                return nullptr;

            if ( mp.y < rtAreaItems.y || mp.y >= rtAreaItems.y + rtAreaItems.height ) // out of boundaries
                return nullptr;

            if ( mp.x < rtAreaItems.x || mp.x >= rtAreaItems.x + rtAreaItems.width ) // out of boundaries
                return nullptr;

            const int id = ( mp.y - rtAreaItems.y ) * maxItems / rtAreaItems.height;
            if ( _topId + id >= _size() ) // out of items
                return nullptr;

            return &( *content )[_topId + id];
        }

        void SetCurrent( size_t posId )
        {
            if ( posId < content->size() )
                _currentId = static_cast<int>( posId );

            SetCurrentVisible();
        }

        void SetCurrentVisible()
        {
            Verify();

            if ( !IsValid() ) {
                Reset();
                return;
            }

            if ( _currentId >= 0 && ( _topId > _currentId || _topId + maxItems <= _currentId ) ) { // out of view
                if ( _topId > _currentId ) { // scroll up, put current id on top
                    _topId = _currentId;
                }
                else if ( _topId + maxItems <= _currentId ) { // scroll down, put current id at bottom
                    _topId = _currentId + 1 - maxItems;
                }
            }

            UpdateScrollbarRange();
            _scrollbar.moveToIndex( _topId );
        }

        // Move visible area to the position with given element ID being on the top of the list.
        void setTopVisibleItem( const int topId )
        {
            Verify();

            if ( !IsValid() ) {
                Reset();
                return;
            }

            _topId = std::max( 0, std::min( topId, _size() - maxItems ) );

            UpdateScrollbarRange();
            _scrollbar.moveToIndex( _topId );
        }

        void SetCurrent( const Item & item )
        {
            typename std::vector<Item>::iterator pos = std::find( content->begin(), content->end(), item );
            if ( pos == content->end() )
                Reset();
            else
                _currentId = static_cast<int>( pos - content->begin() );

            SetCurrentVisible();
        }

        void RemoveSelected()
        {
            if ( content != nullptr && _currentId >= 0 && _currentId < _size() )
                content->erase( content->begin() + _currentId );
        }

        bool isSelected() const
        {
            return IsValid() && _currentId >= 0;
        }

        void Unselect()
        {
            Verify();
            if ( IsValid() )
                _currentId = -1;
        }

        bool QueueEventProcessing() override
        {
            LocalEvent & le = LocalEvent::Get();

            le.MousePressLeft( buttonPgUp.area() ) ? buttonPgUp.drawOnPress() : buttonPgUp.drawOnRelease();
            le.MousePressLeft( buttonPgDn.area() ) ? buttonPgDn.drawOnPress() : buttonPgDn.drawOnRelease();

            if ( !IsValid() )
                return false;

            if ( useHotkeys && le.KeyPress( fheroes2::Key::KEY_PAGE_UP ) && ( _topId > 0 ) ) {
                needRedraw = true;

                if ( _topId > maxItems )
                    _topId -= maxItems;
                else
                    _topId = 0;

                UpdateScrollbarRange();
                _scrollbar.moveToIndex( _topId );

                return true;
            }
            if ( useHotkeys && le.KeyPress( fheroes2::Key::KEY_PAGE_DOWN ) && ( _topId + maxItems < _size() ) ) {
                needRedraw = true;

                _topId += maxItems;
                if ( _topId + maxItems >= _size() )
                    _topId = _size() - maxItems;

                UpdateScrollbarRange();
                _scrollbar.moveToIndex( _topId );

                return true;
            }
            if ( useHotkeys && le.KeyPress( fheroes2::Key::KEY_UP ) && ( _currentId > 0 ) ) {
                needRedraw = true;

                --_currentId;
                SetCurrentVisible();
                ActionCurrentUp();

                return true;
            }
            if ( useHotkeys && le.KeyPress( fheroes2::Key::KEY_DOWN ) && ( _currentId + 1 < _size() ) ) {
                needRedraw = true;

                ++_currentId;
                SetCurrentVisible();
                ActionCurrentDn();

                return true;
            }
            if ( ( le.MouseClickLeft( buttonPgUp.area() ) || le.MouseWheelUp( rtAreaItems ) || le.MouseWheelUp( _scrollbar.getArea() )
                   || _timedButtonPgUp.isDelayPassed() )
                 && ( _topId > 0 ) ) {
                needRedraw = true;

                --_topId;
                _scrollbar.backward();

                return true;
            }
            if ( ( le.MouseClickLeft( buttonPgDn.area() ) || le.MouseWheelDn( rtAreaItems ) || le.MouseWheelDn( _scrollbar.getArea() )
                   || _timedButtonPgDn.isDelayPassed() )
                 && ( _topId + maxItems < _size() ) ) {
                needRedraw = true;

                ++_topId;
                _scrollbar.forward();

                return true;
            }
            if ( le.MousePressLeft( _scrollbar.getArea() ) && ( _size() > maxItems ) ) {
                const fheroes2::Point mousePosition = le.GetMouseCursor();

                int32_t prevX = _scrollbar.x();
                int32_t prevY = _scrollbar.y();

                UpdateScrollbarRange();

                _scrollbar.moveToPos( mousePosition );

                // We don't need to render the scrollbar if it's position is not changed.
                if ( ( _scrollbar.x() == prevX ) && ( _scrollbar.y() == prevY ) ) {
                    return false;
                }

                _topId = _scrollbar.currentIndex();

                needRedraw = true;
                _updateScrollbar = true;

                return true;
            }

            if ( _updateScrollbar ) {
                _updateScrollbar = false;
                if ( _scrollbar.updatePosition() ) {
                    needRedraw = true;
                    return true;
                }
            }

            const fheroes2::Point & mousePos = le.GetMouseCursor();
            if ( rtAreaItems & mousePos ) { // within our rectangle
                needRedraw = true;

                const int id = ( mousePos.y - rtAreaItems.y ) * maxItems / rtAreaItems.height + _topId;

                if ( id < _size() ) {
                    Item & item = ( *content )[static_cast<size_t>( id )]; // id is always >= 0
                    const int32_t offsetY = ( id - _topId ) * rtAreaItems.height / maxItems;

                    if ( ActionListCursor( item, mousePos ) )
                        return true;

                    if ( le.MouseClickLeft( rtAreaItems ) ) {
                        if ( id == _currentId ) {
                            ActionListDoubleClick( item, mousePos, rtAreaItems.x, rtAreaItems.y + offsetY );
                        }
                        else {
                            _currentId = id;
                            ActionListSingleClick( item, mousePos, rtAreaItems.x, rtAreaItems.y + offsetY );
                        }
                        return true;
                    }
                    else if ( le.MousePressRight( rtAreaItems ) ) {
                        ActionListPressRight( item, mousePos, rtAreaItems.x, rtAreaItems.y + offsetY );
                        return true;
                    }
                }

                needRedraw = false;
            }

            return false;
        }

    protected:
        bool needRedraw;

        fheroes2::Rect rtAreaItems;

        fheroes2::Button buttonPgUp;
        fheroes2::Button buttonPgDn;

        fheroes2::Scrollbar _scrollbar;

        int VisibleItemCount() const
        {
            return maxItems;
        }

        int _size() const
        {
            return content == nullptr ? 0 : static_cast<int>( content->size() );
        }

    private:
        std::vector<Item> * content;
        int maxItems;

        fheroes2::Point ptRedraw;

        bool useHotkeys;

        bool _updateScrollbar;

        fheroes2::TimedEventValidator _timedButtonPgUp;
        fheroes2::TimedEventValidator _timedButtonPgDn;

        void Verify()
        {
            if ( content == nullptr || content->empty() ) {
                _currentId = -1;
                _topId = -1;
            }
            else {
                if ( _currentId >= _size() )
                    _currentId = -1;
                if ( _topId < 0 || _topId >= _size() )
                    _topId = 0;
            }
        }

        void UpdateScrollbarRange()
        {
            const int maxValue = ( content != nullptr && maxItems < _size() ) ? static_cast<int>( _size() - maxItems ) : 0;
            if ( _scrollbar.maxIndex() != maxValue )
                _scrollbar.setRange( 0, maxValue );
        }
    };
}

#endif
