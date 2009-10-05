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

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif
 
#define LIME_PACKAGE "com.sun.kvem.midp"
#define LIME_GRAPHICS_CLASS "GraphicsBridge"

#define MAX_OVERLAYS          16

/* if keying is used, video overrides only pixels of this color  */
/* note that one single key color is shared by all overlays      */
static javacall_pixel lcd_key_color; 

/* Single overlay channel information. */
typedef struct _OVERLAY
{
    BOOL             in_use;      /* TRUE if overlay is in use                                                   */
    javacall_pixel*  video;       /* pointer to current video frame or NULL if no video is being output          */
    BOOL             use_keying;  /* if TRUE, video is output only to those pixels where color matches key_color */
    int              x;           /* video rectangle left side (not clipped)                                     */
    int              y;           /* video rectangle top side (not clipped)                                      */
    int              w;           /* video rectangle width (not clipped)                                         */
    int              h;           /* video rectangle height (not clipped)                                        */
} OVERLAY;

/* This is logical LCDUI putpixel screen buffer. */
static struct {
    javacall_pixel*  hdc;         /* main offsreen buffer */
    javacall_pixel*  hdc_rotated; /* temporary buffer, used if emulator is rotated and/or updside down */
    char*            mask;        /* if video is output with keying, holds mask derived from hdc contents and key color */
    javacall_pixel*  composite;   /* temporary buffer for video compositing */
    int              width;
    int              height;
    int              full_height; /* screen height in full screen mode */
    int              num_pixels;
    CRITICAL_SECTION cs;          /* used to synchronize video output with UI output */
    OVERLAY          overlay[ MAX_OVERLAYS ];
} VRAM;

static javacall_bool inFullScreenMode;
static javacall_bool isLCDActive = JAVACALL_FALSE;
/* If lcd display is rotated */
static javacall_bool isLCDRotated = JAVACALL_FALSE;
/* If emulator skin is rotated */
static javacall_bool isPhoneRotated = JAVACALL_FALSE;
/* if display content should be turned upside down */
static javacall_bool top_down;
/* If clamshell is opened */
static javacall_bool clamshell_opened;

static void rotate_offscreen_buffer(javacall_pixel* dst, const javacall_pixel *src,
                                    int src_width, int src_height);

const static int MAIN_DISPLAY_ID = 0;
const static int EXTE_DISPLAY_ID = 1;
static int currDisplayId;
static struct {
    int width;
    int height;
    int full_height; /* screen height in full screen mode */
} MAIN_DISPLAY_SIZE, EXTE_DISPLAY_SIZE;

/**
 * Initialize LCD API
 * Will be called once the Java platform is launched
 *
 * @return <tt>1</tt> on success, <tt>0</tt> on failure
 */
javacall_result javacall_lcd_init(void) {
	static LimeFunction *f = NULL;

	int *res, resLen, i;

	if (isLCDActive) {
		javautil_debug_print(JAVACALL_LOG_WARNING, "lcd", "javacall_lcd_init called more than once. Ignored\n");
		return JAVACALL_OK;
	}

    isLCDActive = JAVACALL_TRUE;
    inFullScreenMode = JAVACALL_FALSE;
    top_down = JAVACALL_FALSE;
    clamshell_opened = JAVACALL_TRUE;

#ifdef ENABLE_WTK

    f = NewLimeFunction(LIME_PACKAGE,
                        LIME_GRAPHICS_CLASS,
                       "getDisplayParams");
    f->call(f, &res, &resLen);

    VRAM.width = res[0];
    VRAM.height = res[1];
    VRAM.full_height = res[2];

    VRAM.num_pixels = VRAM.width * VRAM.full_height;

#else

	f = NewLimeFunction(LIME_PACKAGE,
						LIME_GRAPHICS_CLASS,
						"getScreenParams");
  
	f->call(f, &res, &resLen, MAIN_DISPLAY_ID);

    MAIN_DISPLAY_SIZE.width = res[0];
	MAIN_DISPLAY_SIZE.height = res[1];
	MAIN_DISPLAY_SIZE.full_height = res[2];

	f->call(f, &res, &resLen, EXTE_DISPLAY_ID);

    EXTE_DISPLAY_SIZE.width = res[0];
	EXTE_DISPLAY_SIZE.height = res[1];
	EXTE_DISPLAY_SIZE.full_height = res[2];

    currDisplayId = MAIN_DISPLAY_ID;

    VRAM.width = MAIN_DISPLAY_SIZE.width;
    VRAM.height = MAIN_DISPLAY_SIZE.height;
    VRAM.full_height = MAIN_DISPLAY_SIZE.full_height;
    {
        int mainBufferSize = MAIN_DISPLAY_SIZE.width * MAIN_DISPLAY_SIZE.full_height;
        int exteBufferSize = EXTE_DISPLAY_SIZE.width * EXTE_DISPLAY_SIZE.full_height;
        /* External and main display are not available simultaneously,
         * so we will use one screen buffer.  
         */
        VRAM.num_pixels = (mainBufferSize > exteBufferSize) ? mainBufferSize : exteBufferSize;
    }
#endif

    VRAM.hdc = (javacall_pixel*)malloc(VRAM.num_pixels * sizeof(javacall_pixel));
    memset(VRAM.hdc, 0x11, VRAM.num_pixels * sizeof(javacall_pixel));
    /* assuming for win32_emul configuration we have no need to minimize memory consumption */
    VRAM.hdc_rotated = (javacall_pixel*)malloc(VRAM.num_pixels * sizeof(javacall_pixel));
    memset(VRAM.hdc_rotated, 0x11, VRAM.num_pixels * sizeof(javacall_pixel));

    for( i = 0; i < MAX_OVERLAYS; i++ )
    {
        VRAM.overlay[ i ].in_use     = FALSE;
        VRAM.overlay[ i ].video      = NULL;
        VRAM.overlay[ i ].use_keying = FALSE;
    }

    VRAM.mask      = malloc(VRAM.num_pixels);
    VRAM.composite = (javacall_pixel*)malloc(VRAM.num_pixels * sizeof(javacall_pixel));

    InitializeCriticalSection( &VRAM.cs );

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
        if( NULL != VRAM.mask ) {
            free(VRAM.mask);
            VRAM.mask = NULL;
        }
        if( NULL != VRAM.composite ) {
            free(VRAM.composite);
            VRAM.composite = NULL;
        }
        isLCDActive = JAVACALL_FALSE;
        DeleteCriticalSection( &VRAM.cs );
    }
    return JAVACALL_OK;
} 

/**
 * Get screen raster pointer
 *
 * @param hardwareId unique hardware screen id
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
javacall_pixel* javacall_lcd_get_screen(int hardwareId,
                                        int* screenWidth,
                                        int* screenHeight,
                                        javacall_lcd_color_encoding_type* colorEncoding) {

    /* as status bar does not rotate client area rotates without distortion */
  (void)hardwareId;
    if (isPhoneRotated) {
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
 * Blends video frame from overlay channel into offscreen buffer
 * if keying is used, uses VRAM.mask to determine visible video pixels
 * VRAM.mask is generated in javacall_lcd_flush.
 * @param ovl_n         overlay channel number
 * @param buffer target offscreen buffer
 */
static void blend_video_into_buffer( int ovl_n, javacall_pixel* buffer ) {
    int x, y;

    int dst_x0 = VRAM.overlay[ ovl_n ].x;
    int dst_y0 = VRAM.overlay[ ovl_n ].y;
    int src_x0 = 0;
    int src_y0 = 0;
    int nx     = VRAM.overlay[ ovl_n ].w;
    int ny     = VRAM.overlay[ ovl_n ].h;
    int dstw   = (isLCDRotated ? VRAM.full_height : VRAM.width );
    int dsth   = (isLCDRotated ? VRAM.width : VRAM.full_height );

    if( dst_x0 < 0 ) {
        nx -= -dst_x0;
        src_x0 += -dst_x0;
        dst_x0 = 0;
    }

    if( dst_y0 < 0 ) {
        ny -= -dst_y0;
        src_y0 += -dst_y0;
        dst_y0 = 0;
    }

    if( dst_x0 + nx > dstw ) {
        nx -= ( dst_x0 + nx - dstw );
    }

    if( dst_y0 + ny > dsth ) {
        ny -= ( dst_y0 + ny - dsth );
    }

    for( y = 0; y < ny; y++ ) {
        for( x = 0; x < nx; x++ ) {
            int dst_idx = dstw * ( dst_y0 + y ) + dst_x0 + x;

            if( !VRAM.overlay[ ovl_n ].use_keying || VRAM.mask[ dst_idx ] ) {
                buffer[ dst_idx ] =
                    VRAM.overlay[ ovl_n ].video[ 
                              VRAM.overlay[ ovl_n ].w * ( src_y0 + y ) 
                            + src_x0 + x ];
            }
        }
    }
}

/**
 * Checks colors of VRAM.hdc pixels and creates mask by comaring them with lcd_key_color.
 * also creates a copy of VRAM.hdc in VRAM.composite -- for blending video into it.
 */
static void prepare_video_mask_and_composite() {

    int idx;

    for( idx = 0; idx < VRAM.num_pixels; idx++ ) {
        VRAM.mask[ idx ] = (lcd_key_color == VRAM.hdc[ idx ]);
    }

    memcpy( VRAM.composite, VRAM.hdc, VRAM.num_pixels * sizeof(javacall_pixel) );
}


/**
 * Outputs offscreen buffer to device emulator screen.
 * @param hardwareId unique hardware screen id
 * @param buffer     source offscreen buffer (already rotated)
 */
static void send_buffer_to_device( int hardwareId, javacall_pixel* buffer ) {

    static LimeFunction *f = NULL;
    static LimeFunction *f1 = NULL;

    javacall_pixel* hdc;

    short clip[4] = {0,0,VRAM.width, VRAM.height};

    (void)hardwareId;

    if( inFullScreenMode ) {
        clip[3] = VRAM.full_height;
    }

    if (isLCDRotated || top_down) {
        rotate_offscreen_buffer(VRAM.hdc_rotated, buffer,
                                inFullScreenMode ? VRAM.full_height : VRAM.height,
                                VRAM.width);
        hdc = VRAM.hdc_rotated;
    } else {
        hdc = buffer;
    }

#ifdef ENABLE_WTK

    f = NewLimeFunction(LIME_PACKAGE,
        LIME_GRAPHICS_CLASS,
        "drawRGB16");

    if(inFullScreenMode) {
        f->call(f, NULL, 0, 0, clip, 4, 0, hdc, (VRAM.width * VRAM.full_height) << 1, 0, 0, VRAM.width, VRAM.full_height);
    } else {
        f->call(f, NULL, 0, 0, clip, 4, 0, hdc, (VRAM.width * VRAM.height) << 1, 0, 0, VRAM.width, VRAM.height);
    }

    f1 = NewLimeFunction(LIME_PACKAGE,
        LIME_GRAPHICS_CLASS,
        "refresh");
    if(inFullScreenMode) {
        f1->call(f1, NULL, 0, 0, VRAM.width, VRAM.full_height);
    } else {
        f1->call(f1, NULL, 0, 0, VRAM.width, VRAM.height);
    }

#else

    if( NULL == f )
    {
        f  = NewLimeFunction(LIME_PACKAGE, LIME_GRAPHICS_CLASS, "drawRGB16Buffer");
        f1 = NewLimeFunction(LIME_PACKAGE, LIME_GRAPHICS_CLASS, "refreshDisplay");
    }

    if(inFullScreenMode) {
        f->call(f, NULL, currDisplayId, 0, 0, clip, 4, 0, hdc, 
            (VRAM.width * VRAM.full_height) << 1, 0, 0, 
            VRAM.width, VRAM.full_height);
        f1->call(f1, NULL, currDisplayId, 0, 0, VRAM.width, VRAM.full_height);
    } else {
        f->call(f, NULL, currDisplayId, 0, 0, clip, 4, 0, hdc, 
            (VRAM.width * VRAM.height) << 1, 0, 0, 
            VRAM.width, VRAM.height);
        f1->call(f1, NULL, currDisplayId, 0, 0, VRAM.width, VRAM.height);
    }

#endif
}

/**
 * Used internally by MMAPI to allocate overlay channel.
 * @return number of allcoated channel for use in subsequent calls 
 *         or -1 if no channel is available
 */
int lcd_open_overlay()
{
    int i;
    EnterCriticalSection( &VRAM.cs );

    for( i = 0; i < MAX_OVERLAYS; i++ )
    {
        if( !VRAM.overlay[ i ].in_use )
        {
            VRAM.overlay[ i ].in_use = TRUE;
            LeaveCriticalSection( &VRAM.cs );
            return i;
        }
    }

    LeaveCriticalSection( &VRAM.cs );
    return -1;
}

/**
 * Used internally by MMAPI to close overlay channel.
 * @param ovl_n         overlay channel number
 */
void lcd_close_overlay( int ovl_n )
{
    EnterCriticalSection( &VRAM.cs );
    VRAM.overlay[ ovl_n ].video      = NULL;
    VRAM.overlay[ ovl_n ].in_use     = FALSE;
    VRAM.overlay[ ovl_n ].use_keying = FALSE;
    LeaveCriticalSection( &VRAM.cs );
}

/**
 * Used internally by win32_emul MMAPI. Sets "keying color" -- if keying 
 * is enabled, only pixels of that color will be overwritten by video.
 * IMPL_NOTE: one single key color is shared by all overlays
 * @param ovl_n      overlay channel number
 * @param use_keying indicates whether keying is enabled
 * @param key_color  color used to indicate overwritable pixels
 */
void lcd_set_color_key( int ovl_n, javacall_bool use_keying, javacall_pixel key_color )
{
    EnterCriticalSection( &VRAM.cs );
    VRAM.overlay[ ovl_n ].use_keying = ( JAVACALL_TRUE == use_keying );

    if( VRAM.overlay[ ovl_n ].use_keying ) lcd_key_color  = key_color;

    prepare_video_mask_and_composite();

    LeaveCriticalSection( &VRAM.cs );
}

/**
 * Used internally by MMAPI to set rectangle to output video to.
 * Rectangle here is unclipped. clipping occurs later in blend_video_into_buffer()
 * @param ovl_n  overlay channel number
 * @param x      left rectangle side
 * @param y      top rectangle side
 * @param w      rectangle width
 * @param h      rectangle height
 */
void lcd_set_video_rect( int ovl_n, int x, int y, int w, int h )
{
    EnterCriticalSection( &VRAM.cs );
    VRAM.overlay[ ovl_n ].x = x;
    VRAM.overlay[ ovl_n ].y = y;
    VRAM.overlay[ ovl_n ].w = w;
    VRAM.overlay[ ovl_n ].h = h;
    LeaveCriticalSection( &VRAM.cs );
}

/**
 * Used internally by MMAPI to composite decoded video into offscreen buffer.
 * @param ovl_n overlay channel number
 * @param video pointer to video frame buffer. buffer contents are NOT copied,
 *              so calling party must call this function with another value
 *              (next frame address or NULL) before releasing refernced memory.
 *              if video is NULL, video output is disabled.
 */
void lcd_output_video_frame( int ovl_n, javacall_pixel* video ) {

    int i;

    EnterCriticalSection( &VRAM.cs );

    VRAM.overlay[ ovl_n ].video = video;

    for( i = 0; i < MAX_OVERLAYS; i++ ) {
        if( NULL != VRAM.overlay[ i ].video ) {
            blend_video_into_buffer( i, VRAM.composite );
        }
    }

    send_buffer_to_device( javacall_lcd_get_current_hardwareId(), VRAM.composite );

    LeaveCriticalSection( &VRAM.cs );
}

/**
 * The following function is used to flush the image from the Video RAM raster to
 * the LCD display. \n
 * The function call should not be CPU time expensive, and should return
 * immediately. It should avoid memory bulk memory copying of the entire raster.
 *
 * @param hardwareId uniue hardware screen id
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_lcd_flush(int hardwareId) {

    javacall_pixel* hdc;
    int             i;

    EnterCriticalSection( &VRAM.cs );
	 
    // javacall_lcd_flush() may be called again without VRAM.hdc being updated.
    // if we blend video directly into VRAM.hdc, second call with unchanged
    // VRAM.hdc will erase video mask. Hence separate VRAM.composite buffer.
    prepare_video_mask_and_composite();

    for( i = 0; i < MAX_OVERLAYS; i++ ) {
        if( NULL != VRAM.overlay[ i ].video ) {
            blend_video_into_buffer( i, VRAM.composite );
        }
    }

    send_buffer_to_device( hardwareId, VRAM.composite );

    LeaveCriticalSection( &VRAM.cs );

    return JAVACALL_OK;
}

/**
 * Rotates offscreen buffer. Used if emulator is rotated and/or turned upside down
 * @param src        source buffer address
 * @param dst        destination buffer address
 * @param src_width  source buffer width in pixels
 * @param src_height destination buffer width in pixels
 */
static void rotate_offscreen_buffer(javacall_pixel* dst, const javacall_pixel *src, 
                                    int src_width, int src_height) {
    int buffer_length = src_width*src_height;
    if (isLCDRotated) {
        if (!top_down) {
            const javacall_pixel *src_end = src + buffer_length;
            javacall_pixel       *dst_end = dst + buffer_length;
            dst += src_height - 1;
            while (src < src_end) {
                *dst = *(src++);
                dst += src_height;
                if (dst >= dst_end) {
                    dst -= buffer_length + 1;
                }
            }
        } else {
            javacall_pixel *dst_end = dst + buffer_length;
            const javacall_pixel *src_start = src;
            dst += src_height - 1;
            src += buffer_length - 1;
            while (src >= src_start) {
                *dst = *(src--);
                dst += src_height;
                if (dst >= dst_end) {
                    dst -= buffer_length + 1;
                }
            }
        }
    } else {
        if (top_down) {
            const javacall_pixel *src_end = src + buffer_length;
            javacall_pixel       *dst_start = dst;
            dst += buffer_length - 1;
            while (src < src_end) {
                *(dst--) = *(src++);
                if (dst < dst_start) {
                    dst += buffer_length - 1;
                }
            }
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
 * @param hardwareId uniue hardware screen id
 * @param ystart start vertical scan line to start from
 * @param yend last vertical scan line to refresh
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail 
 */
javacall_result /*OPTIONAL*/ javacall_lcd_flush_partial(int hardwareId, int ystart, int yend){
	javacall_lcd_flush(hardwareId);
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
javacall_result javacall_lcd_set_full_screen_mode(int hardwareId,javacall_bool useFullScreen) {
    static LimeFunction *f = NULL;
    inFullScreenMode = useFullScreen;
    (void)hardwareId;
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

void on_screen_rotated() {
      static LimeFunction *f = NULL;
      f = NewLimeFunction(LIME_PACKAGE, LIME_GRAPHICS_CLASS, "screenRotated");
      f->call(f, NULL, currDisplayId, isPhoneRotated, top_down);
}

/**
 * Rotates display according to code.
 * If code is 0 no screen transformations made;
 * If code is 1 then screen orientation is reversed.
 * if code is 2 then screen is turned upside-down.
 * If code is 3 then both screen orientation is reversed
 * and screen is turned upside-down.
 */
void RotateDisplay(short code) {
    top_down = code & 0x02;
    if ((code & 0x01) != isPhoneRotated) {
        isPhoneRotated = !isPhoneRotated;
        javanotify_rotation(javacall_lcd_get_current_hardwareId());
    }
    javacall_lcd_flush(javacall_lcd_get_current_hardwareId());
    on_screen_rotated();
}
/**
 * Called by the system when lcdui display changed its orientation.
 */
javacall_bool javacall_lcd_reverse_orientation(int hardwareId) {
      (void)hardwareId;
      isLCDRotated = !isLCDRotated;
      return isLCDRotated;
}

/**
 * Called when clamshell state changed
 * to switch from internal display to external
 * or vice versa.
 */
void ClamshellStateChanged(short state) {
#ifndef ENABLE_WTK
    if (state == 2) {
       /* IMPL_NOTE: two displays are active - subject to implement.
        * For now we move to main display. 
        */
        if (clamshell_opened == JAVACALL_FALSE) {
            javanotify_clamshell_state_changed(JAVACALL_LCD_CLAMSHELL_OPEN);
            clamshell_opened = JAVACALL_TRUE;
        } 
    } else if (state == 0) {
        if (clamshell_opened == JAVACALL_FALSE) {
           javanotify_clamshell_state_changed(JAVACALL_LCD_CLAMSHELL_OPEN);
           clamshell_opened = JAVACALL_TRUE;
        }
    } else {
        if (clamshell_opened == JAVACALL_TRUE) {
           javanotify_clamshell_state_changed(JAVACALL_LCD_CLAMSHELL_CLOSE);
           clamshell_opened = JAVACALL_FALSE;
        }
    }
#endif
}


/**
 * Handle clamshell event
 */
void javacall_lcd_handle_clamshell() {

#ifndef ENABLE_WTK
    if (clamshell_opened == JAVACALL_TRUE
            && currDisplayId == EXTE_DISPLAY_ID) {
        VRAM.width = MAIN_DISPLAY_SIZE.width;
        VRAM.height = MAIN_DISPLAY_SIZE.height;
        VRAM.full_height = MAIN_DISPLAY_SIZE.full_height;
        currDisplayId = MAIN_DISPLAY_ID;
        javacall_lcd_flush(currDisplayId);                
    } else if (clamshell_opened == JAVACALL_FALSE 
                   && currDisplayId == MAIN_DISPLAY_ID) {
        VRAM.width = EXTE_DISPLAY_SIZE.width;
        VRAM.height = EXTE_DISPLAY_SIZE.height;
        VRAM.full_height = EXTE_DISPLAY_SIZE.full_height;
        currDisplayId = EXTE_DISPLAY_ID;
        javacall_lcd_flush(currDisplayId);                
    }
#endif
}

/**
 * Reverse flag of rotation
 * @param hardwareId unique id of hardware display
 */
 
javacall_bool javacall_lcd_get_reverse_orientation(int hardwareId) {
    (void)hardwareId;
     return isPhoneRotated;
}
  
int javacall_lcd_get_screen_height(int hardwareId) {
  (void)hardwareId;
    if (isPhoneRotated) {
        return VRAM.width;
    } else {
        if(inFullScreenMode) {
            return VRAM.full_height;
        } else {
            return VRAM.height;
        }
    }
}
  
int javacall_lcd_get_screen_width(int hardwareId) {
  (void)hardwareId;
    if (isPhoneRotated) {
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
#ifdef ENABLE_NATIVE_SOFTBUTTONS
    return JAVACALL_TRUE;
#else /* ENABLE_NATIVE_SOFTBUTTONS */
    return JAVACALL_FALSE;
#endif /* ENABLE_NATIVE_SOFTBUTTONS */
}

/*
 * Translates screen coordinates into displayable coordinate system.
 */
void getScreenCoordinates(short screenX, short screenY, short* x, short* y) {
    *x = screenX;
    *y = screenY;

    if (top_down) {
        *x = VRAM.width - screenX;
        if(inFullScreenMode) {
            *y = VRAM.full_height - screenY;
        } else {
            *y = VRAM.height - screenY;
        }
    }

    if (isPhoneRotated) {
        short prevX = *x;
        *x = *y;
        *y = VRAM.width - prevX;
    }
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

/**
 * get currently enabled hardware display id
 */
int javacall_lcd_get_current_hardwareId() {
  return 0;
}
/** 
 * Get display device name by id
 * @param hardwareId unique id of hardware screen
 */
char* javacall_lcd_get_display_name(int hardwareId) {
  (void)hardwareId;
  return NULL;
}


/**
 * Check if the display device is primary
 * @param hardwareId unique id of hardware screen
 */
javacall_bool javacall_lcd_is_display_primary(int hardwareId) {
    (void)hardwareId;
    return JAVACALL_TRUE;
}

/**
 * Check if the display device is build-in
 * @param hardwareId unique id of hardware screen
 */
javacall_bool javacall_lcd_is_display_buildin(int hardwareId) {
    (void)hardwareId; 
    return JAVACALL_TRUE;
}

/**
 * Check if the display device supports pointer events
 * @param hardwareId unique id of hardware screen
 */
javacall_bool javacall_lcd_is_display_pen_supported(int hardwareId) {
    (void)hardwareId; 
    return JAVACALL_TRUE;
}

/**
 * Check if the display device supports pointer motion  events
 * @param hardwareId unique id of hardware screen
 */
javacall_bool javacall_lcd_is_display_pen_motion_supported(int hardwareId){
    (void)hardwareId; 
    return JAVACALL_TRUE;
}

/**
 * Get display device capabilities
 * @param hardwareId unique id of hardware screen
 */
int javacall_lcd_get_display_capabilities(int hardwareId) {
  return 255;
}

static int screen_ids[] =
{
  0
};

/**
 * Get the list of screen ids
 * @param return number of screens 
 * @return the lit of ids 
 */
int* javacall_lcd_get_display_device_ids(int* n) {
  *n = 1;
    return screen_ids;
}
    
#ifdef __cplusplus
} //extern "C"
#endif
