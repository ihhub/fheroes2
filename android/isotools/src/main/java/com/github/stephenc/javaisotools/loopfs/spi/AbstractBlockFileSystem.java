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

import java.io.IOException;
import java.util.Iterator;

import com.github.stephenc.javaisotools.loopfs.api.FileEntry;

public abstract class AbstractBlockFileSystem<T extends FileEntry> extends AbstractFileSystem<T>
{
    private final int blockSize;
    private final int reservedBlocks;
    private VolumeDescriptorSet<T> volumeDescriptorSet;

    protected AbstractBlockFileSystem( final SeekableInput seekable, final boolean readOnly, final int blockSize, final int reservedBlocks ) throws IOException
    {
        super( seekable, readOnly );

        if ( blockSize <= 0 ) {
            throw new IllegalArgumentException( "'blockSize' must be > 0" );
        }
        if ( reservedBlocks < 0 ) {
            throw new IllegalArgumentException( "'reservedBlocks' must be >= 0" );
        }

        this.blockSize = blockSize;
        this.reservedBlocks = reservedBlocks;
    }

    public final Iterator<T> iterator()
    {
        ensureOpen();

        // load the volume descriptors if necessary
        if ( null == this.volumeDescriptorSet ) {
            try {
                loadVolumeDescriptors();
            }
            catch ( IOException ex ) {
                throw new RuntimeException( ex );
            }
        }

        return iterator( this.volumeDescriptorSet.getRootEntry() );
    }

    protected void loadVolumeDescriptors() throws IOException
    {
        final byte[] buffer = new byte[this.blockSize];

        this.volumeDescriptorSet = createVolumeDescriptorSet();

        // skip the reserved blocks, then read volume descriptor blocks sequentially and add them
        // to the VolumeDescriptorSet
        int block = this.reservedBlocks;
        while ( readBlock( block, buffer ) && !this.volumeDescriptorSet.deserialize( buffer ) ) {
            block++;
        }
    }

    protected final boolean readBlock( final long block, final byte[] buffer ) throws IOException
    {
        final int bytesRead = readData( block * this.blockSize, buffer, 0, this.blockSize );

        if ( bytesRead <= 0 ) {
            return false;
        }

        if ( this.blockSize != bytesRead ) {
            throw new IOException( "Could not deserialize a complete block" );
        }

        return true;
    }

    protected final synchronized int readData( final long startPos, final byte[] buffer, final int offset, final int len ) throws IOException
    {
        seek( startPos );
        return read( buffer, offset, len );
    }

    protected final VolumeDescriptorSet<T> getVolumeDescriptorSet()
    {
        return this.volumeDescriptorSet;
    }

    protected abstract Iterator<T> iterator( T root );

    protected abstract VolumeDescriptorSet<T> createVolumeDescriptorSet();
}
