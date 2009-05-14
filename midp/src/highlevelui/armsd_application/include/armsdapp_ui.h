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

#ifndef _ARMSDAPP_UI_H_
#define _ARMSDAPP_UI_H_

/**
 * @defgroup highui_armapp Armsd Application External Interface
 * @ingroup highui
 */

/**
 * @file
 * @ingroup highui_armapp
 *
 * @brief Register definitions to communicate with ARMSD.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define LCD_BUFFER_BASE             0x0c000000
#define LCD_BUFFER_LIMIT            0x0d000000

#define EVENT_TYPE_REG              ((volatile int*)0x0d000000)
#define EVENT_CHAR_REG              ((volatile int*)0x0d000004)
#define EVENT_X_REG                 ((volatile int*)0x0d000008)
#define EVENT_Y_REG                 ((volatile int*)0x0d00000c)

#define ACCEL_OPCODE_REG            ((volatile int*)0x0d000010)
#define ACCEL_X_REG                 ((volatile int*)0x0d000014)
#define ACCEL_Y_REG                 ((volatile int*)0x0d000018)
#define ACCEL_W_REG                 ((volatile int*)0x0d00001c)
#define ACCEL_H_REG                 ((volatile int*)0x0d000020)
#define ACCEL_PIXEL_REG             ((volatile int*)0x0d000024)
#define ACCEL_START_ANGLE_REG       ((volatile int*)0x0d000028)
#define ACCEL_ARC_ANGLE_REG         ((volatile int*)0x0d00002c)
#define ACCEL_DOTTED_REG            ((volatile int*)0x0d000030)
#define ACCEL_CLIPX_REG             ((volatile int*)0x0d000034)
#define ACCEL_CLIPY_REG             ((volatile int*)0x0d000038)
#define ACCEL_CLIPW_REG             ((volatile int*)0x0d00003c)
#define ACCEL_CLIPH_REG             ((volatile int*)0x0d000040)

#define PROFILER_ATTACHED_REG       ((volatile int*)0x0d100000)
#define PROFILER_CYCLES64_MSW_REG   ((volatile int*)0x0d100004)
#define PROFILER_CYCLES64_LSW_REG   ((volatile int*)0x0d100008)
#define PROFILER_FREQUENCY_REG      ((volatile int*)0x0d10000c)

#define ACCEL_OPCODE_FILLRECT       1
#define ACCEL_OPCODE_FILLARC        2
#define ACCEL_OPCODE_STARTSPLASH    3
#define ACCEL_OPCODE_ENDSPLASH      4
#define ACCEL_OPCODE_DRAWLINE       5
#define ACCEL_OPCODE_REFRESH        6

enum ADS_KVMEventTypes {
    ADS_invalidKVMEvent    = -2,
    ADS_appStopKVMEvent    = -1,
    ADS_keyDownKVMEvent    = 0,
    ADS_keyUpKVMEvent      = 1,
    ADS_keyRepeatKVMEvent  = 2,
    ADS_penDownKVMEvent    = 3,
    ADS_penUpKVMEvent      = 4,
    ADS_penMoveKVMEvent    = 5,
    ADS_timerKVMEvent      = 6,
    ADS_commandKVMEvent    = 7,
    ADS_repaintKVMEvent    = 8,
    ADS_keyTypedKVMEvent   = 9,
    ADS_lastKVMEvent       = 9
};

typedef enum {
    ADS_KEY_0        = '0',
    ADS_KEY_1        = '1',
    ADS_KEY_2        = '2',
    ADS_KEY_3        = '3',
    ADS_KEY_4        = '4',
    ADS_KEY_5        = '5',
    ADS_KEY_6        = '6',
    ADS_KEY_7        = '7',
    ADS_KEY_8        = '8',
    ADS_KEY_9        = '9',
    ADS_KEY_ASTERISK = '*',
    ADS_KEY_POUND    = '#',

    ADS_KEY_UP       = -1,
    ADS_KEY_DOWN     = -2,
    ADS_KEY_LEFT     = -3,
    ADS_KEY_RIGHT    = -4,
    ADS_KEY_SELECT   = -5,

    ADS_KEY_SOFT1    = -6,
    ADS_KEY_SOFT2    = -7,
    ADS_KEY_CLEAR    = -8,

    /* these may not be available to java */
    ADS_KEY_SEND     = -10,
    ADS_KEY_END      = -11,
    ADS_KEY_POWER    = -12, 
    ADS_KEY_INVALID  = 0
} ADS_KeyCode;


#define DISPLAY_WIDTH           128
#define DISPLAY_HEIGHT          160

#define MAX_SOFTBUTTON_CHARS    6
#define TOP_BAR_HEIGHT          10
#define BOTTOM_BAR_HEIGHT       0
#define ARROWS_WIDTH            20
#define PAINT_WIDTH             DISPLAY_WIDTH
#define PAINT_HEIGHT            (DISPLAY_HEIGHT - \
                                (TOP_BAR_HEIGHT + BOTTOM_BAR_HEIGHT))
/*
 * This (x,y) coordinate pair refers to the offset of the upper 
 * left corner of the display screen within the MIDP phone handset
 * graphic window 
 */
#define X_SCREEN_OFFSET 0
#define Y_SCREEN_OFFSET TOP_BAR_HEIGHT

#ifdef __cplusplus
}
#endif

#endif /* _ARMSDAPP_UI_H_ */
