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

#include <stdlib.h>
#include <sni.h>
#include <midpError.h>
#include <gx_image.h>
#include <gxapi_graphics.h>
#include <gxutl_graphics.h>

#include "gxapi_intern_graphics.h"

/**
 * Gets an ARGB integer array from this <tt>ImmutableImage</tt>. The
 * array consists of values in the form of 0xAARRGGBB.
 * <p>
 * Java declaration:
 * <pre>
 *     getRGB([IIIIIII)V
 * </pre>
 *
 * @param rgbData The target integer array for the ARGB data
 * @param offset Zero-based index of first ARGB pixel to be saved
 * @param scanlen Number of intervening pixels between pixels in
 *                the same column but in adjacent rows
 * @param x The x coordinate of the upper left corner of the
 *          selected region
 * @param y The y coordinate of the upper left corner of the
 *          selected region
 * @param width The width of the selected region
 * @param height The height of the selected region
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_Image_getRGB) {
    int height = KNI_GetParameterAsInt(7);
    int width = KNI_GetParameterAsInt(6);
    int y = KNI_GetParameterAsInt(5);
    int x = KNI_GetParameterAsInt(4);
    int scanlength = KNI_GetParameterAsInt(3);
    int offset = KNI_GetParameterAsInt(2);
    int buflen;
    int *rgbBuffer;
    int img_width;
    int img_height;
    jboolean iae = KNI_FALSE;
    java_imagedata * srcImageDataPtr = NULL;

    KNI_StartHandles(2);
    KNI_DeclareHandle(rgbData);
    KNI_DeclareHandle(thisObject);

    KNI_GetParameterAsObject(1, rgbData);
    KNI_GetThisPointer(thisObject);

    srcImageDataPtr = GET_IMAGE_PTR(thisObject)->imageData;

    img_width  = srcImageDataPtr->width;
    img_height = srcImageDataPtr->height;

    /* see if absolute value of scanlength is greater than or equal to width */
    if (scanlength >= 0 && scanlength < width) {
        iae = KNI_TRUE;
    } else if (scanlength < 0 && (0 - scanlength) < width) {
        iae = KNI_TRUE;
    }
    if (KNI_IsNullHandle(rgbData)) {
        KNI_ThrowNew(midpNullPointerException, NULL);
    } else if((y < 0) || (x < 0) || (x + width > img_width) ||
              (y + height > img_height) || iae == KNI_TRUE) {
        KNI_ThrowNew(midpIllegalArgumentException, NULL);
    } else if (height < 0 || width < 0 ) {
        /* spec says noop in this case */
    } else {
        buflen = KNI_GetArrayLength(rgbData);
        if (offset < 0
            || offset + ((height - 1) * scanlength) + width > buflen
            || offset + ((height - 1) * scanlength) < 0) {
            KNI_ThrowNew(midpArrayIndexOutOfBoundsException, NULL);
        } else {
  	    gxutl_native_image_error_codes error = GXUTL_NATIVE_IMAGE_NO_ERROR;

	    SNI_BEGIN_RAW_POINTERS;

            rgbBuffer = JavaIntArray(rgbData);
	    gx_get_argb(srcImageDataPtr, rgbBuffer,
		       offset, scanlength,
		       x, y, width, height, &error);

	    SNI_END_RAW_POINTERS;

	    if (error != GXUTL_NATIVE_IMAGE_NO_ERROR) {
		KNI_ThrowNew(midpOutOfMemoryError, NULL);
	    }
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Draws the specified image by using the anchor point.
 * The image can be drawn in different positions relative to
 * the anchor point by passing the appropriate position constants.
 * See <a href="#anchor">anchor points</a>.
 *
 * <p>If the source image contains transparent pixels, the corresponding
 * pixels in the destination image must be left untouched.  If the source
 * image contains partially transparent pixels, a compositing operation
 * must be performed with the destination pixels, leaving all pixels of
 * the destination image fully opaque.</p>
 *
 * <p>If <code>img</code> is the same as the destination of this Graphics
 * object, the result is undefined.  For copying areas within an
 * <code>Image</code>, {@link #copyArea copyArea} should be used instead.
 * </p>
 *
 * @param g the specified Graphics to be drawn
 * @param x the x coordinate of the anchor point
 * @param y the y coordinate of the anchor point
 * @param anchor the anchor point for positioning the image
 * @throws IllegalArgumentException if <code>anchor</code>
 * is not a legal value
 * @throws NullPointerException if <code>g</code> is <code>null</code>
 * @see Image
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(javax_microedition_lcdui_Image_render) {
    jboolean success = KNI_TRUE;

    int anchor = KNI_GetParameterAsInt(4);
    int y      = KNI_GetParameterAsInt(3);
    int x      = KNI_GetParameterAsInt(2);

    KNI_StartHandles(3);
    KNI_DeclareHandle(img);
    KNI_DeclareHandle(g);
    KNI_DeclareHandle(gImg);

    KNI_GetParameterAsObject(1, g);
    KNI_GetThisPointer(img);

    if (GRAPHICS_OP_IS_ALLOWED(g)) {
        /* null checking is handled by the Java layer, but test just in case */
        if (KNI_IsNullHandle(img)) {
            success = KNI_FALSE; //KNI_ThrowNew(midpNullPointerException, NULL);
        } else {
  	    const java_imagedata * srcImageDataPtr =
	      GET_IMAGE_PTR(img)->imageData;

            GET_IMAGE_PTR(gImg) =
	      (struct Java_javax_microedition_lcdui_Image *)
	      (GXAPI_GET_GRAPHICS_PTR(g)->img);
            if (KNI_IsSameObject(gImg, img) || !gxutl_check_anchor(anchor,0)) {
                success = KNI_FALSE; //KNI_ThrowNew(midpIllegalArgumentException, NULL);
            } else if (!gxutl_normalize_anchor(&x, &y, srcImageDataPtr->width, 
					       srcImageDataPtr->height, 
					       anchor)) {
                success = KNI_FALSE;//KNI_ThrowNew(midpIllegalArgumentException, NULL);
            } else {
	        jshort clip[4]; /* Defined in Graphics.java as 4 shorts */
	        const java_imagedata * dstMutableImageDataPtr = 
		  GXAPI_GET_IMAGEDATA_PTR_FROM_GRAPHICS(g);

                GXAPI_TRANSLATE(g, x, y);
		GXAPI_GET_CLIP(g, clip);

		gx_render_image(srcImageDataPtr, dstMutableImageDataPtr,
				clip, x, y);

            }
        }
    }

    KNI_EndHandles();
    KNI_ReturnBoolean(success);
}

/**
 * Renders the given region of this <tt>ImmutableImage</tt> onto the
 * given <tt>Graphics</tt> object.
 * <p>
 * Java declaration:
 * <pre>
 *     renderRegion(Ljavax/microedition/lcdui/ImageImpl;IIIIIIII)V
 * </pre>
 *
 * @param g The <tt>Graphics</tt> object to be drawn
 * @param x_src The x coordinate of the upper-left corner of the
 *              source region
 * @param y_src The y coordinate of the upper-left corner of the
 *              source region
 * @param width The width of the source region
 * @param height The height of the source region
 * @param transform The transform to apply to the selected region.
 * @param x_dest The x coordinate of the destination anchor point
 * @param y_dest The y coordinate of the destination anchor point
 * @param anchor The anchor point for positioning the destination
 *               <tt>Image</tt>
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(javax_microedition_lcdui_Image_renderRegion) {
    int anchor    = KNI_GetParameterAsInt(9);
    int y_dest    = KNI_GetParameterAsInt(8);
    int x_dest    = KNI_GetParameterAsInt(7);
    int transform = KNI_GetParameterAsInt(6);
    int height    = KNI_GetParameterAsInt(5);
    int width     = KNI_GetParameterAsInt(4);
    int y_src     = KNI_GetParameterAsInt(3);
    int x_src     = KNI_GetParameterAsInt(2);
    jboolean success = KNI_TRUE;

    KNI_StartHandles(3);
    KNI_DeclareHandle(img);
    KNI_DeclareHandle(g);
    KNI_DeclareHandle(gImg);

    KNI_GetParameterAsObject(1, g);
    KNI_GetThisPointer(img);
    
    if (GRAPHICS_OP_IS_ALLOWED(g)) {
      if (KNI_IsNullHandle(img)) {
        /* null checking is performed in the Java code, but check just in case */
        success = KNI_FALSE; //KNI_ThrowNew(midpNullPointerException, NULL);
      } else if ((transform < 0) || (transform > 7)) {
        success = KNI_FALSE; //KNI_ThrowNew(midpIllegalArgumentException, NULL);
      } else if (!gxutl_normalize_anchor(&x_dest, &y_dest,
					 width, height, anchor)) {
        success = KNI_FALSE; //KNI_ThrowNew(midpIllegalArgumentException, NULL);
      } else {
	const java_imagedata * srcImageDataPtr = GET_IMAGE_PTR(img)->imageData;
        jint img_width = srcImageDataPtr->width;
        jint img_height = srcImageDataPtr->height;

        GET_IMAGE_PTR(gImg) = (struct Java_javax_microedition_lcdui_Image *)
	                      (GXAPI_GET_GRAPHICS_PTR(g)->img);
        if (KNI_IsSameObject(gImg, img) || 
           (height < 0) || (width < 0) || (x_src < 0) || (y_src < 0) ||
           ((x_src + width) > img_width) || 
           ((y_src + height) > img_height)) {
          success = KNI_FALSE; //KNI_ThrowNew(midpIllegalArgumentException, NULL);
        } else {
	  jshort clip[4]; /* Defined in Graphics.java as 4 shorts */

	  const java_imagedata * dstMutableImageDataPtr = 
	    GXAPI_GET_IMAGEDATA_PTR_FROM_GRAPHICS(g);

	  GXAPI_TRANSLATE(g, x_dest, y_dest);
	  GXAPI_GET_CLIP(g, clip);

	  gx_render_imageregion(srcImageDataPtr, dstMutableImageDataPtr,
				clip, 
				x_src, y_src, 
				width, height,
				x_dest, y_dest, 
				transform);
        }
      }
    }

    KNI_EndHandles();
    KNI_ReturnBoolean(success);
}

#ifndef UNDER_CE
/*
 * Dummy functions for native soft buttons and menus
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_chameleon_layers_SoftButtonLayer_setNativeSoftButton) {
    KNI_ReturnVoid();
}


KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_chameleon_layers_SoftButtonLayer_setNativePopupMenu) {
    KNI_ReturnVoid();
}

/*
 * These are dummy functions for testing native text editor. You can
 * change src/configuration/linux_fb/skin.xml to have TEXTFIELD_NATIVE_EDITOR=1
 * and then look at the stdout for the messages sent by TextFieldLFImpl.java
 */

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_TextFieldLFImpl_enableNativeEditor) {
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_TextFieldLFImpl_setNativeEditorContent) {
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_TextFieldLFImpl_disableNativeEditor) {
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(javax_microedition_lcdui_TextFieldLFImpl_getNativeEditorContent) {
    KNI_StartHandles(1);
    KNI_DeclareHandle(temp);
    KNI_ReleaseHandle(temp);
    KNI_EndHandlesAndReturnObject(temp);
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(javax_microedition_lcdui_TextFieldLFImpl_getNativeEditorCursorIndex) {    
    KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_chameleon_MIDPWindow_disableAndSyncNativeEditor) {
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(javax_microedition_lcdui_TextFieldLFImpl_mallocToJavaChars) {
    KNI_StartHandles(1);
    KNI_DeclareHandle(temp);
    KNI_ReleaseHandle(temp);
    KNI_EndHandlesAndReturnObject(temp);
}
#endif
