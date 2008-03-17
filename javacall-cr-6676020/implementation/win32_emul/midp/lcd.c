/*
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

#include "javacall_lcd.h" 
#include "javacall_logging.h" 
#include "lime.h"
#include "string.h"
#include "stdlib.h"
#include "javacall_penevent.h"
#include "javacall_keypress.h"

#ifdef __cplusplus
extern "C" {
#endif
 
#define LIME_PACKAGE "com.sun.kvem.midp"
#define LIME_GRAPHICS_CLASS "GraphicsBridge"

/* This is logical LCDUI putpixel screen buffer. */
static struct {
    javacall_pixel* hdc;
    javacall_pixel* hdc_rotated;
    int width;
    int height;
    int full_height; /* screen height in full screen mode */
} VRAM;

static javacall_bool inFullScreenMode;
static javacall_bool isLCDActive = JAVACALL_FALSE;
static javacall_bool reverse_orientation;

static void rotate_offscreen_buffer(javacall_pixel* dst, javacall_pixel *src, int src_width, int src_height);

/**
 * Initialize LCD API
 * Will be called once the Java platform is launched
 *
 * @return <tt>1</tt> on success, <tt>0</tt> on failure
 */
javacall_result javacall_lcd_init(void) {
	static LimeFunction *f = NULL;

	int *res, resLen;

	if (isLCDActive) {
		javautil_debug_print(JAVACALL_LOG_WARNING, "lcd", "javacall_lcd_init called more than once. Ignored\n");
		return JAVACALL_OK;
	}

    isLCDActive = JAVACALL_TRUE;
    inFullScreenMode = JAVACALL_FALSE;

	f = NewLimeFunction(LIME_PACKAGE,
						LIME_GRAPHICS_CLASS,
						"getDisplayParams");
  
	f->call(f, &res, &resLen);

	VRAM.width = res[0];
	VRAM.height = res[1];
	VRAM.full_height = res[2];

	VRAM.hdc = (javacall_pixel*)malloc(VRAM.width * VRAM.full_height * sizeof(javacall_pixel));
        memset(VRAM.hdc,0x11,VRAM.width * VRAM.full_height * sizeof(javacall_pixel));
        /* assuming for win32_emul configuration we have no need to minimize memory consumption */
	VRAM.hdc_rotated = (javacall_pixel*)malloc(VRAM.width * VRAM.full_height * sizeof(javacall_pixel));
    return JAVACALL_OK;
}


/**
 * The function javacall_lcd_finalize is called by during Java VM shutdown, 
 * allowing the  * platform to perform device specific lcd-related shutdown
 * operations.  
 * The VM guarantees not to call other lcd functions before calling 
 * javacall_lcd_init( ) again.
 *                
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_lcd_finalize(void){

    if(isLCDActive) {
        free(VRAM.hdc_rotated);
        VRAM.hdc_rotated = NULL;
        free(VRAM.hdc);
        VRAM.hdc = NULL;
        isLCDActive = JAVACALL_FALSE;
    }
    return JAVACALL_OK;
} 

/**
 * Get screen raster pointer
 *
 * @param screenType can be any of the following types:
 * <ul>
 *   <li> <code>JAVACALL_LCD_SCREEN_PRIMARY</code> -
 *        return primary screen size information </li>
 *   <li> <code>JAVACALL_LCD_SCREEN_EXTERNAL</code> -
 *        return external screen size information if supported </li>
 * </ul>
 * @param screenWidth output paramter to hold width of screen
 * @param screenHeight output paramter to hold height of screen
 * @param colorEncoding output paramenter to hold color encoding,
 *        which can take one of the following:
 *              -# JAVACALL_LCD_COLOR_RGB565
 *              -# JAVACALL_LCD_COLOR_ARGB
 *              -# JAVACALL_LCD_COLOR_RGB888
 *              -# JAVACALL_LCD_COLOR_OTHER
 *
 * @return pointer to video ram mapped memory region of size
 *         ( screenWidth * screenHeight )
 *         or <code>NULL</code> in case of failure
 */
javacall_pixel* javacall_lcd_get_screen(javacall_lcd_screen_type screenType,
                                        int* screenWidth,
                                        int* screenHeight,
                                        javacall_lcd_color_encoding_type* colorEncoding) {

    /* as status bar does not rotate client area rotates without distortion */
    if (reverse_orientation) {
        int* temp = screenWidth;
        screenWidth = screenHeight;
        screenHeight = temp;
        
    }

    if(screenWidth) {
        *screenWidth = VRAM.width;
    }

    if(screenHeight) {
        if(inFullScreenMode) {
            *screenHeight = VRAM.full_height;
        } else {
            *screenHeight = VRAM.height;
        }
    }
    if(colorEncoding) {
        *colorEncoding =JAVACALL_LCD_COLOR_RGB565;
    }

    return VRAM.hdc;
}
    
    
/**
 * The following function is used to flush the image from the Video RAM raster to
 * the LCD display. \n
 * The function call should not be CPU time expensive, and should return
 * immediately. It should avoid memory bulk memory copying of the entire raster.
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_lcd_flush(void) {
    static LimeFunction *f = NULL;
    static LimeFunction *f1 = NULL;
    short clip[4] = {0,0,VRAM.width, VRAM.height};
    javacall_pixel *current_hdc;
	 
    if(inFullScreenMode) {
        clip[3] = VRAM.full_height;
    }

    if (reverse_orientation) {
        current_hdc = VRAM.hdc_rotated;
        rotate_offscreen_buffer(current_hdc,
                                VRAM.hdc,
                                inFullScreenMode ? VRAM.full_height : VRAM.height,
                                VRAM.width);
    } else {
        current_hdc = VRAM.hdc;
    }

    if (f == NULL) {
        f = NewLimeFunction(LIME_PACKAGE,
                            LIME_GRAPHICS_CLASS,
                            "drawRGB16");
    }
  
    if(inFullScreenMode) {
        f->call(f, NULL, 0, 0, clip, 4, 0, current_hdc, (VRAM.width * VRAM.full_height) << 1, 0, 0, VRAM.width, VRAM.full_height);
    } else {
        f->call(f, NULL, 0, 0, clip, 4, 0, current_hdc, (VRAM.width * VRAM.height) << 1, 0, 0, VRAM.width, VRAM.height);
    }

    if (f1==NULL)
    {
        f1 = NewLimeFunction(LIME_PACKAGE,
                             LIME_GRAPHICS_CLASS,
                             "refresh");
    }
    if(inFullScreenMode) {
        f1->call(f1, NULL, 0, 0, VRAM.width, VRAM.full_height);
    } else {
        f1->call(f1, NULL, 0, 0, VRAM.width, VRAM.height);
    }

    return JAVACALL_OK;
}
    

static void rotate_offscreen_buffer(javacall_pixel* dst, javacall_pixel *src, int src_width, int src_height) {
    int buffer_length = src_width*src_height;
    javacall_pixel *src_end = src + buffer_length;
    javacall_pixel *dst_end = dst + buffer_length;

    dst += src_height - 1;
    while (src < src_end) {
        *dst = *(src++);
        dst += src_height;
        if (dst >= dst_end) {
            dst -= buffer_length + 1;
        }
    }
}




/**
 * The following native call implements an efficient bulk video memory erasure,
 * and should make use of existing hardware acceleration, either DMA copy or
 * graphics co-processor activation or misc acclerated assembly language
 * implementation
 * 
 * @param offsetInVram offset in number of pixels to start from
 * @param numberOfPixels number of Pixels to clear
 * @param color color to use for clearing 
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_lcd_set_pixels(int offsetInVram, int numberOfPixels, javacall_pixel color){
    return JAVACALL_FAIL;
}

/**
 * Copy a source off-screen image to video RAM
 * This sub section defines a API for blitting memory-mapped raster images to the
 * Video RAM (VRAM). This function should take advantage of Graphics coprocessor
 * and/or DMA to improve performance.
 *
 * @param destScreenPtr a pointer to destination screen memory block 
 *                      (such asthe return value of javacall_lcd_get_screen() )
 * @param srcImage a pointer to memory block holding source image to blit
 * @param imageWidth image width in pixels
 * @param imageHeight image height in pixels, such that the image
 *        pointer points to a memory area of the size
 *        (imageWidth*imageHeight*sizeof(javacall_pixel))
 * @param x x position on VRAM screen to blit to
 * @param y y position on VRAM screen to blit to
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_lcd_bitblit(javacall_pixel* destScreenPtr, 
                          javacall_pixel* srcImage, int imageWidth, int imageHeight, int x, int y){
    return JAVACALL_FAIL;
}
    
/**
 * Flush the screen raster to the display. 
 * This function should not be CPU intensive and should not perform bulk memory
 * copy operations.
 * The following API uses partial flushing of the VRAM, thus may reduce the
 * runtime of the expensive flush operation: It should be implemented on
 * platforms that support it
 * 
 * @param ystart start vertical scan line to start from
 * @param yend last vertical scan line to refresh
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail 
 */
javacall_result /*OPTIONAL*/ javacall_lcd_flush_partial(int ystart, int yend){
	javacall_lcd_flush();
    return JAVACALL_FAIL;
}
    

javacall_pixel* 
javacall_sprint_get_external_screen(
									void) {
	return NULL;
}

javacall_result 
javacall_sprint_init_external(
    int* screenWidth, 
    int* screenHeight, 
    javacall_lcd_color_encoding_type* colorEncoding
    ) {
	   return JAVACALL_FAIL;
}

javacall_result 
javacall_sprint_flush_external(
    void
    ) {
	   return JAVACALL_FAIL;
}

javacall_result 
javacall_sprint_finalize_external(
    void
    ) {
	 return JAVACALL_FAIL;
}
    
int 
javacall_lcd_get_annunciator_height(
    void
    ) {
	return 0;
}

/**
 * Set or unset full screen mode.
 *
 * This function should return <code>JAVACALL_FAIL</code> if full screen mode
 * is not supported.
 * Subsequent calls to <code>javacall_lcd_get_screen()</code> will return
 * a pointer to the relevant offscreen pixel buffer of the corresponding screen
 * mode as well s the corresponding screen dimensions, after the screen mode has
 * changed.
 *
 * @param useFullScreen if <code>JAVACALL_TRUE</code>, turn on full screen mode.
 *                      if <code>JAVACALL_FALSE</code>, use normal screen mode.

 * @retval JAVACALL_OK   success
 * @retval JAVACALL_FAIL failure
 */
javacall_result javacall_lcd_set_full_screen_mode(javacall_bool useFullScreen) {
    static LimeFunction *f = NULL;
    inFullScreenMode = useFullScreen;
	
	if (f==NULL) {
		f = NewLimeFunction(LIME_PACKAGE,
							LIME_GRAPHICS_CLASS,
							"synGrabFullScreen");
	}
	f->call(f, NULL, inFullScreenMode);

    return JAVACALL_OK;
}

/*javacall_result 
javacall_lcd_set_screen_mode(
    javacall_lcd_mode mode
    ) {
	return JAVACALL_FAIL;
}*/

javacall_bool javacall_lcd_reverse_orientation() {
      reverse_orientation = !reverse_orientation;    
      return reverse_orientation;
}
 
javacall_bool javacall_lcd_get_reverse_orientation() {
     return reverse_orientation;
}
  
int javacall_lcd_get_screen_height() {
    if (reverse_orientation) {
        return VRAM.width;
    } else {
        if(inFullScreenMode) {
            return VRAM.full_height;
        } else {
            return VRAM.height;
        }
    }
}
  
int javacall_lcd_get_screen_width() {
    if (reverse_orientation) {
        if(inFullScreenMode) {
            return VRAM.full_height;
        } else {
            return VRAM.height;
        }
    } else {
        return VRAM.width;
    }
}

/**
 * checks the implementation supports native softbutton layer.
 * 
 * @retval JAVACALL_TRUE   implementation supports native softbutton layer
 * @retval JAVACALL_FALSE  implementation does not support native softbutton layer
 */
javacall_bool javacall_lcd_is_native_softbutton_layer_supported () {
    return JAVACALL_TRUE;
}

/**
 * The following function is used to set the softbutton label in the native
 * soft button layer.
 * 
 * @param label the label for the softbutton
 * @param len the length of the label
 * @param index the corresponding index of the command
 * 
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_lcd_set_native_softbutton_label (const javacall_utf16* label,
                                                          int len,
                                                          int index) {
    static LimeFunction *f = NULL;

	if (inFullScreenMode == JAVACALL_TRUE) {
		//returning, no need to paint here
		return JAVACALL_OK;
	}
	if (label == NULL || len < 0 || index < 0) {
	 	return JAVACALL_FAIL;
	} else {
	   if (f==NULL)  {
	       f = NewLimeFunction(LIME_PACKAGE,
	                            LIME_GRAPHICS_CLASS,
	                            "setSoftButtonLabel");
	   	}
		f->call(f, NULL, index, label, len); 
	}
   return JAVACALL_OK;
}

/**
 * Generate softkey event, if a pointer was pressed outside of the screen
 * visible to java.
 * 
 * Returns JAVACALL_TRUE if the event was consumed and softkey was generated 
 */

javacall_bool generateSoftButtonKeys(int x, int y, javacall_penevent_type pentype) {
    javacall_keypress_type keytype;

    if (inFullScreenMode) {
        return JAVACALL_FALSE;
    }
    
    switch (pentype) {
        case JAVACALL_PENPRESSED: keytype = JAVACALL_KEYRELEASED; break;
        case JAVACALL_PENRELEASED: keytype = JAVACALL_KEYPRESSED; break;
        default: return JAVACALL_FALSE;
    }

    if (y > VRAM.height ) {
        if( x < (VRAM.width /3 )) {
            javanotify_key_event(JAVACALL_KEY_SOFT1, keytype);
        }
        if( x > (2*VRAM.width / 3)) {
            javanotify_key_event(JAVACALL_KEY_SOFT2, keytype);
        }
		return JAVACALL_TRUE;
    } 

	return JAVACALL_FALSE;
    
}
    
#ifdef __cplusplus
} //extern "C"
#endif


