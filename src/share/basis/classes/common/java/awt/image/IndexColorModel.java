/*
 * @(#)IndexColorModel.java	1.30 06/10/10
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
import java.math.BigInteger;

/**
 * The <code>IndexColorModel</code> class is a <code>ColorModel</code>
 * class that works with pixel values consisting of a
 * single sample that is an index into a fixed colormap in the default
 * sRGB color space.  The colormap specifies red, green, blue, and
 * optional alpha components corresponding to each index.  All components
 * are represented in the colormap as 8-bit unsigned integral values.
 * Some constructors allow the caller to specify "holes" in the colormap
 * by indicating which colormap entries are valid and which represent
 * unusable colors via the bits set in a <code>BigInteger</code> object.
 * This color model is similar to an X11 PseudoColor visual.
 * <p>
 * Some constructors provide a means to specify an alpha component
 * for each pixel in the colormap, while others either provide no
 * such means or, in some cases, a flag to indicate whether the
 * colormap data contains alpha values.  If no alpha is supplied to
 * the constructor, an opaque alpha component (alpha = 1.0) is
 * assumed for each entry.
 * An optional transparent pixel value can be supplied that indicates a
 * completely transparent pixel, regardless of any alpha component
 * supplied or assumed for that pixel value.
 * Note that the color components in the colormap of an
 * <code>IndexColorModel</code> objects are never pre-multiplied with
 * the alpha components.
 * <p>
 * <a name="transparency">
 * The transparency of an <code>IndexColorModel</code> object is
 * determined by examining the alpha components of the colors in the
 * colormap and choosing the most specific value after considering
 * the optional alpha values and any transparent index specified.
 * The transparency value is <code>Transparency.OPAQUE</code>
 * only if all valid colors in
 * the colormap are opaque and there is no valid transparent pixel.
 * If all valid colors
 * in the colormap are either completely opaque (alpha = 1.0) or
 * completely transparent (alpha = 0.0), which typically occurs when
 * a valid transparent pixel is specified,
 * the value is <code>Transparency.BITMASK</code>.
 * Otherwise, the value is <code>Transparency.TRANSLUCENT</code>, indicating
 * that some valid color has an alpha component that is
 * neither completely transparent nor completely opaque (0.0 < alpha < 1.0).
 * </a>
 * <p>
 * The index represented by a pixel value is stored in the least
 * significant <em>n</em> bits of the pixel representations passed to the
 * methods of this class, where <em>n</em> is the pixel size specified to the
 * constructor for a particular <code>IndexColorModel</code> object;
 * <em>n</em> must be between 1 and 16, inclusive.  
 * Higher order bits in pixel representations are assumed to be zero.
 * For those methods that use a primitive array pixel representation of
 * type <code>transferType</code>, the array length is always one.  
 * The transfer types supported are <code>DataBuffer.TYPE_BYTE</code> and 
 * <code>DataBuffer.TYPE_USHORT</code>.  A single int pixel 
 * representation is valid for all objects of this class, since it is 
 * always possible to represent pixel values used with this class in a 
 * single int.  Therefore, methods that use this representation do 
 * not throw an <code>IllegalArgumentException</code> due to an invalid 
 * pixel value.
 * <p>
 * Many of the methods in this class are final.  The reason for
 * this is that the underlying native graphics code makes assumptions
 * about the layout and operation of this class and those assumptions
 * are reflected in the implementations of the methods here that are
 * marked final.  You can subclass this class for other reaons, but
 * you cannot override or modify the behaviour of those methods.
 *
 * @see ColorModel
 * @see ColorSpace
 * @see DataBuffer
 *
 * @version 10 Feb 1997
 */
public class IndexColorModel extends ColorModel {
    private int rgb[];
    private int map_size;
    private int transparent_index = -1;
    private boolean allgrayopaque;
    private BigInteger validBits;
    
    private static int[] opaqueBits = {8, 8, 8};
    private static int[] alphaBits = {8, 8, 8, 8};

    /**
     * Constructs an <code>IndexColorModel</code> from the specified 
     * arrays of red, green, and blue components.  Pixels described 
     * by this color model all have alpha components of 255 
     * unnormalized (1.0&nbsp;normalized), which means they
     * are fully opaque.  All of the arrays specifying the color 
     * components must have at least the specified number of entries.  
     * The <code>ColorSpace</code> is the default sRGB space.
     * Since there is no alpha information in any of the arguments
     * to this constructor, the transparency value is always 
     * <code>Transparency.OPAQUE</code>.
     * The transfer type is the smallest of <code>DataBuffer.TYPE_BYTE</code>
     * or <code>DataBuffer.TYPE_USHORT</code> that can hold a single pixel.
     * @param bits	the number of bits each pixel occupies
     * @param size	the size of the color component arrays
     * @param r		the array of red color components
     * @param g		the array of green color components
     * @param b		the array of blue color components
     * @throws IllegalArgumentException if <code>bits</code> is less
     *         than 1 or greater than 16
     * @throws IllegalArgumentException if <code>size</code> is less
     *         than 1
     */
    public IndexColorModel(int bits, int size,
                           byte r[], byte g[], byte b[]) {
        super(bits, opaqueBits,
              ColorSpace.getInstance(ColorSpace.CS_sRGB),
              false, false, OPAQUE,
              ColorModel.getDefaultTransferType(bits));
        if (bits < 1 || bits > 16) {
            throw new IllegalArgumentException("Number of bits must be between"
                                               +" 1 and 16.");
        }
        setRGBs(size, r, g, b, null);
    }

    /**
     * Constructs an <code>IndexColorModel</code> from the given arrays 
     * of red, green, and blue components.  Pixels described by this color
     * model all have alpha components of 255 unnormalized 
     * (1.0&nbsp;normalized), which means they are fully opaque, except 
     * for the indicated transparent pixel.  All of the arrays
     * specifying the color components must have at least the specified
     * number of entries.
     * The <code>ColorSpace</code> is the default sRGB space.
     * The transparency value may be <code>Transparency.OPAQUE</code> or
     * <code>Transparency.BITMASK</code> depending on the arguments, as
     * specified in the <a href="#transparency">class description</a> above.
     * The transfer type is the smallest of <code>DataBuffer.TYPE_BYTE</code>
     * or <code>DataBuffer.TYPE_USHORT</code> that can hold a
     * single pixel.
     * @param bits	the number of bits each pixel occupies
     * @param size	the size of the color component arrays
     * @param r		the array of red color components
     * @param g		the array of green color components
     * @param b		the array of blue color components
     * @param trans	the index of the transparent pixel
     * @throws IllegalArgumentException if <code>bits</code> is less than
     *          1 or greater than 16
     * @throws IllegalArgumentException if <code>size</code> is less than
     *          1
     */
    public IndexColorModel(int bits, int size,
                           byte r[], byte g[], byte b[], int trans) {
        super(bits, opaqueBits,
              ColorSpace.getInstance(ColorSpace.CS_sRGB),
              false, false, OPAQUE,
              ColorModel.getDefaultTransferType(bits));
        if (bits < 1 || bits > 16) {
            throw new IllegalArgumentException("Number of bits must be between"
                                               +" 1 and 16.");
        }
        setRGBs(size, r, g, b, null);
        setTransparentPixel(trans);
    }
 
    /**
     * Constructs an <code>IndexColorModel</code> from the given 
     * arrays of red, green, blue and alpha components.  All of the 
     * arrays specifying the components must have at least the specified 
     * number of entries.
     * The <code>ColorSpace</code> is the default sRGB space.
     * The transparency value may be any of <code>Transparency.OPAQUE</code>,
     * <code>Transparency.BITMASK</code>,
     * or <code>Transparency.TRANSLUCENT</code>
     * depending on the arguments, as specified
     * in the <a href="#transparency">class description</a> above.
     * The transfer type is the smallest of <code>DataBuffer.TYPE_BYTE</code>
     * or <code>DataBuffer.TYPE_USHORT</code> that can hold a single pixel.
     * @param bits	the number of bits each pixel occupies
     * @param size	the size of the color component arrays
     * @param r		the array of red color components
     * @param g		the array of green color components
     * @param b		the array of blue color components
     * @param a		the array of alpha value components
     * @throws IllegalArgumentException if <code>bits</code> is less
     *           than 1 or greater than 16
     * @throws IllegalArgumentException if <code>size</code> is less
     *           than 1
     */
    public IndexColorModel(int bits, int size,
                           byte r[], byte g[], byte b[], byte a[]) {
        super (bits, alphaBits,
               ColorSpace.getInstance(ColorSpace.CS_sRGB),
               true, false, TRANSLUCENT,
               ColorModel.getDefaultTransferType(bits));
        if (bits < 1 || bits > 16) {
            throw new IllegalArgumentException("Number of bits must be between"
                                               +" 1 and 16.");
        }
        setRGBs(size, r, g, b, a);
    }

    /**
     * Constructs an <code>IndexColorModel</code> from a single 
     * array of interleaved red, green, blue and optional alpha 
     * components.  The array must have enough values in it to 
     * fill all of the needed component arrays of the specified 
     * size.  The <code>ColorSpace</code> is the default sRGB space.
     * The transparency value may be any of <code>Transparency.OPAQUE</code>,
     * <code>Transparency.BITMASK</code>,
     * or <code>Transparency.TRANSLUCENT</code>
     * depending on the arguments, as specified
     * in the <a href="#transparency">class description</a> above.
     * The transfer type is the smallest of
     * <code>DataBuffer.TYPE_BYTE</code> or <code>DataBuffer.TYPE_USHORT</code> 
     * that can hold a single pixel.
     * 
     * @param bits	the number of bits each pixel occupies
     * @param size	the size of the color component arrays
     * @param cmap	the array of color components
     * @param start	the starting offset of the first color component
     * @param hasalpha	indicates whether alpha values are contained in
     *			the <code>cmap</code> array
     * @throws IllegalArgumentException if <code>bits</code> is less
     *           than 1 or greater than 16
     * @throws IllegalArgumentException if <code>size</code> is less
     *           than 1
     */
  public IndexColorModel(int bits, int size, byte cmap[], int start,
                           boolean hasalpha) {
        this(bits, size, cmap, start, hasalpha, -1);
        if (bits < 1 || bits > 16) {
            throw new IllegalArgumentException("Number of bits must be between"
                                               +" 1 and 16.");
        }
    }

    /**
     * Constructs an <code>IndexColorModel</code> from a single array of 
     * interleaved red, green, blue and optional alpha components.  The 
     * specified transparent index represents a pixel that is considered
     * entirely transparent regardless of any alpha value specified
     * for it.  The array must have enough values in it to fill all
     * of the needed component arrays of the specified size.
     * The <code>ColorSpace</code> is the default sRGB space.
     * The transparency value may be any of <code>Transparency.OPAQUE</code>,
     * <code>Transparency.BITMASK</code>,
     * or <code>Transparency.TRANSLUCENT</code>
     * depending on the arguments, as specified
     * in the <a href="#transparency">class description</a> above.
     * The transfer type is the smallest of
     * <code>DataBuffer.TYPE_BYTE</code> or <code>DataBuffer.TYPE_USHORT</code>
     * that can hold a single pixel.
     * @param bits	the number of bits each pixel occupies
     * @param size	the size of the color component arrays
     * @param cmap	the array of color components
     * @param start	the starting offset of the first color component
     * @param hasalpha	indicates whether alpha values are contained in
     *			the <code>cmap</code> array
     * @param trans	the index of the fully transparent pixel
     * @throws IllegalArgumentException if <code>bits</code> is less than
     *               1 or greater than 16
     * @throws IllegalArgumentException if <code>size</code> is less than
     *               1
     */
  public IndexColorModel(int bits, int size, byte cmap[], int start,
                           boolean hasalpha, int trans) {
        // NOTE: This assumes the ordering: RGB[A]
        super(bits, opaqueBits,
              ColorSpace.getInstance(ColorSpace.CS_sRGB),
              false, false, OPAQUE,
              ColorModel.getDefaultTransferType(bits));

        if (bits < 1 || bits > 16) {
            throw new IllegalArgumentException("Number of bits must be between"
                                               +" 1 and 16.");
        }
        if (size < 1) {
            throw new IllegalArgumentException("Map size ("+size+
                                               ") must be >= 1");
        }
        map_size = size;
        rgb = new int[calcRealMapSize(bits, size)];
        int j = start;
        int alpha = 0xff;
        boolean allgray = true;
        int transparency = OPAQUE;
        for (int i = 0; i < size; i++) {
            int r = cmap[j++] & 0xff;
            int g = cmap[j++] & 0xff;
            int b = cmap[j++] & 0xff;
            allgray = allgray && (r == g) && (g == b);
            if (hasalpha) {
                alpha = cmap[j++] & 0xff;
                if (alpha != 0xff) {
                    if (alpha == 0x00) {
                        if (transparency == OPAQUE) {
                            transparency = BITMASK;
                        }
                        if (transparent_index < 0) {
                            transparent_index = i;
                        }
                    } else {
                        transparency = TRANSLUCENT;
                    }
                    allgray = false;
                }
            }
            rgb[i] = (alpha << 24) | (r << 16) | (g << 8) | b;
        }
        this.allgrayopaque = allgray;
        setTransparency(transparency);
        setTransparentPixel(trans);
    }

    private void setRGBs(int size, byte r[], byte g[], byte b[], byte a[]) {
        if (size < 1) {
            throw new IllegalArgumentException("Map size ("+size+
                                               ") must be >= 1");
        }
        map_size = size;
        rgb = new int[calcRealMapSize(pixel_bits, size)];
        int alpha = 0xff;
        int transparency = OPAQUE;
        boolean allgray = true;
        for (int i = 0; i < size; i++) {
            int rc = r[i] & 0xff;
            int gc = g[i] & 0xff;
            int bc = b[i] & 0xff;
            allgray = allgray && (rc == gc) && (gc == bc);
            if (a != null) {
                alpha = a[i] & 0xff;
                if (alpha != 0xff) {
                    if (alpha == 0x00) {
                        if (transparency == OPAQUE) {
                            transparency = BITMASK;
                        }
                        if (transparent_index < 0) {
                            transparent_index = i;
                        }
                    } else {
                        transparency = TRANSLUCENT;
                    }
                    allgray = false;
                }
            }
            rgb[i] = (alpha << 24) | (rc << 16) | (gc << 8) | bc;
        }
        this.allgrayopaque = allgray;
        setTransparency(transparency);
    }

    private void setRGBs(int size, int cmap[], int start, boolean hasalpha) {
        map_size = size;
        rgb = new int[calcRealMapSize(pixel_bits, size)];
        int j = start;
        int transparency = OPAQUE;
        boolean allgray = true;
        BigInteger validBits = this.validBits;
        for (int i = 0; i < size; i++, j++) {
            if (validBits != null && !validBits.testBit(i)) {
                continue;
            }
            int cmaprgb = cmap[j];
            int r = (cmaprgb >> 16) & 0xff;
            int g = (cmaprgb >>  8) & 0xff;
            int b = (cmaprgb      ) & 0xff;
            allgray = allgray && (r == g) && (g == b);
            if (hasalpha) {
                int alpha = cmaprgb >>> 24;
                if (alpha != 0xff) {
                    if (alpha == 0x00) {
                        if (transparency == OPAQUE) {
                            transparency = BITMASK;
                        }
                        if (transparent_index < 0) {
                            transparent_index = i;
                        }
                    } else {
                        transparency = TRANSLUCENT;
                    }
                    allgray = false;
                }
            } else {
                cmaprgb |= 0xff000000;
            }
            rgb[i] = cmaprgb;
        }
        this.allgrayopaque = allgray;
        setTransparency(transparency);
    }

    private int calcRealMapSize(int bits, int size) {
        int newSize = Math.max(1 << bits, size);
        return Math.max(newSize, 256);
    }

    /**
     * Returns the size of the color/alpha component arrays in this
     * <code>IndexColorModel</code>.
     * @return the size of the color and alpha component arrays.
     */
    final public int getMapSize() {
        return map_size;
    }

    /**
     * Returns the index of the transparent pixel in this 
     * <code>IndexColorModel</code> or -1 if there is no transparent pixel.
     * @return the index of this <code>IndexColorModel</code> object's
     *       transparent pixel, or -1 if there is no such pixel.
     */
    final public int getTransparentPixel() {
        return transparent_index;
    }

    /**
     * Copies the array of red color components into the specified array.  
     * Only the initial entries of the array as specified by 
     * {@link #getMapSize() getMapSize} are written.
     * @param r the specified array into which the elements of the 
     *      array of red color components are copied 
     */
    final public void getReds(byte r[]) {
        for (int i = 0; i < map_size; i++) {
            r[i] = (byte) (rgb[i] >> 16);
        }
    }

    /**
     * Copies the array of green color components into the specified array.  
     * Only the initial entries of the array as specified by 
     * <code>getMapSize</code> are written.
     * @param g the specified array into which the elements of the 
     *      array of green color components are copied 
     */
    final public void getGreens(byte g[]) {
        for (int i = 0; i < map_size; i++) {
            g[i] = (byte) (rgb[i] >> 8);
        }
    }

    /**
     * Copies the array of blue color components into the specified array.  
     * Only the initial entries of the array as specified by 
     * <code>getMapSize</code> are written.
     * @param b the specified array into which the elements of the 
     *      array of blue color components are copied 
     */
    final public void getBlues(byte b[]) {
        for (int i = 0; i < map_size; i++) {
            b[i] = (byte) rgb[i];
        }
    }

    /**
     * Copies the array of alpha transparency components into the 
     * specified array.  Only the initial entries of the array as specified 
     * by <code>getMapSize</code> are written.
     * @param a the specified array into which the elements of the 
     *      array of alpha components are copied 
     */
    final public void getAlphas(byte a[]) {
        for (int i = 0; i < map_size; i++) {
            a[i] = (byte) (rgb[i] >> 24);
        }
    }


    /**
     * Disposes of system resources associated with this
     * <code>ColorModel</code> once this <code>ColorModel</code> is no
     * longer referenced.
     */    
    public void finalize() {
    }

    private void setTransparentPixel(int trans) {
        if (trans >= 0 && trans < map_size) {
            rgb[trans] &= 0x00ffffff;
            transparent_index = trans;
            allgrayopaque = false;
            if (this.transparency == OPAQUE) {
                setTransparency(BITMASK);
            }
        }
    }

    private void setTransparency(int transparency) {
        if (this.transparency != transparency) {
            this.transparency = transparency;
            if (transparency == OPAQUE) {
                supportsAlpha = false;
                numComponents = 3;
                nBits = opaqueBits;
            } else {
                supportsAlpha = true;
                numComponents = 4;
                nBits = alphaBits;
            }
        }
    }

    /**
     * Returns the red color component for the specified pixel, scaled
     * from 0 to 255 in the default RGB ColorSpace, sRGB.  The pixel value
     * is specified as an int.  The returned value is a
     * non pre-multiplied value.
     * @param pixel the specified pixel 
     * @return the value of the red color component for the specified pixel
     */
    final public int getRed(int pixel) {
        return (rgb[pixel] >> 16) & 0xff;
    }

    /**
     * Returns the green color component for the specified pixel, scaled
     * from 0 to 255 in the default RGB ColorSpace, sRGB.  The pixel value
     * is specified as an int.  The returned value is a
     * non pre-multiplied value.
     * @param pixel the specified pixel 
     * @return the value of the green color component for the specified pixel
     */
    final public int getGreen(int pixel) {
        return (rgb[pixel] >> 8) & 0xff;
    }

    /**
     * Returns the blue color component for the specified pixel, scaled
     * from 0 to 255 in the default RGB ColorSpace, sRGB.  The pixel value
     * is specified as an int.  The returned value is a
     * non pre-multiplied value.
     * @param pixel the specified pixel 
     * @return the value of the blue color component for the specified pixel
     */
    final public int getBlue(int pixel) {
        return rgb[pixel] & 0xff;
    }

    /**
     * Returns the alpha component for the specified pixel, scaled
     * from 0 to 255.  The pixel value is specified as an int.
     * @param pixel the specified pixel 
     * @return the value of the alpha component for the specified pixel
     */
    final public int getAlpha(int pixel) {
        return (rgb[pixel] >> 24) & 0xff;
    }

    /**
     * Returns the color/alpha components of the pixel in the default
     * RGB color model format.  The pixel value is specified as an int.
     * The returned value is in a non pre-multiplied format.
     * @param pixel the specified pixel 
     * @return the color and alpha components of the specified pixel
     * @see ColorModel#getRGBdefault
     */
    final public int getRGB(int pixel) {
        return rgb[pixel];
    }

    /**
     * Returns the <code>String</code> representation of the contents of
     * this <code>ColorModel</code>object.
     * @return a <code>String</code> representing the contents of this
     * <code>ColorModel</code> object.
     */
    public String toString() {
        return new String("IndexColorModel: #pixelBits = "+pixel_bits
                          + " numComponents = "+numComponents
                          + " color space = "+colorSpace
                          + " transparency = "+transparency
                          + " transIndex   = "+transparent_index
                          + " has alpha = "+supportsAlpha
                          + " isAlphaPre = "+isAlphaPremultiplied
                          );
    }
}
