/***************************************************************************
 *   Copyright (C) 2012 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2INTERFACE_ITEMSBAR_H
#define H2INTERFACE_ITEMSBAR_H

#include <algorithm>
#include <utility>

#include "screen.h"
#include "splitter.h"
#include "ui_button.h"

namespace Interface
{
    template <class Item>
    class ItemsBar
    {
    protected:
        typedef std::list<Item *> Items;
        typedef typename std::list<Item *>::iterator ItemsIterator;
        typedef std::pair<ItemsIterator, Rect> ItemIterPos;

        Items items;
        Rect barsz;
        Size itemsz;
        Size colrows;
        int hspace;
        int vspace;

    public:
        ItemsBar()
            : colrows( 0, 0 )
            , hspace( 0 )
            , vspace( 0 )
        {}
        virtual ~ItemsBar() {}

        virtual void RedrawBackground( const Rect &, fheroes2::Image & ) {}
        virtual void RedrawItem( Item &, const Rect &, fheroes2::Image & ) {}

        virtual bool ActionBarSingleClick( const Point &, Item &, const Rect & )
        {
            return false;
        }
        virtual bool ActionBarPressRight( const Point &, Item &, const Rect & )
        {
            return false;
        }

        virtual bool ActionBarCursor( const Point &, Item &, const Rect & )
        {
            return false;
        }

        // body
        void SetColRows( u32 col, u32 row )
        {
            colrows.w = col;
            colrows.h = row;
            RescanSize();
        }

        void SetContent( std::list<Item> & content )
        {
            items.clear();
            for ( typename std::list<Item>::iterator it = content.begin(); it != content.end(); ++it )
                items.push_back( &( *it ) );
            SetContentItems();
        }

        void SetContent( std::vector<Item> & content )
        {
            items.clear();
            for ( typename std::vector<Item>::iterator it = content.begin(); it != content.end(); ++it )
                items.push_back( &( *it ) );
            SetContentItems();
        }

        void SetPos( s32 px, s32 py )
        {
            barsz.x = px;
            barsz.y = py;
        }

        void SetItemSize( u32 pw, u32 ph )
        {
            itemsz.w = pw;
            itemsz.h = ph;
            RescanSize();
        }

        void SetHSpace( int val )
        {
            hspace = val;
            RescanSize();
        }

        void SetVSpace( int val )
        {
            vspace = val;
            RescanSize();
        }

        Item * GetItem( const Point & pt )
        {
            ItemsIterator posItem = GetItemIter( pt );
            return posItem != items.end() ? *posItem : NULL;
        }

        Rect * GetItemPos( const Point & pt )
        {
            ItemIterPos posItem = GetItemIterPos( pt );
            return posItem.first != items.end() ? &posItem.second : NULL;
        }

        s32 GetIndex( const Point & pt )
        {
            ItemsIterator posItem = GetItemIter( pt );
            return posItem != items.end() ? std::distance( items.end(), posItem ) : -1;
        }

        const Point & GetPos( void ) const
        {
            return barsz;
        }

        const Rect & GetArea( void ) const
        {
            return barsz;
        }

        const Size & GetColRows( void ) const
        {
            return colrows;
        }

        void Redraw( fheroes2::Image & dstsf = fheroes2::Display::instance() )
        {
            Point dstpt( barsz );

            for ( u32 yy = 0; yy < colrows.h; ++yy ) {
                for ( u32 xx = 0; xx < colrows.w; ++xx ) {
                    RedrawBackground( Rect( dstpt, itemsz.w, itemsz.h ), dstsf );

                    dstpt.x += hspace + itemsz.w;
                }

                dstpt.x = barsz.x;
                dstpt.y += vspace + itemsz.h;
            }

            dstpt = barsz;
            ItemsIterator posItem = GetTopItemIter();

            for ( u32 yy = 0; yy < colrows.h; ++yy ) {
                for ( u32 xx = 0; xx < colrows.w; ++xx ) {
                    if ( posItem != items.end() ) {
                        RedrawItemIter( posItem, Rect( dstpt, itemsz.w, itemsz.h ), dstsf );

                        ++posItem;
                    }

                    dstpt.x += hspace + itemsz.w;
                }

                dstpt.x = barsz.x;
                dstpt.y += vspace + itemsz.h;
            }
        }

        bool QueueEventProcessing( void )
        {
            const Point & cursor = LocalEvent::Get().GetMouseCursor();

            return isItemsEmpty() ? false : ActionCursorItemIter( cursor, GetItemIterPos( cursor ) );
        }

    protected:
        virtual void SetContentItems( void ) {}

        ItemsIterator GetBeginItemIter( void )
        {
            return items.begin();
        }

        ItemsIterator GetEndItemIter( void )
        {
            return items.end();
        }

        virtual ItemsIterator GetTopItemIter( void )
        {
            return items.begin();
        }

        virtual ItemsIterator GetCurItemIter( void )
        {
            return items.end();
        }

        virtual void RedrawItemIter( ItemsIterator it, const Rect & pos, fheroes2::Image & dstsf )
        {
            RedrawItem( **it, pos, dstsf );
        }

        virtual bool ActionCursorItemIter( const Point & cursor, ItemIterPos iterPos )
        {
            if ( iterPos.first != GetEndItemIter() ) {
                LocalEvent & le = LocalEvent::Get();

                if ( ActionBarCursor( cursor, **iterPos.first, iterPos.second ) )
                    return true;
                else if ( le.MouseClickLeft( iterPos.second ) )
                    return ActionBarSingleClick( cursor, **iterPos.first, iterPos.second );
                else if ( le.MousePressRight( iterPos.second ) )
                    return ActionBarPressRight( cursor, **iterPos.first, iterPos.second );
            }

            return false;
        }

        bool isItemsEmpty( void )
        {
            return items.empty();
        }

        ItemsIterator GetItemIter( const Point & pt )
        {
            return GetItemIterPos( pt ).first;
        }

        ItemIterPos GetItemIterPos( const Point & pt )
        {
            Rect dstrt( barsz, itemsz.w, itemsz.h );
            ItemsIterator posItem = GetTopItemIter();

            for ( u32 yy = 0; yy < colrows.h; ++yy ) {
                for ( u32 xx = 0; xx < colrows.w; ++xx ) {
                    if ( posItem != items.end() ) {
                        if ( dstrt & pt )
                            return ItemIterPos( posItem, dstrt );
                        ++posItem;
                    }

                    dstrt.x += hspace + itemsz.w;
                }

                dstrt.x = barsz.x;
                dstrt.y += vspace + itemsz.h;
            }

            return std::pair<ItemsIterator, Rect>( items.end(), Rect() );
        }

    private:
        void RescanSize( void )
        {
            barsz.w = colrows.w ? colrows.w * itemsz.w + ( colrows.w - 1 ) * hspace : 0;
            barsz.h = colrows.h ? colrows.h * itemsz.h + ( colrows.h - 1 ) * vspace : 0;
        }
    };

    template <class Item>
    class ItemsActionBar : public ItemsBar<Item>
    {
    protected:
        typedef typename ItemsBar<Item>::ItemsIterator ItemsIterator;
        typedef typename ItemsBar<Item>::ItemIterPos ItemIterPos;

        ItemsIterator topItem;
        ItemIterPos curItemPos;

    public:
        ItemsActionBar()
        {
            ResetSelected();
        }

        virtual ~ItemsActionBar() {}

        virtual void RedrawItem( Item &, const Rect &, fheroes2::Image & ) {}
        virtual void RedrawItem( Item &, const Rect &, bool, fheroes2::Image & ) {}

        virtual bool ActionBarSingleClick( const Point &, Item &, const Rect &, Item &, const Rect & )
        {
            return false;
        }
        virtual bool ActionBarPressRight( const Point &, Item &, const Rect &, Item &, const Rect & )
        {
            return false;
        }

        virtual bool ActionBarSingleClick( const Point &, Item &, const Rect & )
        {
            return false;
        }
        virtual bool ActionBarDoubleClick( const Point & cursor, Item & item, const Rect & pos )
        {
            return ActionBarSingleClick( cursor, item, pos );
        }
        virtual bool ActionBarPressRight( const Point &, Item &, const Rect & )
        {
            return false;
        }

        virtual bool ActionBarCursor( const Point &, Item &, const Rect & )
        {
            return false;
        }
        virtual bool ActionBarCursor( const Point &, Item &, const Rect &, Item &, const Rect & )
        {
            return false;
        }

        // body
        Item * GetSelectedItem( void )
        {
            return *GetCurItemIter();
        }

        Rect * GetSelectedPos( void )
        {
            return &curItemPos.second;
        }

        s32 GetSelectedIndex( void )
        {
            return std::distance( ItemsBar<Item>::GetBeginItemIter(), GetCurItemIter() );
        }

        bool isSelected( void )
        {
            return GetCurItemIter() != ItemsBar<Item>::GetEndItemIter();
        }

        void ResetSelected( void )
        {
            topItem = ItemsBar<Item>::GetBeginItemIter();
            curItemPos = ItemIterPos( ItemsBar<Item>::items.end(), Rect() );
        }

        bool QueueEventProcessing( void )
        {
            return ItemsBar<Item>::QueueEventProcessing();
        }

        bool QueueEventProcessing( ItemsActionBar<Item> & other )
        {
            const Point & cursor = LocalEvent::Get().GetMouseCursor();

            if ( ItemsBar<Item>::isItemsEmpty() && other.isItemsEmpty() )
                return false;

            return other.isSelected() ? ActionCursorItemIter( cursor, other ) : ActionCursorItemIter( cursor, ItemsBar<Item>::GetItemIterPos( cursor ) );
        }

    protected:
        ItemsIterator GetTopItemIter( void )
        {
            return topItem;
        }

        ItemsIterator GetCurItemIter( void )
        {
            return curItemPos.first;
        }

        void SetContentItems( void )
        {
            ResetSelected();
        }

        void RedrawItemIter( ItemsIterator it, const Rect & pos, fheroes2::Image & dstsf )
        {
            RedrawItem( **it, pos, GetCurItemIter() == it, dstsf );
        }

        bool ActionCursorItemIter( const Point & cursor, ItemIterPos iterPos )
        {
            if ( iterPos.first != ItemsBar<Item>::GetEndItemIter() ) {
                LocalEvent & le = LocalEvent::Get();

                if ( ActionBarCursor( cursor, **iterPos.first, iterPos.second ) )
                    return true;
                else if ( le.MouseClickLeft( iterPos.second ) ) {
                    if ( iterPos.first == GetCurItemIter() ) {
                        return ActionBarDoubleClick( cursor, **iterPos.first, iterPos.second );
                    }
                    else {
                        if ( ActionBarSingleClick( cursor, **iterPos.first, iterPos.second ) )
                            curItemPos = iterPos;
                        else
                            ResetSelected();

                        return true;
                    }
                }
                else if ( le.MousePressRight( iterPos.second ) )
                    return ActionBarPressRight( cursor, **iterPos.first, iterPos.second );
            }

            return false;
        }

        bool ActionCursorItemIter( const Point & cursor, ItemsActionBar<Item> & other )
        {
            ItemIterPos iterPos1 = ItemsBar<Item>::GetItemIterPos( cursor );
            ItemIterPos iterPos2 = other.curItemPos;

            if ( iterPos1.first != ItemsBar<Item>::GetEndItemIter() ) {
                LocalEvent & le = LocalEvent::Get();

                if ( ActionBarCursor( cursor, **iterPos1.first, iterPos1.second, **iterPos2.first, iterPos2.second ) )
                    return true;
                else if ( le.MouseClickLeft( iterPos1.second ) ) {
                    if ( ActionBarSingleClick( cursor, **iterPos1.first, iterPos1.second, **iterPos2.first, iterPos2.second ) )
                        curItemPos = iterPos1;
                    else
                        ResetSelected();

                    other.ResetSelected();
                    return true;
                }
                else if ( le.MousePressRight( iterPos1.second ) ) {
                    other.ResetSelected();
                    return ActionBarPressRight( cursor, **iterPos1.first, iterPos1.second, **iterPos2.first, iterPos2.second );
                }
            }

            return false;
        }
    };
}

namespace Interface
{
    template <class Item>
    class ItemsScroll : public ItemsActionBar<Item>
    {
    protected:
        fheroes2::Button buttonPgUp;
        fheroes2::Button buttonPgDn;
        Splitter splitterIndicator;

    public:
        ItemsScroll() {}
    };
}

#endif
