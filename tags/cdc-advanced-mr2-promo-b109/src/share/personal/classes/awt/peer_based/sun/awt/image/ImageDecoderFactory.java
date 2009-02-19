/*
 * @(#)ImageDecoderFactory.java	1.5 06/10/10
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

import java.io.InputStream ;
import java.io.IOException ;

/**
 * <code>ImageDecoderFactory</code> allows the peer implementations to
 * provide alternative image decoder implementations that can be used
 * image decoding framework. There is always a default instance created 
 * that uses the decoders that are part of the <code>sun.awt.image</code>
 * package.
 * <p>
 * <code>sun.awt.image.InputStreamImageSource.getDecoder()</code> method
 * gets the instance of <code>ImageDecoderFactory</code> using 
 * <code>ImageDecoderFactory.getInstance()</code> and uses the factory to
 * create various decoders.
 * <h3>Plugging Alternative Factories</h3>
 * The peer implementation can provide alternate implementations for the
 * image decoders. To do that they need to do the following
 * <ul>
 *   <li>Subclass <code>sun.awt.image.ImageDecoderFactory</code> and provide 
 *   implementation for <code>ImageDecoderFactory.newImageDecoder()</code></li>
 *   <li>Create an instance of the subclass and register it with 
 *   <code>sun.awt.image.ImageDecoderFactory.setFactory()</code></li>
 * </ul>
 *
 */
public class ImageDecoderFactory {
    /**
     * GIF Image format
     */
    public static final String IMG_FORMAT_GIF = "gif" ;

    /**
     * JPEG Image format
     */
    public static final String IMG_FORMAT_JPG = "jpeg" ;

    /**
     * PNG Image format
     */
    public static final String IMG_FORMAT_PNG = "png" ;

    /**
     * XBM Image format
     */
    public static final String IMG_FORMAT_XBM = "xbm" ;

    /**
     * The instance of the <code>ImageDecoderFactory</code> configured.
     */
    private static ImageDecoderFactory defaultInstance = null ;

    /**
     * Returns the <code>ImageDecoderFactory</code> instance.
     */
    public synchronized static ImageDecoderFactory getInstance() {
        if ( defaultInstance == null ) {
            /* the GUI toolkit has not configured any factory, we will 
             * create a ImageDecoderFactory using the ImageDecoders that
             * are part of this package
             */
            defaultInstance = new ImageDecoderFactory() ;
        }
        return defaultInstance ;
    }

    /**
     * Sets the <code>ImageDecoderFactory</code> instance. This can be
     * called by the peer implementation to change the decoder factory
     */
    public synchronized static void setFactory(ImageDecoderFactory factory) {
        defaultInstance = factory ;
    }

    /**
     * Creates an instance of a new <code>ImageDecoder</code> for the 
     * image format specified.
     */
    public ImageDecoder newImageDecoder(InputStreamImageSource source,
                                        InputStream input,
                                        String imgFormat) throws IOException {
        ImageDecoder decoder = null ;
        if ( imgFormat == IMG_FORMAT_GIF ) {
            decoder = new GifImageDecoder(source, input) ;
        }
        else
        if ( imgFormat == IMG_FORMAT_JPG ) {
            decoder = new JPEGImageDecoder(source, input) ;
        }
        else
        if ( imgFormat == IMG_FORMAT_PNG ) {
            decoder = new PNGImageDecoder(source, input) ;
        }
        else
        if ( imgFormat == IMG_FORMAT_XBM ) {
            decoder = new XbmImageDecoder(source, input) ;
        }
        else {
            throw new IllegalArgumentException("Unsupported image format:"+
                                               imgFormat) ;
        }

        return decoder ;
    }
}
