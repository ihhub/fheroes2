/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023                                                    *
 *                                                                         *
 *   Copyright (c) 2010 Stephen Connolly.                                  *
 *   Copyright (c) 2006-2007 loopy project (http://loopy.sourceforge.net)  *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Lesser General Public            *
 *   License as published by the Free Software Foundation; either          *
 *   version 2.1 of the License, or (at your option) any later version.    *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

package com.github.stephenc.javaisotools.loopfs.spi;

import com.github.stephenc.javaisotools.loopfs.api.FileEntry;
import com.github.stephenc.javaisotools.loopfs.api.FileSystem;
import java.io.IOException;

public abstract class AbstractFileSystem<T extends FileEntry> implements FileSystem<T>
{
    private SeekableInput channel;

    protected AbstractFileSystem( final SeekableInput seekable, final boolean readOnly )
    {
        if ( !readOnly ) {
            throw new IllegalArgumentException( "Currently, only read-only is supported" );
        }
        this.channel = seekable;
    }

    // TODO: close open streams automatically

    public synchronized void close() throws IOException
    {
        if ( isClosed() ) {
            return;
        }
        try {
            this.channel.close();
        }
        finally {
            this.channel = null;
        }
    }

    public synchronized boolean isClosed()
    {
        return ( null == this.channel );
    }

    protected final void ensureOpen() throws IllegalStateException
    {
        if ( isClosed() ) {
            throw new IllegalStateException( "File has been closed" );
        }
    }

    protected final void seek( long pos ) throws IOException
    {
        ensureOpen();
        this.channel.seek( pos );
    }

    protected final int read( byte[] buffer, int offset, int length ) throws IOException
    {
        ensureOpen();
        return readFully( buffer, offset, length );
    }

    private int readFully( byte[] buffer, int offset, int length ) throws IOException
    {
        int bytesRead;
        int remaining = length;

        // read doesn't guarantee a full buffer is read. Reading until we have a full buffer or end of stream
        while ( remaining != 0 && ( bytesRead = this.channel.read( buffer, offset, remaining ) ) != -1 ) {
            offset += bytesRead;
            remaining -= bytesRead;
        }
        return length - remaining;
    }
}
