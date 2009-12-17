/*
 * @(#)BufferedImagePeer.java	1.12 06/10/10
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

package sun.awt.image;

import java.awt.image.ColorModel;
import java.awt.image.ImageObserver;
import java.awt.image.ImageProducer;
import java.awt.image.BufferedImage;
import java.awt.Graphics;

/**	Provides a peer for the implementation of BufferedImage. In basis and personal
 profile, BufferedImage is a very much stripped down version of its J2SE couterpart.
 In effect, it only allows the getting and setting of RGBs and cannot be constructed
 with any public constructors. Instead, it can only be constructed by the
 <code>GraphicsConfiguration.createCompatiableImage</create> method. The result is
 that BufferedImage in basis and personal is abstract and final and an instance can only
 be created via the factory method. This has the advantage of reducing the graphics
 formats to be supported by BufferedImage to the formats of the native graphics library.
 To implement BufferedImage this interface is used to provide an interface to the implemntation.
 BufferedImage delegates its method calls to an instance of this interface.

 @author Nicholas Allen */

public interface BufferedImagePeer {
    /**
     * Returns the image type.  If it is not one of the known types,
     * TYPE_CUSTOM is returned.
     * @return the image type of this <code>BufferedImage</code>.
     * @see #TYPE_INT_RGB
     * @see #TYPE_INT_ARGB
     * @see #TYPE_INT_ARGB_PRE
     * @see #TYPE_INT_BGR
     * @see #TYPE_3BYTE_BGR
     * @see #TYPE_4BYTE_ABGR
     * @see #TYPE_4BYTE_ABGR_PRE
     * @see #TYPE_BYTE_GRAY
     * @see #TYPE_BYTE_BINARY
     * @see #TYPE_BYTE_INDEXED
     * @see #TYPE_USHORT_GRAY
     * @see #TYPE_USHORT_565_RGB
     * @see #TYPE_USHORT_555_RGB
     * @see #TYPE_CUSTOM
     */

    int getType();
    /**
     * Returns the <code>ColorModel</code>.
     * @return the <code>ColorModel</code> of this
     *  <code>BufferedImage</code>.
     */
    ColorModel getColorModel();
    /**
     * Returns an integer pixel in the default RGB color model
     * (TYPE_INT_ARGB) and default sRGB colorspace.  Color
     * conversion takes place if this default model does not match
     * the image <code>ColorModel</code>.  There are only 8-bits of
     * precision for each color component in the returned data when using
     * this method.
     * @param x,&nbsp;y the coordinates of the pixel from which to get
     *          the pixel in the default RGB color model and sRGB
     *          color space
     * @return an integer pixel in the default RGB color model and
     *          default sRGB colorspace.
     */
    int getRGB(int x, int y);
    /**
     * Returns an array of integer pixels in the default RGB color model
     * (TYPE_INT_ARGB) and default sRGB color space,
     * from a portion of the image data.  Color conversion takes
     * place if the default model does not match the image
     * <code>ColorModel</code>.  There are only 8-bits of precision for
     * each color component in the returned data when
     * using this method.  With a specified coordinate (x,&nbsp;y) in the
     * image, the ARGB pixel can be accessed in this way:
     * <pre>
     *    pixel   = rgbArray[offset + (y-startY)*scansize + (x-startX)];
     * </pre>
     * @param startX,&nbsp; startY the starting coordinates
     * @param w           width of region
     * @param h           height of region
     * @param rgbArray    if not <code>null</code>, the rgb pixels are
     *          written here
     * @param offset      offset into the <code>rgbArray</code>
     * @param scansize    scanline stride for the <code>rgbArray</code>
     * @return            array of RGB pixels.
     * @exception <code>IllegalArgumentException</code> if an unknown
     *		datatype is specified
     */
    int[] getRGB(int startX, int startY, int w, int h,
        int[] rgbArray, int offset, int scansize);
    /**
     * Sets a pixel in this <code>BufferedImage</code> to the specified
     * RGB value. The pixel is assumed to be in the default RGB color
     * model, TYPE_INT_ARGB, and default sRGB color space.  For images
     * with an <code>IndexColorModel</code>, the index with the nearest
     * color is chosen.
     * @param x,&nbsp;y the coordinates of the pixel to set
     * @param rgb the RGB value
     */
    void setRGB(int x, int y, int rgb);
    /**
     * Sets an array of integer pixels in the default RGB color model
     * (TYPE_INT_ARGB) and default sRGB color space,
     * into a portion of the image data.  Color conversion takes place
     * if the default model does not match the image
     * <code>ColorModel</code>.  There are only 8-bits of precision for
     * each color component in the returned data when
     * using this method.  With a specified coordinate (x,&nbsp;y) in the
     * this image, the ARGB pixel can be accessed in this way:
     * <pre>
     *    pixel   = rgbArray[offset + (y-startY)*scansize + (x-startX)];
     * </pre>
     * WARNING: No dithering takes place.
     *
     * @param startX,&nbsp;startY the starting coordinates
     * @param w           width of the region
     * @param h           height of the region
     * @param rgbArray    the rgb pixels
     * @param offset      offset into the <code>rgbArray</code>
     * @param scansize    scanline stride for the <code>rgbArray</code>
     */
    void setRGB(int startX, int startY, int w, int h,
        int[] rgbArray, int offset, int scansize);
    /**
     * Returns the width of the <code>BufferedImage</code>.
     * @return the width of this <code>BufferedImage</code>.
     */
    int getWidth();
    /**
     * Returns the height of the <code>BufferedImage</code>.
     * @return the height of this <code>BufferedImage</code>.
     */
    int getHeight();
    /**
     * Returns the actual width of the image.  If the width is not known
     * yet then the {@link ImageObserver} is notified later and
     * <code>-1</code> is returned.
     * @param observer the <code>ImageObserver</code> that receives
     *          information about the image
     * @return the width of the image or <code>-1</code> if the width
     *          is not yet known.
     * @see java.awt.Image#getHeight(ImageObserver)
     * @see ImageObserver
     */
    int getWidth(ImageObserver observer);
    /**
     * Returns the actual height of the image.  If the height is not known
     * yet then the <code>ImageObserver</code> is notified later and
     * <code>-1</code> is returned.
     * @param observer the <code>ImageObserver</code> that receives
     *          information about the image
     * @return the height of the image or <code>-1</code> if the height
     *          is not yet known.
     * @see java.awt.Image#getWidth(ImageObserver)
     * @see ImageObserver
     */
    int getHeight(ImageObserver observer);
    /**
     * Returns the object that produces the pixels for the image.
     * @return the {@link ImageProducer} that is used to produce the
     * pixels for this image.
     * @see ImageProducer
     */
    ImageProducer getSource();
    /**
     * Returns a property of the image by name.  Individual property names
     * are defined by the various image formats.  If a property is not
     * defined for a particular image, this method returns the
     * <code>UndefinedProperty</code> field.  If the properties
     * for this image are not yet known, then this method returns
     * <code>null</code> and the <code>ImageObserver</code> object is
     * notified later.  The property name "comment" should be used to
     * store an optional comment that can be presented to the user as a
     * description of the image, its source, or its author.
     * @param name the property name
     * @param observer the <code>ImageObserver</code> that receives
     *  notification regarding image information
     * @return an {@link Object} that is the property referred to by the
     *          specified <code>name</code> or <code>null</code> if the
     *          properties of this image are not yet known.
     * @see ImageObserver
     * @see java.awt.Image#UndefinedProperty
     */
    Object getProperty(String name, ImageObserver observer);
    /**
     * Returns a property of the image by name.
     * @param name the property name
     * @return an <code>Object</code> that is the property referred to by
     *          the specified <code>name</code>.
     */
    Object getProperty(String name);
    /**
     * Flushes all resources being used to cache optimization information.
     * The underlying pixel data is unaffected.
     */
    void flush();
    /**
     * This method returns a {@link Graphics2D}, but is here
     * for backwards compatibility.  {@link #createGraphics() createGraphics} is more
     * convenient, since it is declared to return a
     * <code>Graphics2D</code>.
     * @return a <code>Graphics2D</code>, which can be used to draw into
     *          this image.
     */
    Graphics getGraphics();
    /**
     * Returns a subimage defined by a specified rectangular region.
     * The returned <code>BufferedImage</code> shares the same
     * data array as the original image.
     * @param x,&nbsp;y the coordinates of the upper-left corner of the
     *          specified rectangular region
     * @param w the width of the specified rectangular region
     * @param h the height of the specified rectangular region
     * @return a <code>BufferedImage</code> that is the subimage of this
     *          <code>BufferedImage</code>.
     * @exception <code>RasterFormatException</code> if the specified
     * area is not contained within this <code>BufferedImage</code>.
     */
    BufferedImage getSubimage(int x, int y, int w, int h);
    /**
     * Returns an array of names recognized by
     * {@link #getProperty(String) getProperty(String)}
     * or <code>null</code>, if no property names are recognized.
     * @return a <code>String</code> array containing all of the property
     *          names that <code>getProperty(String)</code> recognizes;
     *		or <code>null</code> if no property names are recognized.
     */
    String[] getPropertyNames();
}
