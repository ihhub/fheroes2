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

#include "button.h"
#include "cursor.h"
#include "gamedefs.h"
#include "icn.h"
#include "settings.h"
#include "splitter.h"
#include "sprite.h"

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
        typedef typename std::vector<Item>::iterator ItemsIterator;

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

        // Original code had Item & item, const Point & cursor, s32 ox, s32 oy
        virtual void ActionListDoubleClick( Item & item, const Point &, s32, s32 )
        {
            ActionListDoubleClick( item );
        }
        virtual void ActionListSingleClick( Item & item, const Point &, s32, s32 )
        {
            ActionListSingleClick( item );
        }
        virtual void ActionListPressRight( Item & item, const Point &, s32, s32 )
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

        void SetScrollButtonUp( int icn, u32 index1, u32 index2, const Point & pos )
        {
            buttonPgUp.SetSprite( icn, index1, index2 );
            buttonPgUp.SetPos( pos );
        }

        void SetScrollButtonDn( int icn, u32 index1, u32 index2, const Point & pos )
        {
            buttonPgDn.SetSprite( icn, index1, index2 );
            buttonPgDn.SetPos( pos );
        }

        void SetScrollSplitter( const Sprite & sp, const Rect & area )
        {
            splitter.SetArea( area );
            splitter.SetSprite( sp );
        }

        Splitter & GetSplitter( void )
        {
            return splitter;
        }

        void SetAreaMaxItems( int maxValue )
        {
            maxItems = maxValue;
            Reset();
        }

        void SetAreaItems( const Rect & rt )
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
                splitter.SetRange( 0, 0 );
                splitter.MoveCenter();
            }
            else {
                _currentId = -1; // no selection
                _topId = 0;

                if ( maxItems < _size() ) {
                    splitter.MoveIndex( 0 );
                    splitter.SetRange( 0, _size() - maxItems );
                }
                else {
                    splitter.MoveCenter();
                    splitter.SetRange( 0, 0 );
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

            buttonPgUp.Draw();
            buttonPgDn.Draw();
            splitter.RedrawCursor();

            Verify(); // reset values if they are wrong

            if ( IsValid() ) { // we have something to display
                int id = _topId;
                const int end = ( _topId + maxItems ) > _size() ? _size() - _topId : _topId + maxItems;
                for ( ; id < end; ++id )
                    RedrawItem( ( *content )[id], rtAreaItems.x, rtAreaItems.y + ( id - _topId ) * rtAreaItems.h / maxItems, id == _currentId );
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

            if ( mp.y < rtAreaItems.y || mp.y >= rtAreaItems.y + rtAreaItems.h ) // out of boundaries
                return NULL;

            if ( mp.x < rtAreaItems.x || mp.x >= rtAreaItems.x + rtAreaItems.w ) // out of boundaries
                return NULL;

            const int id = ( mp.y - rtAreaItems.y ) * maxItems / rtAreaItems.h;
            if ( _topId + id >= _size() ) // out of items
                return NULL;

            return &( *content )[id];
        }

        void SetCurrent( size_t posId )
        {
            if ( posId >= 0 && posId < content->size() )
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
                if ( _currentId + maxItems < _size() ) {
                    _topId = ( _currentId / maxItems ) * maxItems + ( _currentId % maxItems ) / 2;
                }
                else if ( maxItems < _size() ) {
                    _topId = _size() - maxItems;
                }
                else {
                    _topId = 0;
                }

                UpdateSplitterRange();
                splitter.MoveIndex( _topId );
            }
        }

        void SetCurrent( const Item & item )
        {
            typename std::vector<Item>::iterator pos = std::find( content->begin(), content->end(), item );
            if ( pos == content->end() )
                Reset();
            else
                _currentId = pos - content->begin();

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

            le.MousePressLeft( buttonPgUp ) ? buttonPgUp.PressDraw() : buttonPgUp.ReleaseDraw();
            le.MousePressLeft( buttonPgDn ) ? buttonPgDn.PressDraw() : buttonPgDn.ReleaseDraw();

            if ( !IsValid() )
                return false;

            if ( useHotkeys && le.KeyPress( KEY_PAGEUP ) && ( _topId > 0 ) ) {
                cursor.Hide();
                if ( _topId > maxItems )
                    _topId -= maxItems;
                else
                    _topId = 0;

                UpdateSplitterRange();
                splitter.MoveIndex( _topId );
                return true;
            }
            else if ( useHotkeys && le.KeyPress( KEY_PAGEDOWN ) && ( _topId + maxItems < _size() ) ) {
                cursor.Hide();
                _topId += maxItems;
                if ( _topId + maxItems >= _size() )
                    _topId = _size() - maxItems;

                UpdateSplitterRange();
                splitter.MoveIndex( _topId );
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
            else if ( ( le.MouseClickLeft( buttonPgUp ) || le.MouseWheelUp( rtAreaItems ) || le.MouseWheelUp( splitter.GetRect() ) ) && ( _topId > 0 ) ) {
                cursor.Hide();
                --_topId;
                splitter.Backward();
                return true;
            }
            else if ( ( le.MouseClickLeft( buttonPgDn ) || le.MouseWheelDn( rtAreaItems ) || le.MouseWheelDn( splitter.GetRect() ) )
                      && ( _topId + maxItems < _size() ) ) {
                cursor.Hide();
                ++_topId;
                splitter.Forward();
                return true;
            }
            else if ( le.MousePressLeft( splitter.GetRect() ) && ( _size() > maxItems ) ) {
                cursor.Hide();
                UpdateSplitterRange();
                _topId = ( le.GetMouseCursor().y - splitter.GetRect().y ) * 100 / splitter.GetStep();
                if ( _topId < splitter.Min() )
                    _topId = splitter.Min();
                else if ( _topId > splitter.Max() )
                    _topId = splitter.Max();
                splitter.MoveIndex( _topId );
                return true;
            }

            const Point mousePos = le.GetMouseCursor();
            if ( rtAreaItems & mousePos ) { // within our rectangle
                const int id = ( mousePos.y - rtAreaItems.y ) * maxItems / rtAreaItems.h + _topId;
                cursor.Hide();

                if ( id < _size() ) {
                    Item & item = ( *content )[static_cast<size_t>( id )]; // id is always >= 0

                    if ( ActionListCursor( item, le.GetMouseCursor() ) )
                        return true;

                    if ( le.MouseClickLeft( rtAreaItems ) ) {
                        if ( id == _currentId ) {
                            ActionListDoubleClick( item, le.GetMouseCursor(), rtAreaItems.x, mousePos.y );
                        }
                        else {
                            _currentId = id;
                            ActionListSingleClick( item, le.GetMouseCursor(), rtAreaItems.x, mousePos.y );
                        }
                        return true;
                    }
                    else if ( le.MousePressRight( rtAreaItems ) ) {
                        ActionListPressRight( item, le.GetMouseCursor(), rtAreaItems.x, mousePos.y );
                        return true;
                    }
                }

                cursor.Show();
            }

            return false;
        }

    protected:
        Rect rtAreaItems;

        Button buttonPgUp;
        Button buttonPgDn;

        Splitter splitter;

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

        void UpdateSplitterRange( void )
        {
            const int maxValue = ( content != NULL && maxItems < _size() ) ? static_cast<int>( _size() - maxItems ) : 0;
            if ( splitter.Max() != maxValue )
                splitter.SetRange( 0, maxValue );
        }
    };
}

#endif
