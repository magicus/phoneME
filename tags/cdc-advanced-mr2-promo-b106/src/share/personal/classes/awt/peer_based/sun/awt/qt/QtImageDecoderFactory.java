/*
 * @(#)QtImageDecoderFactory.java	1.5 06/10/10
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

package sun.awt.qt;

import java.io.* ;
import sun.awt.image.* ;

/**
 * <code>QtImageDecoderFactory</code> produces <code>QtImageDecoder</code>
 * for the GIF, JPEG and PNG image formats on QWS platform.
 * <code>QtToolkit</code> is responsible for instantiating this class 
 * and set as the default decoder factory.
 * <p>
 * Setting <b>-Dsun.awt.qt.img.decoder.factory.enable=false</b> on the 
 * application command line will disable the installation of this 
 * factory and will use java image decoders.
 *
 * @see sun.awt.image.ImageDecoderFactory
 */
class QtImageDecoderFactory extends ImageDecoderFactory {
    java.util.HashMap nativeDecoderList ;

    QtImageDecoderFactory() {
        this.nativeDecoderList = new java.util.HashMap() ;
    }

    /**
     * Create an instance of <codeQtImageDecoderFactory</code> with a 
     * list of QT image formats that should be enabled
     */
    QtImageDecoderFactory(String[] qtImgFormats) {
        this() ;
        enableNativeDecoder(qtImgFormats) ;
    }

    /**
     * Creates an instance of a new <code>ImageDecoder</code> for the 
     * image format specified.
     */
    public ImageDecoder newImageDecoder(InputStreamImageSource source,
                                        InputStream input,
                                        String imgFormat) throws IOException {
        ImageDecoder decoder = null ;
        if ( this.nativeDecoderList.get(imgFormat) == QtImageDecoder.class ) {
            decoder = new QtImageDecoder(source, input, imgFormat) ;
        }
        else {
            /* for all other formats, let us delegate to the 
             * super class */
            decoder = super.newImageDecoder(source,input,imgFormat) ;
        }

        return decoder ;
    }

    /**
     * Enable the Qt's native decoder for the image format specified
     *
     * @param imgFormat image format as specified by the constants defined
     *        in <code>sun.awt.image.ImageDecoderFactory</code>
     */
    synchronized void enableNativeDecoder(String imgFormat) {
        this.nativeDecoderList.put(imgFormat, QtImageDecoder.class) ;
    }

    /**
     * Disable the Qt's native decoder for the image format specified
     *
     * @param imgFormat image format as specified by the constants defined
     *        in <code>sun.awt.image.ImageDecoderFactory</code>
     */
    synchronized void disableNativeDecoder(String imgFormat) {
        this.nativeDecoderList.put(imgFormat, null) ;
    }

    /**
     * Enable the native decoders for the image formats supported by 
     * Qt.
     *
     * @param qtImgFormats QT image formats. On Qt 2.3.2 the following are
     *        the constants of interest
     *        <ul>
     *        <li><b>GIF</b></li>
     *        <li><b>JPEG</b></li>
     *        <li><b>PNG</b></li>
     *        <li><b>XBM</b></li>
     *        </ul>
     */
    private void enableNativeDecoder(String[] qtImgFormats) {
         /* For all the image formats supported by Personal profile, 
          * enable the native decoder if the Qt platform supports it
          */
        for ( int i = 0 ; i < qtImgFormats.length ; i++ ) {
            if ( qtImgFormats[i].equals("GIF") ) {
                enableNativeDecoder(ImageDecoderFactory.IMG_FORMAT_GIF) ;
            }
            else
            if ( qtImgFormats[i].equals("JPEG") ) {
                enableNativeDecoder(ImageDecoderFactory.IMG_FORMAT_JPG) ;
            }
            else
            if ( qtImgFormats[i].equals("PNG") ) {
                enableNativeDecoder(ImageDecoderFactory.IMG_FORMAT_PNG) ;
            }

            /*
             * Note :- Eventhough Qt supports XBM, it does not render very
             * well.  We get a black rectangle. This could be because of 
             * the background color of the window. The same behavior is seen
             * if we perform the same operations using a native QT app. So
             * we are using the Java decoder for XBM which works fine.
             */
        }
    }


}
