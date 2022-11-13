/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2012 by Andrey Afletdinov <fheroes2@gmail.com>          *
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
#include <cassert>
#include <utility>

#include "gamedefs.h"
#include "image.h"
#include "localevent.h"

namespace Interface
{
    template <class Item>
    class ItemsBar
    {
    public:
        ItemsBar() = default;

        virtual ~ItemsBar() = default;

        virtual void RedrawBackground( const fheroes2::Rect &, fheroes2::Image & ) = 0;
        virtual void RedrawItem( Item &, const fheroes2::Rect &, fheroes2::Image & ) = 0;

        void setTableSize( const fheroes2::Size & size )
        {
            assert( size.width > 0 && size.height > 0 );

            tableSize = size;

            calculateRoi();
        }

        void SetContent( std::vector<Item> & content )
        {
            items.clear();
            items.reserve( content.size() );

            for ( Item & item : content ) {
                items.push_back( &item );
            }

            SetContentItems();
        }

        void setRenderingOffset( const fheroes2::Point & offset )
        {
            renderingRoi.x = offset.x;
            renderingRoi.y = offset.y;
        }

        void setSingleItemSize( const fheroes2::Size & size )
        {
            assert( size.width > 0 && size.height > 0 );

            singleItemRoi = size;

            calculateRoi();
        }

        void setInBetweenItemsOffset( const fheroes2::Size & offset )
        {
            offsetBetweenItems = offset;

            calculateRoi();
        }

        Item * GetItem( const fheroes2::Point & pt )
        {
            ItemsIterator posItem = GetItemIter( pt );
            return posItem != items.end() ? *posItem : nullptr;
        }

        const fheroes2::Rect & GetArea() const
        {
            return renderingRoi;
        }

        void Redraw( fheroes2::Image & output )
        {
            fheroes2::Point itemOffset{ renderingRoi.x, renderingRoi.y };

            for ( int32_t y = 0; y < tableSize.height; ++y ) {
                for ( int32_t x = 0; x < tableSize.width; ++x ) {
                    RedrawBackground( { itemOffset.x, itemOffset.y, singleItemRoi.width, singleItemRoi.height }, output );

                    itemOffset.x += offsetBetweenItems.width + singleItemRoi.width;
                }

                itemOffset.x = renderingRoi.x;
                itemOffset.y += offsetBetweenItems.height + singleItemRoi.height;
            }

            itemOffset = { renderingRoi.x, renderingRoi.y };

            ItemsIterator itemIter = GetTopItemIter();

            for ( int32_t y = 0; y < tableSize.height; ++y ) {
                for ( int32_t x = 0; x < tableSize.width; ++x ) {
                    if ( itemIter == items.end() ) {
                        return;
                    }

                    RedrawItemIter( itemIter, { itemOffset.x, itemOffset.y, singleItemRoi.width, singleItemRoi.height }, output );
                    ++itemIter;

                    itemOffset.x += offsetBetweenItems.width + singleItemRoi.width;
                }

                itemOffset.x = renderingRoi.x;
                itemOffset.y += offsetBetweenItems.height + singleItemRoi.height;
            }
        }

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

        bool QueueEventProcessing()
        {
            if ( isItemsEmpty() ) {
                return false;
            }

            const fheroes2::Point & cursor = LocalEvent::Get().GetMouseCursor();
            return ActionCursorItemIter( cursor, GetItemIterPos( cursor ) );
        }

    protected:
        // Since we store pointers and the number of elements do not change in a container, vector is the most efficient container to be used.
        using ItemsIterator = typename std::vector<Item *>::iterator;
        using ItemIterPos = std::pair<ItemsIterator, fheroes2::Rect>;

        std::vector<Item *> items;

        virtual void SetContentItems()
        {
            // Do nothing.
        }

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

        virtual void RedrawItemIter( ItemsIterator it, const fheroes2::Rect & pos, fheroes2::Image & output )
        {
            RedrawItem( **it, pos, output );
        }

        virtual bool ActionCursorItemIter( const fheroes2::Point &, ItemIterPos iterPos )
        {
            if ( iterPos.first == GetEndItemIter() ) {
                return false;
            }

            if ( ActionBarCursor( **iterPos.first ) ) {
                return true;
            }

            LocalEvent & le = LocalEvent::Get();
            if ( le.MouseClickLeft( iterPos.second ) ) {
                return ActionBarLeftMouseSingleClick( **iterPos.first );
            }

            if ( le.MousePressRight( iterPos.second ) ) {
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

        ItemIterPos GetItemIterPos( const fheroes2::Point & position )
        {
            fheroes2::Rect dstrt( renderingRoi.x, renderingRoi.y, singleItemRoi.width, singleItemRoi.height );
            ItemsIterator posItem = GetTopItemIter();

            for ( int32_t y = 0; y < tableSize.height; ++y ) {
                for ( int32_t x = 0; x < tableSize.width; ++x ) {
                    if ( posItem != items.end() ) {
                        if ( dstrt & position )
                            return ItemIterPos( posItem, dstrt );
                        ++posItem;
                    }

                    dstrt.x += offsetBetweenItems.width + singleItemRoi.width;
                }

                dstrt.x = renderingRoi.x;
                dstrt.y += offsetBetweenItems.height + singleItemRoi.height;
            }

            return { items.end(), {} };
        }

    private:
        void calculateRoi()
        {
            if ( tableSize.width > 0 ) {
                renderingRoi.width = tableSize.width * singleItemRoi.width + ( tableSize.width - 1 ) * offsetBetweenItems.width;
            }
            else {
                renderingRoi.width = 0;
            }

            if ( tableSize.height > 0 ) {
                renderingRoi.height = tableSize.height * singleItemRoi.height + ( tableSize.height - 1 ) * offsetBetweenItems.height;
            }
            else {
                renderingRoi.height = 0;
            }
        }

        fheroes2::Rect renderingRoi;
        fheroes2::Size singleItemRoi;
        fheroes2::Size tableSize;
        fheroes2::Size offsetBetweenItems;
    };

    template <class Item>
    class ItemsActionBar : public ItemsBar<Item>
    {
    public:
        ItemsActionBar()
        {
            ResetSelected();
        }

        ~ItemsActionBar() override = default;

        void RedrawItem( Item &, const fheroes2::Rect &, fheroes2::Image & ) override
        {
            // Do nothing.
        }

        virtual void RedrawItem( Item &, const fheroes2::Rect &, bool, fheroes2::Image & )
        {
            // Do nothing.
        }

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

        virtual bool ActionBarLeftMouseHold( Item &, const fheroes2::Rect & )
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

        Item * GetSelectedItem()
        {
            return *GetCurItemIter();
        }

        fheroes2::Rect * GetSelectedPos()
        {
            return &curItemPos.second;
        }

        int32_t GetSelectedIndex()
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
            const LocalEvent & le = LocalEvent::Get();
            const fheroes2::Point & cursor = le.GetMouseCursor();

            if ( ItemsBar<Item>::isItemsEmpty() && other.isItemsEmpty() ) {
                return false;
            }

            if ( other.GetItem( le.GetMousePressLeft() ) && ActionCrossItemBarDrag( cursor, other ) ) {
                return true;
            }

            return other.isSelected() ? ActionCursorItemIter( cursor, other ) : ActionCursorItemIter( cursor, ItemsBar<Item>::GetItemIterPos( cursor ) );
        }

    protected:
        using ItemsIterator = typename ItemsBar<Item>::ItemsIterator;
        using ItemIterPos = typename ItemsBar<Item>::ItemIterPos;

        ItemsIterator topItem;
        ItemIterPos curItemPos;

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

        void RedrawItemIter( ItemsIterator it, const fheroes2::Rect & pos, fheroes2::Image & output ) override
        {
            RedrawItem( **it, pos, GetCurItemIter() == it, output );
        }

        bool ActionCrossItemBarDrag( const fheroes2::Point & cursor, ItemsActionBar<Item> & other )
        {
            LocalEvent & le = LocalEvent::Get();
            Item * otherItemPress = other.GetItem( le.GetMousePressLeft() );

            // already did check for this before we go here, maybe not necessary?
            if ( !otherItemPress ) {
                return false;
            }

            ItemIterPos iterPos1 = ItemsBar<Item>::GetItemIterPos( cursor );
            if ( iterPos1.first == ItemsBar<Item>::GetEndItemIter() )
                return false;

            if ( le.MousePressLeft( iterPos1.second ) ) {
                return ActionBarLeftMouseHold( **iterPos1.first, *otherItemPress );
            }

            if ( le.MouseReleaseLeft( iterPos1.second ) ) {
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
            if ( iterPos.first == ItemsBar<Item>::GetEndItemIter() ) {
                return false;
            }

            if ( ActionBarCursor( **iterPos.first ) ) {
                return true;
            }

            LocalEvent & le = LocalEvent::Get();
            if ( le.MouseClickLeft( iterPos.second ) ) {
                if ( iterPos.first == GetCurItemIter() ) {
                    return ActionBarLeftMouseDoubleClick( **iterPos.first );
                }

                if ( ActionBarLeftMouseSingleClick( **iterPos.first ) ) {
                    curItemPos = iterPos;
                }
                else {
                    ResetSelected();
                }

                return true;
            }

            if ( le.MousePressLeft( iterPos.second ) ) {
                return ActionBarLeftMouseHold( **iterPos.first, iterPos.second );
            }

            if ( le.MouseReleaseLeft( iterPos.second ) ) {
                return ActionBarLeftMouseRelease( **iterPos.first );
            }

            if ( le.MouseClickRight( iterPos.second ) ) {
                return ActionBarRightMouseSingleClick( **iterPos.first );
            }

            if ( le.MousePressRight( iterPos.second ) ) {
                return ActionBarRightMouseHold( **iterPos.first );
            }

            return false;
        }

        bool ActionCursorItemIter( const fheroes2::Point & cursor, ItemsActionBar<Item> & other )
        {
            ItemIterPos iterPos1 = ItemsBar<Item>::GetItemIterPos( cursor );
            if ( iterPos1.first == ItemsBar<Item>::GetEndItemIter() ) {
                return false;
            }

            ItemIterPos iterPos2 = other.curItemPos;
            if ( ActionBarCursor( **iterPos1.first, **iterPos2.first ) ) {
                return true;
            }

            LocalEvent & le = LocalEvent::Get();
            if ( le.MouseClickLeft( iterPos1.second ) ) {
                if ( ActionBarLeftMouseSingleClick( **iterPos1.first, **iterPos2.first ) )
                    curItemPos = iterPos1;
                else
                    ResetSelected();

                other.ResetSelected();
                return true;
            }

            if ( le.MousePressLeft( iterPos1.second ) ) {
                return ActionBarLeftMouseHold( **iterPos1.first, **iterPos2.first );
            }

            // let ActionCrossItemBarDrag handle MousePressRelease instead
            if ( le.MouseClickRight( iterPos1.second ) ) {
                ActionBarRightMouseSingleClick( **iterPos1.first, **iterPos2.first );
                other.ResetSelected();

                // has to return true to display selection reset
                return true;
            }

            if ( le.MousePressRight( iterPos1.second ) ) {
                return ActionBarRightMouseHold( **iterPos1.first, **iterPos2.first );
            }

            return false;
        }
    };
}

#endif
