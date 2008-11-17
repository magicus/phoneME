/*
 * @(#)DirectColorModel.java	1.28 06/10/10
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

import java.awt.AWTException;
import java.awt.Transparency;
import java.awt.color.ColorSpace;

/**
 * A ColorModel class that specifies a translation from pixel values
 * to alpha, red, green, and blue color components for pixels which
 * have the color components embedded directly in the bits of the
 * pixel itself.  This color model is similar to an X11 TrueColor
 * visual.

 * <p>Many of the methods in this class are final. This is because the
 * underlying native graphics code makes  assumptions about the layout
 * and operation of this class  and those assumptions are reflected in
 * the implementations of the methods here that are marked final.  You
 * can subclass this class  for other reaons,  but you cannot override
 * or modify the behaviour of those methods.
 *
 * @see ColorModel
 *
 * @version	1.24 08/21/02
 * @author 	Jim Graham
 */
public class DirectColorModel extends ColorModel {
    private int red_mask;
    private int green_mask;
    private int blue_mask;
    private int alpha_mask;
    private int red_offset;
    private int green_offset;
    private int blue_offset;
    private int alpha_offset;
    private int red_scale;
    private int green_scale;
    private int blue_scale;
    private int alpha_scale;
    /**
     * Constructs a DirectColorModel from the given masks specifying
     * which bits in the pixel contain the red, green and blue color
     * components.  Pixels described by this color model will all
     * have alpha components of 255 (fully opaque).  All of the bits
     * in each mask must be contiguous and fit in the specified number
     * of least significant bits of the integer.
     */
    public DirectColorModel(int bits,
        int rmask, int gmask, int bmask) {
        this(bits, rmask, gmask, bmask, 0);
    }

    /**
     * Constructs a DirectColorModel from the given masks specifying
     * which bits in the pixel contain the alhpa, red, green and blue
     * color components.  All of the bits in each mask must be contiguous
     * and fit in the specified number of least significant bits of the
     * integer.
     */
    public DirectColorModel(int bits,
        int rmask, int gmask, int bmask, int amask) {
        super(bits);
        nBits = createBitsArray(rmask, gmask, bmask, amask);
        red_mask = rmask;
        green_mask = gmask;
        blue_mask = bmask;
        alpha_mask = amask;
        calculateOffsets();
        /* 4709812
         * Set transparency to OPAQUE if amask is 0.
         */
        if (amask == 0) {
            supportsAlpha = false;
            transparency = Transparency.OPAQUE;
        }
    }

    /**
     * Constructs a <code>DirectColorModel</code> from the specified
     * parameters.  Color components are in the specified
     * <code>ColorSpace</code>, which must be of type ColorSpace.TYPE_RGB.
     * The masks specify which bits in an <code>int</code> pixel
     * representation contain the red, green and blue color samples and
     * the alpha sample, if present.  If <code>amask</code> is 0, pixel
     * values do not contain alpha information and all pixels are treated
     * as opaque, which means that alpha&nbsp;=&nbsp;1.0.  All of the
     * bits in each mask must be contiguous and fit in the specified number
     * of least significant bits of an <code>int</code> pixel
     * representation.  If there is alpha, the <code>boolean</code>
     * <code>isAlphaPremultiplied</code> specifies how to interpret
     * color and alpha samples in pixel values.  If the <code>boolean</code>
     * is <code>true</code>, color samples are assumed to have been
     * multiplied by the alpha sample.  The transparency value is
     * Transparency.OPAQUE, if no alpha is present, or
     * Transparency.TRANSLUCENT otherwise.  The transfer type
     * is the type of primitive array used to represent pixel values and
     * must be one of DataBuffer.TYPE_BYTE, DataBuffer.TYPE_USHORT, or
     * DataBuffer.TYPE_INT.
     * @param space the specified <code>ColorSpace</code>
     * @param bits the number of bits in the pixel values; for example,
     *         the sum of the number of bits in the masks.
     * @param rmask specifies a mask indicating which bits in an
     *         integer pixel contain the red component
     * @param gmask specifies a mask indicating which bits in an
     *         integer pixel contain the green component
     * @param bmask specifies a mask indicating which bits in an
     *         integer pixel contain the blue component
     * @param amask specifies a mask indicating which bits in an
     * 	       integer pixel contain the alpha component
     * @param isAlphaPremultiplied <code>true</code> if color samples are
     *        premultiplied by the alpha sample; <code>false</code> otherwise
     * @param transferType the type of array used to represent pixel values
     */
    public DirectColorModel(ColorSpace space, int bits, int rmask,
        int gmask, int bmask, int amask,
        boolean isAlphaPremultiplied,
        int transferType) {
        super(bits,
            createBitsArray(rmask, gmask, bmask, amask),
            space,
            amask == 0 ? false : true,
            isAlphaPremultiplied,
            amask == 0 ? Transparency.OPAQUE : Transparency.TRANSLUCENT,
            transferType);
        red_mask = rmask;
        green_mask = gmask;
        blue_mask = bmask;
        alpha_mask = amask;
        calculateOffsets();
    }

    private final static int[] createBitsArray(int rmask, int gmask, int bmask,
        int amask) {
        int[] arr = new int[3 + (amask == 0 ? 0 : 1)];
        arr[0] = countBits(rmask);
        arr[1] = countBits(gmask);
        arr[2] = countBits(bmask);
        if (arr[0] < 0) {
            throw new IllegalArgumentException("Noncontiguous red mask ("
                    + Integer.toHexString(rmask));
        } else if (arr[1] < 0) {
            throw new IllegalArgumentException("Noncontiguous green mask ("
                    + Integer.toHexString(gmask));
        } else if (arr[2] < 0) {
            throw new IllegalArgumentException("Noncontiguous blue mask ("
                    + Integer.toHexString(bmask));
        }
        if (amask != 0) {
            arr[3] = countBits(amask);
            if (arr[3] < 0) {
                throw new IllegalArgumentException("Noncontiguous alpha mask ("
                        + Integer.toHexString(amask));
            }
        }
        return arr;
    }

    private final static int countBits(int mask) {
        int saveMask = mask;
        int count = 0;
        if (mask != 0) {
            while ((mask & 1) == 0) {
                mask >>>= 1;
            }
            while ((mask & 1) == 1) {
                mask >>>= 1;
                count++;
            }
        }
        if (mask != 0) {
            return -1;
        }
        return count;
    }

    /**
     * Returns the mask indicating which bits in a pixel contain the red
     * color component.
     */
    final public int getRedMask() {
        return red_mask;
    }

    /**
     * Returns the mask indicating which bits in a pixel contain the green
     * color component.
     */
    final public int getGreenMask() {
        return green_mask;
    }

    /**
     * Returns the mask indicating which bits in a pixel contain the blue
     * color component.
     */
    final public int getBlueMask() {
        return blue_mask;
    }

    /**
     * Returns the mask indicating which bits in a pixel contain the alpha
     * transparency component.
     */
    final public int getAlphaMask() {
        return alpha_mask;
    }
    private int accum_mask = 0;
    /*
     * A utility function to decompose a single mask and verify that it
     * fits in the specified pixel size, and that it does not overlap any
     * other color component.  The values necessary to decompose and
     * manipulate pixels are calculated as a side effect.
     */
    private void decomposeMask(int mask, String componentName, int values[]) {
        if ((mask & accum_mask) != 0) {
            throw new IllegalArgumentException(componentName + " mask bits not unique");
        }
        int off = 0;
        int count = 0;
        if (mask != 0) {
            while ((mask & 1) == 0) {
                mask >>>= 1;
                off++;
            }
            while ((mask & 1) == 1) {
                mask >>>= 1;
                count++;
            }
        }
        if (mask != 0) {
            throw new IllegalArgumentException(componentName + " mask bits not contiguous");
        }
        if (off + count > pixel_bits) {
            throw new IllegalArgumentException(componentName + " mask overflows pixel");
        }
        int scale;
        if (count < 8) {
            scale = (1 << count) - 1;
        } else {
            scale = 0;
            if (count > 8) {
                off += (count - 8);
            }
        }
        values[0] = off;
        values[1] = scale;
    }

    /*
     * A utility function to verify all of the masks and to store
     * the auxilliary values needed to manipulate the pixels.
     */
    private void calculateOffsets() {
        int values[] = new int[2];
        decomposeMask(red_mask, "red", values);
        red_offset = values[0];
        red_scale = values[1];
        decomposeMask(green_mask, "green", values);
        green_offset = values[0];
        green_scale = values[1];
        decomposeMask(blue_mask, "blue", values);
        blue_offset = values[0];
        blue_scale = values[1];
        decomposeMask(alpha_mask, "alpha", values);
        alpha_offset = values[0];
        alpha_scale = values[1];
    }

    /**
     * Returns the red color compoment for the specified pixel in the
     * range 0-255.
     */
    final public int getRed(int pixel) {
        int r = ((pixel & red_mask) >>> red_offset);
        if (red_scale != 0) {
            r = r * 255 / red_scale;
        }
        if (isAlphaPremultiplied) {
            int alpha = getAlpha(pixel);
            if (alpha == 0)
                return 0;
            r *= 255;
            r /= alpha;
            r = Math.min(255, r);
        }
        return r;
    }

    /**
     * Returns the green color compoment for the specified pixel in the
     * range 0-255.
     */
    final public int getGreen(int pixel) {
        int g = ((pixel & green_mask) >>> green_offset);
        if (green_scale != 0) {
            g = g * 255 / green_scale;
        }
        if (isAlphaPremultiplied) {
            int alpha = getAlpha(pixel);
            if (alpha == 0)
                return 0;
            g *= 255;
            g /= alpha;
            g = Math.min(255, g);
        }
        return g;
    }

    /**
     * Returns the blue color compoment for the specified pixel in the
     * range 0-255.
     */
    final public int getBlue(int pixel) {
        int b = ((pixel & blue_mask) >>> blue_offset);
        if (blue_scale != 0) {
            b = b * 255 / blue_scale;
        }
        if (isAlphaPremultiplied) {
            int alpha = getAlpha(pixel);
            if (alpha == 0)
                return 0;
            b *= 255;
            b /= alpha;
            b = Math.min(255, b);
        }
        return b;
    }

    /**
     * Return the alpha transparency value for the specified pixel in the
     * range 0-255.
     */
    final public int getAlpha(int pixel) {
        if (alpha_mask == 0) return 255;
        int a = ((pixel & alpha_mask) >>> alpha_offset);
        if (alpha_scale != 0) {
            a = a * 255 / alpha_scale;
        }
        return a;
    }

    /**
     * Returns the color of the pixel in the default RGB color model.
     * @see ColorModel#getRGBdefault
     */
    final public int getRGB(int pixel) {
        return (getAlpha(pixel) << 24)
            | (getRed(pixel) << 16)
            | (getGreen(pixel) << 8)
            | (getBlue(pixel) << 0);
    }
}
