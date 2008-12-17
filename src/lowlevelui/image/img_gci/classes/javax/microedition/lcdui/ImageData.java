/*
 *  
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package javax.microedition.lcdui;

import com.sun.me.gci.surface.GCIJavaDrawingSurface;
import com.sun.me.gci.surface.GCIDrawingSurface;
import com.sun.me.gci.surface.GCISurfaceInfo;
import com.sun.me.gci.renderer.GCIImageRenderer;

/**
 * AbstractImageData implementation based 
 * on putpixel graphics library and stores data on Java heap.
 */
final class ImageData implements AbstractImageData {

    /**
     * The width, height of this Image
     */
    private int width, height;

    /**
     * If this Image is mutable.
     */
    private boolean isMutable;

    /**
     * Image pixel data byte array.
     * If this is null, use nativePixelData and nativeAlphaData instead.
     */
    private byte[] pixelData;

    /**
     * Image alpha data byte array.
     * If pixelData is null, use nativePixelData instead.
     */
    private byte[] alphaData;

    /**
     * Image native romized pixel data.
     * Set in native by loadRomizedImage().
     * Must remain 0 unless pixelData is null.
     */
    private int nativePixelData;
    
    /**
     * Image native romized alpha data.
     * Set in native by loadRomizedImage().
     * Must remain 0 unless pixelData is null.
     */
    private int nativeAlphaData;


    /**
     * <code>GCIDrawingSurface</code> that is used to render
     * pixelData.
     */
    GCIDrawingSurface gciDrawingSurface;

    /**
     * Constructs empty <code>ImageData </code>.
     */
    ImageData() {
    }


    /**
     * Constructs <code>ImageData </code> using passed in width and height.
     * Alpha array is allocated if allocateAlpha is true.
     *
     * @param width The width of the <code>ImageData </code> to be created.
     * @param height The height of the <code>ImageData </code> to be created.
     * @param isMutable true to create mutable <code>ImageData</code>,
     *                  false to create immutable <code>ImageData</code>
     * @param clearPixelData if true pixel data whould be set to 0xff for
     *                       each pixel
     * @param allocateAlpha true if alpha data should be allocated,
     *                      false - if not.
     */
    ImageData(int width, int height, boolean isMutable,
              boolean clearPixelData,
              boolean allocateAlpha) {

        initImageData(width, height, isMutable, allocateAlpha);

        if (clearPixelData) {
            for (int i = 0; i < pixelData.length; i++) {
                pixelData[i] = (byte)0xFF;
            }
        }

        createGCISurfaces();
    }

    /**
     * Constructs mutable or immutable <code> ImageData </code>
     * using passed in width, height and pixel data.
     *
     * @param width The width of the <code>ImageData </code> to be created.
     * @param height The height of the <code>ImageData </code> to be created.
     * @param isMutable true to create mutable <code>ImageData</code>,
     *                  false to create immutable <code>ImageData</code>
     * @param pixelData byte array that contains pixel data for the 
     *                  <code>ImageData</code>.
     */
    ImageData(int width, int height, boolean isMutable,
              byte[] pixelData) {
        this.width = width;
        this.height = height;
        this.isMutable = isMutable;

        int length = width * height * 2;
        byte[] newPixelData = new byte[length];
        System.arraycopy(pixelData, 0, newPixelData, 0, length);

        this.pixelData = newPixelData;

	createGCISurfaces();
    }

    /**
     * Initializes mutable or immutable <code> ImageData </code> 
     * using passed in width, height.
     * Alpha array is allocated if allocateAlpha is true.
     *
     * @param width The width of the <code>ImageData </code> to be created.
     * @param height The height of the <code>ImageData </code> to be created.
     * @param isMutable true to create mutable <code>ImageData</code>,
     *                  false to create immutable <code>ImageData</code>
     * @param allocateAlpha true if alpha data should be allocated,
     *                      false - if not.
     */
    void initImageData(int width, int height, boolean isMutable,
                       boolean allocateAlpha) {
        this.width = width;
        this.height = height;
        this.isMutable = isMutable;

        pixelData = new byte[width * height * 2];

        if (allocateAlpha) {
            alphaData = new byte[width * height];
        } else {
            alphaData = null;
        }
    }

    void createGCISurfaces() {
	gciDrawingSurface = 
	    new GCIJavaDrawingSurface(GCISurfaceInfo.TYPE_JAVA_ARRAY_INT,
				      width, height,
				      (alphaData == null ?
				      GCIJavaDrawingSurface.FORMAT_XRGB_8888 :
				      GCIJavaDrawingSurface.FORMAT_ARGB_8888));
       

        Graphics g = new Graphics(gciDrawingSurface);
        // g.img = img;
        g.setDimensions(width, height);
        g.resetGC();

        GCIImageRenderer imgRenderer = g.getGCIImageRenderer();
        
	GCIDrawingSurface gciColorDrawingSurface = 
	    new GCIJavaDrawingSurface(pixelData, 0,
				      16, width * 16,
				      width, height,
				      GCIDrawingSurface.FORMAT_RGB_565 );
        if (alphaData == null) {
            imgRenderer.drawImage(gciColorDrawingSurface,
				  0, 0, width, height, 0, 0);
         
        } else {
	    GCIDrawingSurface gciMaskDrawingSurface = 
		new GCIJavaDrawingSurface(alphaData, 0,
					  8, width * 8,
					  width, height,
					  GCIDrawingSurface.FORMAT_GRAY_8);

            imgRenderer.maskBlit(gciColorDrawingSurface,
				 gciMaskDrawingSurface,
				 0, 0, width, height, 0, 0, 0, 0);
        }
    }

    /**
     * Gets the width of the image in pixels. The value returned
     * must reflect the actual width of the image when rendered.
     * @return width of the image
     */
    public int getWidth() {
        return width;
    }

    /**
     * Gets the height of the image in pixels. The value returned
     * must reflect the actual height of the image when rendered.
     * @return height of the image
     */
    public int getHeight() {
        return height;
    }

    /**
     * Check if this image is mutable. Mutable images can be modified by
     * rendering to them through a <code>Graphics</code> object
     * obtained from the
     * <code>getGraphics()</code> method of this object.
     * @return <code>true</code> if the image is mutable,
     * <code>false</code> otherwise
     */
    public boolean isMutable() {
        return isMutable;
    }

    /**
     * Implements <code>AbstractImageData.getRGB() </code>.
     * See javadoc comments there.
     *
     * @param rgbData an array of integers in which the ARGB pixel data is
     * stored
     * @param offset the index into the array where the first ARGB value
     * is stored
     * @param scanlength the relative offset in the array between
     * corresponding pixels in consecutive rows of the region
     * @param x the x-coordinate of the upper left corner of the region
     * @param y the y-coordinate of the upper left corner of the region
     * @param w the width of the region
     * @param h the height of the region
     */
    public void getRGB(int[] rgbData, int offset, int scanlength,
		       int x, int y, int w, int h) {
	
        
	gciDrawingSurface.getPixels(x, y, w, h,
				    rgbData, offset, scanlength);
    }

    /**
     * Returns true if <code>ImageData</code> contains alpha data.
     * 
     * @return true if <code>ImageData</code> contains alpha data.
     */
    public boolean hasAlpha() {
        return (alphaData != null || nativeAlphaData != 0);
    }

    /**
     * Removes alpha data information 
     */
    public void removeAlpha() {
        alphaData = null;
    }

    /**
     * Gets pixel data associated with this <code> ImageData</code> instance.
     * @return byte arra that represents pixel data associated with this
     *         <code>ImageData</code> instance.
     */
    byte[] getPixelData() {
        return pixelData;
    }

    /**
     * Gets pixel data associated with this <code> ImageData</code> instance.
     * @return byte arra that represents pixel data associated with this
     *         <code>ImageData</code> instance.
     */
    byte[] getAlphaData() {
        return alphaData;
    }
}
