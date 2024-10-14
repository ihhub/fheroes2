/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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
            calculateItemsPos();
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

            calculateItemsPos();
        }

        void setSingleItemSize( const fheroes2::Size & size )
        {
            assert( size.width > 0 && size.height > 0 );

            singleItemSize = size;

            calculateRoi();
            calculateItemsPos();
        }

        void setInBetweenItemsOffset( const fheroes2::Size & offset )
        {
            offsetBetweenItems = offset;

            calculateRoi();
            calculateItemsPos();
        }

        Item * GetItem( const fheroes2::Point & pt )
        {
            const ItemsIterator posItem = GetItemIter( pt );
            return posItem != items.end() ? *posItem : nullptr;
        }

        const fheroes2::Rect & GetArea() const
        {
            return renderingRoi;
        }

        void Redraw( fheroes2::Image & output )
        {
            if ( tableSize.width <= 0 || tableSize.height <= 0 ) {
                return;
            }

            ItemsIterator itemIter = items.begin();
            auto itemPosIter = itemPos.begin();

            const bool useCustomCountX = ( _customItemsCountInRow.size() == static_cast<size_t>( tableSize.height ) );

            for ( int32_t y = 0; y < tableSize.height; ++y ) {
                const int32_t coutX = useCustomCountX ? _customItemsCountInRow[y] : tableSize.width;
                for ( int32_t x = 0; x < coutX; ++x ) {
                    assert( itemPosIter != itemPos.end() );

                    const fheroes2::Rect itemRoi{ *itemPosIter, singleItemSize };
                    RedrawBackground( itemRoi, output );

                    ++itemPosIter;

                    if ( itemIter == items.end() ) {
                        continue;
                    }

                    RedrawItemIter( itemIter, itemRoi, output );
                    ++itemIter;
                }
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

            const fheroes2::Point & cursor = LocalEvent::Get().getMouseCursorPos();
            return ActionCursorItemIter( cursor, GetItemIterPos( cursor ) );
        }

        void setCustomItemsCountInRow( std::vector<int32_t> itemsXCounts )
        {
            _customItemsCountInRow = std::move( itemsXCounts );

            if ( !_customItemsCountInRow.empty() ) {
                const int32_t maxWidth = *std::max_element( _customItemsCountInRow.begin(), _customItemsCountInRow.end() );
                if ( tableSize.width != maxWidth ) {
                    tableSize.width = maxWidth;
                    calculateRoi();
                }
            }

            calculateItemsPos();
        }

    protected:
        // Since we store pointers and the number of elements do not change in a container, vector is the most efficient container to be used.
        using ItemsIterator = typename std::vector<Item *>::iterator;
        using ItemIterPos = std::pair<ItemsIterator, fheroes2::Rect>;

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

            if ( le.isMouseRightButtonPressedInArea( iterPos.second ) ) {
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
            if ( tableSize.width <= 0 || tableSize.height <= 0 ) {
                return { items.end(), {} };
            }

            ItemsIterator posItem = items.begin();
            auto itemPosIter = itemPos.begin();

            const bool useCustomCountX = ( _customItemsCountInRow.size() == static_cast<size_t>( tableSize.height ) );

            for ( int32_t y = 0; y < tableSize.height; ++y ) {
                const int32_t coutX = useCustomCountX ? _customItemsCountInRow[y] : tableSize.width;
                for ( int32_t x = 0; x < coutX; ++x ) {
                    if ( posItem != items.end() ) {
                        assert( itemPosIter != itemPos.end() );

                        const fheroes2::Rect itemRoi( *itemPosIter, singleItemSize );
                        if ( itemRoi & position ) {
                            return ItemIterPos( posItem, itemRoi );
                        }
                        ++posItem;
                        ++itemPosIter;
                    }
                }
            }

            return { items.end(), {} };
        }

        std::vector<Item *> items;
        std::vector<int32_t> _customItemsCountInRow;

    private:
        void calculateItemsPos()
        {
            itemPos.clear();

            if ( tableSize.width <= 0 || tableSize.height <= 0 ) {
                return;
            }

            const bool useCustomCountX = ( _customItemsCountInRow.size() == static_cast<size_t>( tableSize.height ) );
            const int32_t stepX = ( offsetBetweenItems.width + singleItemSize.width );
            const int32_t stepY = ( offsetBetweenItems.height + singleItemSize.height );

            for ( int32_t y = 0; y < tableSize.height; ++y ) {
                const int32_t coutX = useCustomCountX ? _customItemsCountInRow[y] : tableSize.width;
                const int32_t offsetX = useCustomCountX ? ( tableSize.width - _customItemsCountInRow[y] ) * stepX / 2 : 0;

                for ( int32_t x = 0; x < coutX; ++x ) {
                    itemPos.emplace_back( renderingRoi.x + x * stepX + offsetX, renderingRoi.y + y * stepY );
                }
            }
        }

        void calculateRoi()
        {
            if ( tableSize.width > 0 ) {
                renderingRoi.width = tableSize.width * singleItemSize.width + ( tableSize.width - 1 ) * offsetBetweenItems.width;
            }
            else {
                renderingRoi.width = 0;
            }

            if ( tableSize.height > 0 ) {
                renderingRoi.height = tableSize.height * singleItemSize.height + ( tableSize.height - 1 ) * offsetBetweenItems.height;
            }
            else {
                renderingRoi.height = 0;
            }
        }

        fheroes2::Rect renderingRoi;
        fheroes2::Size singleItemSize;
        std::vector<fheroes2::Point> itemPos;
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
            const fheroes2::Point & cursor = le.getMouseCursorPos();

            if ( ItemsBar<Item>::isItemsEmpty() && other.isItemsEmpty() ) {
                return false;
            }

            if ( other.GetItem( le.getMouseLeftButtonPressedPos() ) && ActionCrossItemBarDrag( cursor, other ) ) {
                return true;
            }

            return other.isSelected() ? ActionCursorItemIter( cursor, other ) : ActionCursorItemIter( cursor, ItemsBar<Item>::GetItemIterPos( cursor ) );
        }

    protected:
        using ItemsIterator = typename ItemsBar<Item>::ItemsIterator;
        using ItemIterPos = typename ItemsBar<Item>::ItemIterPos;

        ItemsIterator GetCurItemIter()
        {
            return curItemPos.first;
        }

        void SetContentItems() final
        {
            ResetSelected();
        }

        void RedrawItemIter( ItemsIterator it, const fheroes2::Rect & pos, fheroes2::Image & output ) override
        {
            RedrawItem( **it, pos, GetCurItemIter() == it, output );
        }

        bool ActionCrossItemBarDrag( const fheroes2::Point & cursor, ItemsActionBar<Item> & other )
        {
            const LocalEvent & le = LocalEvent::Get();

            Item * otherItemPress = other.GetItem( le.getMouseLeftButtonPressedPos() );
            assert( otherItemPress != nullptr );

            const ItemIterPos iterPos1 = ItemsBar<Item>::GetItemIterPos( cursor );
            if ( iterPos1.first == ItemsBar<Item>::GetEndItemIter() )
                return false;

            if ( le.isMouseLeftButtonPressedInArea( iterPos1.second ) ) {
                return ActionBarLeftMouseHold( **iterPos1.first, *otherItemPress );
            }

            if ( le.isMouseLeftButtonReleasedInArea( iterPos1.second ) ) {
                if ( ActionBarLeftMouseRelease( **iterPos1.first, *otherItemPress ) ) {
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

            if ( le.isMouseLeftButtonPressedInArea( iterPos.second ) ) {
                return ActionBarLeftMouseHold( **iterPos.first, iterPos.second );
            }

            if ( le.isMouseLeftButtonReleasedInArea( iterPos.second ) ) {
                return ActionBarLeftMouseRelease( **iterPos.first );
            }

            if ( le.MouseClickRight( iterPos.second ) ) {
                return ActionBarRightMouseSingleClick( **iterPos.first );
            }

            if ( le.isMouseRightButtonPressedInArea( iterPos.second ) ) {
                return ActionBarRightMouseHold( **iterPos.first );
            }

            return false;
        }

        bool ActionCursorItemIter( const fheroes2::Point & cursor, ItemsActionBar<Item> & other )
        {
            const ItemIterPos iterPos1 = ItemsBar<Item>::GetItemIterPos( cursor );
            if ( iterPos1.first == ItemsBar<Item>::GetEndItemIter() ) {
                return false;
            }

            const ItemIterPos iterPos2 = other.curItemPos;
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

            if ( le.isMouseLeftButtonPressedInArea( iterPos1.second ) ) {
                return ActionBarLeftMouseHold( **iterPos1.first, **iterPos2.first );
            }

            // Let the ActionCrossItemBarDrag() handle the case of isMouseLeftButtonReleased()

            if ( le.MouseClickRight( iterPos1.second ) ) {
                ActionBarRightMouseSingleClick( **iterPos1.first, **iterPos2.first );
                other.ResetSelected();

                // has to return true to display selection reset
                return true;
            }

            if ( le.isMouseRightButtonPressedInArea( iterPos1.second ) ) {
                return ActionBarRightMouseHold( **iterPos1.first, **iterPos2.first );
            }

            return false;
        }

        ItemsIterator topItem;
        ItemIterPos curItemPos;
    };
}

#endif
