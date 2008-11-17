/*
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


import java.awt.im.InputMethodHighlight;
import java.awt.image.ColorModel;
import java.awt.image.ImageObserver;
import java.awt.image.ImageProducer;
import sun.awt.image.ByteArrayImageSource;

import sun.awt.SunToolkit;

/** The toolkit used by this AWT implementation based on the Qt library.
    @version 1.8, 11/27/01
*/

class QtToolkit extends SunToolkit {
//    private EventQueue eventQueue = new EventQueue();
    private QtGraphicsEnvironment localEnv = new QtGraphicsEnvironment();
    private QtGraphicsConfiguration defaultGC = (QtGraphicsConfiguration)localEnv.defaultScreenDevice.getDefaultConfiguration();

    public QtToolkit () {
        super();
    }
	
    Graphics getGraphics(Window window) {
        return new QtGraphics(window);
    }
	
    GraphicsEnvironment getLocalGraphicsEnvironment() {
        return localEnv;
    }

    /**
     * Returns the screen resolution in dots-per-inch.
     * @return    this toolkit's screen resolution, in dots-per-inch.
     * @since     JDK1.0
     */
    public native int getScreenResolution();

    /**
     * Determines the color model of this toolkit's screen.
     * <p>
     * <code>ColorModel</code> is an  class that
     * encapsulates the ability to translate between the
     * pixel values of an image and its red, green, blue,
     * and alpha components.
     * <p>
     * This toolkit method is called by the
     * <code>getColorModel</code> method
     * of the <code>Component</code> class.
     * @return    the color model of this toolkit's screen.
     * @see       java.awt.image.ColorModel
     * @see       java.awt.Component#getColorModel
     * @since     JDK1.0
     */
    public ColorModel getColorModel() {

        return GraphicsEnvironment.getLocalGraphicsEnvironment().getDefaultScreenDevice().getDefaultConfiguration().getColorModel();
    }

    /**
     * Returns the names of the available fonts in this toolkit.<p>
     * For 1.1, the following font names are deprecated (the replacement
     * name follows):
     * <ul>
     * <li>TimesRoman (use Serif)
     * <li>Helvetica (use SansSerif)
     * <li>Courier (use Monospaced)
     * </ul><p>
     * The ZapfDingbats font is also deprecated in 1.1, but only as a
     * separate fontname.  Unicode defines the ZapfDingbat characters
     * starting at \u2700, and as of 1.1 Java supports those characters.
     * @return    the names of the available fonts in this toolkit.
     * @since     JDK1.0
     */
    public String[] getFontList() {
        return QtFontMetrics.getFontList();
    }

    /**
     * Gets the screen metrics of the font.
     * @param     font   a font.
     * @return    the screen metrics of the specified font in this toolkit.
     * @since     JDK1.0
     */
    public FontMetrics getFontMetrics(Font font) {
        return QtFontMetrics.getFontMetrics(font, true);
    }

    /**
     * Synchronizes this toolkit's graphics state. Some window systems
     * may do buffering of graphics events.
     * <p>
     * This method ensures that the display is up-to-date. It is useful
     * for animation.
     * @since     JDK1.0
     */
    public void sync() { /* localEnv.sync(); */ }
    
    /**
     * Prepares an image for rendering.
     * <p>
     * If the values of the width and height arguments are both
     * <code>-1</code>, this method prepares the image for rendering
     * on the default screen; otherwise, this method prepares an image
     * for rendering on the default screen at the specified width and height.
     * <p>
     * The image data is downloaded asynchronously in another thread,
     * and an appropriately scaled screen representation of the image is
     * generated.
     * <p>
     * This method is called by components <code>prepareImage</code>
     * methods.
     * <p>
     * Information on the flags returned by this method can be found
     * with the definition of the <code>ImageObserver</code> interface.

     * @param     image      the image for which to prepare a
     *                           screen representation.
     * @param     width      the width of the desired screen
     *                           representation, or <code>-1</code>.
     * @param     height     the height of the desired screen
     *                           representation, or <code>-1</code>.
     * @param     observer   the <code>ImageObserver</code>
     *                           object to be notified as the
     *                           image is being prepared.
     * @return    <code>true</code> if the image has already been
     *                 fully prepared; <code>false</code> otherwise.
     * @see       java.awt.Component#prepareImage(java.awt.Image,
     *                 java.awt.image.ImageObserver)
     * @see       java.awt.Component#prepareImage(java.awt.Image,
     *                 int, int, java.awt.image.ImageObserver)
     * @see       java.awt.image.ImageObserver
     * @since     JDK1.0
     */
    public  boolean prepareImage(Image image, int width, int height,        
        ImageObserver observer) {

		if(image == null)
			throw new NullPointerException("image can't be null");
		
        if (!(image instanceof QtImage)) {
            return false;
        }        
                    
		QtImage qtimg = (QtImage) image;                                
	  
        return qtimg.prepareImage(width, height, observer);
    }

    /**
     * Indicates the construction status of a specified image that is
     * being prepared for display.
     * <p>
     * If the values of the width and height arguments are both
     * <code>-1</code>, this method returns the construction status of
     * a screen representation of the specified image in this toolkit.
     * Otherwise, this method returns the construction status of a
     * scaled representation of the image at the specified width
     * and height.
     * <p>
     * This method does not cause the image to begin loading.
     * An application must call <code>prepareImage</code> to force
     * the loading of an image.
     * <p>
     * This method is called by the component's <code>checkImage</code>
     * methods.
     * <p>
     * Information on the flags returned by this method can be found
     * with the definition of the <code>ImageObserver</code> interface.
     * @param     image   the image whose status is being checked.
     * @param     width   the width of the scaled version whose status is
     *                 being checked, or <code>-1</code>.
     * @param     height  the height of the scaled version whose status
     *                 is being checked, or <code>-1</code>.
     * @param     observer   the <code>ImageObserver</code> object to be
     *                 notified as the image is being prepared.
     * @return    the bitwise inclusive <strong>OR</strong> of the
     *                 <code>ImageObserver</code> flags for the
     *                 image data that is currently available.
     * @see       java.awt.Toolkit#prepareImage(java.awt.Image,
     *                 int, int, java.awt.image.ImageObserver)
     * @see       java.awt.Component#checkImage(java.awt.Image,
     *                 java.awt.image.ImageObserver)
     * @see       java.awt.Component#checkImage(java.awt.Image,
     *                 int, int, java.awt.image.ImageObserver)
     * @see       java.awt.image.ImageObserver
     * @since     JDK1.0
     */
    public  int checkImage(Image image, int width, int height,
        ImageObserver observer) {

		if(image == null) throw new NullPointerException("image can't be null");
		
        if (!(image instanceof QtImage)) {
            return ImageObserver.ALLBITS;
        }
	  
        QtImage qtimg = (QtImage) image;

        return qtimg.getStatus(observer);
    }

    /**
     * Creates an image with the specified image producer.
     * @param     producer the image producer to be used.
     * @return    an image with the specified image producer.
     * @see       java.awt.Image
     * @see       java.awt.image.ImageProducer
     * @see       java.awt.Component#createImage(java.awt.image.ImageProducer)
     * @since     JDK1.0
     */
    public  Image createImage(ImageProducer producer) {
	if(producer==null) throw new NullPointerException("Specify valid producer");
        	return new QtImage(producer);
    }

    /**
     * Creates an image which decodes the image stored in the specified
     * byte array, and at the specified offset and length.
     * The data must be in some image format, such as GIF or JPEG,
     * that is supported by this toolkit.
     * @param     imagedata   an array of bytes, representing
     *                         image data in a supported image format.
     * @param     imageoffset  the offset of the beginning
     *                         of the data in the array.
     * @param     imagelength  the length of the data in the array.
     * @return    an image.
     * @since     JDK1.1
     */
    public  Image createImage(byte[] imagedata, int imageoffset, int imagelength) {

	if(imagedata==null) 
		throw new NullPointerException("Must supply image data");

	boolean isBadData = (imageoffset+imagelength>imagedata.length ||
		   imagelength <= 0 ||
		   imagedata.length == 0 );
		
        ImageProducer ip = new ByteArrayImageSource(imagedata, imageoffset, imagelength);
        Image newImage = new QtImage(ip, isBadData);

        return newImage;
    }
	
    Image createImage(Component component, int width, int height) {
        float size = (float) width * height;

        if (width <= 0 || height <= 0) {
            throw new IllegalArgumentException("Width (" + width + ") and height (" +
                    height + ") must be > 0");
        }
        if (size >= Integer.MAX_VALUE) {
            throw new IllegalArgumentException("Dimensions (width=" + width +
                    " height=" + height + ") are too large");
        }

        return new QtOffscreenImage(component, width, height, defaultGC);
    }

    /**
     * Emits an audio beep.
     * @since     JDK1.1
     */
    public native void beep();

    /* abstract method from SunToolkit */
    public String getDefaultCharacterEncoding() {
        return ""; // not needed for pbp
    }

    protected int getScreenWidth() {
        Rectangle dims = GraphicsEnvironment.getLocalGraphicsEnvironment().getDefaultScreenDevice().getDefaultConfiguration().getBounds();
        return dims.width;
    }

    protected int getScreenHeight() {
        Rectangle dims = GraphicsEnvironment.getLocalGraphicsEnvironment().getDefaultScreenDevice().getDefaultConfiguration().getBounds();
        return dims.height;
    }

    private native void hideNative();

    /**
     *  Show the specified window in a multi-vm environment
     */
    public void activate(Window window) {
        if (window == null) {
            return;
        }
        window.setVisible(true);
    }

    /**
     *  Hide the specified window in a multi-vm environment
     */
    public void deactivate(Window window) {
        if (window == null) {
            return;
        }
	window.setVisible(false);
        hideNative();
    }
}
