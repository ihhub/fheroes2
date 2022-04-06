/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2022                                                    *
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

#include "memory_pool.h"

namespace fheroes2
{
    MemoryAllocator & MemoryAllocator::instance()
    {
        static MemoryAllocator allocator;
        return allocator;
    }

    void MemoryAllocator::reserve( size_t size )
    {
        if ( size == 0 ) {
            throw std::logic_error( "Memory size cannot be 0" );
        }

        if ( size == _size ) {
            return;
        }

        if ( _size != size && size > 0 ) {
            if ( !_allocatedChunk.empty() ) {
                throw std::logic_error( "Cannot free a memory. Not all objects were previously deallocated from the allocator." );
            }

            _free();

            const size_t alignment = 32u; // AVX alignment requirement
            _data = new uint8_t[size + alignment];
            const std::uintptr_t dataAddress = reinterpret_cast<std::uintptr_t>( _data );
            _alignedData = ( ( dataAddress % alignment ) == 0 ) ? _data : _data + ( alignment - ( dataAddress % alignment ) );

            _size = size;
        }

        size_t usedSize = 0;

        while ( size > 0 ) {
            uint8_t levelCount = _getAllocationLevel( size );
            size_t value = static_cast<size_t>( 1 ) << levelCount;

            if ( value > size ) {
                value >>= 1;
                --levelCount;
            }

            if ( usedSize == 0 ) {
                _freeChunk.resize( levelCount + 1u );
            }

            _freeChunk[levelCount].insert( usedSize );

            usedSize += value;
            size -= value;
        }
    }

    void MemoryAllocator::_free()
    {
        if ( _data != nullptr ) {
            delete[] _data;
            _data = nullptr;
            _alignedData = nullptr;
        }

        _allocatedChunk.clear();

        _freeChunk.clear();
        _size = 0;
    }

    uint8_t MemoryAllocator::_getAllocationLevel( const size_t initialSize )
    {
        size_t size = 1;
        uint8_t level = 0;

        while ( size < initialSize ) {
            size <<= 1;
            ++level;
        }

        return level;
    }

    bool MemoryAllocator::_split( const uint8_t from )
    {
        bool levelFound = false;
        uint8_t startLevel = from;

        for ( uint8_t i = from; i < _freeChunk.size(); ++i ) {
            if ( !_freeChunk[i].empty() ) {
                startLevel = i;
                levelFound = true;
                break;
            }
        }

        if ( !levelFound ) {
            return false;
        }

        if ( startLevel > from ) {
            size_t memorySize = static_cast<size_t>( 1 ) << ( startLevel - 1 );

            for ( ; startLevel > from; --startLevel, memorySize >>= 1 ) {
                const size_t previousLevelValue = *_freeChunk[startLevel].begin();
                _freeChunk[startLevel - 1u].insert( previousLevelValue );
                _freeChunk[startLevel - 1u].insert( previousLevelValue + memorySize );
                _freeChunk[startLevel].erase( _freeChunk[startLevel].begin() );
            }
        }

        return true;
    }

    void MemoryAllocator::_merge( size_t offset, uint8_t from )
    {
        size_t memorySize = static_cast<size_t>( 1 ) << from;

        for ( std::vector<std::set<size_t>>::iterator level = _freeChunk.begin() + from; level < _freeChunk.end(); ++level, memorySize <<= 1 ) {
            std::set<size_t>::iterator pos = level->find( offset );
            std::set<size_t>::iterator neighbour = pos;
            ++neighbour;

            if ( neighbour != level->end() ) {
                if ( *neighbour - *pos == memorySize ) {
                    offset = *pos;
                    ( level + 1 )->insert( offset );
                    level->erase( pos, ++neighbour );
                    continue;
                }
            }

            if ( pos != level->begin() ) {
                neighbour = pos;
                --neighbour;

                if ( *pos - *neighbour == memorySize ) {
                    offset = *neighbour;
                    ( level + 1 )->insert( offset );
                    level->erase( neighbour, ++pos );
                    continue;
                }
            }

            return;
        }
    }
}
