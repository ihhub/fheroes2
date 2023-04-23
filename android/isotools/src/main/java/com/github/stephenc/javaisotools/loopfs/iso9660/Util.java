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

import com.github.stephenc.javaisotools.loopfs.util.LittleEndian;
import java.io.UnsupportedEncodingException;

public final class Util
{
    public static int getUInt8( byte[] block, int pos )
    {
        return LittleEndian.getUInt8( block, pos - 1 );
    }

    public static int getUInt16Both( byte[] block, int pos )
    {
        return LittleEndian.getUInt16( block, pos - 1 );
    }

    public static long getUInt32LE( byte[] block, int pos )
    {
        return LittleEndian.getUInt32( block, pos - 1 );
    }

    public static String getDChars( byte[] block, int pos, int length )
    {
        return new String( block, pos - 1, length ).trim();
    }

    public static String getDChars( byte[] block, int pos, int length, String encoding )
    {
        try {
            return new String( block, pos - 1, length, encoding ).trim();
        }
        catch ( UnsupportedEncodingException ex ) {
            throw new RuntimeException( ex );
        }
    }

    private Util() {}
}
