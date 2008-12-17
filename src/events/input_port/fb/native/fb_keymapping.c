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

/**
 * @file
 * Key mappings for received key signals handling
 */

#include <unistd.h>

#include <kni.h>
#include <keymap_input.h>
#include <midp_input_port.h>
#include "fb_keymapping.h"

KeyMapping *mapping = NULL;
jboolean bitscale_mode = KNI_FALSE;

/** Keyboard info for the ARM Versatile and Integrator boards */
KeyMapping versatile_integrator_keys[] = {
    {KEYMAP_KEY_0,                11,   139},
    {KEYMAP_KEY_1,                 2,   130},
    {KEYMAP_KEY_2,                 3,   131},
    {KEYMAP_KEY_3,                 4,   132},
    {KEYMAP_KEY_4,                 5,   133},
    {KEYMAP_KEY_5,                 6,   134},
    {KEYMAP_KEY_6,                 7,   135},
    {KEYMAP_KEY_7,                 8,   136},
    {KEYMAP_KEY_8,                 9,   137},
    {KEYMAP_KEY_9,                10,   138},

    {KEYMAP_KEY_UP,              103,   231},
    {KEYMAP_KEY_UP,               72,   200},   /* Integrator-CP */
    {KEYMAP_KEY_DOWN,            108,   236},
    {KEYMAP_KEY_DOWN,             80,   208},   /* Integrator-CP */
    {KEYMAP_KEY_LEFT,            105,   233},
    {KEYMAP_KEY_LEFT,             75,   203},   /* Integrator-CP */
    {KEYMAP_KEY_RIGHT,           106,   234},
    {KEYMAP_KEY_RIGHT,            77,   205},   /* Integrator-CP */

    {KEYMAP_KEY_SELECT,           28,   156},   /* Enter key */
    {KEYMAP_KEY_SELECT,           57,   185},   /* Space key */

    {KEYMAP_KEY_SOFT1,            59,   187},   /* F1 key */
    {KEYMAP_KEY_SOFT2,            60,   188},   /* F2 key */

    {KEYMAP_KEY_POWER,            25,   153},   /* P key */

    {KEYMAP_KEY_SEND,             31,   159},   /* S key */

    {KEYMAP_KEY_END,              18,   146},   /* P key */
    {KEYMAP_KEY_END,              16,   144},   /* Q key */
    {KEYMAP_KEY_END,               1,   129},   /* Escape key */

    {KEYMAP_KEY_CLEAR,            46,   174},   /* C key */
    {KEYMAP_KEY_CLEAR,            14,   142},   /* Backspace key */

    {KEYMAP_KEY_INVALID,           0,     0},   /* end of table */
};

/* Keyboard info for the Sharp Zaurus SL5500 */
KeyMapping zaurus_sl5500_keys[] = {
    {'q',                       0x11,   0x91},
    {'w',                       0x17,   0x97},
    {'e',                       0x05,   0x85},
    {'r',                       0x12,   0x92},
    {'t',                       0x14,   0x94},
    {'y',                       0x19,   0x99},
    {'u',                       0x15,   0x95},
    {'i',                       0x09,   0x89},
    {'o',                       0x0f,   0x8f},
    {'p',                       0x10,   0x90},
    {'a',                       0x01,   0x81},
    {'s',                       0x13,   0x93},
    {'d',                       0x04,   0x84},
    {'f',                       0x06,   0x86},
    {'g',                       0x07,   0x87},
    {'h',                       0x08,   0x88},
    {'j',                       0x0a,   0x8a},
    {'k',                       0x0b,   0x8b},
    {'l',                       0x0c,   0x8c},
    {KEYMAP_KEY_CLEAR,          0x1f,   0x9f},  /* backspace on keyboard */
    {'z',                       0x1a,   0x9a},
    {'x',                       0x18,   0x98},
    {'c',                       0x03,   0x83},
    {'v',                       0x16,   0x96},
    {'b',                       0x02,   0x82},
    {'n',                       0x0e,   0x8e},
    {'m',                       0x0d,   0x8d},
    {'\t',                      0x3f,   0xbf},
    {'/',                       0x41,   0xc1},
    {' ',                       0x45,   0xc5},
    {'\'',                      0x5c,   0xdc},
    {'.',                       0x46,   0xc6},
    {'\n',                      0x40,   0xc0},

    {KEYMAP_KEY_SOFT1,          0x58,   0xd8},  /* note button */
    {KEYMAP_KEY_SOFT1,          0x59,   0xd9},  /* contact button */

    {KEYMAP_MD_KEY_HOME,        0x28,   0xa8},  /* home button */

    {KEYMAP_KEY_SOFT2,          0x1d,   0x9d},  /* schedule button */
    {KEYMAP_KEY_SOFT2,          0x5a,   0xda},  /* mail button */

    {KEYMAP_KEY_END,            0x22,   0xa2},  /* Cancel button */

    {KEYMAP_KEY_UP,             0x24,   0xa4},  /* up button */
    {KEYMAP_KEY_DOWN,           0x25,   0xa5},  /* down button */
    {KEYMAP_KEY_LEFT,           0x23,   0xa3},  /* left button */
    {KEYMAP_KEY_RIGHT,          0x26,   0xa6},  /* right button */
    {KEYMAP_KEY_SELECT,         0x5b,   0xdb},  /* Middle button */

    {KEYMAP_KEY_SELECT,         0x27,   0xa7},  /* OK button */

    {KEYMAP_KEY_INVALID,           0,      0},  /* end of table */
};

/*
 * Keypad info for the OMAP 730
 * Key press values shall be the values with one and only one bit set
 * Key release values are not used.
 */
KeyMapping omap_730_keys[] = {
    {KEYMAP_KEY_UP,                1,   0},
    {KEYMAP_KEY_RIGHT,             2,   0},
    {KEYMAP_KEY_LEFT,              4,   0},
    {KEYMAP_KEY_DOWN,              8,   0},
    {KEYMAP_KEY_SELECT,           16,   0},
    {KEYMAP_KEY_SOFT2,            64,   0},    /* Right button */
    {KEYMAP_KEY_SOFT1,          4096,   0},    /* Left button */
    {KEYMAP_KEY_SEND,            128,   0},    /* Call button */
    {KEYMAP_KEY_POWER,           128,   0},
    {KEYMAP_KEY_END,             256,   0},    /* Hangup button */
    {KEYMAP_MD_KEY_HOME,    16777216,   0},    /* Home button */
    {KEYMAP_KEY_BACKSPACE,    262144,   0},    /* Enter button */
    {KEYMAP_KEY_0,           4194304,   0},
    {KEYMAP_KEY_1,          33554432,   0},
    {KEYMAP_KEY_2,            524288,   0},
    {KEYMAP_KEY_3,              8192,   0},
    {KEYMAP_KEY_4,          67108864,   0},
    {KEYMAP_KEY_5,           1048576,   0},
    {KEYMAP_KEY_6,             16384,   0},
    {KEYMAP_KEY_7,         134217728,   0},
    {KEYMAP_KEY_8,           2097152,   0},
    {KEYMAP_KEY_9,             32768,   0},
    {KEYMAP_KEY_ASTERISK,  268435456,   0},
    {KEYMAP_KEY_POUND,         65536,   0},
    {KEYMAP_KEY_SCREEN_ROT,      512,   0},    /* Left side down button */
    {KEYMAP_MD_KEY_SWITCH_APP,  2048,   0},    /* Right side button */
    {KEYMAP_KEY_VIRT_KEYB,      1024,   0},    /* Left side up button */
    {KEYMAP_KEY_INVALID,           0,   0},    /* end of table */
};
