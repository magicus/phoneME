/*
 * @(#)ColorModel.java	1.38 06/10/10
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

package java.awt.image;

import java.awt.Transparency;
import java.awt.color.ColorSpace;

/**
 * A class that encapsulates the methods for translating from pixel values
 * to alpha, red, green, and blue color components for an image.  This
 * class is abstract.
 *
 * @see IndexColorModel
 * @see DirectColorModel
 *
 * @version	1.22 02/20/02
 * @author 	Jim Graham
 */
public abstract class ColorModel implements Transparency {
    private int pData;
    protected int pixel_bits;
    int transparency = Transparency.TRANSLUCENT;
    int nBits[];        
    boolean supportsAlpha = true;
    boolean isAlphaPremultiplied = false;
    int numComponents = -1;
    int numColorComponents = -1;
    ColorSpace colorSpace = ColorSpace.getInstance(ColorSpace.CS_sRGB);
    /**
     * Data type of the array used to represent pixel values.
     */
    private int transferType;        
    private static ColorModel RGBdefault;
    /**
     * Return a ColorModel which describes the default format for
     * integer RGB values used throughout the AWT image interfaces.
     * The format for the RGB values is an integer with 8 bits
     * each of alpha, red, green, and blue color components ordered
     * correspondingly from the most significant byte to the least
     * significant byte, as in:  0xAARRGGBB
     */
    public static ColorModel getRGBdefault() {
        if (RGBdefault == null) {
            RGBdefault = new DirectColorModel(32,
                        0x00ff0000,	// Red
                        0x0000ff00,	// Green
                        0x000000ff,	// Blue
                        0xff000000	// Alpha
                    );
        }
        return RGBdefault;
    }

    /**
     * Returns whether or not alpha is supported in this
     * <code>ColorModel</code>.
     * @return <code>true</code> if alpha is supported in this
     * <code>ColorModel</code>; <code>false</code> otherwise.
     */
    final public boolean hasAlpha() {
        return supportsAlpha;
    }

    /**
     * Returns whether or not the alpha has been premultiplied in the
     * pixel values to be translated by this <code>ColorModel</code>.
     * If the boolean is <code>true</code>, this <code>ColorModel</code>
     * is to be used to interpret pixel values in which color and alpha
     * information are represented as separate spatial bands, and color
     * samples are assumed to have been multiplied by the
     * alpha sample.
     * @return <code>true</code> if the alpha values are premultiplied
     *		in the pixel values to be translated by this
     *		<code>ColorModel</code>; <code>false</code> otherwise.
     */
    final public boolean isAlphaPremultiplied() {
        return isAlphaPremultiplied;
    }

    /**
     * Constructs a ColorModel which describes a pixel of the specified
     * number of bits.
     */
    public ColorModel(int bits) {
        pixel_bits = bits;
        if (bits < 1) {
            throw new IllegalArgumentException("Number of bits must be > 0");
        }
        numComponents = 4;
        numColorComponents = 3;
        //maxBits = bits;
        // TODO: make sure transferType is set correctly
        transferType = ColorModel.getDefaultTransferType(bits);
    }

    /**
     * Constructs a <code>ColorModel</code> that translates pixel values
     * to color/alpha components.  Color components will be in the
     * specified <code>ColorSpace</code>. <code>pixel_bits</code> is the
     * number of bits in the pixel values.  The bits array
     * specifies the number of significant bits per color and alpha component.
     * Its length should be the number of components in the
     * <code>ColorSpace</code> if there is no alpha information in the
     * pixel values, or one more than this number if there is alpha
     * information.  <code>hasAlpha</code> indicates whether or not alpha
     * information is present.  The <code>boolean</code>
     * <code>isAlphaPremultiplied</code> specifies how to interpret pixel
     * values in which color and alpha information are represented as
     * separate spatial bands.  If the <code>boolean</code>
     * is <code>true</code>, color samples are assumed to have been
     * multiplied by the alpha sample.  The <code>transparency</code>
     * specifies what alpha values can be represented by this color model.
     * The transfer type is the type of primitive array used to represent
     * pixel values.  Note that the bits array contains the number of
     * significant bits per color/alpha component after the translation
     * from pixel values.  For example, for an
     * <code>IndexColorModel</code> with <code>pixel_bits</code> equal to
     * 16, the bits array might have four elements with each element set
     * to 8.
     * @param pixel_bits the number of bits in the pixel values
     * @param bits array that specifies the number of significant bits
     *		per color and alpha component
     * @param cspace the specified <code>ColorSpace</code>
     * @param hasAlpha <code>true</code> if alpha information is present;
     *		<code>false</code> otherwise
     * @param isAlphaPremultiplied <code>true</code> if color samples are
     *		assumed to be premultiplied by the alpha samples;
     *		<code>false</code> otherwise
     * @param transparency what alpha values can be represented by this
     *		color model
     * @param transferType the type of the array used to represent pixel
     *		values
     * @throws IllegalArgumentException if the length of
     *		the bit array is less than the number of color or alpha
     *		components in this <code>ColorModel</code>, or if the
     *          transparency is not a valid value.
     * @throws IllegalArgumentException if the sum of the number
     * 		of bits in <code>bits</code> is less than 1 or if
     *          any of the elements in <code>bits</code> is less than 0.
     * @see java.awt.Transparency
     */
    protected ColorModel(int pixel_bits, int[] bits, ColorSpace cspace,
        boolean hasAlpha,
        boolean isAlphaPremultiplied,
        int transparency,
        int transferType) {
        colorSpace = cspace;
        numColorComponents = cspace.getNumComponents();
        numComponents = numColorComponents + (hasAlpha ? 1 : 0);
        supportsAlpha = hasAlpha;
        if (bits.length < numComponents) {
            throw new IllegalArgumentException("Number of color/alpha " +
                    "components should be " +
                    numComponents +
                    " but length of bits array is " +
                    bits.length);
        }
        // 4186669
        if (transparency < Transparency.OPAQUE ||
            transparency > Transparency.TRANSLUCENT) {
            throw new IllegalArgumentException("Unknown transparency: " +
                    transparency);
        }
        if (supportsAlpha == false) {
            this.isAlphaPremultiplied = false;
            this.transparency = Transparency.OPAQUE;
        } else {
            this.isAlphaPremultiplied = isAlphaPremultiplied;
            this.transparency = transparency;
        }
        nBits = (int[]) bits.clone();
        this.pixel_bits = pixel_bits;
        if (pixel_bits <= 0) {
            throw new IllegalArgumentException("Number of pixel bits must " +
                    "be > 0");
        }
        // Check for bits < 0
        //maxBits = 0;
        int maxBits = 0;
        for (int i = 0; i < bits.length; i++) {
            // bug 4304697
            if (bits[i] < 0) {
                throw new
                    IllegalArgumentException("Number of bits must be >= 0");
            }
            if (maxBits < bits[i]) {
                maxBits = bits[i];
            }
        }
        // Make sure that we don't have all 0-bit components
        if (maxBits == 0) {
            throw new IllegalArgumentException("There must be at least " +
                    "one component with > 0 " +
                    "pixel bits.");
        }
        // Save the transfer type
        this.transferType = transferType;
    }

    /**
     * Returns the number of bits per pixel described by this ColorModel.
     */
    public int getPixelSize() {
        return pixel_bits;
    }

    /**
     * The subclass must provide a function which provides the red
     * color compoment for the specified pixel.
     * @return		The red color component ranging from 0 to 255
     */
    public abstract int getRed(int pixel);

    /**
     * The subclass must provide a function which provides the green
     * color compoment for the specified pixel.
     * @return		The green color component ranging from 0 to 255
     */
    public abstract int getGreen(int pixel);

    /**
     * The subclass must provide a function which provides the blue
     * color compoment for the specified pixel.
     * @return		The blue color component ranging from 0 to 255
     */
    public abstract int getBlue(int pixel);

    /**
     * The subclass must provide a function which provides the alpha
     * color compoment for the specified pixel.
     * @return		The alpha transparency value ranging from 0 to 255
     */
    public abstract int getAlpha(int pixel);

    /**
     * Returns the color of the pixel in the default RGB color model.
     * @see ColorModel#getRGBdefault
     */
    public int getRGB(int pixel) {
        return (getAlpha(pixel) << 24)
            | (getRed(pixel) << 16)
            | (getGreen(pixel) << 8)
            | (getBlue(pixel) << 0);
    }

    /**
     * Tests if the specified <code>Object</code> is an instance of
     * <code>ColorModel</code> and if it equals this
     * <code>ColorModel</code>.
     * @param obj the <code>Object</code> to test for equality
     * @return <code>true</code> if the specified <code>Object</code>
     * is an instance of <code>ColorModel</code> and equals this
     * <code>ColorModel</code>; <code>false</code> otherwise.
     */
    public boolean equals(Object obj) {
        if (!(obj instanceof ColorModel)) {
            return false;
        }
        ColorModel cm = (ColorModel) obj;
        if (this == cm) {
            return true;
        }
        if (supportsAlpha != cm.hasAlpha() ||
            isAlphaPremultiplied != cm.isAlphaPremultiplied() ||
            transparency != cm.getTransparency() ||
            pixel_bits != cm.getPixelSize()) {
            return false;
        }
        return true;
    }

    /**
     * Disposes of system resources associated with this
     * <code>ColorModel</code> once this <code>ColorModel</code> is no
     * longer referenced.
     */
    public void finalize() {
    }

    /**
     * Returns the hash code for this ColorModel.
     *
     * @return    a hash code for this ColorModel.
     */
    public int hashCode() {
        int result = 0;
        result = (supportsAlpha ? 2 : 3) +
                (isAlphaPremultiplied ? 4 : 5) +
                pixel_bits * 6 +
                transparency * 7;
        return result;
    }

    /**
     * Returns the <code>ColorSpace</code> associated with this
     * <code>ColorModel</code>.
     * @return the <code>ColorSpace</code> of this
     * <code>ColorModel</code>.
     */
    final public ColorSpace getColorSpace() {
        return colorSpace;
    }

    /**
     * Returns the transparency.  Returns either OPAQUE, BITMASK,
     * or TRANSLUCENT.
     * @return the transparency of this <code>ColorModel</code>.
     * @see Transparency#OPAQUE
     * @see Transparency#BITMASK
     * @see Transparency#TRANSLUCENT
     */
    public int getTransparency() {
        return transparency;
    }
        
    /**
     * Returns the number of bits for the specified color/alpha component.
     * Color components are indexed in the order specified by the 
     * <code>ColorSpace</code>.  Typically, this order reflects the name
     * of the color space type. For example, for TYPE_RGB, index 0
     * corresponds to red, index 1 to green, and index 2
     * to blue.  If this <code>ColorModel</code> supports alpha, the alpha
     * component corresponds to the index following the last color
     * component.
     * @param componentIdx the index of the color/alpha component
     * @return the number of bits for the color/alpha component at the
     *		specified index.
     * @throws ArrayIndexOutOfBoundsException if <code>componentIdx</code> 
     *         is greater than the number of components or
     *         less than zero
     * @throws NullPointerException if the number of bits array is 
     *         <code>null</code>
     */
    public int getComponentSize(int componentIdx) {
        // NOTE
        if (nBits == null) {
            throw new NullPointerException("Number of bits array is null.");
        }
        return nBits[componentIdx];
    }

    /**
     * Returns an array of the number of bits per color/alpha component.  
     * The array contains the color components in the order specified by the
     * <code>ColorSpace</code>, followed by the alpha component, if
     * present.
     * @return an array of the number of bits per color/alpha component
     */
    public int[] getComponentSize() {
        if (nBits != null) {
            return (int[]) nBits.clone();
        }
        return null;
    }        

    /**
     * Returns the number of components, including alpha, in this
     * <code>ColorModel</code>.  This is equal to the number of color
     * components, optionally plus one, if there is an alpha component.
     * @return the number of components in this <code>ColorModel</code>
     */
    public int getNumComponents() {
        return numComponents;
    }

    /**
     * Returns the number of color components in this
     * <code>ColorModel</code>.
     * This is the number of components returned by
     * {@link ColorSpace#getNumComponents}.
     * @return the number of color components in this
     * <code>ColorModel</code>.
     * @see ColorSpace#getNumComponents
     */
    public int getNumColorComponents() {
        return numColorComponents;
    }    

    /**
     * Returns the transfer type of this <code>ColorModel</code>.
     * The transfer type is the type of primitive array used to represent
     * pixel values as arrays.
     * @return the transfer type.
     */
    public final int getTransferType() {
        return transferType;
    }    

    static final int getDefaultTransferType(int pixel_bits) {
        if (pixel_bits <= 8) {
            return DataBuffer.TYPE_BYTE;
        } else if (pixel_bits <= 16) {
            return DataBuffer.TYPE_USHORT;
        } else if (pixel_bits <= 32) {
            return DataBuffer.TYPE_INT;
        } else {
            return DataBuffer.TYPE_UNDEFINED;
        }
    }     

    /**
     * Returns the <code>String</code> representation of the contents of
     * this <code>ColorModel</code>object.
     * @return a <code>String</code> representing the contents of this
     * <code>ColorModel</code> object.
     */
    public String toString() {
        return new String("ColorModel: #pixelBits = " + pixel_bits
                + " numComponents = " + numComponents
                + " color space = " + colorSpace
                + " transparency = " + transparency
                + " has alpha = " + supportsAlpha
                + " isAlphaPre = " + isAlphaPremultiplied
            );
    }

}
