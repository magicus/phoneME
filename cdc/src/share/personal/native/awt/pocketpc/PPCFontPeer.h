/*
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

#ifndef _WINCE_FONTPEER_H
#define _WINCE_FONTPEER_H

#include "awt.h"
#include "PPCObjectPeer.h"
#include "PPCUnicode.h"

#include <java_awt_Font.h> 
#include <sun_awt_pocketpc_PPCFontMetrics.h> 
 
#define GET_PLATFORMFONT(OBJ) PDATA(OBJ, peer))


class AwtFont : public AwtObject {
public:
    // The argument is used to determine how many handles of
    // Windows font the instance has.
    AwtFont(int num);
    ~AwtFont();    // Releases all resources

// Access methods
    INLINE int GetHFontNum()     { return m_hFontNum; }
    INLINE HFONT GetHFont(int i) { 
        ASSERT(m_hFont[i] != NULL); 
        return m_hFont[i]; 
    }

    // Used to keep English version unchanged as much as possiple.
    INLINE HFONT GetHFont() { 
        ASSERT(m_hFont[0] != NULL); 
        return m_hFont[0]; 
    }
    INLINE int GetInputHFontIndex() { return m_textInput; }

    INLINE BOOL NeedConverted(int i) { return m_needConvert[i]; }

    INLINE void SetAscent(int ascent) { m_ascent = ascent; }
    INLINE int GetAscent()           { return m_ascent; }
    INLINE int GetOverhang()         { return m_overhang; }

// Font methods
    // Returns the AwtFont object associated with the pFontJavaObject.
    // If none exists, create one.
    // Use this also for the FontMetrics version
    static AwtFont* GetFont(JNIEnv* env, jobject hFont);
    
    // Creates the specified font.  name names the font.  style is a bit
    // vector that describes the style of the font.  height is the point
    // size of the font.
    static AwtFont* Create(JNIEnv* env, jobject hFont);
    static HFONT CreateHFont(WCHAR* name, int style, int height);
    //static HFONT CreateHFont(char* name, int style, int height);

    static void Cleanup();

// FontMetrics methods

    // Loads the metrics of the associated font.
    // See Font.GetFont for purpose of pWS.  (Also, client should provide
    // Font java object instead of getting it from the FontMetrics instance 
    // variable.)
    static void LoadMetrics(JNIEnv *env, jobject hFontMetrics);

    static long StringWidth(jobject hFontMetrics, 
                            jstring str);

    // Returns the AwtFont associated with this metrics.
    static AwtFont* GetFontFromMetrics(jobject hFontMetrics);

    // Sets the ascent of the font.  This member should be called if 
    // font->m_nAscent < 0.
    static void SetupAscent(AwtFont* font);

    // Determines the average dimension of the character in the
    // specified font 'font' and multiplies it by the specified number of
    // rows and columns.  
    // 'font' can be a temporary object.
    static SIZE TextSize(AwtFont* font, int columns, int rows);

    // If 'font' is NULL, the SYSTEM_FONT is used to compute the size.
    // 'font' can be a temporary object.
    static SIZE StringWidth(HFONT font, char* str);
    static SIZE StringWidth(AwtFont* font, char* str);
    
    static int getFontDescriptorNumber(JNIEnv* env, jobject font,
                                       jobject fd);

    static SIZE DrawStringSize_sub(jstring str, HDC hDC,
                    jobject jhFont, long x, long y, BOOL draw);

    INLINE static SIZE drawMFStringSize(HDC hDC, jobject hFont, 
                                        jstring str, 
                                        long x, long y) {
        return DrawStringSize_sub(str, hDC, hFont, x, y, TRUE);
    }

    INLINE static SIZE getMFStringSize(HDC hDC, jobject hFont, 
                                       jstring str) {
        return DrawStringSize_sub(str, hDC, hFont, 0, 0, FALSE);
    }
    
    INLINE static long getMFStringWidth(HDC hDC, jobject hFont, 
                                        jstring str) {
        return getMFStringSize(hDC, hFont, str).cx;
    }    
    
    INLINE static void drawMFString(HDC hDC, jobject hFont, 
                                    jstring str, 
                                    long x, long y) {
        DrawStringSize_sub(str, hDC, hFont, x, y, TRUE);
    }

// Helper routines
    INLINE static jstring JavaCharArray2JavaString(
        jcharArray arr) {
	/* FIX
        return (Hjava_lang_String *)execute_java_constructor(
            EE(), "java/lang/String", NULL, "([C)", arr);
	*/
	return NULL;
    }

INLINE static jobjectArray GetComponentFonts(JNIEnv* env, jobject hFont) {
    jobject platformFont = env->GetObjectField(hFont, WCachedIDs.java_awt_Font_peerFID);
    return (jobjectArray)env->GetObjectField(platformFont, WCachedIDs.sun_awt_PlatformFont_componentFontsFID);
}

// Variables
private:
    // The array of assocated font handles.
    HFONT* m_hFont;
    // Does associated m_hFont[i] need to be converted?
    // Such a font must be described in font.properties file as:
    // serif.3=WingDings,NEED_CONVERTED
    BOOL*  m_needConvert;
    // The number of handles.
    int    m_hFontNum;
    // The index of the handle used to be set to AwtTextComponent.
    int    m_textInput;
    // The ascent of this font.
    int m_ascent;
    // The overhang, or amount added to a string's width, of this font.
    int m_overhang;
};

class AwtFontCache {
public:
    AwtFontCache() { m_head = NULL; }
    void  Add(WCHAR* name, HFONT font);
    HFONT Lookup(WCHAR* name);
    BOOL  Search(HFONT font);
    void  Remove(HFONT font);
    void Clear();

private:
    class Item {
    public:
        Item(const WCHAR* s, HFONT f, Item* n = NULL);
        ~Item();

        WCHAR*    name;
        HFONT    font;
        Item*    next;
    };
  
    Item* m_head;
};

#define GET_FONT(OBJ) \
    env->CallObjectMethod(OBJ, WCachedIDs.java_awt_Component_getFontMID)

/* FIX
#define JAVA_CHARSUBARRAY_TO_JAVA_STRING(str, offset, length) \
    ((Hjava_lang_String *)execute_java_constructor \
        (EE(), "java/lang/String", NULL, "([CII)", str, offset, length))
*/
#endif // _WINCE_FONTPEER_H
