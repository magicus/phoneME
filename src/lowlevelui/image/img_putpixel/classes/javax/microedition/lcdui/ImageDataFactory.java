/*
 *
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

import java.io.InputStream;
import java.io.IOException;

/**
 * ImageFactory implementation based on putpixel graphics library and stores
 * data on Java heap.
 */
class ImageDataFactory implements AbstractImageDataFactory {

    /**
     * PNG Header Data
     */
    private static final byte[] pngHeader = new byte[] {
         (byte)0x89, (byte)0x50, (byte)0x4e, (byte)0x47,
         (byte)0x0d, (byte)0x0a, (byte)0x1a, (byte)0x0a,
         (byte)0x00, (byte)0x00, (byte)0x00, (byte)0x0d,
         (byte)0x49, (byte)0x48, (byte)0x44, (byte)0x52
    };

    /**
     * JPEG Header Data
     */
    private static final byte[] jpegHeader = new byte[] {
         (byte)0xff, (byte)0xd8, (byte)0xff, (byte)0xe0
    };

    /**
     * RAW Header Data
     */
    private static final byte[] rawHeader = new byte[] {
         (byte)0x89, (byte)0x53, (byte)0x55, (byte)0x4E
    };

    /** Reference to a image cache. */
    private SuiteImageCache imageCache;

    /** Initialize the image cache factory. */
    private ImageDataFactory() {
        imageCache = SuiteImageCacheFactory.getCache();
    }

    /**
     * ImageDataFactory singleton used for <code>ImageData</code>
     * creation.
     */
    private static ImageDataFactory imageDataFactory =
        new ImageDataFactory();

    /**
     * Returns the singleton <code>ImageDataFactory</code> instance.
     *
     * @return the singleton <code>ImageDataFactory</code> instance.
     */
    public static AbstractImageDataFactory getImageDataFactory() {
        return imageDataFactory;
    }

    /**
     * Creates a new, mutable image for off-screen drawing. Every pixel
     * within the newly created image is white.  The width and height of the
     * image must both be greater than zero.
     *
     * @param width the width of the new image, in pixels
     * @param height the height of the new image, in pixels
     * @return the created image
     */
    public ImageData createOffScreenImageData(int width, int height) {
        return new ImageData(width, height, true, true, false);
    }

    /**
     * Creates an immutable <code>ImageData</code> from
     * a <code>mutableSource ImageData</code>.
     * If the source image data is mutable, an immutable copy is created and
     * returned.  If the source image data is immutable, the implementation may
     * simply return it without creating a new image.  If an immutable source
     * image data contains transparency information,
     * this information is copied to the new image data unchanged.
     *
     * <p> This method is useful for placing the contents of mutable images
     * into <code>Choice</code> objects.  The application can create
     * an off-screen image
     * using the
     * {@link #createImage(int, int) createImage(w, h)}
     * method, draw into it using a <code>Graphics</code> object
     * obtained with the
     * {@link #getGraphics() getGraphics()}
     * method, and then create an immutable copy of it with this method.
     * The immutable copy may then be placed into <code>Choice</code>
     * objects. </p>
     *
     * @param mutableSource the source mutable image to be copied
     * @return the new immutable image data
     *
     * @throws NullPointerException if <code>source</code> is <code>null</code>
     */
    public ImageData createImmutableCopy(ImageData mutableSource) {
        int width  = mutableSource.getWidth();
        int height = mutableSource.getHeight();
        int length = width * height * 2;

        return  new ImageData(width, height, false,
                              mutableSource.getPixelData());
    }

    /**
     * Creates an immutable <code>ImageData</code>
     * from decoded image data obtained from the
     * named resource.  The name parameter is a resource name as defined by
     * {@link Class#getResourceAsStream(String)
     * Class.getResourceAsStream(name)}.  The rules for resolving resource
     * names are defined in the
     * <a href="../../../java/lang/package-summary.html">
     * Application Resource Files</a> section of the
     * <code>java.lang</code> package documentation.
     *
     * @param name the name of the resource containing the image data in one of
     * the supported image formats
     * @return the created image data
     * @throws NullPointerException if <code>name</code> is <code>null</code>
     * @throws java.io.IOException if the resource does not exist,
     * the data cannot
     * be loaded, or the image data cannot be decoded
     */
    public ImageData createResourceImageData(String name) throws IOException {
        ImageData data = new ImageData();

        /*
         * Load native image data from cache and create
         * image, if available. If image is not cached,
         * proceed to load and create image normally.
         */
        if (!loadCachedImage(data, name)) {
            createImageFromStream(data,
                                  ImageData.class.getResourceAsStream(name));
        }

        return data;
    }

    /**
     * Creates an immutable <code>ImageData</code>
     * which is decoded from the data stored in
     * the specified byte array at the specified offset and length. The data
     * must be in a self-identifying image file format supported by the
     * implementation, such as <a href="#PNG">PNG</A>.
     *
     * <p>The <code>imageoffset</code> and <code>imagelength</code>
     * parameters specify a range of
     * data within the <code>imageData</code> byte array. The
     * <code>imageOffset</code> parameter
     * specifies the
     * offset into the array of the first data byte to be used. It must
     * therefore lie within the range
     * <code>[0..(imageData.length-1)]</code>. The
     * <code>imageLength</code>
     * parameter specifies the number of data bytes to be used. It must be a
     * positive integer and it must not cause the range to extend beyond
     * the end
     * of the array. That is, it must be true that
     * <code>imageOffset + imageLength &lt; imageData.length</code>. </p>
     *
     * <p> This method is intended for use when loading an
     * image from a variety of sources, such as from
     * persistent storage or from the network.</p>
     *
     * @param imageBytes the array of image data in a supported image format
     * @param imageOffset the offset of the start of the data in the array
     * @param imageLength the length of the data in the array
     *
     * @return the created image
     * @throws ArrayIndexOutOfBoundsException if <code>imageOffset</code>
     * and <code>imageLength</code>
     * specify an invalid range
     * @throws NullPointerException if <code>imageData</code> is
     * <code>null</code>
     * @throws IllegalArgumentException if <code>imageData</code> is incorrectly
     * formatted or otherwise cannot be decoded
     */
    public ImageData createImmutableImageData(byte[] imageBytes,
                                              int imageOffset,
                                              int imageLength) {
        ImageData data = new ImageData();
        // parse the pixel data
        decode(data, imageBytes, imageOffset, imageLength);
        return data;
    }

    /**
     * Creates an immutable <code>ImageData</code>
     * using pixel data from the specified
     * region of a source <code>ImageData</code>, transformed as specified.
     *
     * <p>The source image dara may be mutable or immutable.
     * For immutable source image data,
     * transparency information, if any, is copied to the new
     * image data unchanged.</p>
     *
     * <p>On some devices, pre-transformed images may render more quickly
     * than images that are transformed on the fly using
     * <code>drawRegion</code>.
     * However, creating such images does consume additional heap space,
     * so this technique should be applied only to images whose rendering
     * speed is critical.</p>
     *
     * <p>The transform function used must be one of the following, as defined
     * in the {@link javax.microedition.lcdui.game.Sprite Sprite} class:<br>
     *
     * <code>Sprite.TRANS_NONE</code> - causes the specified image
     * region to be copied unchanged<br>
     * <code>Sprite.TRANS_ROT90</code> - causes the specified image
     * region to be rotated clockwise by 90 degrees.<br>
     * <code>Sprite.TRANS_ROT180</code> - causes the specified image
     * region to be rotated clockwise by 180 degrees.<br>
     * <code>Sprite.TRANS_ROT270</code> - causes the specified image
     * region to be rotated clockwise by 270 degrees.<br>
     * <code>Sprite.TRANS_MIRROR</code> - causes the specified image
     * region to be reflected about its vertical center.<br>
     * <code>Sprite.TRANS_MIRROR_ROT90</code> - causes the specified image
     * region to be reflected about its vertical center and then rotated
     * clockwise by 90 degrees.<br>
     * <code>Sprite.TRANS_MIRROR_ROT180</code> - causes the specified image
     * region to be reflected about its vertical center and then rotated
     * clockwise by 180 degrees.<br>
     * <code>Sprite.TRANS_MIRROR_ROT270</code> - causes the specified image
     * region to be reflected about its vertical center and then rotated
     * clockwise by 270 degrees.<br></p>
     *
     * <p>
     * The size of the returned image will be the size of the specified region
     * with the transform applied.  For example, if the region is
     * <code>100&nbsp;x&nbsp;50</code> pixels and the transform is
     * <code>TRANS_ROT90</code>, the
     * returned image will be <code>50&nbsp;x&nbsp;100</code> pixels.</p>
     *
     * <p><strong>Note:</strong> If all of the following conditions
     * are met, this method may
     * simply return the source <code>Image</code> without creating a
     * new one:</p>
     * <ul>
     * <li>the source image is immutable;</li>
     * <li>the region represents the entire source image; and</li>
     * <li>the transform is <code>TRANS_NONE</code>.</li>
     * </ul>
     *
     * @param dataSource the source image data to be copied from
     * @param x the horizontal location of the region to be copied
     * @param y the vertical location of the region to be copied
     * @param w the width of the region to be copied
     * @param h the height of the region to be copied
     * @param transform the transform to be applied to the region
     * @return the new, immutable image
     *
     * @throws NullPointerException if <code>image</code> is <code>null</code>
     * @throws IllegalArgumentException if the region to be copied exceeds
     * the bounds of the source image
     * @throws IllegalArgumentException if either <code>width</code> or
     * <code>height</code> is zero or less
     * @throws IllegalArgumentException if the <code>transform</code>
     * is not valid
     *
     */
    public ImageData createImmutableImageData(ImageData dataSource,
                                              int x, int y, int w, int h,
                                              int transform) {

        ImageData dataDest = null;

        if ((transform & Image.TRANSFORM_SWAP_AXIS) ==
            Image.TRANSFORM_SWAP_AXIS) {
            dataDest = new ImageData(h, w, false, false,
                                     dataSource.hasAlpha());
        } else {
            dataDest = new ImageData(w, h, false, false,
                                     dataSource.hasAlpha());
        }


        // Copy either the Java or romized data to its own array
        loadRegion(dataDest, dataSource, x, y, w, h, transform);

        return dataDest;
    }

    /**
     * Creates an immutable <code>ImageData</code>
     * from a sequence of ARGB values, specified
     * as <code>0xAARRGGBB</code>.
     * The ARGB data within the <code>rgb</code> array is arranged
     * horizontally from left to right within each row,
     * row by row from top to bottom.
     * If <code>processAlpha</code> is <code>true</code>,
     * the high-order byte specifies opacity; that is,
     * <code>0x00RRGGBB</code> specifies
     * a fully transparent pixel and <code>0xFFRRGGBB</code> specifies
     * a fully opaque
     * pixel.  Intermediate alpha values specify semitransparency.  If the
     * implementation does not support alpha blending for image rendering
     * operations, it must replace any semitransparent pixels with fully
     * transparent pixels.  (See <a href="#alpha">Alpha Processing</a>
     * for further discussion.)  If <code>processAlpha</code> is
     * <code>false</code>, the alpha values
     * are ignored and all pixels must be treated as fully opaque.
     *
     * <p>Consider <code>P(a,b)</code> to be the value of the pixel
     * located at column <code>a</code> and row <code>b</code> of the
     * Image, where rows and columns are numbered downward from the
     * top starting at zero, and columns are numbered rightward from
     * the left starting at zero. This operation can then be defined
     * as:</p>
     *
     * <TABLE BORDER="2">
     * <TR>
     * <TD ROWSPAN="1" COLSPAN="1">
     *    <pre><code>
     *    P(a, b) = rgb[a + b * width];    </code></pre>
     * </TD>
     * </TR>
     * </TABLE>
     * <p>for</p>
     *
     * <TABLE BORDER="2">
     * <TR>
     * <TD ROWSPAN="1" COLSPAN="1">
     *    <pre><code>
     *     0 &lt;= a &lt; width
     *     0 &lt;= b &lt; height    </code></pre>
     * </TD>
     * </TR>
     * </TABLE>
     * <p> </p>
     *
     * @param rgb an array of ARGB values that composes the image
     * @param width the width of the image
     * @param height the height of the image
     * @param processAlpha <code>true</code> if <code>rgb</code>
     * has an alpha channel,
     * <code>false</code> if all pixels are fully opaque
     * @return the created <code>ImageData</code>
     * @throws NullPointerException if <code>rgb</code> is <code>null</code>.
     * @throws IllegalArgumentException if either <code>width</code> or
     * <code>height</code> is zero or less
     * @throws ArrayIndexOutOfBoundsException if the length of
     * <code>rgb</code> is
     * less than<code> width&nbsp;*&nbsp;height</code>.
     *
     */
    public ImageData createImmutableImageData(int rgb[], int width, int height,
                                              boolean processAlpha) {
        ImageData data = new ImageData(width, height, false, false,
                                       processAlpha);
        loadRGB(data, rgb);

        return data;
    }

    /**
     * Function to decode an <code>ImageData</code> from romized data.
     *
     * @param imageDataArrayPtr native pointer to image data as Java int
     * @param imageDataArrayLength length of image data array
     * @return <code>ImageData</code> created from romized data.
     * @throws IllegalArgumentException if the id is invalid
     */
    public ImageData createImmutableImageData(int imageDataArrayPtr,
            int imageDataArrayLength) {

        ImageData data = new ImageData();
        if (!loadRomizedImage(data, imageDataArrayPtr,
                    imageDataArrayLength)) {
            throw new IllegalArgumentException();
        }
        return data;
    }

    /**
     * Creates an immutable image from decoded image data obtained from an
     * <code>InputStream</code>.  This method blocks until all image data has
     * been read and decoded.  After this method completes (whether by
     * returning or by throwing an exception) the stream is left open and its
     * current position is undefined.
     *
     * @param stream the name of the resource containing the image data
     * in one of the supported image formats
     *
     * @return the created image
     * @throws NullPointerException if <code>stream</code> is <code>null</code>
     * @throws java.io.IOException if an I/O error occurs, if the image data
     * cannot be loaded, or if the image data cannot be decoded
     *
     */
    public ImageData createImmutableImageData(InputStream stream)
        throws IOException {

        ImageData data = new ImageData();
        createImageFromStream(data, stream);
        return data;
    }

    /**
     * Populates an immutable <code>ImageData</code>
     * from decoded data obtained from an <code>InputStream</code>.
     * This method blocks until all image data has
     * been read and decoded.  After this method completes (whether by
     * returning or by throwing an exception) the stream is left open and its
     * current position is undefined.
     *
     * @param data The <code>ImageData</code> to be populated
     * @param stream the name of the resource containing the image data
     * in one of the supported image formats
     *
     * @throws NullPointerException if <code>stream</code> is <code>null</code>
     * @throws java.io.IOException if an I/O error occurs, if the image data
     * cannot be loaded, or if the image data cannot be decoded
     *
     */
    private void createImageFromStream(ImageData data, InputStream stream)
        throws java.io.IOException {

        if (stream == null) {
            throw new java.io.IOException();
        } else {
            /*
             * Allocate an array assuming available is correct.
             * Only reading an EOF is the real end of file
             * so allocate an extra byte to read the EOF into.
             * If available is incorrect, increase the buffer
             * size and keep reading.
             */
            int l = stream.available();
            byte[] buffer = new byte[l+1];
            int length = 0;

            // TBD: Guard against an implementation with incorrect available
            while ((l = stream.read(buffer, length, buffer.length-length))
                   != -1) {
                length += l;
                if (length == buffer.length) {
                    byte[] b = new byte[buffer.length + 4096];
                    System.arraycopy(buffer, 0, b, 0, length);
                    buffer = b;
                }
            }

            stream.close();

            try {
                decode(data, buffer, 0, length);
            } catch (IllegalArgumentException e) {
                throw new java.io.IOException();
            }
        }
    }

    /**
     * Load an <code>ImageData</code> from cache. The real work is done in
     * the native function.
     *
     * @param   imageData The <code>ImageData</code> to be populated
     * @param   resName  Image resource name
     * @return  true if image was loaded and created, false otherwise
     */
    private boolean loadCachedImage(ImageData imageData,
                                    String resName) {
        return imageCache.loadAndCreateImmutableImageData(imageData, 
                                                            resName);
    }

    /**
     * Function to decode an <code>ImageData</code> from PNG data.
     *
     * @param imageData the <code>ImageData</code> to be populated
     * @param imageBytes the array of image data in a supported image format
     * @param imageOffset the offset of the start of the data in the array
     * @param imageLength the length of the data in the array
     */
    private void decodePNG(ImageData imageData,
                           byte[] imageBytes,
                           int imageOffset, int imageLength) {
        // find the format of the image data
        if (imageLength < pngHeader.length + 8) {
            throw new IllegalArgumentException();
        }

        int width = ((imageBytes[imageOffset + 16] & 0x0ff) << 24) +
                    ((imageBytes[imageOffset + 17] & 0x0ff) << 16) +
                    ((imageBytes[imageOffset + 18] & 0x0ff) <<  8) +
                    (imageBytes[imageOffset + 19] & 0x0ff);

        int height = ((imageBytes[imageOffset + 20] & 0x0ff) << 24) +
                     ((imageBytes[imageOffset + 21] & 0x0ff) << 16) +
                     ((imageBytes[imageOffset + 22] & 0x0ff) <<  8) +
                     (imageBytes[imageOffset + 23] & 0x0ff);

        if (width <= 0 || height <= 0) {
            throw new IllegalArgumentException();
        }

        imageData.initImageData(width, height, false, true);

        // load the decoded PNG data into the data byte arrays
        if (loadPNG(imageData, imageBytes, imageOffset, imageLength) ==
            false) {
            // if loadPNG returns false, the image contains
            // only opaque pixel and the alpha data is not needed
            imageData.removeAlpha();
        }
    }

    /**
     * Function to decode an <code>ImageData</code> from JPEG data.
     *
     * @param imageData the <code>ImageData</code> to be populated
     * @param imageBytes the array of image data in a supported image format
     * @param imageOffset the offset of the start of the data in the array
     * @param imageLength the length of the data in the array
     */
    private void decodeJPEG(ImageData imageData,
                            byte[] imageBytes,
                            int imageOffset,
                            int imageLength) {
        // find the format of the image data
        if (imageLength < jpegHeader.length + 8) {
            throw new IllegalArgumentException();
        }

        int width = 0;
        int height = 0;

        /**
         * Find SOF (Start Of Frame) marker:
         * format of SOF
         *   2 bytes    Marker Identity  (0xff 0xc<N>)
         *   2 bytes    Length of block
         *   1 byte     bits/sample
         *   2 bytes    Image Height
         *   2 bytes    Image Width
         *   1 bytes    Number of components
         *   n bytes    the components
         *
         * Searching for the byte pair representing SOF is unsafe
         * because a prior marker might contain the SOFn pattern
         * so we must skip over the preceding markers.
         *
         * When editing this code, don't forget to make the appropriate changes
         * in src/lowlevelui/graphics_util/reference/native/gxutl_image_util.c.
         */
        int idx = imageOffset + 2;

        while (idx + 8 < imageOffset + imageLength) {
            if (imageBytes[idx] != (byte)0xff) {
                break;
            }

            if ((byte) (imageBytes[idx + 1] & 0xf0) == (byte) 0xc0) {
                byte code = imageBytes[idx + 1];

                if (code != (byte) 0xc4 && code != (byte) 0xc8 &&
                        code != (byte) 0xcc) {
                    /* Big endian */
                    height = ((imageBytes[idx + 5] & 0xff) << 8) +
                            (imageBytes[idx + 6] & 0xff);
                    width = ((imageBytes[idx + 7] & 0xff) << 8) +
                            (imageBytes[idx + 8] & 0xff);
                    break;
                }
            }

            /* Go to the next marker */
            int field_len = ((imageBytes[idx + 2] & 0xff) << 8) +
                    (imageBytes[idx + 3] & 0xff);
            idx += field_len + 2;
        }

        if (width <= 0 || height <= 0) {
            throw new IllegalArgumentException();
        }

        imageData.initImageData(width, height, false, false);

        // load the decoded JPEG data into the pixelData array
        loadJPEG(imageData, imageBytes, imageOffset, imageLength);
    }

    /**
     * Function to decode an <code>ImageData</code> from RAW data.
     *
     * @param imageData the <code>ImageData</code> to be populated
     * @param imageBytes the array of image data in a supported image format
     * @param imageOffset the offset of the start of the data in the array
     * @param imageLength the length of the data in the array
     */
    private void decodeRAW(ImageData imageData,
                            byte[] imageBytes,
                            int imageOffset,
                            int imageLength) {
        // find the format of the image data
        if (imageLength < rawHeader.length + 8) {
            throw new IllegalArgumentException();
        }
        loadRAW(imageData, imageBytes, imageOffset, imageLength);
    }

    /**
     * Function to compare byte data to the given header
     *
     * @param header header data to compare imageData with
     * @param imageData the array of image data in a supported image format
     * @param imageOffset the offset of the start of the data in the array
     * @param imageLength the length of the data in the array
     * @return true if the header.length bytes at imageData[imageOffset]
     * are equal to the bytes in header array, false otherwise
     */
    private boolean headerMatch(byte[] header,
                                byte[] imageData,
                                int imageOffset,
                                int imageLength) {
        if (imageLength < header.length) {
            return false;
        }

        for (int i = 0; i < header.length; i++) {
            if (imageData[imageOffset + i] != header[i]) {
                return false;
            }
        }

        return true;
    }

    /**
     * Function to decode an <code>ImageData</code> from byte data.
     *
     * @param imageData the <code>ImageData<code> to populate
     * @param imageBytes the array of image data in a supported image format
     * @param imageOffset the offset of the start of the data in the array
     * @param imageLength the length of the data in the array
     * @throws IllegalArgumentException if the data is not formatted correctly
     */
    private void decode(ImageData imageData,
                        byte[] imageBytes, int imageOffset, int imageLength) {
        // check if it is a PNG image
        if (headerMatch(pngHeader, imageBytes, imageOffset, imageLength)) {
            // image type is PNG
            decodePNG(imageData, imageBytes, imageOffset, imageLength);
        } else if (headerMatch(jpegHeader, imageBytes,
                               imageOffset, imageLength)) {
            // image type is JPEG
            decodeJPEG(imageData, imageBytes, imageOffset, imageLength);
        } else if (headerMatch(rawHeader, imageBytes,
                               imageOffset, imageLength)) {
            // image type is RAW
            decodeRAW(imageData, imageBytes, imageOffset, imageLength);
        } else {
            // does not match supported image type
            throw new IllegalArgumentException();
        }
    }

    /**
     * Native function to load an <code>ImageData</code> from PNG data.
     *
     * @param imageData the <code>ImageData</code> to load to
     * @param imageBytes the array of image data in a supported image format
     * @param imageOffset the offset of the start of the data in the array
     * @param imageLength the length of the data in the array
     *
     * @return true if there is alpha data
     */
    private native boolean loadPNG(ImageData imageData,
                                   byte[] imageBytes,
                                   int imageOffset,
                                   int imageLength);

    /**
     * Native function to load an <code>ImageData </code>from JPEG data.
     *
     * @param imageData the <code>ImageData</code> to load to
     * @param imageBytes the array of image data in a supported image format
     * @param imageOffset the offset of the start of the data in the array
     * @param imageLength the length of the data in the array
     */
    private native void loadJPEG(ImageData imageData,
                                 byte[] imageBytes,
                                 int imageOffset,
                                 int imageLength);

    /**
     * Native function to load an <code>ImageData</code>
     * directly out of the rom image.
     *
     * @param data The <code>ImageData</code> to load to
     * @param imageDataArrayPtr native pointer to image data as Java int
     * @param imageDataArrayLength length of image data array
     * @return true if romized image was loaded successfully,
     *         false - otherwise
     */
    private native boolean loadRomizedImage(ImageData data,
            int imageDataArrayPtr, int imageDataArrayLength);

    /**
     * Native function to load an <code>ImageData</code> from ARGB data.
     *
     * @param imgData The <code>ImageData</code> to load to
     * @param rgb the array of image data in a ARGB format
     */
    private native void loadRGB(ImageData imgData, int[] rgb);

    /**
     * Native function to load an <code>ImageData</code> from RAW data.
     *
     * @param imgData The <code>ImageData</code> to load to
     * @param imageBytes the array of image data in a RAW format
     */
    private native void loadRAW(ImageData imgData,
                                byte[] imageBytes,
                                int imageOffset,
                                int imageLength);

    /**
     * Copy either Java or romized data into another <code>ImageData</code>
     * from an <code>ImageData</code> region.
     *
     * @param dest the <code>ImageData</code> to copy to
     * @param source the source image to be copied from
     * @param x the horizontal location of the region to be copied
     * @param y the vertical location of the region to be copied
     * @param width the width of the region to be copied
     * @param height the height of the region to be copied
     * @param transform the transform to be applied to the region
     */
    private native void loadRegion(ImageData dest, ImageData source,
                                   int x, int y,
                                   int width, int height,
                                   int transform);
}
