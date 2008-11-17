/*
 * @(#)KeyCodes.h	1.7 06/10/10
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

#ifndef _KEY_CODES_H_
#define _KEY_CODES_H_

/* Virtual key codes. */

#define VK_ENTER  '\n'
#define VK_BACK_SPACE  '\b'
#define VK_TAB  '\t'
#define VK_CANCEL  0x03
#define VK_CLEAR  0x0C
#define VK_SHIFT  0x10
#define VK_CONTROL  0x11
#define VK_ALT  0x12
#define VK_PAUSE  0x13
#define VK_CAPS_LOCK  0x14
#define VK_ESCAPE  0x1B
#define VK_SPACE  0x20
#define VK_PAGE_UP  0x21
#define VK_PAGE_DOWN  0x22
#define VK_END  0x23
#define VK_HOME  0x24

/**
 * Constant for the non-numpad <b>left</b> arrow key.
 * @see #VK_KP_LEFT
 */
#define VK_LEFT  0x25

/**
 * Constant for the non-numpad <b>up</b> arrow key.
 * @see #VK_KP_UP
 */
#define VK_UP  0x26

/**
 * Constant for the non-numpad <b>right</b> arrow key.
 * @see #VK_KP_RIGHT
 */
#define VK_RIGHT  0x27

/**
 * Constant for the non-numpad <b>down</b> arrow key.
 * @see #VK_KP_DOWN
 */
#define VK_DOWN  0x28
#define VK_COMMA  0x2C

/**
 * Constant for the "-" key.
 * @since 1.2
 */
#define VK_MINUS  0x2D

#define VK_PERIOD  0x2E
#define VK_SLASH  0x2F

/** VK_0 thru VK_9 are the same as ASCII '0' thru '9' (0x30 - 0x39) */
#define VK_0  0x30
#define VK_1  0x31
#define VK_2  0x32
#define VK_3  0x33
#define VK_4  0x34
#define VK_5  0x35
#define VK_6  0x36
#define VK_7  0x37
#define VK_8  0x38
#define VK_9  0x39

#define VK_SEMICOLON  0x3B
#define VK_EQUALS  0x3D

/** VK_A thru VK_Z are the same as ASCII 'A' thru 'Z' (0x41 - 0x5A) */
#define VK_A  0x41
#define VK_B  0x42
#define VK_C  0x43
#define VK_D  0x44
#define VK_E  0x45
#define VK_F  0x46
#define VK_G  0x47
#define VK_H  0x48
#define VK_I  0x49
#define VK_J  0x4A
#define VK_K  0x4B
#define VK_L  0x4C
#define VK_M  0x4D
#define VK_N  0x4E
#define VK_O  0x4F
#define VK_P  0x50
#define VK_Q  0x51
#define VK_R  0x52
#define VK_S  0x53
#define VK_T  0x54
#define VK_U  0x55
#define VK_V  0x56
#define VK_W  0x57
#define VK_X  0x58
#define VK_Y  0x59
#define VK_Z  0x5A

#define VK_OPEN_BRACKET  0x5B
#define VK_BACK_SLASH  0x5C
#define VK_CLOSE_BRACKET  0x5D

#define VK_NUMPAD0  0x60
#define VK_NUMPAD1  0x61
#define VK_NUMPAD2  0x62
#define VK_NUMPAD3  0x63
#define VK_NUMPAD4  0x64
#define VK_NUMPAD5  0x65
#define VK_NUMPAD6  0x66
#define VK_NUMPAD7  0x67
#define VK_NUMPAD8  0x68
#define VK_NUMPAD9  0x69
#define VK_MULTIPLY  0x6A
#define VK_ADD  0x6B

/**
 * This constant is obsolete, and is included only for backwards compatibility.
 * @see VK_SEPARATOR
 */
#define VK_SEPARATER  0x6C

/**
 * Constant for the Numpad Separator key.
 * @since 1.4
 */
//#define VK_SEPARATOR       VK_SEPARATER

#define VK_SUBTRACT  0x6D
#define VK_DECIMAL  0x6E
#define VK_DIVIDE  0x6F
#define VK_DELETE  0x7F /* ASCII DEL */
#define VK_NUM_LOCK  0x90
#define VK_SCROLL_LOCK  0x91

/** Constant for the F1 function key. */
#define VK_F1  0x70

/** Constant for the F2 function key. */
#define VK_F2  0x71

/** Constant for the F3 function key. */
#define VK_F3  0x72

/** Constant for the F4 function key. */
#define VK_F4  0x73

/** Constant for the F5 function key. */
#define VK_F5  0x74

/** Constant for the F6 function key. */
#define VK_F6  0x75

/** Constant for the F7 function key. */
#define VK_F7  0x76

/** Constant for the F8 function key. */
#define VK_F8  0x77

/** Constant for the F9 function key. */
#define VK_F9  0x78

/** Constant for the F10 function key. */
#define VK_F10  0x79

/** Constant for the F11 function key. */
#define VK_F11  0x7A

/** Constant for the F12 function key. */
#define VK_F12  0x7B

/**
 * Constant for the F13 function key.
 * @since 1.2
 */

/* F13 - F24 are used on IBM 3270 keyboard use random range for constants. */
#define VK_F13  0xF000

/**
 * Constant for the F14 function key.
 * @since 1.2
 */
#define VK_F14  0xF001

/**
 * Constant for the F15 function key.
 * @since 1.2
 */
#define VK_F15  0xF002

/**
 * Constant for the F16 function key.
 * @since 1.2
 */
#define VK_F16  0xF003

/**
 * Constant for the F17 function key.
 * @since 1.2
 */
#define VK_F17  0xF004

/**
 * Constant for the F18 function key.
 * @since 1.2
 */
#define VK_F18  0xF005

/**
 * Constant for the F19 function key.
 * @since 1.2
 */
#define VK_F19  0xF006

/**
 * Constant for the F20 function key.
 * @since 1.2
 */
#define VK_F20  0xF007

/**
 * Constant for the F21 function key.
 * @since 1.2
 */
#define VK_F21  0xF008

/**
 * Constant for the F22 function key.
 * @since 1.2
 */
#define VK_F22  0xF009

/**
 * Constant for the F23 function key.
 * @since 1.2
 */
#define VK_F23  0xF00A

/**
 * Constant for the F24 function key.
 * @since 1.2
 */
#define VK_F24  0xF00B

#define VK_PRINTSCREEN  0x9A
#define VK_INSERT  0x9B
#define VK_HELP  0x9C
#define VK_META  0x9D

#define VK_BACK_QUOTE  0xC0
#define VK_QUOTE  0xDE

/**
 * Constant for the numeric keypad <b>up</b> arrow key.
 * @see #VK_UP
 * @since 1.2
 */
#define VK_KP_UP  0xE0

/**
 * Constant for the numeric keypad <b>down</b> arrow key.
 * @see #VK_DOWN
 * @since 1.2
 */
#define VK_KP_DOWN  0xE1

/**
 * Constant for the numeric keypad <b>left</b> arrow key.
 * @see #VK_LEFT
 * @since 1.2
 */
#define VK_KP_LEFT  0xE2

/**
 * Constant for the numeric keypad <b>right</b> arrow key.
 * @see #VK_RIGHT
 * @since 1.2
 */
#define VK_KP_RIGHT  0xE3

/* For European keyboards */

/** @since 1.2 */
#define VK_DEAD_GRAVE  0x80

/** @since 1.2 */
#define VK_DEAD_ACUTE  0x81

/** @since 1.2 */
#define VK_DEAD_CIRCUMFLEX  0x82

/** @since 1.2 */
#define VK_DEAD_TILDE  0x83

/** @since 1.2 */
#define VK_DEAD_MACRON  0x84

/** @since 1.2 */
#define VK_DEAD_BREVE  0x85

/** @since 1.2 */
#define VK_DEAD_ABOVEDOT  0x86

/** @since 1.2 */
#define VK_DEAD_DIAERESIS  0x87

/** @since 1.2 */
#define VK_DEAD_ABOVERING  0x88

/** @since 1.2 */
#define VK_DEAD_DOUBLEACUTE  0x89

/** @since 1.2 */
#define VK_DEAD_CARON  0x8a

/** @since 1.2 */
#define VK_DEAD_CEDILLA  0x8b

/** @since 1.2 */
#define VK_DEAD_OGONEK  0x8c

/** @since 1.2 */
#define VK_DEAD_IOTA  0x8d

/** @since 1.2 */
#define VK_DEAD_VOICED_SOUND  0x8e

/** @since 1.2 */
#define VK_DEAD_SEMIVOICED_SOUND  0x8f

/** @since 1.2 */
#define VK_AMPERSAND  0x96

/** @since 1.2 */
#define VK_ASTERISK  0x97

/** @since 1.2 */
#define VK_QUOTEDBL  0x98

/** @since 1.2 */
#define VK_LESS  0x99

/** @since 1.2 */
#define VK_GREATER  0xa0

/** @since 1.2 */
#define VK_BRACELEFT  0xa1

/** @since 1.2 */
#define VK_BRACERIGHT  0xa2

/**
 * Constant for the "@" key.
 * @since 1.2
 */
#define VK_AT  0x0200

/**
 * Constant for the ":" key.
 * @since 1.2
 */
#define VK_COLON  0x0201

/**
 * Constant for the "^" key.
 * @since 1.2
 */
#define VK_CIRCUMFLEX  0x0202

/**
 * Constant for the "$" key.
 * @since 1.2
 */
#define VK_DOLLAR  0x0203

/**
 * Constant for the Euro currency sign key.
 * @since 1.2
 */
#define VK_EURO_SIGN  0x0204

/**
 * Constant for the "!" key.
 * @since 1.2
 */
#define VK_EXCLAMATION_MARK  0x0205

/**
 * Constant for the inverted exclamation mark key.
 * @since 1.2
 */
#define VK_INVERTED_EXCLAMATION_MARK  0x0206

/**
 * Constant for the "(" key.
 * @since 1.2
 */
#define VK_LEFT_PARENTHESIS  0x0207

/**
 * Constant for the "#" key.
 * @since 1.2
 */
#define VK_NUMBER_SIGN  0x0208

/**
 * Constant for the "+" key.
 * @since 1.2
 */
#define VK_PLUS  0x0209

/**
 * Constant for the ")" key.
 * @since 1.2
 */
#define VK_RIGHT_PARENTHESIS  0x020A

/**
 * Constant for the "_" key.
 * @since 1.2
 */
#define VK_UNDERSCORE  0x020B

/* for input method support on Asian Keyboards */

/* not clear what self means - listed in Win32 API */
#define VK_FINAL  0x0018

/** Constant for the Convert function key. */

/* Japanese PC 106 keyboard, Japanese Solaris keyboard: henkan */
#define VK_CONVERT  0x001C

/** Constant for the Don't Convert function key. */

/* Japanese PC 106 keyboard: muhenkan */
#define VK_NONCONVERT  0x001D

/** Constant for the Accept or Commit function key. */

/* Japanese Solaris keyboard: kakutei */
#define VK_ACCEPT  0x001E

/* not clear what self means - listed in Win32 API */
#define VK_MODECHANGE  0x001F

/* replaced by VK_KANA_LOCK for Win32 and Solaris might still be used on other platforms */
#define VK_KANA  0x0015

/* replaced by VK_INPUT_METHOD_ON_OFF for Win32 and Solaris might still be used for other platforms */
#define VK_KANJI  0x0019

/**
 * Constant for the Alphanumeric function key.
 * @since 1.2
 */

/* Japanese PC 106 keyboard: eisuu */
#define VK_ALPHANUMERIC  0x00F0

/**
 * Constant for the Katakana function key.
 * @since 1.2
 */

/* Japanese PC 106 keyboard: katakana */
#define VK_KATAKANA  0x00F1

/**
 * Constant for the Hiragana function key.
 * @since 1.2
 */

/* Japanese PC 106 keyboard: hiragana */
#define VK_HIRAGANA  0x00F2

/**
 * Constant for the Full-Width Characters function key.
 * @since 1.2
 */

/* Japanese PC 106 keyboard: zenkaku */
#define VK_FULL_WIDTH  0x00F3

/**
 * Constant for the Half-Width Characters function key.
 * @since 1.2
 */

/* Japanese PC 106 keyboard: hankaku */
#define VK_HALF_WIDTH  0x00F4

/**
 * Constant for the Roman Characters function key.
 * @since 1.2
 */

/* Japanese PC 106 keyboard: roumaji */
#define VK_ROMAN_CHARACTERS  0x00F5

/**
 * Constant for the All Candidates function key.
 * @since 1.2
 */

/* Japanese PC 106 keyboard - VK_CONVERT + ALT: zenkouho */
#define VK_ALL_CANDIDATES  0x0100

/**
 * Constant for the Previous Candidate function key.
 * @since 1.2
 */

/* Japanese PC 106 keyboard - VK_CONVERT + SHIFT: maekouho */
#define VK_PREVIOUS_CANDIDATE  0x0101

/**
 * Constant for the Code Input function key.
 * @since 1.2
 */

/* Japanese PC 106 keyboard - VK_ALPHANUMERIC + ALT: kanji bangou */
#define VK_CODE_INPUT  0x0102

/**
 * Constant for the Japanese-Katakana function key.
 * This key switches to a Japanese input method and selects its Katakana input mode.
 * @since 1.2
 */

/* Japanese Macintosh keyboard - VK_JAPANESE_HIRAGANA + SHIFT */
#define VK_JAPANESE_KATAKANA  0x0103

/**
 * Constant for the Japanese-Hiragana function key.
 * This key switches to a Japanese input method and selects its Hiragana input mode.
 * @since 1.2
 */

/* Japanese Macintosh keyboard */
#define VK_JAPANESE_HIRAGANA  0x0104

/**
 * Constant for the Japanese-Roman function key.
 * This key switches to a Japanese input method and selects its Roman-Direct input mode.
 * @since 1.2
 */

/* Japanese Macintosh keyboard */
#define VK_JAPANESE_ROMAN  0x0105

/**
 * Constant for the locking Kana function key.
 * This key locks the keyboard into a Kana layout.
 * @since 1.3
 */

/* Japanese PC 106 keyboard with special Windows driver - eisuu + Control Japanese Solaris keyboard: kana */
#define VK_KANA_LOCK  0x0106

/**
 * Constant for the input method on/off key.
 * @since 1.3
 */

/* Japanese PC 106 keyboard: kanji. Japanese Solaris keyboard: nihongo */
#define VK_INPUT_METHOD_ON_OFF  0x0107

/* for Sun keyboards */

/** @since 1.2 */
#define VK_CUT  0xFFD1

/** @since 1.2 */
#define VK_COPY  0xFFCD

/** @since 1.2 */
#define VK_PASTE  0xFFCF

/** @since 1.2 */
#define VK_UNDO  0xFFCB

/** @since 1.2 */
#define VK_AGAIN  0xFFC9

/** @since 1.2 */
#define VK_FIND  0xFFD0

/** @since 1.2 */
#define VK_PROPS  0xFFCA

/** @since 1.2 */
#define VK_STOP  0xFFC8

/**
 * Constant for the Compose function key.
 * @since 1.2
 */
#define VK_COMPOSE  0xFF20

/**
 * Constant for the AltGraph function key.
 * @since 1.2
 */
#define VK_ALT_GRAPH  0xFF7E

/**
 * This value is used to indicate that the keyCode is unknown.
 * KEY_TYPED events do not have a keyCode value self value
 * is used instead.
 */
#define VK_UNDEFINED  0x0

#endif
