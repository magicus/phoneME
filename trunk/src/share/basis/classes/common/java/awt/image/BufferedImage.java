/*
 * @(#)BufferedImage.java	1.21 06/10/10
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

import java.awt.Toolkit;
import java.util.Hashtable;
import java.util.Vector;
import sun.awt.image.BufferedImagePeer;
import java.awt.GraphicsEnvironment;
import java.awt.Graphics2D;

/**
 *
 * The <code>BufferedImage</code> subclass describes an {@link Image} with
 * an accessible buffer of image data.
 * A <code>BufferedImage</code> is comprised of a {@link ColorModel} and a
 * {@link Raster} of image data.
 * The number and types of bands in the {@link SampleModel} of the
 * <code>Raster</code> must match the number and types required by the
 * <code>ColorModel</code> to represent its color and alpha components.
 * All <code>BufferedImage</code> objects have an upper left corner
 * coordinate of (0,&nbsp;0).  Any <code>Raster</code> used to construct a
 * <code>BufferedImage</code> must therefore have minX=0 and minY=0.
 * @see ColorModel
 * @see Raster
 * @see WritableRaster
 * @version 10 Feb 1997
 */

public class BufferedImage extends java.awt.Image //                           implements WritableRenderedImage
{
    //int        imageType = TYPE_CUSTOM;

    /** The peer used to implement BufferedImage. */

    private transient BufferedImagePeer peer;
    // ### Serialization difference
    //  WritableRaster raster;

    // ### Serialization difference
    // OffScreenImageSource osis;

    //    Hashtable properties;

    //    boolean    isAlphaPremultiplied;// If true, alpha has been premultiplied in
    // color channels

    /**
     * Image Type Constants
     */

    /**
     * Image type is not recognized so it must be a customized
     * image.  This type is only used as a return value for the getType()
     * method.
     */
    public static final int TYPE_CUSTOM = 0;
    /**
     * Represents an image with 8-bit RGB color components packed into
     * integer pixels.  The image has a {@link DirectColorModel} without
     * alpha.
     */
    public static final int TYPE_INT_RGB = 1;
    /**
     * Represents an image with 8-bit RGBA color components packed into
     * integer pixels.  The image has a <code>DirectColorModel</code>
     * with alpha. The color data in this image is considered not to be
     * premultiplied with alpha.  When this type is used as the
     * <code>imageType</code> argument to a <code>BufferedImage</code>
     * constructor, the created image is consistent with images
     * created in the JDK1.1 and earlier releases.
     */
    public static final int TYPE_INT_ARGB = 2;
    /**
     * Represents an image with 8-bit RGBA color components packed into
     * integer pixels.  The image has a <code>DirectColorModel</code>
     * with alpha.  The color data in this image is considered to be
     * premultiplied with alpha.
     */
    public static final int TYPE_INT_ARGB_PRE = 3;
    /**
     * Represents an image with 8-bit RGB color components, corresponding
     * to a Windows- or Solaris- style BGR color model, with the colors
     * Blue, Green, and Red packed into integer pixels.  There is no alpha.
     * The image has a {@link ComponentColorModel}.
     */
    public static final int TYPE_INT_BGR = 4;
    /**
     * Represents an image with 8-bit RGB color components, corresponding
     * to a Windows-style BGR color model) with the colors Blue, Green,
     * and Red stored in 3 bytes.  There is no alpha.  The image has a
     * <code>ComponentColorModel</code>.
     */
    //    public static final int TYPE_3BYTE_BGR = 5;

    /**
     * Represents an image with 8-bit RGBA color components with the colors
     * Blue, Green, and Red stored in 3 bytes and 1 byte of alpha.  The
     * image has a <code>ComponentColorModel</code> with alpha.  The
     * color data in this image is considered not to be premultiplied with
     * alpha.  The byte data is interleaved in a single
     * byte array in the order A, B, G, R
     * from lower to higher byte addresses within each pixel.
     */
    //    public static final int TYPE_4BYTE_ABGR = 6;

    /**
     * Represents an image with 8-bit RGBA color components with the colors
     * Blue, Green, and Red stored in 3 bytes and 1 byte of alpha.  The
     * image has a <code>ComponentColorModel</code> with alpha. The color
     * data in this image is considered to be premultiplied with alpha.
     * The byte data is interleaved in a single byte array in the order
     * A, B, G, R from lower to higher byte addresses within each pixel.
     */
    //    public static final int TYPE_4BYTE_ABGR_PRE = 7;

    /**
     * Represents an image with 5-6-5 RGB color components (5-bits red,
     * 6-bits green, 5-bits blue) with no alpha.  This image has
     * a <code>DirectColorModel</code>.
     */
    public static final int TYPE_USHORT_565_RGB = 8;
    /**
     * Represents an image with 5-5-5 RGB color components (5-bits red,
     * 5-bits green, 5-bits blue) with no alpha.  This image has
     * a <code>DirectColorModel</code>.
     */
    public static final int TYPE_USHORT_555_RGB = 9;
    /**
     * Represents a unsigned byte grayscale image, non-indexed.  This
     * image has a <code>ComponentColorModel</code> with a CS_GRAY
     * {@link ColorSpace}.
     */
    //    public static final int TYPE_BYTE_GRAY = 10;

    /**
     * Represents an unsigned short grayscale image, non-indexed).  This
     * image has a <code>ComponentColorModel</code> with a CS_GRAY
     * <code>ColorSpace</code>.
     */
    //    public static final int TYPE_USHORT_GRAY = 11;

    /**
     * Represents an opaque byte-packed binary image.  The
     * image has an {@link IndexColorModel} without alpha.  When this
     * type is used as the <code>imageType</code> argument to the
     * <code>BufferedImage</code> constructor that takes an
     * <code>imageType</code> argument but no <code>ColorModel</code>
     * argument, an <code>IndexColorModel</code> is created with
     * two colors in the default sRGB <code>ColorSpace</code>:
     * {0,&nbsp;0,&nbsp;0} and {255,&nbsp;255,&nbsp;255}.
     */
    public static final int TYPE_BYTE_BINARY = 12;
    /**
     * Represents an indexed byte image.  When this type is used as the
     * <code>imageType</code> argument to the <code>BufferedImage</code>
     * constructor that takes an <code>imageType</code> argument
     * but no <code>ColorModel</code> argument, an
     * <code>IndexColorModel</code> is created with
     * a 256-color 6/6/6 color cube palette with the rest of the colors
     * from 216-255 populated by grayscale values in the
     * default sRGB ColorSpace.
     */
    public static final int TYPE_BYTE_INDEXED = 13;
    //    private static final int DCM_RED_MASK   = 0x00ff0000;
    //    private static final int DCM_GREEN_MASK = 0x0000ff00;
    //    private static final int DCM_BLUE_MASK  = 0x000000ff;
    //    private static final int DCM_ALPHA_MASK = 0xff000000;
    //    private static final int DCM_565_RED_MASK = 0xf800;
    //    private static final int DCM_565_GRN_MASK = 0x07E0;
    //    private static final int DCM_565_BLU_MASK = 0x001F;
    //    private static final int DCM_555_RED_MASK = 0x7C00;
    //    private static final int DCM_555_GRN_MASK = 0x03E0;
    //    private static final int DCM_555_BLU_MASK = 0x001F;
    //    private static final int DCM_BGR_RED_MASK = 0x0000ff;
    //    private static final int DCM_BGR_GRN_MASK = 0x00ff00;
    //    private static final int DCM_BGR_BLU_MASK = 0xff0000;


    /**
     * Constructs a <code>BufferedImage</code> of one of the predefined
     * image types.  The <code>ColorSpace</code> for the image is the
     * default sRGB space.
     * @param width     width of the created image
     * @param height    height of the created image
     * @param imageType type of the created image
     * @see ColorSpace
     * @see #TYPE_INT_RGB
     * @see #TYPE_INT_ARGB
     * @see #TYPE_INT_ARGB_PRE
     * @see #TYPE_INT_BGR
     * @see #TYPE_3BYTE_BGR
     * @see #TYPE_4BYTE_ABGR
     * @see #TYPE_4BYTE_ABGR_PRE
     * @see #TYPE_BYTE_GRAY
     * @see #TYPE_USHORT_GRAY
     * @see #TYPE_BYTE_BINARY
     * @see #TYPE_BYTE_INDEXED
     * @see #TYPE_USHORT_565_RGB
     * @see #TYPE_USHORT_555_RGB
     */

    /*
     BufferedImage(int width, int height, int imageType) {



     }
     */

    /**
     * Constructs a <code>BufferedImage</code> of one of the predefined
     * image types:
     * TYPE_BYTE_BINARY or TYPE_BYTE_INDEXED
     * @param width     width of the created image
     * @param height    height of the created image
     * @param imageType type of the created image
     * @param cm        <code>IndexColorModel</code> of the created image
     * @throws IllegalArgumentException   if the imageType is not
     * TYPE_BYTE_BINARY or TYPE_BYTE_INDEXED
     * @see #TYPE_BYTE_BINARY
     * @see #TYPE_BYTE_INDEXED
     */

    /*
     public BufferedImage (int width,
     int height,
     int imageType,
     IndexColorModel cm) {
     }
     */

    /**
     * Constructs a new <code>BufferedImage</code> with a specified
     * <code>ColorModel</code> and <code>Raster</code>.  If the number and
     * types of bands in the <code>SampleModel</code> of the
     * <code>Raster</code> do not match the number and types required by
     * the <code>ColorModel</code> to represent its color and alpha
     * components, a {@link RasterFormatException} is thrown.  This
     * method can multiply or divide the color <code>Raster</code> data by
     * alpha to match the <code>alphaPremultiplied</code> state
     * in the <code>ColorModel</code>.  Properties for this
     * <code>BufferedImage</code> can be established by passing
     * in a {@link Hashtable} of <code>String</code>/<code>Object</code>
     * pairs.
     * @param ColorModel <code>ColorModel</code> for the new image
     * @param raster     <code>Raster</code> for the image data
     * @param isRasterPremultiplied   if <code>true</code>, the data in
     *                  the raster has been premultiplied with alpha.
     * @param properties <code>Hashtable</code> of
     *                  <code>String</code>/<code>Object</code> pairs.
     * @exception <code>RasterFormatException</code> if the number and
     * types of bands in the <code>SampleModel</code> of the
     * <code>Raster</code> do not match the number and types required by
     * the <code>ColorModel</code> to represent its color and alpha
     * components.
     * @exception <code>IllegalArgumentException</code> if
     *		<code>raster</code> is incompatible with <code>cm</code>
     * @see ColorModel
     * @see Raster
     * @see WritableRaster
     */


    /*
     *
     *  FOR NOW THE CODE WHICH DEFINES THE RASTER TYPE IS DUPLICATED BY DVF
     *  SEE THE METHOD DEFINERASTERTYPE @ RASTEROUTPUTMANAGER
     *
     */

    /*
     public BufferedImage (ColorModel cm,
     WritableRaster raster,
     boolean isRasterPremultiplied,
     Hashtable properties) {

     }
     */

    /** Constructs a new BufferedImage using <code>peer</code> for delegating its imlementation to.
     This constructor is private to ensure that it is not possible to create BufferedImages using
     constructors in basis or personal profile. It also prevents sub classing of BufferedImage.
     Implementors should implement the BufferedImagePeer interface and call this constructor,
     using JNI for example, from their implementation of <code>GraphicsConfiguration.createCompatiableImage()</code>
     method. */

    private BufferedImage (BufferedImagePeer peer) {
        this.peer = peer;
    }

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


    public int getType() {
        return peer.getType();
    }

    /**
     * Returns the <code>ColorModel</code>.
     * @return the <code>ColorModel</code> of this
     *  <code>BufferedImage</code>.
     */
    public ColorModel getColorModel() {
        return peer.getColorModel();
    }

    /**
     * Returns the {@link WritableRaster}.
     * @return the <code>WriteableRaster</code> of this
     *  <code>BufferedImage</code>.
     */

    /*
     public WritableRaster getRaster() {
     return raster;
     }
     */

    /**
     * Creates a <code>Graphics2D</code>, which can be used to draw into
     * this <code>BufferedImage</code>.
     * @return a <code>Graphics2D</code>, used for drawing into this
     *          image.
     */
    public Graphics2D createGraphics() {
        return (Graphics2D) peer.getGraphics();
    }

    /**
     * Returns a <code>WritableRaster</code> representing the alpha
     * channel for <code>BufferedImage</code> objects
     * with <code>ColorModel</code> objects that support a separate
     * spatial alpha channel, such as <code>ComponentColorModel</code> and
     * <code>DirectColorModel</code>.  Returns <code>null</code> if there
     * is no alpha channel associated with the <code>ColorModel</code> in
     * this image.  This method assumes that for all
     * <code>ColorModel</code> objects other than
     * <code>IndexColorModel</code>, if the <code>ColorModel</code>
     * supports alpha, there is a separate alpha channel
     * which is stored as the last band of image data.
     * If the image uses an <code>IndexColorModel</code> that
     * has alpha in the lookup table, this method returns
     * <code>null</code> since there is no spatially discrete alpha
     * channel.  This method creates a new
     * <code>WritableRaster</code>, but shares the data array.
     * @return a <code>WritableRaster</code> or <code>null</code> if this
     *          <code>BufferedImage</code> has no alpha channel associated
     *          with its <code>ColorModel</code>.
     */

    /*
     public WritableRaster getAlphaRaster() {
     return null;
     }
     */

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
    public int getRGB(int x, int y) {
        return peer.getRGB(x, y);
    }

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
    public int[] getRGB(int startX, int startY, int w, int h,
        int[] rgbArray, int offset, int scansize) {
        return peer.getRGB(startX, startY, w, h, rgbArray, offset, scansize);
    }

    /**
     * Sets a pixel in this <code>BufferedImage</code> to the specified
     * RGB value. The pixel is assumed to be in the default RGB color
     * model, TYPE_INT_ARGB, and default sRGB color space.  For images
     * with an <code>IndexColorModel</code>, the index with the nearest
     * color is chosen.
     * @param x,&nbsp;y the coordinates of the pixel to set
     * @param rgb the RGB value
     */
    public synchronized void setRGB(int x, int y, int rgb) {
        peer.setRGB(x, y, rgb);
    }

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
    public void setRGB(int startX, int startY, int w, int h,
        int[] rgbArray, int offset, int scansize) {
        peer.setRGB(startX, startY, w, h, rgbArray, offset, scansize);
    }

    /**
     * Returns the width of the <code>BufferedImage</code>.
     * @return the width of this <code>BufferedImage</code>.
     */
    public int getWidth() {
        return peer.getWidth();
    }

    /**
     * Returns the height of the <code>BufferedImage</code>.
     * @return the height of this <code>BufferedImage</code>.
     */
    public int getHeight() {
        return peer.getHeight();
    }

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
    public int getWidth(ImageObserver observer) {
        return peer.getWidth(observer);
    }

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
    public int getHeight(ImageObserver observer) {
        return peer.getHeight(observer);
    }

    /**
     * Returns the object that produces the pixels for the image.
     * @return the {@link ImageProducer} that is used to produce the
     * pixels for this image.
     * @see ImageProducer
     */
    public ImageProducer getSource() {
        return peer.getSource();
    }

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
    public Object getProperty(String name, ImageObserver observer) {
        return peer.getProperty(name, observer);
    }

    /**
     * Returns a property of the image by name.
     * @param name the property name
     * @return an <code>Object</code> that is the property referred to by
     *          the specified <code>name</code>.
     */
    public Object getProperty(String name) {
        return peer.getProperty(name);
    }

    /**
     * Flushes all resources being used to cache optimization information.
     * The underlying pixel data is unaffected.
     */
    public void flush() {
        peer.flush();
    }

    /**
     * This method returns a {@link Graphics2D}, but is here
     * for backwards compatibility.  {@link #createGraphics() createGraphics} is more
     * convenient, since it is declared to return a
     * <code>Graphics2D</code>.
     * @return a <code>Graphics2D</code>, which can be used to draw into
     *          this image.
     */
    public java.awt.Graphics getGraphics() {
        return peer.getGraphics();
    }

    /**
     * Creates a <code>Graphics2D</code>, which can be used to draw into
     * this <code>BufferedImage</code>.
     * @return a <code>Graphics2D</code>, used for drawing into this
     *          image.
     */

    /*
     public Graphics2D createGraphics() {
     }
     */

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
    public BufferedImage getSubimage(int x, int y, int w, int h) {
        return peer.getSubimage(x, y, w, h);
    }

    /**
     * Returns a <code>String</code> representation of this
     * <code>BufferedImage</code> object and its values.
     * @return a <code>String</code> representing this
     *          <code>BufferedImage</code>.
     */
    public String toString() {
        return peer.toString();
    }

    /**
     * Returns a {@link Vector} of {@link RenderedImage} objects that are
     * the immediate sources, not the sources of these immediate sources,
     * of image data for this <code>BufferedImage</code>.  This
     * method returns <code>null</code> if the <code>BufferedImage</code>
     * has no information about its immediate sources.  It returns an
     * empty <code>Vector</code> if the <code>BufferedImage</code> has no
     * immediate sources.
     * @return a <code>Vector</code> containing immediate sources of
     *          this <code>BufferedImage</code> object's image date, or
     *          <code>null</code> if this <code>BufferedImage</code> has
     *          no information about its immediate sources, or an empty
     *          <code>Vector</code> if this <code>BufferedImage</code>
     *          has no immediate sources.
     */

    /*
     public Vector getSources() {
     return null;
     }
     */

    /**
     * Returns an array of names recognized by
     * {@link #getProperty(String) getProperty(String)}
     * or <code>null</code>, if no property names are recognized.
     * @return a <code>String</code> array containing all of the property
     *          names that <code>getProperty(String)</code> recognizes;
     *		or <code>null</code> if no property names are recognized.
     */
    public String[] getPropertyNames() {
        return peer.getPropertyNames();
    }
    /**
     * Returns the minimum x coordinate of this
     * <code>BufferedImage</code>.  This is always zero.
     * @return the minimum x coordinate of this
     *          <code>BufferedImage</code>.
     */

    /*    public int getMinX() {
     return 0;
     }
     */

    /**
     * Returns the minimum y coordinate of this
     * <code>BufferedImage</code>.  This is always zero.
     * @return the minimum y coordinate of this
     *          <code>BufferedImage</code>.
     */

    /*    public int getMinY() {
     return 0;
     }
     */
}
