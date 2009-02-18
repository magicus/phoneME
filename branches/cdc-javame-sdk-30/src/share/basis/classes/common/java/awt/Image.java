/*
 * @(#)Image.java	1.35 06/10/10
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
package java.awt;

import java.awt.image.ImageProducer;
import java.awt.image.ImageObserver;
import java.awt.image.ImageFilter;
import java.awt.image.FilteredImageSource;
import java.awt.image.AreaAveragingScaleFilter;
import java.awt.image.ReplicateScaleFilter;

/**
 * The abstract class <code>Image</code> is the superclass of all 
 * classes that represent graphical images. The image must be 
 * obtained in a platform-specific manner.
 * <h3>Compatibility</h3>
 * The Image class in Personal Java supported GIF(CompuServ 89a),
 * JPEG(JFIF), and XBM(X Bitmap).  In addition to these, Personal
 * Profile supports the PNG image file format.  Images are created using
 * the <code>Toolkit.createImage()</code> method or by an imageProducer.
 *
 * @version 	1.31, 08/19/02
 * @author 	Sami Shaio
 * @author 	Arthur van Hoff
 * @see java.awt.Toolkit#createImage
 * @since       JDK1.0
 */
public abstract class Image {
    /**
     * Determines the width of the image. If the width is not yet known, 
     * this method returns <code>-1</code> and the specified   
     * <code>ImageObserver</code> object is notified later.
     * @param     observer   an object waiting for the image to be loaded.
     * @return    the width of this image, or <code>-1</code> 
     *                   if the width is not yet known.
     * @see       java.awt.Image#getHeight
     * @see       java.awt.image.ImageObserver
     * @since     JDK1.0
     */
    public abstract int getWidth(ImageObserver observer);

    /**
     * Determines the height of the image. If the height is not yet known, 
     * this method returns <code>-1</code> and the specified  
     * <code>ImageObserver</code> object is notified later.
     * @param     observer   an object waiting for the image to be loaded.
     * @return    the height of this image, or <code>-1</code> 
     *                   if the height is not yet known.
     * @see       java.awt.Image#getWidth
     * @see       java.awt.image.ImageObserver
     * @since     JDK1.0
     */
    public abstract int getHeight(ImageObserver observer);

    /**
     * Gets the object that produces the pixels for the image.
     * This method is called by the image filtering classes and by 
     * methods that perform image conversion and scaling.
     * @return     the image producer that produces the pixels 
     *                                  for this image.
     * @see        java.awt.image.ImageProducer
     */
    public abstract ImageProducer getSource();

    /**
     * Creates a graphics context for drawing to an off-screen image. 
     * This method can only be called for off-screen images. 
     * @return  a graphics context to draw to the off-screen image. 
     * @see     java.awt.Graphics
     * @see     java.awt.Component#createImage(int, int)
     * @since   JDK1.0
     */
    public abstract Graphics getGraphics();

    /**
     * Gets a property of this image by name. 
     * <p>
     * Individual property names are defined by the various image 
     * formats. If a property is not defined for a particular image, this 
     * method returns the <code>UndefinedProperty</code> object. 
     * <p>
     * If the properties for this image are not yet known, this method 
     * returns <code>null</code>, and the <code>ImageObserver</code> 
     * object is notified later. 
     * <p>
     * The property name <code>"comment"</code> should be used to store 
     * an optional comment which can be presented to the application as a 
     * description of the image, its source, or its author. 
     * @param       name   a property name.
     * @param       observer   an object waiting for this image to be loaded.
     * @return      the value of the named property.
     * @see         java.awt.image.ImageObserver
     * @see         java.awt.Image#UndefinedProperty
     * @since       JDK1.0
     */
    public abstract Object getProperty(String name, ImageObserver observer);
    /**
     * The <code>UndefinedProperty</code> object should be returned whenever a
     * property which was not defined for a particular image is fetched.
     * @since    JDK1.0
     */
    public static final Object UndefinedProperty = new Object();
    /**
     * Creates a scaled version of this image.
     * A new <code>Image</code> object is returned which will render 
     * the image at the specified <code>width</code> and 
     * <code>height</code> by default.  The new <code>Image</code> object
     * may be loaded asynchronously even if the original source image
     * has already been loaded completely.  If either the <code>width</code> 
     * or <code>height</code> is a negative number then a value is 
     * substituted to maintain the aspect ratio of the original image 
     * dimensions.
     * @param width the width to which to scale the image.
     * @param height the height to which to scale the image.
     * @param hints flags to indicate the type of algorithm to use
     * for image resampling.
     * @return     a scaled version of the image.
     * @see        java.awt.Image#SCALE_DEFAULT
     * @see        java.awt.Image#SCALE_FAST 
     * @see        java.awt.Image#SCALE_SMOOTH
     * @see        java.awt.Image#SCALE_REPLICATE
     * @see        java.awt.Image#SCALE_AREA_AVERAGING
     * @since      JDK1.1
     */
    public Image getScaledInstance(int width, int height, int hints) {
        ImageFilter filter;
        if ((hints & (SCALE_SMOOTH | SCALE_AREA_AVERAGING)) != 0) {
            filter = new AreaAveragingScaleFilter(width, height);
        } else {
            filter = new ReplicateScaleFilter(width, height);
        }
        ImageProducer prod;
        prod = new FilteredImageSource(getSource(), filter);
        return Toolkit.getDefaultToolkit().createImage(prod);
    }
    /**
     * Use the default image-scaling algorithm.
     * @since JDK1.1
     */
    public static final int SCALE_DEFAULT = 1;
    /**
     * Choose an image-scaling algorithm that gives higher priority
     * to scaling speed than smoothness of the scaled image.
     * @since JDK1.1
     */
    public static final int SCALE_FAST = 2;
    /**
     * Choose an image-scaling algorithm that gives higher priority
     * to image smoothness than scaling speed.
     * @since JDK1.1
     */
    public static final int SCALE_SMOOTH = 4;
    /**
     * Use the image scaling algorithm embodied in the 
     * <code>ReplicateScaleFilter</code> class.  
     * The <code>Image</code> object is free to substitute a different filter 
     * that performs the same algorithm yet integrates more efficiently
     * into the imaging infrastructure supplied by the toolkit.
     * @see        java.awt.image.ReplicateScaleFilter
     * @since      JDK1.1
     */
    public static final int SCALE_REPLICATE = 8;
    /**
     * Use the Area Averaging image scaling algorithm.  The
     * image object is free to substitute a different filter that
     * performs the same algorithm yet integrates more efficiently
     * into the image infrastructure supplied by the toolkit.
     * @see java.awt.image.AreaAveragingScaleFilter
     * @since JDK1.1
     */
    public static final int SCALE_AREA_AVERAGING = 16;
    /**
     * Flushes all resources being used by this Image object.  This
     * includes any pixel data that is being cached for rendering to
     * the screen as well as any system resources that are being used
     * to store data or pixels for the image.  The image is reset to
     * a state similar to when it was first created so that if it is
     * again rendered, the image data will have to be recreated or
     * fetched again from its source.
     * @since JDK1.0
     */
    public abstract void flush();
}
