/*
 *  Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
 *  DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License version
 *  2 only, as published by the Free Software Foundation.
 *  
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  General Public License version 2 for more details (a copy is
 *  included at /legal/license.txt).
 *  
 *  You should have received a copy of the GNU General Public License
 *  version 2 along with this work; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 *  02110-1301 USA
 *  
 *  Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 *  Clara, CA 95054 or visit www.sun.com if you need additional
 *  information or have any questions.
 */

package com.sun.mmedia;

import javax.microedition.media.Control;

public class Jsr234Proxy {
    private static Jsr234Proxy _instance;
    private static String[] _emptyNamesList = { };
    
    protected Jsr234Proxy() {}
    
    public static synchronized Jsr234Proxy getInstance() {
        if( null == _instance )
        {
            try {
                _instance = ( Jsr234Proxy )Class.forName(
                   "com.sun.amms.player.SupplementsToMMAPI" ).newInstance();
            } catch ( Exception e )
            {
                _instance = new Jsr234Proxy();
            }
        }
        
        return _instance;
    }
    
    public boolean isJsr234Available()
    {
        return false;
    }

    public String [] getJsr234PlayerControlNames() {
        return _emptyNamesList;
    }
    
    public Control getNewControl( HighLevelPlayer p, String controlName )
    {
        return null;
    }

}
