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


package gunit.framework ;

import java.awt.* ;
import java.awt.image.* ;
//import gunit.image.ImageComparator ;
//import gunit.image.ImageUtils ;

/**
 * <code>TestCase</code> extends <code>BaseTestCase</code> and 
 * provides implementations using personal specification.
 *
 */
public class TestCase extends BaseTestCase {

    public void clearToBackgroundColor(Graphics g, Color c, int x, int y,
                                       int w, int h) {
        Container cont = getContainer();
        Color old = g.getColor();
        g.setColor(c);
        g.fillRect(x, y, w, h);
        g.setColor(old);
    }

    protected BufferedImage createBufferedImage(int width, int height) {
        return GraphicsEnvironment.getLocalGraphicsEnvironment().
            getDefaultScreenDevice().
            getDefaultConfiguration().createCompatibleImage(width, 
                                      height);
    }


    protected void assertImageEquals(String message, 
                                     Object expected, 
                                     Object actual) {
    }

    protected void encodeImage(BufferedImage image, String filename){
    }
}
