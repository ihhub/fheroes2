/***************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2INTERFACE_LIST_H
#define H2INTERFACE_LIST_H

#include <algorithm>

#include "cursor.h"
#include "ui_button.h"
#include "ui_scrollbar.h"

namespace Interface
{
    struct ListBasic
    {
        virtual ~ListBasic() {}
        virtual void Redraw( void ) = 0;
        virtual bool QueueEventProcessing( void ) = 0;
    };

    template <class Item>
    class ListBox : public ListBasic
    {
    public:
        ListBox( const Point & pt = Point() )
            : content( NULL )
            , _currentId( -1 )
            , _topId( -1 )
            , maxItems( 0 )
            , ptRedraw( pt )
            , useHotkeys( true )
        {}
        virtual ~ListBox() {}

        virtual void RedrawItem( const Item &, s32 ox, s32 oy, bool current ) = 0;
        virtual void RedrawBackground( const Point & ) = 0;

        virtual void ActionCurrentUp( void ) = 0;
        virtual void ActionCurrentDn( void ) = 0;

        virtual void ActionListDoubleClick( Item & ) = 0;
        virtual void ActionListSingleClick( Item & ) = 0;
        virtual void ActionListPressRight( Item & ) = 0;

        virtual void ActionListDoubleClick( Item & item, const Point & /*mousePos*/, int32_t /*itemOffsetX*/, int32_t /*itemOffsetY*/ )
        {
            ActionListDoubleClick( item );
        }

        virtual void ActionListSingleClick( Item & item, const Point & /*mousePos*/, int32_t /*itemOffsetX*/, int32_t /*itemOffsetY*/ )
        {
            ActionListSingleClick( item );
        }

        virtual void ActionListPressRight( Item & item, const Point & /*mousePos*/, int32_t /*itemOffsetX*/, int32_t /*itemOffsetY*/ )
        {
            ActionListPressRight( item );
        }

        virtual bool ActionListCursor( Item &, const Point & )
        {
            return false;
        }

        void SetTopLeft( const Point & tl )
        {
            ptRedraw = tl;
        }

        void SetScrollButtonUp( int icn, u32 index1, u32 index2, const fheroes2::Point & pos )
        {
            buttonPgUp.setICNInfo( icn, index1, index2 );
            buttonPgUp.setPosition( pos.x, pos.y );
        }

        void SetScrollButtonDn( int icn, u32 index1, u32 index2, const fheroes2::Point & pos )
        {
            buttonPgDn.setICNInfo( icn, index1, index2 );
            buttonPgDn.setPosition( pos.x, pos.y );
        }

        void SetScrollBar( const fheroes2::Image & image, const fheroes2::Rect & area )
        {
            _scrollbar.setArea( area );
            _scrollbar.setImage( image );
        }

        fheroes2::Scrollbar & GetScrollbar( void )
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

        void Reset( void )
        {
            if ( content == NULL || content->empty() ) { // empty content. Must be non-initialized array
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

        void Redraw( void )
        {
            Cursor::Get().Hide();

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
            return content != NULL && !content->empty() && _topId >= 0 && _topId < _size() && _currentId < _size() && maxItems > 0;
        }

        Item & GetCurrent( void ) // always call this function only after IsValid()!
        {
            return ( *content )[_currentId];
        }

        Item * GetFromPosition( const Point & mp )
        {
            Verify();
            if ( !IsValid() )
                return NULL;

            if ( mp.y < rtAreaItems.y || mp.y >= rtAreaItems.y + rtAreaItems.height ) // out of boundaries
                return NULL;

            if ( mp.x < rtAreaItems.x || mp.x >= rtAreaItems.x + rtAreaItems.width ) // out of boundaries
                return NULL;

            const int id = ( mp.y - rtAreaItems.y ) * maxItems / rtAreaItems.height;
            if ( _topId + id >= _size() ) // out of items
                return NULL;

            return &( *content )[id];
        }

        void SetCurrent( size_t posId )
        {
            if ( posId < content->size() )
                _currentId = static_cast<int>( posId );

            SetCurrentVisible();
        }

        void SetCurrentVisible( void )
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

                UpdateScrollbarRange();
                _scrollbar.moveToIndex( _topId );
            }
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

        void RemoveSelected( void )
        {
            if ( content != NULL && _currentId >= 0 && _currentId < _size() )
                content->erase( content->begin() + _currentId );
        }

        bool isSelected( void ) const
        {
            return IsValid() && _currentId >= 0;
        }

        void Unselect( void )
        {
            Verify();
            if ( IsValid() )
                _currentId = -1;
        }

        bool QueueEventProcessing( void )
        {
            LocalEvent & le = LocalEvent::Get();
            Cursor & cursor = Cursor::Get();

            le.MousePressLeft( buttonPgUp.area() ) ? buttonPgUp.drawOnPress() : buttonPgUp.drawOnRelease();
            le.MousePressLeft( buttonPgDn.area() ) ? buttonPgDn.drawOnPress() : buttonPgDn.drawOnRelease();

            if ( !IsValid() )
                return false;

            if ( useHotkeys && le.KeyPress( KEY_PAGEUP ) && ( _topId > 0 ) ) {
                cursor.Hide();
                if ( _topId > maxItems )
                    _topId -= maxItems;
                else
                    _topId = 0;

                UpdateScrollbarRange();
                _scrollbar.moveToIndex( _topId );
                return true;
            }
            else if ( useHotkeys && le.KeyPress( KEY_PAGEDOWN ) && ( _topId + maxItems < _size() ) ) {
                cursor.Hide();
                _topId += maxItems;
                if ( _topId + maxItems >= _size() )
                    _topId = _size() - maxItems;

                UpdateScrollbarRange();
                _scrollbar.moveToIndex( _topId );
                return true;
            }
            else if ( useHotkeys && le.KeyPress( KEY_UP ) && ( _currentId > 0 ) ) {
                cursor.Hide();
                --_currentId;
                SetCurrentVisible();
                ActionCurrentUp();
                return true;
            }
            else if ( useHotkeys && le.KeyPress( KEY_DOWN ) && ( _currentId + 1 < _size() ) ) {
                cursor.Hide();
                ++_currentId;
                SetCurrentVisible();
                ActionCurrentDn();
                return true;
            }
            else if ( ( le.MouseClickLeft( buttonPgUp.area() ) || le.MouseWheelUp( rtAreaItems ) || le.MouseWheelUp( _scrollbar.getArea() ) ) && ( _topId > 0 ) ) {
                cursor.Hide();
                --_topId;
                _scrollbar.backward();
                return true;
            }
            else if ( ( le.MouseClickLeft( buttonPgDn.area() ) || le.MouseWheelDn( rtAreaItems ) || le.MouseWheelDn( _scrollbar.getArea() ) )
                      && ( _topId + maxItems < _size() ) ) {
                cursor.Hide();
                ++_topId;
                _scrollbar.forward();
                return true;
            }
            else if ( le.MousePressLeft( _scrollbar.getArea() ) && ( _size() > maxItems ) ) {
                cursor.Hide();
                UpdateScrollbarRange();

                const Point & mousePos = le.GetMouseCursor();
                _scrollbar.moveToPos( fheroes2::Point( mousePos.x, mousePos.y ) );
                _topId = _scrollbar.currentIndex();
                return true;
            }

            const Point & position = le.GetMouseCursor();
            const fheroes2::Point mousePos( position.x, position.y );
            if ( rtAreaItems & mousePos ) { // within our rectangle
                const int id = ( mousePos.y - rtAreaItems.y ) * maxItems / rtAreaItems.height + _topId;
                cursor.Hide();

                if ( id < _size() ) {
                    Item & item = ( *content )[static_cast<size_t>( id )]; // id is always >= 0
                    const int32_t offsetY = ( id - _topId ) * rtAreaItems.height / maxItems;

                    if ( ActionListCursor( item, position ) )
                        return true;

                    if ( le.MouseClickLeft( rtAreaItems ) ) {
                        if ( id == _currentId ) {
                            ActionListDoubleClick( item, position, rtAreaItems.x, rtAreaItems.y + offsetY );
                        }
                        else {
                            _currentId = id;
                            ActionListSingleClick( item, position, rtAreaItems.x, rtAreaItems.y + offsetY );
                        }
                        return true;
                    }
                    else if ( le.MousePressRight( rtAreaItems ) ) {
                        ActionListPressRight( item, position, rtAreaItems.x, rtAreaItems.y + offsetY );
                        return true;
                    }
                }

                cursor.Show();
            }

            return false;
        }

    protected:
        fheroes2::Rect rtAreaItems;

        fheroes2::Button buttonPgUp;
        fheroes2::Button buttonPgDn;

        fheroes2::Scrollbar _scrollbar;

        int VisibleItemCount() const
        {
            return maxItems;
        }

    private:
        std::vector<Item> * content;
        int _currentId;
        int _topId;
        int maxItems;

        Point ptRedraw;

        bool useHotkeys;

        void Verify()
        {
            if ( content == NULL || content->empty() ) {
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

        int _size() const
        {
            return content == NULL ? 0 : static_cast<int>( content->size() );
        }

        void UpdateScrollbarRange()
        {
            const int maxValue = ( content != NULL && maxItems < _size() ) ? static_cast<int>( _size() - maxItems ) : 0;
            if ( _scrollbar.maxIndex() != maxValue )
                _scrollbar.setRange( 0, maxValue );
        }
    };
}

#endif
