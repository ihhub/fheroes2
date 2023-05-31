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

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;

public class SeekableInputFile implements SeekableInput
{
    private final RandomAccessFile channel;

    public SeekableInputFile( File file ) throws IOException
    {
        if ( !file.exists() ) {
            throw new FileNotFoundException( "File does not exist: " + file );
        }
        this.channel = new RandomAccessFile( file, "r" );
    }

    public void seek( long pos ) throws IOException
    {
        this.channel.seek( pos );
    }

    public int read( byte[] b, int off, int len ) throws IOException
    {
        return this.channel.read( b, off, len );
    }

    public void close() throws IOException
    {
        this.channel.close();
    }
}
