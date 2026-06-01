/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023 - 2026                                             *
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

#include "history_manager.h"

#include <cassert>
#include <cstdint>
#include <map>

#include "map_format_helper.h"
#include "map_format_info.h"
#include "serialize.h"
#include "world_object_uid.h"

namespace
{
    class BaseMapAction : public fheroes2::Action
    {
    public:
        BaseMapAction() = default;

        ~BaseMapAction() override = default;

        // Disable the copy and move (implicitly) constructors and assignment operators.
        BaseMapAction( const BaseMapAction & ) = delete;
        BaseMapAction & operator=( const BaseMapAction & ) = delete;

        virtual bool prepare() = 0;
    };

    // This class holds 2 copies of MapFormat objects in a compressed format:
    // - one copy before the action
    // - one copy after the action
    class GenericMapAction final : public BaseMapAction
    {
    public:
        explicit GenericMapAction( Maps::Map_Format::MapFormat & mapFormat )
            : _mapFormat( mapFormat )
            , _latestObjectUIDBefore( Maps::getLastObjectUID() )
        {
            if ( !Maps::Map_Format::saveMap( _beforeMapFormat, _mapFormat ) ) {
                assert( 0 );
            }
        }

        bool prepare() override
        {
            if ( !Maps::Map_Format::saveMap( _afterMapFormat, _mapFormat ) ) {
                assert( 0 );
                return false;
            }

            _latestObjectUIDAfter = Maps::getLastObjectUID();

            return true;
        }

        bool redo() override
        {
            if ( !Maps::Map_Format::loadMap( _afterMapFormat, _mapFormat ) ) {
                assert( 0 );
                return false;
            }

            _afterMapFormat.seek( 0 );

            if ( !Maps::readMapInEditor( _mapFormat ) ) {
                // If this assertion blows up then something is really wrong with the Editor.
                assert( 0 );
                return false;
            }

            Maps::setLastObjectUID( _latestObjectUIDAfter );

            return true;
        }

        bool undo() override
        {
            if ( !Maps::Map_Format::loadMap( _beforeMapFormat, _mapFormat ) ) {
                assert( 0 );
                return false;
            }

            _beforeMapFormat.seek( 0 );

            if ( !Maps::readMapInEditor( _mapFormat ) ) {
                // If this assertion blows up then something is really wrong with the Editor.
                assert( 0 );
                return false;
            }

            Maps::setLastObjectUID( _latestObjectUIDBefore );

            return true;
        }

    private:
        Maps::Map_Format::MapFormat & _mapFormat;

        RWStreamBuf _beforeMapFormat;
        RWStreamBuf _afterMapFormat;

        const uint32_t _latestObjectUIDBefore{ 0 };
        uint32_t _latestObjectUIDAfter{ 0 };
    };

    template <typename T>
    class MetadataMapAction : public BaseMapAction
    {
    public:
        explicit MetadataMapAction( std::map<uint32_t, T> & metadata )
            : _metadata( metadata )
            , _beforeMetadata( metadata )
        {
            // Do nothing.
        }

        bool prepare() override
        {
            _afterMetadata = _metadata;
            return true;
        }

        bool redo() override
        {
            _metadata = _afterMetadata;
            return true;
        }

        bool undo() override
        {
            _metadata = _beforeMetadata;
            return true;
        }

    private:
        std::map<uint32_t, T> & _metadata;

        const std::map<uint32_t, T> _beforeMetadata;
        std::map<uint32_t, T> _afterMetadata;
    };
}

namespace fheroes2
{
    ActionCreator::ActionCreator( HistoryManager & manager, Maps::Map_Format::MapFormat & mapFormat, const ActionType type )
        : _manager( manager )
    {
        switch ( type ) {
        case ActionType::GENERIC:
            _action = std::make_unique<GenericMapAction>( mapFormat );
            break;
        case ActionType::HERO_METADATA:
            _action = std::make_unique<MetadataMapAction<Maps::Map_Format::HeroMetadata>>( mapFormat.heroMetadata );
            break;
        case ActionType::CASTLE_METADATA:
            _action = std::make_unique<MetadataMapAction<Maps::Map_Format::CastleMetadata>>( mapFormat.castleMetadata );
            break;
        case ActionType::SPHINX_METADATA:
            _action = std::make_unique<MetadataMapAction<Maps::Map_Format::SphinxMetadata>>( mapFormat.sphinxMetadata );
            break;
        case ActionType::SIGN_METADATA:
            _action = std::make_unique<MetadataMapAction<Maps::Map_Format::SignMetadata>>( mapFormat.signMetadata );
            break;
        case ActionType::ADVENTURE_MAP_EVENT_METADATA:
            _action = std::make_unique<MetadataMapAction<Maps::Map_Format::AdventureMapEventMetadata>>( mapFormat.adventureMapEventMetadata );
            break;
        case ActionType::SELECTION_METADATA:
            _action = std::make_unique<MetadataMapAction<Maps::Map_Format::SelectionObjectMetadata>>( mapFormat.selectionObjectMetadata );
            break;
        case ActionType::MONSTER_METADATA:
            _action = std::make_unique<MetadataMapAction<Maps::Map_Format::MonsterMetadata>>( mapFormat.monsterMetadata );
            break;
        case ActionType::ARTIFACT_METADATA:
            _action = std::make_unique<MetadataMapAction<Maps::Map_Format::ArtifactMetadata>>( mapFormat.artifactMetadata );
            break;
        case ActionType::RESOURCE_METADATA:
            _action = std::make_unique<MetadataMapAction<Maps::Map_Format::ResourceMetadata>>( mapFormat.resourceMetadata );
            break;
        default:
            // Did you add a new action type? Add the missing logic!
            assert( 0 );
            break;
        }
    }

    void ActionCreator::commit()
    {
        auto * action = dynamic_cast<BaseMapAction *>( _action.get() );
        if ( action == nullptr ) {
            // How is it even possible? Did you call this method twice?
            assert( 0 );
            return;
        }

        if ( action->prepare() ) {
            _manager.add( std::move( _action ) );
        }
    }
}
