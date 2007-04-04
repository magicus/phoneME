/*
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
#ifndef __JSROP_LOGGING_MD_H
#define __JSROP_LOGGING_MD_H

#include <javavm/include/utils.h>

#if defined __cplusplus 
extern "C" { 
#endif /* __cplusplus */

#define REPORT_INFO(ch, msg) \
    CVMdebugPrintf(("REPORT: <channel:%d> ", ch)); \
    CVMdebugPrintf((msg)); \
    CVMdebugPrintf(((const char *)"\n")); 
#define REPORT_INFO1(ch, msg, p1) \
    CVMdebugPrintf(("REPORT: <channel:%d> ", ch)); \
    CVMdebugPrintf(((const char *)msg, p1)); \
    CVMdebugPrintf(((const char *)"\n")); 
#define REPORT_INFO2(ch, msg, p1, p2) \
    CVMdebugPrintf(("REPORT: <channel:%d> ", ch)); \
    CVMdebugPrintf(((const char *)msg, p1, p2)); \
    CVMdebugPrintf(((const char *)"\n")); 
#define REPORT_INFO3(ch, msg, p1, p2, p3) \
    CVMdebugPrintf(("REPORT: <channel:%d> ", ch)); \
    CVMdebugPrintf(((const char *)msg, p1, p2, p3)); \
    CVMdebugPrintf(((const char *)"\n")); 
#define REPORT_INFO4(ch, msg, p1, p2, p3, p4) \
    CVMdebugPrintf(("REPORT: <channel:%d> ", ch)); \
    CVMdebugPrintf(((const char *)msg, p1, p2, p3, p4)); \
    CVMdebugPrintf(((const char *)"\n")); 

#define REPORT_ERROR(ch, msg) \
    CVMconsolePrintf((char *)"REPORT: <channel:%d> ", ch); \
    CVMconsolePrintf((char *)msg); \
    CVMconsolePrintf("\n"); 
#define REPORT_ERROR1(ch, msg, p1) \
    CVMconsolePrintf((char *)"ERROR: <channel:%d> ", ch); \
    CVMconsolePrintf((char *)msg, p1); \
    CVMconsolePrintf("\n"); 
#define REPORT_ERROR2(ch, msg, p1, p2) \
    CVMconsolePrintf((char *)"ERROR: <channel:%d> ", ch); \
    CVMconsolePrintf((char *)msg, p1, p2); \
    CVMconsolePrintf("\n"); 
#define REPORT_ERROR3(ch, msg, p1, p2, p3) \
    CVMconsolePrintf((char *)"ERROR: <channel:%d> ", ch); \
    CVMconsolePrintf((char *)msg, p1, p2, p3); \
    CVMconsolePrintf("\n"); 
#define REPORT_ERROR4(ch, msg, p1, p2, p3, p4) \
    CVMconsolePrintf((char *)("ERROR: <channel:%d> ", ch); \
    CVMconsolePrintf((char *)msg, p1, p2, p3, p4); \
    CVMconsolePrintf("\n"); 

/** Logging channels */

/** No channel specified */
#define LC_NONE 0
/** AMS module channel */
#define LC_AMS  500
/** Core module channel */
#define LC_CORE 1000
/** Core midpString channel */
#define LC_MIDPSTRING 1100
/** Core malloc channel */
#define LC_MALLOC 1200
/** Core stack usage tracing channel */
#define LC_CORE_STACK 1300
/** Low level UI module channel */
#define LC_LOWUI 2000
/** I18N module channel */
#define LC_I18N 3000
/** High level UI module channel */
#define LC_HIGHUI 4000
/** High level UI item layout channel */
#define LC_HIGHUI_ITEM_LAYOUT 4100
/** High level UI item repaint channel */
#define LC_HIGHUI_ITEM_REPAINT 4200
/** High level UI form layout channel */
#define LC_HIGHUI_FORM_LAYOUT 4300
/** High level UI form paint channel */
#define LC_HIGHUI_ITEM_PAINT 4400
/** Protocol module channel */
#define LC_PROTOCOL 5000
/** RMS module channel */
#define LC_RMS 6000
/** Security module channel */
#define LC_SECURITY 7000
/** Services module channel */
#define LC_SERVICES 8000
/** Tools module channel */
#define LC_TOOL 9000
/** Event queue module channel */
#define LC_EVENTS 9500
/** File Storage module channel */
#define LC_STORAGE 10000
/** Push registry module channel */
#define LC_PUSH 10500
/** MMAPI module channel */
#define LC_MMAPI 11000
    
#if defined __cplusplus 
} 
#endif /* __cplusplus */
#endif /* __JSROP_LOGGING_MD_H */

