/*
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

#ifndef __SKINS_H
#define __SKINS_H

#include "res/resource.h"

#define SKINS_MENU_SUPPORTED

#define NUMBEROF(x) (sizeof(x)/sizeof(x[0]))

/* key definitons */
typedef struct _Rectangle {
    int x;
    int y;
    int width;
    int height;
} XRectangle;

typedef struct {
    javacall_key button;
    XRectangle bounds;
    char *name;
} WKey;

typedef struct {
   /*
    * Display bounds
    */
    XRectangle displayRect;
    /*
     * Key mapping
     */
    const WKey* Keys;
    /*
     * Number of keys
     */
    int keyCnt;

    /*
     * Emulator skin bitmap resource ID
     */
    int resourceID;
    /*
     * Emulator skin bitmap
     */
    HBITMAP hBitmap;

} ESkin;

#define KEY_POWER  (JAVACALL_KEY_GAME_RIGHT - 100)
#define KEY_END    (JAVACALL_KEY_GAME_RIGHT - 101)
#define KEY_SEND   (JAVACALL_KEY_GAME_RIGHT - 102)

const static WKey VKeys[] = {
#ifdef NO_POWER_BUTTON
{KEY_POWER,    {-10, -10,  1,  1}, "POWER"},
#else
{KEY_POWER,    {281, 48, 19, 19}, "POWER"},
#endif

//#define USE_SWAP_SOFTBUTTON
#ifndef USE_SWAP_SOFTBUTTON // !USE_SWAP_SOFTBUTTON 
{JAVACALL_KEY_SOFT1,    {54, 418, 79, 18}, "SOFT1"},//
{JAVACALL_KEY_SOFT2,    {221, 418, 78, 18}, "SOFT2"},//
#else // USE_SWAP_SOFTBUTTON 
{JAVACALL_KEY_SOFT2,    {54, 418, 79, 18}, "SOFT2"},//
{JAVACALL_KEY_SOFT1,    {221, 418, 78, 18}, "SOFT1"},//
#endif

{JAVACALL_KEY_UP,       {154, 439, 43, 16}, "UP"},//
{JAVACALL_KEY_DOWN,     {154, 504, 43, 16}, "DOWN"},//
{JAVACALL_KEY_LEFT,     {137, 457, 16, 45}, "LEFT"},//
{JAVACALL_KEY_RIGHT,    {199, 457, 15, 45}, "RIGHT"},//
{JAVACALL_KEY_SELECT,   {157, 460, 37, 39}, "SELECT"},//

{JAVACALL_KEY_SEND,     {55, 450, 76, 26}, "SEND"},//
{KEY_END,               {220, 450, 76, 26}, "END"},//
{JAVACALL_KEY_CLEAR,    {220, 482, 76, 26}, "CLEAR"},//

{JAVACALL_KEY_1,        {55, 526, 76, 31}, "1"},//
{JAVACALL_KEY_2,        {138, 526, 76, 31}, "2"},//
{JAVACALL_KEY_3,        {221, 526, 76, 31}, "3"},//
{JAVACALL_KEY_4,        {55, 560, 76, 31}, "4"},//
{JAVACALL_KEY_5,        {138, 560, 76, 31}, "5"},//
{JAVACALL_KEY_6,        {221, 560, 76, 31}, "6"},//
{JAVACALL_KEY_7,        {55, 594, 76, 31}, "7"},//
{JAVACALL_KEY_8,        {138, 594, 76, 31}, "8"},//
{JAVACALL_KEY_9,        {221, 594, 76, 31}, "9"},//
{JAVACALL_KEY_ASTERISK, {55, 628, 76, 31}, "*"},//
{JAVACALL_KEY_0,        {138, 628, 76, 31}, "0"},//
{JAVACALL_KEY_POUND,    {221, 628, 76, 31}, "#"},//

};

static ESkin VSkin = {
    {56, 94, 240, 320}, //displayRect
    VKeys,
    NUMBEROF(VKeys),
    IDB_BITMAP_PHONE_V, //resource ID
    NULL //hBitmap
};

static ESkin HSkin = {
    {56, 94, 320, 240}, //displayRect
    NULL, 0,
    IDB_BITMAP_PHONE_H, //resource ID
    NULL //hBitmap
};


#endif /* __SKINS_H */
