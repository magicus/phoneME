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

#include <midp_logging.h>
#include <armsdapp_export.h>
#include <armsdapp_ui.h>
#include <gxj_putpixel.h>
#include <midpMalloc.h>

gxj_screen_buffer gxj_system_screen_buffer;

/**
 * @file
 * Additional porting API for Java Widgets based port of abstract
 * command manager.
 */

/**
 * Initializes the ARMSD native resources.
 */
void armsdapp_init() {
    int screenSize = sizeof(gxj_pixel_type) * CHAM_WIDTH * CHAM_HEIGHT;
    
    // Check if we are attached to the VM memprof. If not, no
    // graphics will be displayed, so there's no point going any further
    *PROFILER_ATTACHED_REG = 0;
    if (*PROFILER_ATTACHED_REG != 1) {
        // If memprof is installed, we will read an 1 back after writing 0
        // to this register. If this is not the case, memprof is not installed.
        printf("*** Graphics not available. ADS memprof not installed. ***\n");
        printf("Did you install the customed mapfile.dll or mapfile.so?\n");
        printf("Did you have a valid armsd.map file in curent dir?\n");
        exit(0);
    }

    gxj_system_screen_buffer.width = CHAM_WIDTH;
    gxj_system_screen_buffer.height = CHAM_HEIGHT;
    gxj_system_screen_buffer.alphaData = 0;
    gxj_system_screen_buffer.pixelData = (gxj_pixel_type *)midpMalloc(screenSize);
    memset(gxj_system_screen_buffer.pixelData, 0, screenSize);    
}

/**
 * Finalize the ARMSD native resources.
 */
void armsdapp_finalize() {
    midpFree(gxj_system_screen_buffer.pixelData);
}

/**
 * Bridge function to request a repaint
 * of the area specified.
 *
 * @param x1 top-left x coordinate of the area to refresh
 * @param y1 top-left y coordinate of the area to refresh
 * @param x2 bottom-right x coordinate of the area to refresh
 * @param y2 bottom-right y coordinate of the area to refresh
 */
void armsdapp_refresh(int x1, int y1, int x2, int y2)
{
    gxj_pixel_type *src = gxj_system_screen_buffer.pixelData;
    gxj_pixel_type *dst = (gxj_pixel_type*)LCD_BUFFER_BASE;
    int srcWidth, srcHeight;

    // Make sure the copied lines are 4-byte aligned for faster memcpy
    if ((x1 & 2) == 1) {
        x1 -= 1;
    }
    if ((x2 & 2) == 1) {
        x2 += 1;
    }
    srcWidth = x2 - x1;
    srcHeight = y2 - y1;


    src += y1 * CHAM_WIDTH + x1;
    dst += y1 * CHAM_WIDTH + x1;

    if (x1 == 0 && srcWidth == CHAM_WIDTH) {
        __rt_memcpy_w(dst, src, srcWidth * sizeof(gxj_pixel_type) * srcHeight);
    } else {
        for (; y1 < y2; y1++) {
            __rt_memcpy_w(dst, src, srcWidth * sizeof(gxj_pixel_type));
            src += CHAM_WIDTH;
            dst += CHAM_WIDTH;
        }
    }

    *ACCEL_OPCODE_REG = ACCEL_OPCODE_REFRESH;
}

char * strdup(const char *s) {
    char *result = (char*)malloc(strlen(s)+1);
    if (result) {
        strcpy(result, s);
    }
    return result;
}
