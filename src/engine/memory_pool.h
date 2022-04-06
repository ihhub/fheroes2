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

#pragma once

#include <map>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <vector>

namespace fheroes2
{
    class MemoryAllocator
    {
    public:
        static MemoryAllocator & instance();

        ~MemoryAllocator()
        {
            _free();
        }

        // Allocates a chunk of memory. It is recommended to call this method only one time at the startup of an application.
        // Do not reallocate memory if some objects in your source code are allocated through this allocator.
        void reserve( size_t size );

        // Returns a pointer to an allocated memory. If memory size of the allocated memory chuck is enough for requested size
        // then returns a pointer from preallocated memory, otherwise allocates heap memory.
        template <typename _DataType = uint8_t>
        _DataType* allocate( size_t size = 1 )
        {
            if ( _data != nullptr ) {
                const size_t overallSize = size * sizeof( _DataType );

                if ( overallSize < _size ) {
                    const uint8_t level = _getAllocationLevel( overallSize );

                    if ( _split( level ) ) {
                        std::set<size_t>::iterator chunk = _freeChunk[level].begin();
                        _DataType* address = reinterpret_cast<_DataType*>( _alignedData + *chunk );
                        _allocatedChunk.emplace( *chunk, level );
                        _freeChunk[level].erase( chunk );
                        return address;
                    }
                }
            }

            // If no space in the preallocated memory, allocate as a usual heap memory.
            return new _DataType[size];
        }

        // Deallocates a memory by input address. If a pointer points on allocated chuck of memory inside the allocator then
        // the allocator just removes a reference to such area without any cost, otherwise heap allocation.
        template <typename _DataType>
        void free( _DataType * address )
        {
            if ( address == nullptr ) {
                return;
            }

            if ( _data != nullptr && reinterpret_cast<uint8_t*>( address ) >= _alignedData ) {
                std::map<size_t, uint8_t>::iterator pos = _allocatedChunk.find( static_cast<size_t>( reinterpret_cast<uint8_t*>( address ) - _alignedData ) );

                if ( pos != _allocatedChunk.end() ) {
                    _freeChunk[pos->second].insert( pos->first );
                    _merge( pos->first, pos->second );
                    _allocatedChunk.erase( pos );
                    return;
                }
            }

            delete[] address;
        }
    private:
        std::vector<std::set<size_t>> _freeChunk; // free memory in preallocated memory

        size_t _size = 0; // Size of memory allocated chunk.
        uint8_t * _data = nullptr; // Pointer to memory allocated chunk.
        uint8_t * _alignedData = nullptr; // Aligned pointer for possible SIMD access.

        // A map which holds an information about allocated memory in preallocated memory chunk
        // first parameter is an offset from preallocated memory, second parameter is a power of 2 (level)
        std::map<size_t, uint8_t> _allocatedChunk;

        MemoryAllocator() = default;
        MemoryAllocator( const MemoryAllocator & ) = delete;
        MemoryAllocator & operator=( const MemoryAllocator & ) = delete;

        // Free resources.
        void _free();

        // Returns a level (power of 2) needed for a required size
        static uint8_t _getAllocationLevel( const size_t initialSize );

        // Splits the preallocated memory by levels
        bool _split( const uint8_t from );

        // Merges preallocated memory by levels.
        void _merge( size_t offset, uint8_t from );
    };
}
