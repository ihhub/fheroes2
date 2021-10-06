/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2021                                                    *
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

#include "map_object_info.h"
#include "icn.h"

#include <array>
#include <cassert>
#include <map>

namespace
{
    struct ObjectInfoCollection
    {
        void addNewObject()
        {
            assert( _objects.empty() || !_objects.back().parts.empty() );

            _objects.emplace_back();
        }

        void addNewObjectPart( const fheroes2::Point & position, const uint8_t level, const MP2::MapObjectType type, const uint32_t animationFrameCount = 0 )
        {
            assert( !_objects.empty() );

            _objects.back().parts.emplace_back( _counter, animationFrameCount, position, type, level );

            _types[_counter] = type;

            ++_counter;
        }

        std::vector<fheroes2::MapObjectInfo> _objects;

        std::array<MP2::MapObjectType, 256> _types = { MP2::OBJ_ZERO };

        uint32_t _counter = 0;
    };

    const std::vector<fheroes2::MapObjectInfo> emptyObjectArray;

    ObjectInfoCollection objncrck;

    void generateObjncrck()
    {
        objncrck.addNewObject();

        objncrck.addNewObjectPart( { 0, -1 }, 2, MP2::OBJN_ARTESIANSPRING );
        objncrck.addNewObjectPart( { 1, -1 }, 2, MP2::OBJN_ARTESIANSPRING );
        objncrck.addNewObjectPart( { -1, 0 }, 2, MP2::OBJN_ARTESIANSPRING );
        objncrck.addNewObjectPart( { 0, 0 }, 1, MP2::OBJ_ARTESIANSPRING );
        objncrck.addNewObjectPart( { 1, 0 }, 1, MP2::OBJ_ARTESIANSPRING );

        objncrck.addNewObject();

        objncrck.addNewObjectPart( { -1, 0 }, 3, MP2::OBJ_ZERO );
        objncrck.addNewObjectPart( { 0, 0 }, 3, MP2::OBJ_ZERO );

        objncrck.addNewObject();

        objncrck.addNewObjectPart( { 0, -1 }, 2, MP2::OBJ_STONES );
        objncrck.addNewObjectPart( { 1, -1 }, 2, MP2::OBJ_STONES );
        objncrck.addNewObjectPart( { -1, 0 }, 2, MP2::OBJ_STONES );
        objncrck.addNewObjectPart( { 0, 0 }, 1, MP2::OBJ_STONES );
        objncrck.addNewObjectPart( { 1, 0 }, 1, MP2::OBJ_STONES );

        objncrck.addNewObject();

        objncrck.addNewObjectPart( { 0, -1 }, 2, MP2::OBJ_CACTUS );
        objncrck.addNewObjectPart( { -1, 0 }, 2, MP2::OBJ_CACTUS );
        objncrck.addNewObjectPart( { 0, 0 }, 1, MP2::OBJ_CACTUS );

        objncrck.addNewObject();

        objncrck.addNewObjectPart( { -1, 0 }, 2, MP2::OBJ_FLOWERS );
        objncrck.addNewObjectPart( { 0, 0 }, 1, MP2::OBJ_FLOWERS );

        objncrck.addNewObject();

        objncrck.addNewObjectPart( { 0, 0 }, 1, MP2::OBJ_NOTHINGSPECIAL );

        objncrck.addNewObject();

        objncrck.addNewObjectPart( { 0, 0 }, 1, MP2::OBJ_ZERO );

        objncrck.addNewObject();

        objncrck.addNewObjectPart( { 0, -1 }, 2, MP2::OBJ_STONES );
        objncrck.addNewObjectPart( { -1, 0 }, 2, MP2::OBJ_STONES );
        objncrck.addNewObjectPart( { 0, 0 }, 1, MP2::OBJ_STONES );
        objncrck.addNewObjectPart( { 1, 0 }, 1, MP2::OBJ_STONES );
    }

    void generateAllObjects()
    {
        static bool objectsInitialized = false;
        if ( objectsInitialized ) {
            return;
        }

        generateObjncrck();
    }
}

namespace fheroes2
{
    MP2::MapObjectType getObjectType( const int icnId, const uint32_t index )
    {
        assert( index < 256 );

        generateAllObjects();

        switch ( icnId ) {
        case ICN::OBJNCRCK:
            return objncrck._types[index];
        default:
            break;
        }

        // This is unknown type of map objects. If you added the one please add the corresponding logic here.
        assert( 0 );

        return MP2::OBJ_ZERO;
    }

    const std::vector<MapObjectInfo> & getObjects( const int icnId )
    {
        generateAllObjects();

        switch ( icnId ) {
        case ICN::OBJNCRCK:
            return objncrck._objects;
        default:
            break;
        }

        // This is unknown type of map objects. If you added the one please add the corresponding logic here.
        assert( 0 );

        return emptyObjectArray;
    }
}
