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

#ifndef _AWTIMAGE_H_
#define _AWTIMAGE_H_

#include "jni.h"

#include "java_awt_Image.h"
#include "java_awt_Color.h"
#include "java_awt_image_ImageObserver.h"
#include "sun_awt_image_OffScreenImageSource.h"
#include "sun_awt_pocketpc_PPCImageRepresentation.h"
#include "java_awt_image_ImageObserver.h"

#define HINTS_DITHERFLAGS (java_awt_image_ImageConsumer_TOPDOWNLEFTRIGHT)


#define HINTS_OFFSCREENSEND (java_awt_image_ImageConsumer_TOPDOWNLEFTRIGHT |  \
			     java_awt_image_ImageConsumer_COMPLETESCANLINES | \
			     java_awt_image_ImageConsumer_SINGLEPASS)

#define IMAGE_SIZEINFO (java_awt_image_ImageObserver_WIDTH |	\
			java_awt_image_ImageObserver_HEIGHT)

#define IMAGE_DRAWINFO (java_awt_image_ImageObserver_WIDTH |	\
			java_awt_image_ImageObserver_SOMEBITS |	\
			java_awt_image_ImageObserver_HEIGHT)

#define IMAGE_OFFSCRNINFO (java_awt_image_ImageObserver_WIDTH |		\
			   java_awt_image_ImageObserver_HEIGHT |	\
			   java_awt_image_ImageObserver_SOMEBITS |	\
			   java_awt_image_ImageObserver_ALLBITS)


#define CMAPSIZE	256	/* number of colors to use in default cmap */
#define VIRTCUBESIZE	32	/* per-axis size of cmap to optimize */
#define LOOKUPSIZE	32	/* per-axis size of dithering lookup table */

extern "C" {
    typedef struct _ColorInfo {
	RGBQUAD *m_pPaletteColors;
    } ColorInfo;

    extern ColorInfo g_colorInfo[];
}
	
class AwtImage
{
public:
    // FIX ImgConvertData m_cvdata;
  BYTE *pixBuffer; // A pointer for holding the address of pixel bits
  BYTE *currentPtr;

  // Mask Buffer pointers
  BYTE *maskBuffer;
  BYTE *currmaskPtr;

  int bmp_stride;
  int bmp_padding;
  
  int mask_stride;
  int mask_padding;

#ifdef USE_MFC
    // Debug link used for list of ALL objects...
    DEFINE_AWT_LIST_LINK
#endif

	AwtImage(int width, int height, 
                 HBITMAP dibBitmap,
                 HBITMAP maskBitmap, 
                 BYTE **pBits, BYTE **maskBits);
	~AwtImage();

 public:
        HBITMAP hbitmap; // Our device independent bitmap to be created        
        HBITMAP maskBitmap; 

    static AwtImage
	*getAwtImageForSetPixels(JNIEnv *env, 
                                 jobject thisObj, 
                                 jint x,
                                 jint y,
                                 jint w, 
                                 jint h,
                                 jobject colorModel,
                                 jobject pixels,
                                 jint offset,
                                 jint scansize);
// Operations
	// Creates an offscreen DIB of the specified width and height
	BOOL OffScreenInit(JNIEnv *env, 
                           jobject thisObj,
                           jobject ch,
                           jint nW, 
                           jint nH);

        // Initialize the transparency mask bitmap
        void initializeTransparencyMask(HDC hDC);

	// Returns the width of the bitmap.
	jint GetWidth() {return dstWidth;};

	// Returns the height of the bitmap.
	jint GetHeight() {return dstHeight;};

	// Returns the width of the source data.
	jint GetSrcWidth() {return srcWidth;};

	// Returns the height of the source data.
	jint GetSrcHeight() {return srcHeight;};

	// Returns the depth of the output device.
	static jint GetDepth() {/* FIXME return
				   m_clrdata.bitsperpixel; */ return 0L;};
        
	static jint WinToJavaPixel(USHORT b, USHORT g, USHORT r);

	// Returns the actual color definition of a given pixel.
	static RGBQUAD *PixelColor(int pixel) {
	    return g_colorInfo[1].m_pPaletteColors + pixel;
	}

	// Returns the closest pixel match to the given r, g, b, pairing.
	static int ColorMatch(int r, int g, int b) {
	    return ClosestMatchPalette(g_colorInfo[1].m_pPaletteColors,
				       256, r, g, b);
	}

        // Sets the alpha value in the Mask Buffer
        void SetMaskBuffer(int alpha, int index);

        BOOL hasTransparentPixels;

        // Sets the RGB values into DIB pixel buffer.
        void SetDIBRGB(int blue, int green, int red);

	// Sends the pixels associated with this offscreen image to
	// an ImageConsumer.
	void sendPixels(JNIEnv *env, jobject osis);

	// Returns the handle to the bitmap.  May be NULL.
	// The bitmap may not yet be converted to a DDB in which case this
	// call will cause the conversion.  Assumes that the palette is
	// already selected into 'hDC'.
	// <<note: I've tried to convert the bitmap using a DC created with
	// ::CreateCompatibleDC(NULL) and selecting in the palette; this
	// doesn't seem to work. 
	HBITMAP GetDDB(HDC hDC) {return hbitmap;};

        HBITMAP GetMaskDDB(HDC hdc) {return maskBitmap;};

	// Draws the image on 'hDC' at ('nX', 'nY') with optional background
	// color c, in copy or xor mode.
	void Draw(JNIEnv *env, HDC hDC, jint nX, jint nY,
		  int xormode, COLORREF xorcolor, HBRUSH xorBrush,
		  jobject c);

	// Draws the subrectangle [('nSX', 'nSY') => 'nSW' x 'nSH'] of
	// the image inside the rectangle [('nDX', 'nDY') => 'nDW' x 'nDH']
	// on hDC with optional background color c, in copy or xor mode.
	void Draw(JNIEnv *env, HDC hDC, jint nDX, jint nDY, jint nDW, jint nDH,
		  jint nSX, jint nSY, jint nSW, jint nSH,
		  int xormode, COLORREF xorcolor, HBRUSH xorBrush,
		  jobject c);

	// Returns an icon from image.
	HICON ToIcon();

	// Returns a ColorModel object which represents the color space
	// of the screen.
	static jobject getColorModel(JNIEnv *env);

	// This member should be called by the application class during its finalization.
	static void CleanUp();

	// Takes the specified rgb value, goes through m_paletteColors looking for the closest
	// matching color and returns an index into m_paletteColors.  'r','g','b' can be
	// greater than 256.
	static int ClosestMatchPalette(RGBQUAD *p, int nPaletteSize, int r, int g, int b);

	// Allocates/verifies the output buffer for pixel conversion.
	// returns result code - nonzero for success.
	// Retrieve and optionally initialize an AwtImage object
	static AwtImage* getAwtImageFromJNI(JNIEnv *env, 
                                            jobject thisObj,
                                            jobject background); 

        // The followin functions modify the current pointer value
        // for Image Buffer and the Mask Buffer
        void skipToEndOfRow(int x, int w);
        void bmpPadding();
        void maskPadding();
        void skipBeginningOfRow(int x);

        // return the number of bits per pixel for this platform 
        static int GetBitsPerPixel();

          
// Variables

	COLORREF m_bgcolor;

	// Pointer to the function for converting data for the
	// color format of this screen.
	// FIX static ImgConvertFcn *m_pImageConvert[NUM_IMGCV];

	// If non-NULL, the image is an offscreen image.  To paint on the
	// image, grab the DC from this shared DC object.  Only one
 	// AwtGraphics object can be associated with the image at one time.
	class AwtSharedDC *m_pSharedDC;
//private:
// Variables

	// Non-NULL if image was converted to an icon.
	HICON m_hIcon;

	// The dimensions of the original image data.
	jint srcWidth;
	jint srcHeight;

	// The dimensions of the resulting bitmap.
	jint dstWidth;
	jint dstHeight;

	// The rendering hints.
	jint m_nHints;

	// The pixel delivery tracking fields.
	char *m_pSeen;
	HRGN m_hRgn;

};

#endif // _AWTIMAGE_H_
