/*
 * @(#)QtDefaultGraphicsConfiguration.java	1.14 06/10/10
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

package java.awt;


import java.awt.image.BufferedImage;
import java.awt.image.ColorModel;
import java.awt.image.DirectColorModel;
import java.awt.image.IndexColorModel;


/**
 @version 1.4, 05/03/02 */

class QtDefaultGraphicsConfiguration extends QtGraphicsConfiguration {
	
	// See QtBackEnd.cpp for implementation
	private static native ColorModel getQtColorModel();
    
	private ColorModel colorModel;

    QtDefaultGraphicsConfiguration(QtGraphicsDevice device) {
        super(device);
        getColorModel(); // side-effect of creating the imageType
    }
	
    public synchronized ColorModel getColorModel() {
        if (colorModel == null) {
            colorModel = getQtColorModel();
            imageType = toBufferedImageType(colorModel);
        }

        return colorModel;
    }

    private int toBufferedImageType(ColorModel cm) {
        int type = BufferedImage.TYPE_CUSTOM;
        switch (cm.getPixelSize()) {
        case 32 :
            if (cm.isAlphaPremultiplied()) {
                type = BufferedImage.TYPE_INT_ARGB_PRE ;
            }
            else {
                type = BufferedImage.TYPE_INT_ARGB;
            }
            break;
        case 24 :
            type = BufferedImage.TYPE_INT_RGB;
            if ( cm instanceof DirectColorModel ) {
                int blue_mask = ((DirectColorModel)cm).getBlueMask();
                if ( (blue_mask & 0x00FF0000) != 0 ) // is it BGR
                    type = BufferedImage.TYPE_INT_BGR;
            }
            break;
        case 15 :
            type = BufferedImage.TYPE_USHORT_555_RGB;
            break;
        case 16 :
            type = BufferedImage.TYPE_USHORT_565_RGB;
            break;
        case 8 :
            if ( cm instanceof IndexColorModel ) {
                type = BufferedImage.TYPE_BYTE_INDEXED;
            }
            break;
        }

        return type;
    }
}



