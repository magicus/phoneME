/*
 * @(#)PPCFontPeer.cpp	1.13 06/10/10
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
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "java_awt_Font.h"
#include "PPCFontPeer.h"

#include "java_awt_Dimension.h"
#include "sun_awt_pocketpc_PPCDefaultFontCharset.h"
#include "sun_awt_pocketpc_PPCFontMetrics.h"
#include "sun_awt_pocketpc_PPCFontPeer.h"

AwtFontCache fontCache;

static BOOL IsMultiFont(JNIEnv* env, jobject OBJ) 
{
    jobject platformFont = env->GetObjectField(OBJ, WCachedIDs.java_awt_Font_peerFID);
    return ((env->GetObjectField(platformFont, WCachedIDs.sun_awt_PlatformFont_propsFID)) != NULL) ;
}

static jstring GetFontProperty(JNIEnv *env, jobject font, jstring key) 
{
    /* should we be checking for nulls ? -br */
    jobject peer = env->GetObjectField(font, WCachedIDs.java_awt_Font_peerFID);
    ASSERT(peer != NULL);
    jobject props = env->GetObjectField(peer,WCachedIDs.sun_awt_PlatformFont_propsFID);
    ASSERT(props != NULL);

    jclass clazz = env->FindClass("java/util/Properties"); 
    jmethodID methodID = env->GetMethodID(clazz, "getProperty", "(Ljava/lang/String;)Ljava/lang/String;"); 
    jstring result = (jstring) env->CallObjectMethod(props, methodID, key);
    return result;
}

AwtFont::AwtFont(int num)
{
    if (num == 0) {  // not multi-font
        num = 1;
    }

    m_hFontNum = num;
    m_hFont = new HFONT[num];
    m_needConvert = new BOOL[num];
    for (int i = 0; i < num; i++) {
        m_hFont[i] = NULL;
        m_needConvert[i] = FALSE;
    }  

    m_textInput = -1;
    m_ascent = -1;
    m_overhang = 0;
}

AwtFont::~AwtFont()
{
    for (int i = 0; i < m_hFontNum; i++) {
        HFONT font = m_hFont[i];
        if (font != NULL && fontCache.Search(font)) {
            fontCache.Remove(font);
            VERIFY(::DeleteObject(font));
        }
    }
    delete[] m_hFont;
    delete[] m_needConvert;
    /* Do we need to do the following too???
       JNIEnv *env;
       if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
       return;
       }
       jobject localObj = env->NewLocalRef(m_javaFont);
       if (localObj != NULL) {
       env->SetIntField(localObj, WCachedIDs.java_awt_Font_pDataFID, NULL);
       env->DeleteLocalRef(localObj);
       }
    */
}

AwtFont* AwtFont::GetFont(JNIEnv* env, jobject hFont)
{
    AwtFont* font = (AwtFont *) env->GetIntField(hFont, WCachedIDs.java_awt_Font_pDataFID);
    if (font != NULL) {
	return font;
    }

    font = Create(env, hFont);
    env->SetIntField(hFont, WCachedIDs.java_awt_Font_pDataFID,(jint)font);
    return font;
}


AwtFont* AwtFont::Create(JNIEnv* env, jobject hFont)
{
    AwtFont* font = NULL;
    jobjectArray hCompFont;
    int cfnum;

    if (IsMultiFont(env, hFont)) {
        hCompFont = GetComponentFonts(env, hFont);
        cfnum = env->GetArrayLength(hCompFont);
    } else {
        hCompFont = NULL;
        cfnum = 0;
    }

    WCHAR* wName;

    font = new AwtFont(cfnum);
    if (cfnum > 0) {
        // Ask java.awt.Font for the input charset 
        jstring key = env->NewStringUTF("inputtextcharset");
        jstring hTextin = GetFontProperty(env, hFont, key);
        WCHAR* textin = TO_WSTRING(hTextin);

        env->DeleteLocalRef(key);

        for (int i = 0; i < cfnum; i++) { 
            // hNativeName is a pair of platform fontname and its charset
            // tied with a comma; "Times New Roman,ANSI_CHARSET".            
           
            jobject sun_awt_fontDescriptor = env->GetObjectArrayElement(hCompFont, i);
            wName = TO_WSTRING((jstring)env->GetObjectField(sun_awt_fontDescriptor, WCachedIDs.sun_awt_FontDescriptor_nativeNameFID));
	    ASSERT(wName);

            // Check to see if this font is suitable for input
	    // on AwtTextComponent?
            if ((textin != NULL) && (wcsstr(wName, textin) != NULL)) {
		font->m_textInput = i;
	    }
            font->m_hFont[i] = CreateHFont(wName, 
               env->CallIntMethod(hFont, WCachedIDs.java_awt_Font_getStyleMID),
               env->CallIntMethod(hFont, WCachedIDs.java_awt_Font_getSizeMID));
            if (wcsstr(wName, L"NEED_CONVERTED"))
                font->m_needConvert[i] = TRUE;
            else
                font->m_needConvert[i] = FALSE;
        }
	if (textin == NULL) {
	    // no entry "inputtextcharset" in font.properties or
	    // CHARSET for "inputtextcharset" is different from any CHARSET
	    // for Native font
	    font->m_textInput = 0;
	}
    } else { 
        // Instantiation for English version.
        wName = TO_WSTRING((jstring)env->CallObjectMethod(hFont, WCachedIDs.java_awt_Font_getNameMID));
        WCHAR* wEName;
        if (!wcscmp(wName, L"Helvetica") || !wcscmp(wName, L"SansSerif")) {
	    wEName = L"Arial";
        } else if (!wcscmp(wName, L"TimesRoman") || !wcscmp(wName, L"Serif")) {
	    wEName = L"Times New Roman";
        } else if (!wcscmp(wName, L"Courier") || !wcscmp(wName, L"Monospaced")) {
	    wEName = L"Courier New";
        } else if (!wcscmp(wName, L"Dialog")) {
	    wEName = L"MS Sans Serif";
        } else if (!wcscmp(wName, L"DialogInput")) {
	    wEName = L"MS Sans Serif";
        } else if (!wcscmp(wName, L"ZapfDingbats")) {
	    wEName = L"WingDings";
        } else
            wEName = L"Arial";

	font->m_textInput = 0;
        font->m_hFont[0] = CreateHFont(wEName, 
               env->CallIntMethod(hFont, WCachedIDs.java_awt_Font_getStyleMID),
               env->CallIntMethod(hFont, WCachedIDs.java_awt_Font_getSizeMID)); 
    }
    env->SetIntField(hFont, WCachedIDs.java_awt_Font_pDataFID, (jint)font);
    return font;
}

// Get suitable CHARSET from charset string in properties file.
static int GetNativeCharset(WCHAR* name)
{
    if (wcsstr(name, L"ANSI_CHARSET"))
        return ANSI_CHARSET;
    if (wcsstr(name, L"DEFAULT_CHARSET"))
        return DEFAULT_CHARSET;
    if (wcsstr(name, L"SYMBOL_CHARSET") || wcsstr(name, L"WingDings"))
        return SYMBOL_CHARSET;
    if (wcsstr(name, L"SHIFTJIS_CHARSET"))
        return SHIFTJIS_CHARSET;
    if (wcsstr(name, L"GB2312_CHARSET"))
        return GB2312_CHARSET;
    if (wcsstr(name, L"HANGEUL_CHARSET"))
        return HANGEUL_CHARSET;
    if (wcsstr(name, L"CHINESEBIG5_CHARSET"))
        return CHINESEBIG5_CHARSET;
    if (wcsstr(name, L"OEM_CHARSET"))
        return OEM_CHARSET;
    if (wcsstr(name, L"JOHAB_CHARSET"))
        return JOHAB_CHARSET;
    if (wcsstr(name, L"HEBREW_CHARSET"))
        return HEBREW_CHARSET;
    if (wcsstr(name, L"ARABIC_CHARSET"))
        return ARABIC_CHARSET;
    if (wcsstr(name, L"GREEK_CHARSET"))
        return GREEK_CHARSET;
    if (wcsstr(name, L"TURKISH_CHARSET"))
        return TURKISH_CHARSET;
    if (wcsstr(name, L"VIETNAMESE_CHARSET"))
        return VIETNAMESE_CHARSET;
    if (wcsstr(name, L"THAI_CHARSET"))
        return THAI_CHARSET;
    if (wcsstr(name, L"EASTEUROPE_CHARSET"))
        return EASTEUROPE_CHARSET;
    if (wcsstr(name, L"RUSSIAN_CHARSET"))
        return RUSSIAN_CHARSET;
    if (wcsstr(name, L"MAC_CHARSET"))
        return MAC_CHARSET;
    if (wcsstr(name, L"BALTIC_CHARSET"))
        return BALTIC_CHARSET;
    return ANSI_CHARSET;
}

static HFONT CreateHFont_sub(WCHAR* name, int style, int height)
{
    LOGFONTW logFont;

    logFont.lfWidth = 0;
    logFont.lfEscapement = 0;
    logFont.lfOrientation = 0;
    logFont.lfUnderline = FALSE;
    logFont.lfStrikeOut = FALSE;
    logFont.lfCharSet = GetNativeCharset(name);
    logFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    logFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    logFont.lfQuality = DEFAULT_QUALITY;
    logFont.lfPitchAndFamily = DEFAULT_PITCH;

    // Set style
    logFont.lfWeight = (style & java_awt_Font_BOLD) ? FW_BOLD : FW_NORMAL;
    logFont.lfItalic = (style & java_awt_Font_ITALIC) != 0;
    logFont.lfUnderline = 0;//(style & java_awt_Font_UNDERLINE) != 0;

    // Get point size
    logFont.lfHeight = -height;

    // Set font name
    WCHAR tmpname[80];
    wcscpy(tmpname, name);
    WCHAR* delimit = wcschr(tmpname, L',');
    if (delimit != NULL)
        *delimit = L'\0';  // terminate the string after the font name.
    wcscpy(&(logFont.lfFaceName[0]), tmpname);

    HFONT hFont;
    // before wrapper function of CreateFontIndirect is made,
    // we have to distinguish the running environment
#ifndef WINCE
    if (IS_NT) {
        hFont = ::CreateFontIndirectW(&logFont);
        ASSERT(hFont != NULL);
    } else {
#ifdef WIN32
        ASSERT(IS_WIN95);
#endif

        LOGFONTA logFontA;
        logFontA.lfHeight           = logFont.lfHeight;
        logFontA.lfWidth            = logFont.lfWidth;
        logFontA.lfEscapement       = logFont.lfEscapement;
        logFontA.lfOrientation      = logFont.lfOrientation;
        logFontA.lfWeight           = logFont.lfWeight;
        logFontA.lfItalic           = logFont.lfItalic;
        logFontA.lfUnderline        = logFont.lfUnderline;
        logFontA.lfStrikeOut        = logFont.lfStrikeOut;
        logFontA.lfCharSet          = logFont.lfCharSet;
        logFontA.lfOutPrecision     = logFont.lfOutPrecision;
        logFontA.lfClipPrecision    = logFont.lfClipPrecision;
        logFontA.lfQuality          = logFont.lfQuality;
        logFontA.lfPitchAndFamily   = logFont.lfPitchAndFamily;
        
        // Some fontnames of Non-ASCII fonts like 'MS Minchou' are themselves
        // Non-ASCII.  They are assumed to be written in Unicode.
        // Hereby, they are converted into platform codeset. 
	LPCWSTR lpszTmp = logFont.lfFaceName;
        int tmplen = wcslen(lpszTmp);

        int mblen = WideCharToMultiByte(CP_ACP, 0,
	    lpszTmp, tmplen, NULL, 0, NULL, NULL);
        char* chartmp = new char[mblen+1];
        chartmp[mblen] = '\0';
        WideCharToMultiByte(CP_ACP, 0,
	    lpszTmp, tmplen, chartmp, mblen, NULL, NULL);
        strcpy(&logFontA.lfFaceName[0], chartmp);
        
        // Use CreateFontIndirectA explicitly as CreateFontIndirectW is not
        // available on Windows 95.
        hFont = ::CreateFontIndirectA(&logFontA); 
        ASSERT(hFont != NULL);
    }
#else /* WINCE */
    hFont = ::CreateFontIndirectW(&logFont);
    ASSERT(hFont != NULL);
#endif
    return hFont;
}

HFONT AwtFont::CreateHFont(WCHAR* name, int style, int height)
{
    WCHAR longName[80];
    // 80 > (max face name(=30) + strlen("CHINESEBIG5_CHARSET")
    //       + strlen("NEED_CONVERTED") + and so)
    // longName doesn't have to be printable.  So, it is OK not to convert.
    swprintf(longName, L"%s-%d-%d", name, style, height);
    HFONT hFont = fontCache.Lookup(longName);
    if (hFont != NULL) {
        return hFont;
    }

    hFont = CreateHFont_sub(name, style, height);

    fontCache.Add(longName, hFont);
    return hFont;
} 

void AwtFont::Cleanup()
{
    fontCache.Clear();
}


AwtFont* AwtFont::GetFontFromMetrics(jobject hFontMetrics)
{
    JNIEnv *env;
    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return NULL;
    }

    jobject hFont = env->GetObjectField(hFontMetrics, 
                                        WCachedIDs.java_awt_FontMetrics_fontFID);
    return (AwtFont*)env->GetIntField(hFont, WCachedIDs.java_awt_Font_pDataFID);  
}


void AwtFont::SetupAscent(AwtFont* font)
{
    HDC hDC = ::GetDC(0);
    ASSERT(hDC != NULL);
    HGDIOBJ oldFont = ::SelectObject(hDC, font->GetHFont());

    TEXTMETRIC metrics;
    VERIFY(::GetTextMetrics(hDC, &metrics));
    font->SetAscent(metrics.tmAscent);

    ::SelectObject(hDC, oldFont); 
    VERIFY(::ReleaseDC(0, hDC));
}


void AwtFont::LoadMetrics(JNIEnv *env, jobject fontMetrics)
{
    jobject jfont = env->GetObjectField(fontMetrics, 
                                       WCachedIDs.java_awt_FontMetrics_fontFID );
    AwtFont* font = AwtFont::GetFont(env, jfont);
    
    jintArray widths = env->NewIntArray(256);

    if (widths == NULL) {
        /* no local refs to delete yet. */
	return;
    }
    
    HDC hDC = ::GetDC(0);
    ASSERT(hDC != NULL);

    HGDIOBJ oldFont = ::SelectObject(hDC, font->GetHFont());
    TEXTMETRIC metrics;
    VERIFY(::GetTextMetrics(hDC, &metrics));

    font->m_ascent = metrics.tmAscent;
    env->SetIntField(fontMetrics, WCachedIDs.PPCFontMetrics_ascentFID, metrics.tmAscent);
    env->SetIntField(fontMetrics, WCachedIDs.PPCFontMetrics_descentFID, metrics.tmDescent);
    env->SetIntField(fontMetrics, WCachedIDs.PPCFontMetrics_leadingFID,
                     metrics.tmExternalLeading);
    env->SetIntField(fontMetrics, WCachedIDs.PPCFontMetrics_heightFID, 
                     metrics.tmAscent + metrics.tmDescent + metrics.tmExternalLeading);
    env->SetIntField(fontMetrics, WCachedIDs.PPCFontMetrics_maxAscentFID, 
                     metrics.tmAscent);
    env->SetIntField(fontMetrics, WCachedIDs.PPCFontMetrics_maxDescentFID,
                     metrics.tmDescent);
    env->SetIntField(fontMetrics, WCachedIDs.PPCFontMetrics_maxHeightFID, 
                     metrics.tmAscent + metrics.tmDescent + metrics.tmExternalLeading);
    env->SetIntField(fontMetrics, WCachedIDs.PPCFontMetrics_maxAdvanceFID, 
                     metrics.tmMaxCharWidth);
    font->m_overhang = metrics.tmOverhang;

    jint intWidths[256];
    memset(intWidths, 0, 256 * sizeof(int));
#ifndef WINCE
    VERIFY(::GetCharWidth(hDC, metrics.tmFirstChar, 
                          min(metrics.tmLastChar, 255), 
                          (int*) &intWidths[metrics.tmFirstChar]));
#else // WINCE 
    {
        //WinCE does not have a GetCharWidth, so we must get the extents of each character
	
	int i;
	WCHAR wchar[2];
	wchar[1] = 0;
	SIZE size;

        /* Somehow metrics.tmLastChar value is not set correctly hence 
         changing it to 255. We might want to revisit this. */
        for (i = metrics.tmFirstChar; i < 255/*min(metrics.tmLastChar, 255)*/; i++) {
	    wchar[0] = i;
	    ::GetTextExtentPoint32(hDC, wchar, 1, &size);
	    intWidths[i] = size.cx - metrics.tmOverhang;
	}
    }

#endif //WINCE 
    env->SetIntArrayRegion(widths, 0, 256, intWidths);
    env->SetObjectField(fontMetrics, WCachedIDs.PPCFontMetrics_widthsFID, widths);

    // Get font metrics on remaining fonts (if multifont).
    for (int j = 1; j < font->GetHFontNum(); j++) {
	::SelectObject(hDC, font->GetHFont(j));
	VERIFY(::GetTextMetrics(hDC, &metrics));
        
        int ascent = (int) env->GetIntField(fontMetrics, WCachedIDs.PPCFontMetrics_ascentFID);
        int maxDescent = (int) env->GetIntField(fontMetrics, WCachedIDs.PPCFontMetrics_maxDescentFID);
        int maxHeight = (int) env->GetIntField(fontMetrics, WCachedIDs.PPCFontMetrics_maxHeightFID);
        int maxAdvance = (int) env->GetIntField(fontMetrics, WCachedIDs.PPCFontMetrics_maxAdvanceFID);
        
        env->SetIntField(fontMetrics, WCachedIDs.PPCFontMetrics_maxAscentFID, 
                         max(ascent, metrics.tmAscent));
        env->SetIntField(fontMetrics, WCachedIDs.PPCFontMetrics_maxDescentFID,
                         max(maxDescent, metrics.tmDescent));
        env->SetIntField(fontMetrics, WCachedIDs.PPCFontMetrics_maxHeightFID, 
                         max(maxHeight, metrics.tmAscent + metrics.tmDescent 
                             + metrics.tmExternalLeading));
        env->SetIntField(fontMetrics, WCachedIDs.PPCFontMetrics_maxAdvanceFID,
                         max(maxAdvance, metrics.tmMaxCharWidth));
    }

    VERIFY(::SelectObject(hDC, oldFont));
    VERIFY(::ReleaseDC(0, hDC));
}


long AwtFont::StringWidth(jobject hFontMetrics, jstring jstr)
{
    HDC hDC = ::GetDC(0);
    ASSERT(hDC != NULL);

    JNIEnv *env;
    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return 0;
    }
    jobject hFont = env->GetObjectField(hFontMetrics, 
                                        WCachedIDs.java_awt_FontMetrics_fontFID);
    long ret;
    if (IsMultiFont(env, hFont)) {
        ret = getMFStringWidth(hDC, hFont, jstr);
    } else {    // end of Multifont portion
        int len = env->GetStringLength(jstr);
        char* str = new char[len * 2 + 1];
        str = (char*) env->GetStringChars(jstr, NULL); //Is this right?
        AwtFont* font = GetFontFromMetrics(hFontMetrics);
        ret = StringWidth(font, str).cx;
        delete[] str;
    }

    VERIFY(::ReleaseDC(0, hDC));
    return ret;
}

SIZE AwtFont::StringWidth(AwtFont* font, char* str)
{
    HDC hDC = ::GetDC(0);
    ASSERT(hDC != NULL);
    HGDIOBJ oldFont = ::SelectObject(hDC, (font == NULL)
                                           ? ::GetStockObject(SYSTEM_FONT)
                                           : font->GetHFont());

    SIZE size;
#ifdef WINCE
    {
        int len;
	unsigned short wStr[256];
	if ((len = strlen(str)) > 255) {
	    len=255;
	}
	//ASCII2UNICODE
	MultiByteToWideChar(CP_ACP, 0, str, -1, (LPWSTR)wStr, len);

	VERIFY(::GetTextExtentPoint32(hDC, (LPWSTR)wStr, len, &size));
    }
#else
    VERIFY(::GetTextExtentPoint32(hDC, str, strlen(str), &size));
#endif /* WINCE */

    VERIFY(::SelectObject(hDC, oldFont));
    VERIFY(::ReleaseDC(0, hDC));
    return size;
}

SIZE AwtFont::TextSize(AwtFont* font, int columns, int rows)
{
    HDC hDC = ::GetDC(0);
    ASSERT(hDC != NULL);
    HGDIOBJ oldFont = ::SelectObject(hDC, (font == NULL)
                                           ? ::GetStockObject(SYSTEM_FONT)
                                           : font->GetHFont());

    SIZE size;
    VERIFY(::GetTextExtentPoint(hDC, 
	       TEXT("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"),
				52, &size));

    VERIFY(::SelectObject(hDC, oldFont));
    VERIFY(::ReleaseDC(0, hDC));

    size.cx = size.cx * columns / 52;
    size.cy = size.cy * rows;
    return size;
}

int AwtFont::getFontDescriptorNumber(JNIEnv* env, jobject font, jobject fd)
{
    int i, num;
    jobject cfd;
    jobjectArray hao;

    jobject peer = env->GetObjectField(font, 
                                       WCachedIDs.java_awt_Font_peerFID);
    if (IsMultiFont(env, font)) {
        hao = GetComponentFonts(env, font);
        num = env->GetArrayLength(hao);
    } else {
        hao = NULL;
        num = 0;
    }
    
    for (i = 0; i < num; i++){
	// Trying to identify the same FontDescriptor by comparing the pointers.
	// This code makes the same effect to using java "equals" method.
        cfd = env->GetObjectArrayElement(hao, i);
	if (env->IsSameObject(cfd, fd)) {
	    return i;
	}
    }
    return 0;	// Not found.  Use default.
}


SIZE AwtFont::DrawStringSize_sub(jstring str, HDC hDC,
                                 jobject jhFont, long x, long y, BOOL draw)
{
    SIZE size, temp;
    size.cx = size.cy = 0;

    if ((str == NULL) || (jhFont == NULL))
	return size;

    JNIEnv *env;

    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return size;
    }

    if (env->GetStringLength(str) == 0)
        return size;
    
    jobjectArray jhAO;
    AwtFont* af;
    int segment_num;

    if (IsMultiFont(env, jhFont)) {
        jobject peer = env->GetObjectField(jhFont, WCachedIDs.java_awt_Font_peerFID);
        jhAO = (jobjectArray) env->CallObjectMethod(peer, WCachedIDs.sun_awt_PlatformFont_makeMultiCharsetStringMID, str); 
       
        segment_num = env->GetArrayLength(jhAO);
    } else {
        jhAO = NULL;
        segment_num = 0;
    } 
    af = AwtFont::GetFont(env, jhFont);
    HFONT oldFont = (HFONT) ::SelectObject(hDC, af->GetHFont());

    if (segment_num == 0) {
        int length = env->GetStringLength(str);
        WCHAR* string = TO_WSTRING(str);
        VERIFY(::SelectObject(hDC, af->GetHFont()));
#ifndef WINCE
        VERIFY(!draw || ::TextOutW(hDC, x, y, string, length));
        VERIFY(::GetTextExtentPoint32W(hDC, string, length, &size));
#else
	//WinCE does not have TextOut, but has ExtTextOut... 
	VERIFY(!draw ||
	       ::ExtTextOutW(hDC, x, y, 0, NULL,
			     (LPCWSTR)string, length, NULL));
        VERIFY(::GetTextExtentPoint32(hDC, (LPCWSTR)string, length, &size));
#endif
    } else {
        for (int i = 0; i < segment_num; i++) {
            jobject jhCS = env->GetObjectArrayElement(jhAO, i);
            jobject jhFD = env->GetObjectField(jhCS,
					       WCachedIDs.sun_awt_CharsetString_fontDescriptorFID); 
            int fdIndex = getFontDescriptorNumber(env, jhFont, jhFD);
	    int length = env->GetIntField(jhCS, WCachedIDs.sun_awt_CharsetString_lengthFID);
            VERIFY(::SelectObject(hDC, af->GetHFont(fdIndex)));

#ifndef WINCE
            if (af->NeedConverted(fdIndex)) {
		jobject jhCScharsetChars = env->GetObjectField(jhCS, WCachedIDs.sun_awt_CharsetString_charsetCharsFID);
		jint jhCSoffset = env->GetIntField(jhCS, WCachedIDs.sun_awt_CharsetString_offsetFID);

                int bytelen = length * sizeof(WCHAR);
                jbyteArray buffer = env->NewByteArray(bytelen);
                if (buffer == NULL)
                    env->ThrowNew(WCachedIDs.OutOfMemoryExceptionClass, "");
                
                jobject fontCharset = env->GetObjectField(jhFD, WCachedIDs.sun_awt_FontDescriptor_fontCharsetFID);
                jmethodID convertMID = env->GetMethodID(fontCharset, "convert", 
                                                        "([CII[BII)I");
                jint buflen = env->CallIntMethod(fontCharset,
						 convertMID,
						 jhCScharsetChars,
						 jhCSoffset,
						 jhCSoffset +
						 length, 
						 buffer, 
						 0, 
						 bytelen);

                ASSERT(!env->ExceptionCheck()); 
                VERIFY(!draw || ::TextOutA(hDC, x, y, unhand(buffer)->body, buflen));
                VERIFY(::GetTextExtentPoint32A(hDC, unhand(buffer)->body, buflen, &temp));
            } else {
#else //WINCE 
	    {
#endif
                jcharArray chars = (jcharArray)
		    env->GetObjectField(jhCS,
					WCachedIDs.sun_awt_CharsetString_charsetCharsFID);
		jsize charsLen = env->GetArrayLength(chars);
		jboolean isCopy;
		jchar *charsElems = env->GetCharArrayElements(chars,
							      &isCopy);
                int offset = (int) env->GetIntField(jhCS, WCachedIDs.sun_awt_CharsetString_offsetFID);
                WCHAR* string = new WCHAR[charsLen - offset + 1];

		for (int pos = offset; pos < charsLen; pos++) {
		    string[pos-offset] = (WCHAR) charsElems[pos];
		}
		string[charsLen - offset] = '\0';

#ifndef WINCE
                VERIFY(!draw || ::TextOutW(hDC, x, y, string, length));
                VERIFY(::GetTextExtentPoint32W(hDC, string, length, &temp));
#else // WINCE
		VERIFY(!draw || ::ExtTextOutW(hDC, x, y, 0, NULL, string, length, NULL));
                VERIFY(::GetTextExtentPoint32W(hDC, string, length, &temp));
#endif //WINCE 
		delete string;
		env->ReleaseCharArrayElements(chars, charsElems, 0);
            }

            x += temp.cx;
            size.cx += temp.cx;
            size.cy = (size.cy < temp.cy) ? temp.cy : size.cy;
        }
    }

    VERIFY(::SelectObject(hDC, oldFont)); 
    return size;
}


/////////////////////////////////////////////////////////////////////////////////////////////
// PPCFont native methods

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCFontPeer_init(JNIEnv *env, 
				       jobject thisObj, 
				       jobject font) 
{
    env->SetObjectField(font, WCachedIDs.java_awt_Font_peerFID, thisObj);
}

JNIEXPORT jobject JNICALL
Java_sun_awt_pocketpc_PPCFontPeer_getPeer(JNIEnv *env,
					  jclass cls,
					  jobject font) 
{
    return env->GetObjectField(font, WCachedIDs.java_awt_Font_peerFID);
}

/************************************************************************
 * PPCFontMetrics native methods
 */

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCFontMetrics_init(JNIEnv *env, jobject thisObj)
{
    AwtFont::LoadMetrics(env, thisObj);
}


JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCFontMetrics_initIDs (JNIEnv *env, jclass cls)
{
    WCachedIDs.PPCFontMetricsClass = (jclass) env->NewGlobalRef(cls);

    GET_METHOD_ID(PPCFontMetrics_getWidthsMID, "getWidths", "()[I");
    GET_METHOD_ID(PPCFontMetrics_getHeightMID, "getHeight", "()I");
    GET_STATIC_METHOD_ID(PPCFontMetrics_getFontMetricsMID, 
                         "getFontMetrics", 
                         "(Ljava/awt/Font;)Ljava/awt/FontMetrics;" );

    GET_FIELD_ID(PPCFontMetrics_widthsFID, "widths", "[I");
    GET_FIELD_ID(PPCFontMetrics_ascentFID, "ascent", "I");
    GET_FIELD_ID(PPCFontMetrics_descentFID, "descent", "I");
    GET_FIELD_ID(PPCFontMetrics_leadingFID, "leading", "I");
    GET_FIELD_ID(PPCFontMetrics_heightFID, "height", "I");
    GET_FIELD_ID(PPCFontMetrics_maxAscentFID, "maxAscent", "I");
    GET_FIELD_ID(PPCFontMetrics_maxDescentFID, "maxDescent", "I");
    GET_FIELD_ID(PPCFontMetrics_maxHeightFID, "maxHeight", "I");
    GET_FIELD_ID(PPCFontMetrics_maxAdvanceFID, "maxAdvance", "I");
}


JNIEXPORT jboolean JNICALL 
Java_sun_awt_pocketpc_PPCFontMetrics_needsConversion (JNIEnv * env, 
                                                      jclass self, 
                                                      jobject font, 
                                                      jobject des)
{
    AWT_TRACE((TEXT("%x: FontMetrics.needsConversion(%x, %x)\n"), self, font, des));
    CHECK_NULL_RETURN(font, "null font");
    CHECK_NULL_RETURN(des, "null FontDescriptor");

#ifndef WINCE
    AwtFont* af = AwtFont::GetFont(env, font);
    ASSERT(af != NULL);
    int fdIndex = AwtFont::getFontDescriptorNumber(env, font, des);
    return af->NeedConverted(fdIndex) ? 1 : 0;
#else /*WINCE */
	return 0;
#endif

}

JNIEXPORT jint JNICALL 
Java_sun_awt_pocketpc_PPCFontMetrics_bytesWidth (JNIEnv * env, 
                                                 jobject thisObj, 
                                                 jbyteArray str, 
                                                 jint off, jint len)
{
    if (str == NULL) {
        env->ThrowNew(WCachedIDs.NullPointerExceptionClass, "null bytes");
        return 0;
    }
    int strLen = (int)env->GetArrayLength(str);
    
    if ((len < 0) || (off < 0) || (len+off > strLen) || 
        (len > strLen) || (off > strLen)) {    
        env->ThrowNew(WCachedIDs.ArrayIndexOutOfBoundsExceptionClass,
		      "off or len argument");
	return 0;
    }
    jintArray array = (jintArray)env->CallObjectMethod(thisObj,
						       WCachedIDs.PPCFontMetrics_getWidthsMID);

    jint result = 0;

    char* pStrBody = (char *)env->GetPrimitiveArrayCritical(str, 0);
    char *pStr = pStrBody + off;

    jint* widths = (jint *)env->GetPrimitiveArrayCritical(array, 0);
    
    for (; len; len--) {
	result += widths[*pStr++];
    }

    env->ReleasePrimitiveArrayCritical(array, widths, 0);
    env->ReleasePrimitiveArrayCritical(str, pStrBody, 0);
    return result;
}

JNIEXPORT jint JNICALL
Java_sun_awt_pocketpc_PPCFontMetrics_getMFCharSegmentWidth(JNIEnv* env, 
                                                           jobject self, 
                                                           jobject font,
                                                           jobject des, 
                                                           jboolean converted,
                                                           jcharArray chars, 
                                                           jint offset, 
                                                           jint charLength, 
                                                           jbyteArray bytes, 
                                                           jint length)
{
    AWT_TRACE((TEXT("%x: WFontMetrics.getMFCharSegmentWidth(%x, %x, %d, %x, %d, %d, %x, %d)\n"), 
	       self, font, des, 
	       converted, chars, offset, charLength, bytes, length));
    CHECK_NULL_RETURN(font, "null font");
    CHECK_NULL_RETURN(des, "null FontDescriptor");
    SIZE size;

    HDC hDC = ::GetDC(0);
    ASSERT(hDC != NULL);

    // Select this char segment's Win32 font.
    AwtFont* af = AwtFont::GetFont(env, font);
    ASSERT(af != NULL);
    int fdIndex = AwtFont::getFontDescriptorNumber(env, font, des);
    HGDIOBJ hFont = af->GetHFont(fdIndex);

    // Get segment size.
    HGDIOBJ oldFont = ::SelectObject(hDC, hFont);
#ifndef WINCE
    if (converted) {
        CHECK_NULL_RETURN(bytes, "null bytes");
        VERIFY(::GetTextExtentPoint32A(hDC, unhand(bytes)->body, length, &size));
    } else {
        CHECK_NULL_RETURN(chars, "null chars");
        VERIFY(::GetTextExtentPoint32W(hDC, &unhand(chars)->body[offset],
                                       charLength, &size));
    }
#else /* WINCE */
    WCHAR *carray = (WCHAR*) env->GetCharArrayElements(chars, NULL);
    WCHAR *offset_carray= carray+offset;
    
    /* Compress the array if it has invalid character (char value 0-31)*/
    int j = 0;
    for (int i=0; i<charLength; i++) {
        if (offset_carray[i] >= 32) 
            offset_carray[j++] = offset_carray[i];
        else
            if (charLength == 1)
                charLength = 0 ;            
    }
    
    CHECK_NULL_RETURN(chars, "null chars");
    VERIFY(::GetTextExtentPoint32W(hDC, offset_carray,
                                       charLength, &size));
    //KEEP_POINTER_ALIVE(carray);
#endif
    ::SelectObject(hDC, oldFont);

    VERIFY(::ReleaseDC(0, hDC));
    env->ReleaseCharArrayElements(chars, (jchar*)carray, 0);
    return size.cx;
}

/////////////////////////////////////////////////////////////////////////////
// FontCache methods

void AwtFontCache::Add(WCHAR* name, HFONT font)
{
    fontCache.m_head = new Item(name, font, fontCache.m_head);
}

HFONT AwtFontCache::Lookup(WCHAR* name)
{
    Item* item = fontCache.m_head;
    Item* lastItem = NULL;

    while (item != NULL) {
	if (wcscmp(item->name, name) == 0) {
	    return item->font;
	}
	lastItem = item;
	item = item->next;
    }
    return NULL;
}

BOOL AwtFontCache::Search(HFONT font)
{
    Item* item = fontCache.m_head;
    Item* lastItem = NULL;

    while (item != NULL) {
	if (item->font == font) {
            return TRUE;
	}
	lastItem = item;
	item = item->next;
    }
    return FALSE;
}

void AwtFontCache::Remove(HFONT font)
{
    Item* item = fontCache.m_head;
    Item* lastItem = NULL;

    while (item != NULL) {
	if (item->font == font) {
	    if (lastItem == NULL) {
		fontCache.m_head = item->next;
	    } else {
		lastItem->next = item->next;
	    }
	    delete item;
	    return;
	}
	lastItem = item;
	item = item->next;
    }
}

void AwtFontCache::Clear()
{
    Item* item = m_head;
    Item* next;

    while (item != NULL) {
	next = item->next;
	delete item;
	item = next;
    }

    m_head = NULL;
}

AwtFontCache::Item::Item(const WCHAR* s, HFONT f, AwtFontCache::Item* n)
{
    name = wcsdup(s);
    font = f;
    next = n;
}

AwtFontCache::Item::~Item() {
    free(name);
}


#ifndef WINCE
/////////////////////////////////////////////////////////////////////////////
// for canConvert native method of PPCDefaultFontCharset

class CSegTableComponent
{
public:
    CSegTableComponent();
    ~CSegTableComponent();
    virtual void Create(LPWSTR name);
    virtual BOOL In(USHORT iChar) { ASSERT(FALSE); return FALSE; };
    LPWSTR GetFontName(){
	ASSERT(m_lpszFontName != NULL); return m_lpszFontName;
    };

private:
    LPWSTR m_lpszFontName;
};

CSegTableComponent::CSegTableComponent()
{
    m_lpszFontName = NULL;
}

CSegTableComponent::~CSegTableComponent()
{
    if (m_lpszFontName != NULL)
	free(m_lpszFontName);
}

void CSegTableComponent::Create(LPWSTR name)
{
    m_lpszFontName = wcsdup(name);
    ASSERT(m_lpszFontName);
}

#define CMAPHEX 0x70616d63 // = "cmap" (reversed)

// CSegTable: Segment table describing character coverage for a font
class CSegTable : public CSegTableComponent
{
public:
    CSegTable();
    ~CSegTable();
    void Create(LPWSTR name);
    BOOL In(USHORT iChar);
    BOOL HasCmap();
    virtual BOOL IsEUDC() { ASSERT(FALSE); return FALSE; };

protected:
    virtual void GetData(DWORD dwOffset, LPVOID lpData, DWORD cbData) {
	ASSERT(FALSE); };
    void MakeTable();
    static void SwapShort(USHORT& p);
    static void SwapULong(ULONG& p);

private:
    USHORT m_cSegCount;	    // number of segments
    PUSHORT m_piStart;	    // pointer to array of starting values
    PUSHORT m_piEnd;	    // pointer to array of ending values (inclusive)
    USHORT m_cSeg;	    // current segment (cache)
};

CSegTable::CSegTable()
{
    m_cSegCount = 0;
    m_piStart = NULL;
    m_piEnd = NULL;
    m_cSeg = 0;
}

CSegTable::~CSegTable()
{
    if (m_piStart != NULL)
	delete[] m_piStart;
    if (m_piEnd != NULL)
	delete[] m_piEnd;
}

#define OFFSETERROR 0

void CSegTable::MakeTable()
{
typedef struct tagTABLE{
    USHORT  platformID;
    USHORT  encodingID;
    ULONG   offset;
} TABLE, *PTABLE;

typedef struct tagSUBTABLE{
    USHORT  format;
    USHORT  length;
    USHORT  version;
    USHORT  segCountX2;
    USHORT  searchRange;
    USHORT  entrySelector;
    USHORT  rangeShift;
} SUBTABLE, *PSUBTABLE;

    USHORT aShort[2];
    (void) GetData(0, aShort, sizeof(aShort));
    USHORT nTables = aShort[1];
    SwapShort(nTables);

    // allocate buffer to hold encoding tables
    DWORD cbData = nTables * sizeof(TABLE);
    PTABLE pTables = new TABLE[nTables];
    PTABLE pTable = pTables;

    // get array of encoding tables.
    (void) GetData(4, (PBYTE) pTable, cbData);

    ULONG offsetFormat4 = OFFSETERROR;
    USHORT i;
    for (i = 0; i < nTables; i++) {
	SwapShort(pTable->encodingID);
	SwapShort(pTable->platformID);
	//for a Unicode font for Windows, platformID == 3, encodingID == 1
	if ((pTable->platformID == 3)&&(pTable->encodingID == 1)) {
	    offsetFormat4 = pTable->offset;
	    SwapULong(offsetFormat4);
	    break;
	}
	pTable++;
    }
    delete[] pTables;
    ASSERT(offsetFormat4 != OFFSETERROR);

    SUBTABLE subTable;
    (void) GetData(offsetFormat4, &subTable, sizeof(SUBTABLE));
    SwapShort(subTable.format);
    SwapShort(subTable.segCountX2);
    ASSERT(subTable.format == 4);

    m_cSegCount = subTable.segCountX2/2;

    // read in the array of segment end values
    m_piEnd = new USHORT[m_cSegCount];
    ASSERT(m_piEnd);
    ULONG offset = offsetFormat4
	+ sizeof(SUBTABLE); //skip constant # bytes in subtable
    cbData = m_cSegCount * sizeof(USHORT);
    (void) GetData(offset, m_piEnd, cbData);
    for (i = 0; i < m_cSegCount; i++)
	SwapShort(m_piEnd[i]);
    ASSERT(m_piEnd[m_cSegCount-1] == 0xffff);

    // read in the array of segment start values
    m_piStart = new USHORT[m_cSegCount];
    ASSERT(m_piStart);
    offset += cbData	    //skip SegEnd array
	+ sizeof(USHORT);   //skip reservedPad
    (void) GetData(offset, m_piStart, cbData);
    for (i = 0; i < m_cSegCount; i++)
	SwapShort(m_piStart[i]);
    ASSERT(m_piStart[m_cSegCount-1] == 0xffff);
}

void CSegTable::Create(LPWSTR name)
{
    CSegTableComponent::Create(name);
}

BOOL CSegTable::In(USHORT iChar)
{
    ASSERT(m_piStart);
    ASSERT(m_piEnd);

    if (iChar > m_piEnd[m_cSeg]) {
	for (; (m_cSeg < m_cSegCount)&&(iChar > m_piEnd[m_cSeg]); m_cSeg++);
    } else if (iChar < m_piStart[m_cSeg]) {
	for (; (m_cSeg > 0)&&(iChar < m_piStart[m_cSeg]); m_cSeg--);
    }

    if ((iChar <= m_piEnd[m_cSeg])&&(iChar >= m_piStart[m_cSeg])&&(iChar != 0xffff))
	return TRUE;
    else
	return FALSE;
}

inline BOOL CSegTable::HasCmap()
{
    return (((m_piEnd)&&(m_piStart)) ? TRUE : FALSE);
}

inline void CSegTable::SwapShort(USHORT& p)
{
    SHORT temp;

    temp = (SHORT)(HIBYTE(p) + (LOBYTE(p) << 8));
    p = temp;
}

inline void CSegTable::SwapULong(ULONG& p)
{
    ULONG temp;

    temp = (LONG) ((BYTE) p);
    temp <<= 8;
    p >>= 8;

    temp += (LONG) ((BYTE) p);
    temp <<= 8;
    p >>= 8;

    temp += (LONG) ((BYTE) p);
    temp <<= 8;
    p >>= 8;

    temp += (LONG) ((BYTE) p);
    p = temp;
}

class CStdSegTable : public CSegTable
{
public:
    CStdSegTable();
    ~CStdSegTable();
    BOOL IsEUDC() { return FALSE; };
    void Create(LPWSTR name);

protected:
    void GetData(DWORD dwOffset, LPVOID lpData, DWORD cbData);

private:
    HDC m_hTmpDC;
};

CStdSegTable::CStdSegTable()
{
    m_hTmpDC = NULL;
}

CStdSegTable::~CStdSegTable()
{
    ASSERT(m_hTmpDC == NULL);
}

inline void CStdSegTable::GetData(DWORD dwOffset,
				  LPVOID lpData, DWORD cbData)
{
#ifndef WINCE /* TODO */
    ASSERT(m_hTmpDC);
    DWORD nBytes =
	::GetFontData(m_hTmpDC, CMAPHEX, dwOffset, lpData, cbData);
    ASSERT(nBytes != GDI_ERROR);
#endif
}

void CStdSegTable::Create(LPWSTR name)
{
#ifndef WINCE
    CSegTable::Create(name);

    HWND hWnd = ::GetDesktopWindow();
    ASSERT(hWnd);
    m_hTmpDC = ::GetWindowDC(hWnd);
    ASSERT(m_hTmpDC);

    HFONT hFont = CreateHFont_sub(name, 0, 20);

    HFONT hOldFont = (HFONT)::SelectObject(m_hTmpDC, hFont);
    ASSERT(hOldFont);

    (void) MakeTable();

    VERIFY(::SelectObject(m_hTmpDC, hOldFont));
    VERIFY(::DeleteObject(hFont));
    VERIFY(::ReleaseDC(hWnd, m_hTmpDC) != 0);
    m_hTmpDC = NULL;
#endif
}

class CEUDCSegTable : public CSegTable
{
public:
    CEUDCSegTable();
    ~CEUDCSegTable();
    BOOL IsEUDC() { return TRUE; };
    void Create(LPWSTR name);

protected:
    void GetData(DWORD dwOffset, LPVOID lpData, DWORD cbData);

private:
    HANDLE m_hTmpFile;
    ULONG m_hTmpCMapOffset;
};

CEUDCSegTable::CEUDCSegTable()
{
    m_hTmpFile = NULL;
    m_hTmpCMapOffset = 0;
}

CEUDCSegTable::~CEUDCSegTable()
{
    ASSERT(m_hTmpFile == NULL);
    ASSERT(m_hTmpCMapOffset == 0);
}

inline void CEUDCSegTable::GetData(DWORD dwOffset,
				   LPVOID lpData, DWORD cbData)
{
#ifndef WINCE
    ASSERT(m_hTmpFile);
    ASSERT(m_hTmpCMapOffset);
    ::SetFilePointer(m_hTmpFile, m_hTmpCMapOffset + dwOffset,
	NULL, FILE_BEGIN);
    DWORD dwRead;
    VERIFY(::ReadFile(m_hTmpFile, lpData, cbData, &dwRead, NULL));
    ASSERT(dwRead == cbData);
#endif
}

void CEUDCSegTable::Create(LPWSTR name)
{
typedef struct tagHEAD{
    FIXED   sfnt_version;
    USHORT  numTables;
    USHORT  searchRange;
    USHORT  entrySelector;
    USHORT  rangeShift;
} HEAD, *PHEAD;

typedef struct tagENTRY{
    ULONG   tag;
    ULONG   checkSum;
    ULONG   offset;
    ULONG   length;
} ENTRY, *PENTRY;

    CSegTable::Create(name);

    // create EUDC font file and make EUDCSegTable
    // after wrapper function for CreateFileW, we use only CreateFileW
    if (IS_NT) {
	m_hTmpFile = ::CreateFileW(name, GENERIC_READ,
	    FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    } else {
#ifdef WIN32
	ASSERT(IS_WIN95);
#endif
	char szFileName[_MAX_PATH];
	::WideCharToMultiByte(CP_ACP, 0, name, -1,
	    szFileName, sizeof(szFileName), NULL, NULL);
	m_hTmpFile = ::CreateFileA(szFileName, GENERIC_READ,
	    FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    }
    if (m_hTmpFile == INVALID_HANDLE_VALUE){
	m_hTmpFile = NULL;
	return;
    }

    HEAD head;
    DWORD dwRead;
    VERIFY(::ReadFile(m_hTmpFile, &head, sizeof(head), &dwRead, NULL));
    ASSERT(dwRead == sizeof(HEAD));
    SwapShort(head.numTables);
    ENTRY entry;
    for (int i = 0; i < head.numTables; i++){
	VERIFY(::ReadFile(m_hTmpFile, &entry, sizeof(entry), &dwRead, NULL));
	ASSERT(dwRead == sizeof(ENTRY));
	if (entry.tag == CMAPHEX)
	    break;
    }
    ASSERT(entry.tag == CMAPHEX);
    SwapULong(entry.offset);
    m_hTmpCMapOffset = entry.offset;

    (void) MakeTable();

    m_hTmpCMapOffset = 0;
    VERIFY(::CloseHandle(m_hTmpFile));
    m_hTmpFile = NULL;
}

class CSegTableManagerComponent
{
public:
    CSegTableManagerComponent();
    ~CSegTableManagerComponent();

protected:
    void MakeBiggerTable();
    CSegTableComponent **m_tables;
    int m_nTable;
    int m_nMaxTable;
};

#define TABLENUM 20

CSegTableManagerComponent::CSegTableManagerComponent()
{
    m_nTable = 0;
    m_nMaxTable = TABLENUM;
    m_tables = new CSegTableComponent*[m_nMaxTable];
}

CSegTableManagerComponent::~CSegTableManagerComponent()
{
    for (int i = 0; i < m_nTable; i++) {
	ASSERT(m_tables[i]);
	delete m_tables[i];
    }
}

void CSegTableManagerComponent::MakeBiggerTable()
{
    CSegTableComponent **tables =
	new CSegTableComponent*[m_nMaxTable + TABLENUM];
    ASSERT(tables);

    for (int i = 0; i < m_nMaxTable; i++)
	tables[i] = m_tables[i];

    delete[] m_tables;

    m_tables = tables;
    m_nMaxTable += TABLENUM;
}

class CSegTableManager : public CSegTableManagerComponent
{
public:
    CSegTable* GetTable(LPWSTR lpszFontName, BOOL fEUDC);
};

CSegTable* CSegTableManager::GetTable(LPWSTR lpszFontName, BOOL fEUDC)
{
    for (int i = 0; i < m_nTable; i++) {
	if ((((CSegTable*)m_tables[i])->IsEUDC() == fEUDC) &&
	    (wcscmp(m_tables[i]->GetFontName(),lpszFontName) == 0))
	    return (CSegTable*) m_tables[i];
    }

    if (m_nTable == m_nMaxTable) {
	(void) MakeBiggerTable();
    }
    ASSERT(m_nTable < m_nMaxTable);

    if (!fEUDC)
	m_tables[m_nTable] = new CStdSegTable;
    else
	m_tables[m_nTable] = new CEUDCSegTable;
    m_tables[m_nTable]->Create(lpszFontName);
    return (CSegTable*) m_tables[m_nTable++];
}

CSegTableManager g_segTableManager;

class CCombinedSegTable : public CSegTableComponent
{
public:
    CCombinedSegTable();
    void Create(LPWSTR name);
    BOOL In(USHORT iChar);

private:
    LPSTR GetCodePageSubkey();
    void GetEUDCFileName(LPWSTR lpszFileName, int cchFileName);
    static char m_szCodePageSubkey[16];
    static WCHAR m_szDefaultEUDCFile[_MAX_PATH];
    static BOOL m_fEUDCSubKeyExist;
    static BOOL m_fTTEUDCFileExist;
    CStdSegTable* m_pStdSegTable;
    CEUDCSegTable* m_pEUDCSegTable;
};

char CCombinedSegTable::m_szCodePageSubkey[16] = "";

WCHAR CCombinedSegTable::m_szDefaultEUDCFile[_MAX_PATH] = L"";

BOOL CCombinedSegTable::m_fEUDCSubKeyExist = TRUE;

BOOL CCombinedSegTable::m_fTTEUDCFileExist = TRUE;

CCombinedSegTable::CCombinedSegTable()
{
    m_pStdSegTable = NULL;
    m_pEUDCSegTable = NULL;
}

#include <locale.h>
LPSTR CCombinedSegTable::GetCodePageSubkey()
{
#ifndef WINCE
    if (strlen(m_szCodePageSubkey) > 0)
	return m_szCodePageSubkey;

    LPSTR lpszLocale = setlocale(LC_CTYPE, "");
    // cf lpszLocale = "Japanese_Japan.932"
    ASSERT(lpszLocale);
    LPSTR lpszCP = strchr(lpszLocale, (int) '.');
    if (lpszCP == NULL)
	return NULL;
    lpszCP++; // cf lpszCP = "932"
    char szSubKey[80];
    strcpy(szSubKey, "EUDC\\");
    strcpy(&(szSubKey[strlen(szSubKey)]), lpszCP);
    strcpy(m_szCodePageSubkey, szSubKey);
#else /* WINCE */
    /* TODO */
#endif /* WINCE */
    return m_szCodePageSubkey;
}

void CCombinedSegTable::GetEUDCFileName(LPWSTR lpszFileName, int cchFileName)
{
    if (m_fEUDCSubKeyExist == FALSE)
	return;

    // get filename of typeface-specific TureType EUDC font
    LPSTR lpszSubKey = GetCodePageSubkey();
    if (lpszSubKey == NULL) {
	m_fEUDCSubKeyExist = FALSE;
	return; // can not get codepage information
    }
    HKEY hRootKey = HKEY_CURRENT_USER;
    HKEY hKey;
    LONG lRet = ::RegOpenKeyExA(hRootKey, lpszSubKey, 0, KEY_ALL_ACCESS, &hKey);
    if (lRet != ERROR_SUCCESS) {
	m_fEUDCSubKeyExist = FALSE;
	return; // no EUDC font
    }

    // get EUDC font file name
    WCHAR szFamilyName[80];
    wcscpy(szFamilyName, GetFontName());
    WCHAR* delimit = wcschr(szFamilyName, L',');
    if (delimit != NULL)
	*delimit = L'\0';
    DWORD dwType;
    UCHAR szFileName[_MAX_PATH];
#ifndef WINCE
    ::ZeroMemory(szFileName, sizeof(szFileName));
#else
    memset(szFileName, 0, sizeof(szFileName));
#endif
    DWORD dwBytes = sizeof(szFileName);
    // try Typeface-specific EUDC font
    char szTmpName[80];
    VERIFY(::WideCharToMultiByte(CP_ACP, 0, szFamilyName, -1,
	szTmpName, sizeof(szTmpName), NULL, NULL));
    LONG lStatus = ::RegQueryValueExA(hKey, (LPCSTR) szTmpName,
	NULL, &dwType, szFileName, &dwBytes);
    BOOL fUseDefault = FALSE;
    if (lStatus != ERROR_SUCCESS){ // try System default EUDC font
	if (m_fTTEUDCFileExist == FALSE)
	    return;
	if (wcslen(m_szDefaultEUDCFile) > 0) {
	    wcscpy(lpszFileName, m_szDefaultEUDCFile);
	    return;
	}
	char szDefault[] = "SystemDefaultEUDCFont";
	lStatus = ::RegQueryValueExA(hKey, (LPCSTR) szDefault,
	    NULL, &dwType, szFileName, &dwBytes);
	fUseDefault = TRUE;
	if (lStatus != ERROR_SUCCESS) {
	    m_fTTEUDCFileExist = FALSE;
	    // This font is associated with no EUDC font
	    // and there is no system default EUDC font
	    return;
	}
    }

    if (strcmp((LPCSTR) szFileName, "userfont.fon") == 0) {
	// This font is associated with no EUDC font
	// and the system default EUDC font is not TrueType
	m_fTTEUDCFileExist = FALSE;
	return;
    }

    ASSERT(strlen((LPCSTR)szFileName) > 0);
    VERIFY(::MultiByteToWideChar(CP_ACP, 0,
	(LPCSTR)szFileName, -1,	lpszFileName, cchFileName) != 0);
    if (fUseDefault)
	wcscpy(m_szDefaultEUDCFile, lpszFileName);
}

void CCombinedSegTable::Create(LPWSTR name)
{
    CSegTableComponent::Create(name);

    m_pStdSegTable =
	(CStdSegTable*) g_segTableManager.GetTable(name, FALSE/*not EUDC*/);
    WCHAR szEUDCFileName[_MAX_PATH];
#ifndef WINCE
    ::ZeroMemory(szEUDCFileName, sizeof(szEUDCFileName));
#else
    memset(szEUDCFileName, 0, sizeof(szEUDCFileName));
#endif
    (void) GetEUDCFileName(szEUDCFileName,
	sizeof(szEUDCFileName)/sizeof(WCHAR));
    if (wcslen(szEUDCFileName) > 0) {
	m_pEUDCSegTable = (CEUDCSegTable*) g_segTableManager.GetTable(
	    szEUDCFileName, TRUE/*EUDC*/);
	if (m_pEUDCSegTable->HasCmap() == FALSE)
	    m_pEUDCSegTable = NULL;
    }
}

BOOL CCombinedSegTable::In(USHORT iChar)
{
    ASSERT(m_pStdSegTable);
    if (m_pStdSegTable->In(iChar))
	return TRUE;

    if (m_pEUDCSegTable != NULL)
	return m_pEUDCSegTable->In(iChar);

    return FALSE;
}

class CCombinedSegTableManager : public CSegTableManagerComponent
{
public:
    CCombinedSegTable* GetTable(LPWSTR lpszFontName);
};

CCombinedSegTable* CCombinedSegTableManager::GetTable(LPWSTR lpszFontName)
{
    for (int i = 0; i < m_nTable; i++) {
	if (wcscmp(m_tables[i]->GetFontName(),lpszFontName) == 0)
	    return (CCombinedSegTable*) m_tables[i];
    }

    if (m_nTable == m_nMaxTable) {
	(void) MakeBiggerTable();
    }
    ASSERT(m_nTable < m_nMaxTable);

    m_tables[m_nTable] = new CCombinedSegTable;
    m_tables[m_nTable]->Create(lpszFontName);

    return (CCombinedSegTable*) m_tables[m_nTable++];
}
#endif /* ! WINCE */

JNIEXPORT jboolean JNICALL 
Java_sun_awt_pocketpc_PPCDefaultFontCharset_canConvert (JNIEnv* env, 
                                                        jobject thisObj,
                                                        jchar ch)
{
#ifndef WINCE
    static CCombinedSegTableManager tableManager;

    CCombinedSegTable* pTable =
            tableManager.GetTable(TO_WSTRING(unhand(self)->fontName));

    return (pTable->In((USHORT) ch) ? 1 : 0);
#else
    return 1;
#endif
}

