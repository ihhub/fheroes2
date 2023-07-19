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

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.util.Iterator;

import com.github.stephenc.javaisotools.loopfs.spi.AbstractBlockFileSystem;
import com.github.stephenc.javaisotools.loopfs.spi.SeekableInput;
import com.github.stephenc.javaisotools.loopfs.spi.SeekableInputFile;
import com.github.stephenc.javaisotools.loopfs.spi.VolumeDescriptorSet;

public class Iso9660FileSystem extends AbstractBlockFileSystem<Iso9660FileEntry>
{
    public Iso9660FileSystem( File file, boolean readOnly ) throws IOException
    {
        this( new SeekableInputFile( file ), readOnly );
    }

    public Iso9660FileSystem( SeekableInput seekable, boolean readOnly ) throws IOException
    {
        super( seekable, readOnly, Constants.DEFAULT_BLOCK_SIZE, Constants.RESERVED_SECTORS );
    }

    public String getEncoding()
    {
        return ( (Iso9660VolumeDescriptorSet)getVolumeDescriptorSet() ).getEncoding();
    }

    public InputStream getInputStream( Iso9660FileEntry entry )
    {
        ensureOpen();
        return new EntryInputStream( entry, this );
    }

    byte[] getBytes( Iso9660FileEntry entry ) throws IOException
    {
        if ( entry.getSize() > Integer.MAX_VALUE ) {
            throw new IOException( "Entry too large" );
        }
        int size = (int)entry.getSize();

        byte[] buf = new byte[size];

        readBytes( entry, 0, buf, 0, size );

        return buf;
    }

    int readBytes( Iso9660FileEntry entry, long entryOffset, byte[] buffer, int bufferOffset, int len ) throws IOException
    {
        long startPos = ( entry.getStartBlock() * Constants.DEFAULT_BLOCK_SIZE ) + entryOffset;
        return readData( startPos, buffer, bufferOffset, len );
    }

    protected Iterator<Iso9660FileEntry> iterator( Iso9660FileEntry rootEntry )
    {
        return new EntryIterator( this, rootEntry );
    }

    protected VolumeDescriptorSet<Iso9660FileEntry> createVolumeDescriptorSet()
    {
        return new Iso9660VolumeDescriptorSet( this );
    }
}
