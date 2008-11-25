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

#ifndef DEFAULTLCDUI_H_INCLUDED
#define DEFAULTLCDUI_H_INCLUDED

typedef unsigned short unicode;

/* reference point locations, from Graphics.java */
#define HCENTER   1
#define VCENTER   2
#define LEFT      4
#define RIGHT     8
#define TOP      16
#define BOTTOM   32
#define BASELINE 64

/* flags for font descriptors */
#define STYLE_PLAIN         0
#define STYLE_BOLD          1
#define STYLE_ITALIC        2
#define STYLE_UNDERLINED    4

/* flags for line types */
#define SOLID 0
#define DOTTED 1

#define SIZE_SMALL          8
#define SIZE_MEDIUM         0
#define SIZE_LARGE         16

#define FACE_SYSTEM         0
#define FACE_MONOSPACE     32
#define FACE_PROPORTIONAL  64

/* flags for LCDUIgetDisplayParams */
#define SUPPORTS_COLOR         1
#define SUPPORTS_POINTER       2
#define SUPPORTS_MOTION        4
#define SUPPORTS_REPEAT        8
#define SUPPORTS_DOUBLEBUFFER 16

/*  The type of events which can be generated.                               */
enum KVMEventTypes {
    invalidKVMEvent    = -2,
    stopKVMEvent       = -1,
    keyDownKVMEvent    = 0,
    keyUpKVMEvent      = 1,
    keyRepeatKVMEvent  = 2,
    penDownKVMEvent    = 3,
    penUpKVMEvent      = 4,
    penMoveKVMEvent    = 5,
    timerKVMEvent      = 6,
    commandKVMEvent    = 7,
    repaintKVMEvent    = 8,
    keyTypedKVMEvent   = 9,
    mmEOMEvent         = 10,
    systemKVMEvent     = 11, /* pause, suspend, kill, etc. */
#ifdef INCLUDE_I18N
    imeKVMEvent        = 12,
    lastKVMEvent       = 12
#else
    lastKVMEvent       = 11
#endif
};

/*  The event record. */
typedef struct {
    enum KVMEventTypes type;
    int                chr;
    short                screenX;
    short                screenY;
#ifdef INCLUDE_I18N
    unicode*           str;
    short              len;
#endif
} KVMEventType;

extern void StoreMIDPEvent(KVMEventType *evt);

/* global - incremented when network activity occurs */
extern int netIndicatorCount;
extern unicode *_digits_;
extern int _digitslen_;
extern unicode *_menu_;
extern int _menulen_;
extern unicode *_back_;
extern int _backlen_;
extern unicode *_cancel_;
extern int _cancellen_;

#define COMMAND_TYPE_SCREEN 1
#define COMMAND_TYPE_BACK   2
#define COMMAND_TYPE_CANCEL 3
#define COMMAND_TYPE_OK     4
#define COMMAND_TYPE_HELP   5
#define COMMAND_TYPE_STOP   6
#define COMMAND_TYPE_EXIT   7
#define COMMAND_TYPE_ITEM   8

typedef struct {
    int priority;
    unicode *chars;
    unsigned int numChars:8;
    unsigned int useLongChars:1;
    unicode *longChars;
    unsigned int numLongChars:8;
    unsigned int type:4;
    unsigned int id:20;
} commandStruct;

typedef struct {
    int idx;         /* index of popup element */
    int numChars;    /* length of "chars" array */
    unicode *chars;  /* string of popup element */
    int useImage;    /* < 0 if "image" is valid for this element */
    int imageHeight;
    int imageWidth;
    void* image;
} popupElementStruct;

typedef enum {
    KEY_0        = '0',
    KEY_1        = '1',
    KEY_2        = '2',
    KEY_3        = '3',
    KEY_4        = '4',
    KEY_5        = '5',
    KEY_6        = '6',
    KEY_7        = '7',
    KEY_8        = '8',
    KEY_9        = '9',
    KEY_ASTERISK = '*',
    KEY_POUND    = '#',

    KEY_UP       = -1,
    KEY_DOWN     = -2,
    KEY_LEFT     = -3,
    KEY_RIGHT    = -4,
    KEY_SELECT   = -5,

    KEY_SOFT1    = -6,
    KEY_SOFT2    = -7,
    KEY_CLEAR    = -8,

    /* these may not be available to java */
    KEY_SEND     = -10,
    KEY_END      = -11,
    KEY_POWER    = -12,
    KEY_INVALID  = 0,

#if ENABLE_JSR_75
    /* File connection Events */
    FILE_SYSTEM_MOUNTED = 6000,
    FILE_SYSTEM_UNMOUNTED = 6001,
#endif

    /* Values copied from EventHandler */
    VK_SUSPEND_ALL      = 1,
    VK_RESUME_ALL       = 2,
    VK_SHUTDOWN         = 3,
    VK_HOME             = 4,
    VK_SELECT_APP       = 5,
    VK_KILL_CURRENT     = 6,
    VK_CHANGE_LOCALE    = 7,
    VK_CLAMSHELL        = 8,
    VK_ROTATE           = 9,

    KEY_USER1    = -32,
    KEY_USER2    = -33,
    KEY_USER3    = -34,
    KEY_USER4    = -35,
    KEY_USER5    = -36,
    KEY_USER6    = -37,
    KEY_USER7    = -38,
    KEY_USER8    = -39,
    KEY_USER9    = -40,
    KEY_USER10   = -41
} KeyType;

#define TRANS_NONE           0
#define TRANS_MIRROR_ROT180  1
#define TRANS_MIRROR         2
#define TRANS_ROT180         3
#define TRANS_MIRROR_ROT270  4
#define TRANS_ROT90          5
#define TRANS_ROT270         6
#define TRANS_MIRROR_ROT90   7

#define FONTPARAMS face, style, size
#define FONTPARAMS_PROTO int face, int style, int size

#endif
