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

package com.github.stephenc.javaisotools.loopfs.iso9660;

import java.io.IOException;
import java.io.InputStream;

class EntryInputStream extends InputStream
{
    // entry within the file system
    private Iso9660FileEntry entry;

    // the parent file system
    private Iso9660FileSystem fileSystem;

    // current position within entry data
    private long pos;

    // number of remaining bytes within entry
    private long rem;

    EntryInputStream( final Iso9660FileEntry entry, final Iso9660FileSystem fileSystem )
    {
        this.fileSystem = fileSystem;
        this.entry = entry;
        this.pos = 0;
        this.rem = entry.getSize();
    }

    @Override
    public int read( final byte[] b, final int off, final int len ) throws IOException
    {
        ensureOpen();

        if ( this.rem <= 0 ) {
            return -1;
        }
        if ( len <= 0 ) {
            return 0;
        }

        int toRead = len;

        if ( toRead > this.rem ) {
            // down cast is safe as toRead is int and greater than this.rem
            toRead = (int)this.rem;
        }

        int read;

        synchronized ( this.fileSystem ) {
            if ( this.fileSystem.isClosed() ) {
                throw new IOException( "ISO file closed." );
            }

            read = this.fileSystem.readBytes( this.entry, this.pos, b, off, toRead );
        }

        if ( read > 0 ) {
            this.pos += read;
            this.rem -= read;
        }

        return read;
    }

    public int read() throws IOException
    {
        ensureOpen();

        final byte[] b = new byte[1];

        if ( read( b, 0, 1 ) == 1 ) {
            return b[0] & 0xff;
        }
        else {
            return -1;
        }
    }

    @Override
    public long skip( final long n )
    {
        ensureOpen();

        final long len = Math.min( n, this.rem );

        this.pos += len;
        this.rem -= len;

        if ( this.rem <= 0 ) {
            close();
        }

        return len;
    }

    @Override
    public int available()
    {
        if ( this.rem > Integer.MAX_VALUE ) {
            return Integer.MAX_VALUE;
        }
        else if ( this.rem < 0 ) {
            return 0;
        }
        else {
            return (int)this.rem;
        }
    }

    public long size()
    {
        ensureOpen();

        return this.entry.getSize();
    }

    @Override
    public void close()
    {
        this.rem = 0;
        this.entry = null;
        this.fileSystem = null;
    }

    private void ensureOpen()
    {
        if ( null == this.entry ) {
            throw new IllegalStateException( "stream has been closed" );
        }
    }
}
