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
#include <list>
#include <utility>

#include "gamedefs.h"
#include "localevent.h"
#include "screen.h"

namespace Interface
{
    template <class Item>
    class ItemsBar
    {
    protected:
        using Items = std::list<Item *>;
        typedef typename std::list<Item *>::iterator ItemsIterator;
        using ItemIterPos = std::pair<ItemsIterator, fheroes2::Rect>;

        Items items;

    public:
        ItemsBar()
            : colrows( 0, 0 )
            , hspace( 0 )
            , vspace( 0 )
        {}

        virtual ~ItemsBar() = default;

        virtual void RedrawBackground( const fheroes2::Rect &, fheroes2::Image & ) = 0;
        virtual void RedrawItem( Item &, const fheroes2::Rect &, fheroes2::Image & ) = 0;

        virtual bool ActionBarLeftMouseSingleClick( Item & )
        {
            return false;
        }

        virtual bool ActionBarRightMouseHold( Item & )
        {
            return false;
        }

        virtual bool ActionBarCursor( Item & )
        {
            return false;
        }

        // body
        void SetColRows( int32_t col, int32_t row )
        {
            colrows.width = col;
            colrows.height = row;
            RescanSize();
        }

        void SetContent( std::vector<Item> & content )
        {
            items.clear();
            for ( typename std::vector<Item>::iterator it = content.begin(); it != content.end(); ++it )
                items.push_back( &( *it ) );
            SetContentItems();
        }

        void SetPos( int32_t px, int32_t py )
        {
            barsz.x = px;
            barsz.y = py;
        }

        void SetItemSize( int32_t pw, int32_t ph )
        {
            itemsz.width = pw;
            itemsz.height = ph;
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

        Item * GetItem( const fheroes2::Point & pt )
        {
            ItemsIterator posItem = GetItemIter( pt );
            return posItem != items.end() ? *posItem : nullptr;
        }

        fheroes2::Point GetPos() const
        {
            return fheroes2::Point( barsz.x, barsz.y );
        }

        const fheroes2::Rect & GetArea() const
        {
            return barsz;
        }

        void Redraw( fheroes2::Image & dstsf = fheroes2::Display::instance() )
        {
            fheroes2::Point dstpt( barsz.x, barsz.y );

            for ( int32_t y = 0; y < colrows.height; ++y ) {
                for ( int32_t x = 0; x < colrows.width; ++x ) {
                    RedrawBackground( fheroes2::Rect( dstpt.x, dstpt.y, itemsz.width, itemsz.height ), dstsf );

                    dstpt.x += hspace + itemsz.width;
                }

                dstpt.x = barsz.x;
                dstpt.y += vspace + itemsz.height;
            }

            dstpt.x = barsz.x;
            dstpt.y = barsz.y;

            ItemsIterator posItem = GetTopItemIter();

            for ( int32_t y = 0; y < colrows.height; ++y ) {
                for ( int32_t x = 0; x < colrows.width; ++x ) {
                    if ( posItem != items.end() ) {
                        RedrawItemIter( posItem, fheroes2::Rect( dstpt.x, dstpt.y, itemsz.width, itemsz.height ), dstsf );

                        ++posItem;
                    }

                    dstpt.x += hspace + itemsz.width;
                }

                dstpt.x = barsz.x;
                dstpt.y += vspace + itemsz.height;
            }
        }

        bool QueueEventProcessing( void )
        {
            const fheroes2::Point & cursor = LocalEvent::Get().GetMouseCursor();

            return isItemsEmpty() ? false : ActionCursorItemIter( cursor, GetItemIterPos( cursor ) );
        }

    protected:
        virtual void SetContentItems() {}

        ItemsIterator GetBeginItemIter()
        {
            return items.begin();
        }

        ItemsIterator GetEndItemIter()
        {
            return items.end();
        }

        virtual ItemsIterator GetTopItemIter()
        {
            return items.begin();
        }

        virtual ItemsIterator GetCurItemIter()
        {
            return items.end();
        }

        virtual void RedrawItemIter( ItemsIterator it, const fheroes2::Rect & pos, fheroes2::Image & dstsf )
        {
            RedrawItem( **it, pos, dstsf );
        }

        virtual bool ActionCursorItemIter( const fheroes2::Point &, ItemIterPos iterPos )
        {
            if ( iterPos.first != GetEndItemIter() ) {
                LocalEvent & le = LocalEvent::Get();

                if ( ActionBarCursor( **iterPos.first ) )
                    return true;
                else if ( le.MouseClickLeft( iterPos.second ) )
                    return ActionBarLeftMouseSingleClick( **iterPos.first );
                else if ( le.MousePressRight( iterPos.second ) )
                    return ActionBarRightMouseHold( **iterPos.first );
            }

            return false;
        }

        bool isItemsEmpty()
        {
            return items.empty();
        }

        ItemsIterator GetItemIter( const fheroes2::Point & pt )
        {
            return GetItemIterPos( pt ).first;
        }

        ItemIterPos GetItemIterPos( const fheroes2::Point & pt )
        {
            const fheroes2::Point position( pt.x, pt.y );
            fheroes2::Rect dstrt( barsz.x, barsz.y, itemsz.width, itemsz.height );
            ItemsIterator posItem = GetTopItemIter();

            for ( int32_t y = 0; y < colrows.height; ++y ) {
                for ( int32_t x = 0; x < colrows.width; ++x ) {
                    if ( posItem != items.end() ) {
                        if ( dstrt & position )
                            return ItemIterPos( posItem, dstrt );
                        ++posItem;
                    }

                    dstrt.x += hspace + itemsz.width;
                }

                dstrt.x = barsz.x;
                dstrt.y += vspace + itemsz.height;
            }

            return std::pair<ItemsIterator, fheroes2::Rect>( items.end(), fheroes2::Rect() );
        }

    private:
        void RescanSize( void )
        {
            barsz.width = colrows.width ? colrows.width * itemsz.width + ( colrows.width - 1 ) * hspace : 0;
            barsz.height = colrows.height ? colrows.height * itemsz.height + ( colrows.height - 1 ) * vspace : 0;
        }

        fheroes2::Rect barsz;
        fheroes2::Size itemsz;
        fheroes2::Size colrows;
        int32_t hspace;
        int32_t vspace;
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

        ~ItemsActionBar() override = default;

        void RedrawItem( Item &, const fheroes2::Rect &, fheroes2::Image & ) override {}
        virtual void RedrawItem( Item &, const fheroes2::Rect &, bool, fheroes2::Image & ) {}

        bool ActionBarCursor( Item & ) override
        {
            return false;
        }

        virtual bool ActionBarCursor( Item &, Item & )
        {
            return false;
        }

        virtual bool ActionBarLeftMouseSingleClick( Item &, Item & )
        {
            return false;
        }

        bool ActionBarLeftMouseSingleClick( Item & ) override
        {
            return false;
        }

        virtual bool ActionBarLeftMouseDoubleClick( Item & item )
        {
            return ActionBarLeftMouseSingleClick( item );
        }

        virtual bool ActionBarLeftMouseRelease( Item & )
        {
            return false;
        }

        virtual bool ActionBarLeftMouseRelease( Item &, Item & )
        {
            return false;
        }

        virtual bool ActionBarRightMouseHold( Item &, Item & )
        {
            return false;
        }

        virtual bool ActionBarLeftMouseHold( Item & )
        {
            return false;
        }

        virtual bool ActionBarLeftMouseHold( Item &, Item & )
        {
            return false;
        }

        bool ActionBarRightMouseHold( Item & ) override
        {
            return false;
        }

        virtual bool ActionBarRightMouseSingleClick( Item & )
        {
            return false;
        }

        virtual bool ActionBarRightMouseSingleClick( Item &, Item & )
        {
            return false;
        }

        virtual bool ActionBarRightMouseRelease( Item & )
        {
            return true;
        }

        virtual bool ActionBarRightMouseRelease( Item &, Item & )
        {
            return true;
        }

        // body
        Item * GetSelectedItem()
        {
            return *GetCurItemIter();
        }

        fheroes2::Rect * GetSelectedPos()
        {
            return &curItemPos.second;
        }

        s32 GetSelectedIndex()
        {
            return std::distance( ItemsBar<Item>::GetBeginItemIter(), GetCurItemIter() );
        }

        bool isSelected()
        {
            return GetCurItemIter() != ItemsBar<Item>::GetEndItemIter();
        }

        void ResetSelected()
        {
            topItem = ItemsBar<Item>::GetBeginItemIter();
            curItemPos = ItemIterPos( ItemsBar<Item>::items.end(), fheroes2::Rect() );
        }

        bool QueueEventProcessing()
        {
            return ItemsBar<Item>::QueueEventProcessing();
        }

        bool QueueEventProcessing( ItemsActionBar<Item> & other )
        {
            LocalEvent & le = LocalEvent::Get();
            const fheroes2::Point & cursor = le.GetMouseCursor();

            if ( ItemsBar<Item>::isItemsEmpty() && other.isItemsEmpty() )
                return false;
            else if ( other.GetItem( le.GetMousePressLeft() ) && ActionCrossItemBarDrag( cursor, other ) )
                return true;

            return other.isSelected() ? ActionCursorItemIter( cursor, other ) : ActionCursorItemIter( cursor, ItemsBar<Item>::GetItemIterPos( cursor ) );
        }

    protected:
        ItemsIterator GetTopItemIter() override
        {
            return topItem;
        }

        ItemsIterator GetCurItemIter() override
        {
            return curItemPos.first;
        }

        void SetContentItems() override
        {
            ResetSelected();
        }

        void RedrawItemIter( ItemsIterator it, const fheroes2::Rect & pos, fheroes2::Image & dstsf ) override
        {
            RedrawItem( **it, pos, GetCurItemIter() == it, dstsf );
        }

        bool ActionCrossItemBarDrag( const fheroes2::Point & cursor, ItemsActionBar<Item> & other )
        {
            LocalEvent & le = LocalEvent::Get();
            Item * otherItemPress = other.GetItem( le.GetMousePressLeft() );

            // already did check for this before we go here, maybe not necessary?
            if ( !otherItemPress )
                return false;

            ItemIterPos iterPos1 = ItemsBar<Item>::GetItemIterPos( cursor );

            if ( iterPos1.first == ItemsBar<Item>::GetEndItemIter() )
                return false;

            if ( le.MousePressLeft( iterPos1.second ) ) {
                return ActionBarLeftMouseHold( **iterPos1.first, *otherItemPress );
            }
            else if ( le.MouseReleaseLeft( iterPos1.second ) ) {
                if ( ActionBarLeftMouseRelease( **iterPos1.first, *otherItemPress ) ) {
                    le.ResetPressLeft();
                    other.ResetSelected();
                }

                return true;
            }

            return false;
        }

        bool ActionCursorItemIter( const fheroes2::Point &, ItemIterPos iterPos ) override
        {
            if ( iterPos.first != ItemsBar<Item>::GetEndItemIter() ) {
                LocalEvent & le = LocalEvent::Get();

                if ( ActionBarCursor( **iterPos.first ) ) {
                    return true;
                }
                else if ( le.MouseClickLeft( iterPos.second ) ) {
                    if ( iterPos.first == GetCurItemIter() ) {
                        return ActionBarLeftMouseDoubleClick( **iterPos.first );
                    }
                    else {
                        if ( ActionBarLeftMouseSingleClick( **iterPos.first ) )
                            curItemPos = iterPos;
                        else
                            ResetSelected();

                        return true;
                    }
                }
                else if ( le.MousePressLeft( iterPos.second ) ) {
                    return ActionBarLeftMouseHold( **iterPos.first );
                }
                else if ( le.MouseReleaseLeft( iterPos.second ) ) {
                    return ActionBarLeftMouseRelease( **iterPos.first );
                }
                else if ( le.MouseClickRight( iterPos.second ) ) {
                    return ActionBarRightMouseSingleClick( **iterPos.first );
                }
                else if ( le.MousePressRight( iterPos.second ) ) {
                    return ActionBarRightMouseHold( **iterPos.first );
                }
                else if ( le.MouseReleaseRight( iterPos.second ) ) {
                    return ActionBarRightMouseRelease( **iterPos.first );
                }
            }

            return false;
        }

        bool ActionCursorItemIter( const fheroes2::Point & cursor, ItemsActionBar<Item> & other )
        {
            ItemIterPos iterPos1 = ItemsBar<Item>::GetItemIterPos( cursor );
            ItemIterPos iterPos2 = other.curItemPos;

            if ( iterPos1.first != ItemsBar<Item>::GetEndItemIter() ) {
                LocalEvent & le = LocalEvent::Get();

                if ( ActionBarCursor( **iterPos1.first, **iterPos2.first ) ) {
                    return true;
                }
                else if ( le.MouseClickLeft( iterPos1.second ) ) {
                    if ( ActionBarLeftMouseSingleClick( **iterPos1.first, **iterPos2.first ) )
                        curItemPos = iterPos1;
                    else
                        ResetSelected();

                    other.ResetSelected();
                    return true;
                }
                else if ( le.MousePressLeft( iterPos1.second ) ) {
                    return ActionBarLeftMouseHold( **iterPos1.first, **iterPos2.first );
                }
                // let ActionCrossItemBarDrag handle MousePressRelease instead
                else if ( le.MouseClickRight( iterPos1.second ) ) {
                    ActionBarRightMouseSingleClick( **iterPos1.first, **iterPos2.first );
                    other.ResetSelected();

                    // has to return true to display selection reset
                    return true;
                }
                else if ( le.MousePressRight( iterPos1.second ) ) {
                    return ActionBarRightMouseHold( **iterPos1.first, **iterPos2.first );
                }
                else if ( le.MouseReleaseRight( iterPos1.second ) ) {
                    return ActionBarRightMouseRelease( **iterPos1.first, **iterPos2.first );
                }
            }

            return false;
        }
    };
}

#endif
