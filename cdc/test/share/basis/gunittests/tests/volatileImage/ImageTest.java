/*
 * 
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation. 
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt). 
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA 
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions. 
 */


package tests.volatileImage ;

import java.awt.* ;
import java.util.* ;
import java.lang.reflect.* ;
import java.lang.* ;
import java.awt.event.* ;
import java.awt.image.* ;
import gunit.framework.* ;

public class ImageTest extends TestCase {
    public void testImage() {
        Graphics g = getGraphics() ;
        Image image = getImage(this.getArguments()) ;
        if ( image != null ) {
            g.drawImage(image,0,0,null) ;
        }
    }

    Image getImage(String[] args) {
        if ( args != null && args.length > 0) {
            return loadImage(args[0]) ;
        }
        else {
            throw new RuntimeException("No image file in test arguments") ;
        }
    }

   protected boolean useBackBuffer() {
       return false;
   }
}

