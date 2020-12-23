/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifdef WITH_NET

#include <algorithm>
#include <cstring>

#include "sdlnet.h"

Network::Socket::Socket()
    : sd( NULL )
    , sdset( NULL )
    , status( 0 )
{}

Network::Socket::Socket( const TCPsocket csd )
    : sd( NULL )
    , sdset( NULL )
    , status( 0 )
{
    Assign( csd );
}

Network::Socket::Socket( const Socket & )
    : sd( NULL )
    , sdset( NULL )
    , status( 0 )
{}

Network::Socket & Network::Socket::operator=( const Socket & )
{
    return *this;
}

Network::Socket::~Socket()
{
    if ( sd )
        Close();
}

void Network::Socket::Assign( const TCPsocket csd )
{
    if ( sd )
        Close();

    if ( csd ) {
        sd = csd;
        sdset = SDLNet_AllocSocketSet( 1 );
        if ( sdset )
            SDLNet_TCP_AddSocket( sdset, sd );
    }
}

u32 Network::Socket::Host( void ) const
{
    IPaddress * remoteIP = sd ? SDLNet_TCP_GetPeerAddress( sd ) : NULL;
    if ( remoteIP )
        return SDLNet_Read32( &remoteIP->host );
    ERROR( SDLNet_GetError() );
    return 0;
}

u16 Network::Socket::Port( void ) const
{
    IPaddress * remoteIP = sd ? SDLNet_TCP_GetPeerAddress( sd ) : NULL;
    if ( remoteIP )
        return SDLNet_Read16( &remoteIP->port );
    ERROR( SDLNet_GetError() );
    return 0;
}

bool Network::Socket::Ready( void ) const
{
    return 0 < SDLNet_CheckSockets( sdset, 1 ) && 0 < SDLNet_SocketReady( sd );
}

bool Network::Socket::Recv( char * buf, int len )
{
    if ( sd && buf && len ) {
        int rcv = 0;

        while ( ( rcv = SDLNet_TCP_Recv( sd, buf, len ) ) > 0 && rcv < len ) {
            buf += rcv;
            len -= rcv;
        }

        if ( rcv != len )
            status |= ERROR_RECV;
    }

    return !( status & ERROR_RECV );
}

bool Network::Socket::Send( const char * buf, int len )
{
    if ( sd && len != SDLNet_TCP_Send( sd, (void *)buf, len ) )
        status |= ERROR_SEND;

    return !( status & ERROR_SEND );
}

bool Network::Socket::Recv32( u32 & v )
{
    if ( Recv( reinterpret_cast<char *>( &v ), sizeof( v ) ) ) {
        SwapBE32( v );
        return true;
    }
    return false;
}

bool Network::Socket::Recv16( u16 & v )
{
    if ( Recv( reinterpret_cast<char *>( &v ), sizeof( v ) ) ) {
        SwapBE16( v );
        return true;
    }
    return false;
}

bool Network::Socket::Send32( const uint32_t v0 )
{
    u32 v = v0;
    SwapBE32( v );

    return Send( reinterpret_cast<char *>( &v ), sizeof( v ) );
}

bool Network::Socket::Send16( const uint16_t v0 )
{
    u16 v = v0;
    SwapBE16( v );

    return Send( reinterpret_cast<char *>( &v ), sizeof( v ) );
}

bool Network::Socket::Open( IPaddress & ip )
{
    Assign( SDLNet_TCP_Open( &ip ) );

    if ( !sd )
        ERROR( SDLNet_GetError() );

    return sd;
}

bool Network::Socket::isValid( void ) const
{
    return sd && 0 == status;
}

void Network::Socket::Close( void )
{
    if ( sd ) {
        if ( sdset ) {
            SDLNet_TCP_DelSocket( sdset, sd );
            SDLNet_FreeSocketSet( sdset );
            sdset = NULL;
        }
        SDLNet_TCP_Close( sd );
        sd = NULL;
    }
}

Network::Server::Server() {}

TCPsocket Network::Server::Accept( void )
{
    return SDLNet_TCP_Accept( sd );
}

bool Network::Init( void )
{
    if ( SDLNet_Init() < 0 ) {
        ERROR( SDLNet_GetError() );
        return false;
    }
    return true;
}

void Network::Quit( void )
{
    SDLNet_Quit();
}

bool Network::ResolveHost( IPaddress & ip, const char * host, u16 port )
{
    if ( SDLNet_ResolveHost( &ip, host, port ) < 0 ) {
        ERROR( SDLNet_GetError() );
        return false;
    }
    return true;
}

const char * Network::GetError( void )
{
    return SDLNet_GetError();
}

#endif
