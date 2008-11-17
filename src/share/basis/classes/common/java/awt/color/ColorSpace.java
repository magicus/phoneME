/*
 * @(#)ColorSpace.java	1.8 06/10/10
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
 *
 */

package java.awt.color;

public abstract class ColorSpace implements java.io.Serializable {
    static final long serialVersionUID = -409452704308689724L;
    private int type;
    private int numComponents;
    // Cache of singletons for the predefined color spaces.
    private static ColorSpace sRGBspace;
    /**
     * Any of the family of RGB color spaces.
     */
    public static final int TYPE_RGB = 5;
    /**
     * The sRGB color space defined at
     * <A href="http://www.w3.org/pub/WWW/Graphics/Color/sRGB.html">
     * http://www.w3.org/pub/WWW/Graphics/Color/sRGB.html
     * </A>.
     */
    public static final int CS_sRGB = 1000;
    /**
     * Constructs a ColorSpace object given a color space type
     * and the number of components.
     * @param type One of the <CODE>ColorSpace</CODE> type constants.
     * @param numcomponents The number of components in the color space.
     */
    protected ColorSpace (int type, int numcomponents) {
        this.type = type;
        this.numComponents = numcomponents;
    }

    /**
     * Returns a ColorSpace representing one of the specific
     * predefined color spaces.
     * @param colorspace a specific color space identified by one of
     *        the predefined class constants (e.g. CS_sRGB, CS_LINEAR_RGB,
     *        CS_CIEXYZ, CS_GRAY, or CS_PYCC)
     * @return The requested <CODE>ColorSpace</CODE> object.
     */
    // NOTE: This method may be called by privileged threads.
    //       DO NOT INVOKE CLIENT CODE ON THIS THREAD!
    public static ColorSpace getInstance(int colorspace) {
        ColorSpace    theColorSpace;
        switch (colorspace) {
        case CS_sRGB:
            if (sRGBspace == null) {
                sRGBspace = new RGBColorSpace();
            }
            theColorSpace = sRGBspace;
            break;

        default:
            throw new IllegalArgumentException ("Unknown color space");
        }
        return theColorSpace;
    }

    /**
     * Returns the name of the component given the component index.
     * @param idx The component index.
     * @return The name of the component at the specified index.
     */
    public String getName(int idx) {
        /* TODO - handle common cases here */
        return new String("Unnamed color component(" + idx + ")");
    }

    /**
     * Returns the number of components of this ColorSpace.
     * @return The number of components in this <CODE>ColorSpace</CODE>.
     */
    public int getNumComponents() {
        return numComponents;
    }

    /**
     * Returns the color space type of this ColorSpace (for example
     * TYPE_RGB, TYPE_XYZ, ...).  The type defines the
     * number of components of the color space and the interpretation,
     * e.g. TYPE_RGB identifies a color space with three components - red,
     * green, and blue.  It does not define the particular color
     * characteristics of the space, e.g. the chromaticities of the
     * primaries.
     * @return The type constant that represents the type of this
     *         <CODE>ColorSpace</CODE>.
     */
    public int getType() {
        return type;
    }

    /**
     * Returns true if the ColorSpace is CS_sRGB.
     * @return <CODE>true</CODE> if this is a <CODE>CS_sRGB</CODE> color
     * space, <code>false</code> if it is not.
     */
    public boolean isCS_sRGB() {
        /* NOTE - make sure we know sRGBspace exists already */
        return (this == sRGBspace);
    }
}

class RGBColorSpace extends ColorSpace {
    public RGBColorSpace() {
        super(TYPE_RGB, 3);
    }
}
