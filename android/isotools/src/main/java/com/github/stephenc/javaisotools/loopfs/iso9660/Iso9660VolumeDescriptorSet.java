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

import com.github.stephenc.javaisotools.loopfs.api.LoopFileSystemException;
import com.github.stephenc.javaisotools.loopfs.spi.VolumeDescriptorSet;

public class Iso9660VolumeDescriptorSet implements VolumeDescriptorSet<Iso9660FileEntry>
{
    public static final int TYPE_BOOTRECORD = 0;
    public static final int TYPE_PRIMARY_DESCRIPTOR = 1;
    public static final int TYPE_SUPPLEMENTARY_DESCRIPTOR = 2;
    public static final int TYPE_PARTITION_DESCRIPTOR = 3;
    public static final int TYPE_TERMINATOR = 255;

    private final Iso9660FileSystem isoFile;

    private String application;
    private Iso9660FileEntry rootDirectoryEntry;

    public String encoding = Constants.DEFAULT_ENCODING;
    public String escapeSequences;

    private boolean hasPrimary = false;
    private boolean hasSupplementary = false;

    public Iso9660VolumeDescriptorSet( Iso9660FileSystem fileSystem )
    {
        this.isoFile = fileSystem;
    }

    public boolean deserialize( byte[] descriptor ) throws IOException
    {
        final int type = Util.getUInt8( descriptor, 1 );

        boolean terminator = false;

        switch ( type ) {
        case TYPE_TERMINATOR:
            if ( !this.hasPrimary ) {
                throw new LoopFileSystemException( "No primary volume descriptor found" );
            }
            terminator = true;
            break;
        case TYPE_PRIMARY_DESCRIPTOR:
            deserializePrimary( descriptor );
            break;
        case TYPE_SUPPLEMENTARY_DESCRIPTOR:
            deserializeSupplementary( descriptor );
            break;
        case TYPE_BOOTRECORD:
        case TYPE_PARTITION_DESCRIPTOR:
        default:
            break;
        }

        return terminator;
    }

    private void deserializePrimary( byte[] descriptor ) throws IOException
    {
        // according to the spec, some ISO 9660 file systems can contain multiple identical primary
        // volume descriptors
        if ( this.hasPrimary ) {
            return;
        }

        validateBlockSize( descriptor );

        if ( !this.hasSupplementary ) {
            deserializeCommon( descriptor );
        }

        this.application = Util.getDChars( descriptor, 575, 128 );

        this.hasPrimary = true;
    }

    private void deserializeSupplementary( byte[] descriptor ) throws IOException
    {
        // for now, only recognize one supplementary descriptor
        if ( this.hasSupplementary ) {
            return;
        }

        validateBlockSize( descriptor );

        String escapeSequences = Util.getDChars( descriptor, 89, 32 );

        String enc = getEncoding( escapeSequences );

        if ( null != enc ) {
            this.encoding = enc;
            this.escapeSequences = escapeSequences;

            deserializeCommon( descriptor );

            this.hasSupplementary = true;
        }
    }

    private void deserializeCommon( byte[] descriptor )
    {
        this.rootDirectoryEntry = new Iso9660FileEntry( this.isoFile, descriptor, 157 );
    }

    private void validateBlockSize( byte[] descriptor ) throws IOException
    {
        int blockSize = Util.getUInt16Both( descriptor, 129 );
        if ( blockSize != Constants.DEFAULT_BLOCK_SIZE ) {
            throw new LoopFileSystemException( "Invalid block size: " + blockSize );
        }
    }

    private String getEncoding( String escapeSequences )
    {
        String encoding = null;

        switch ( escapeSequences ) {
        case "%/@":
            // UCS-2 level 1
        case "%/C":
            // UCS-2 level 2
        case "%/E":
            // UCS-2 level 3
            encoding = "UTF-16BE";
            break;
        default:
            break;
        }

        return encoding;
    }

    public String getApplication()
    {
        return this.application;
    }

    public Iso9660FileEntry getRootEntry()
    {
        return this.rootDirectoryEntry;
    }

    public String getEncoding()
    {
        return this.encoding;
    }
}
