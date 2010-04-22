/*
 * @(#)QtFontPeer.cc	1.23 03/01/16
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
#include <qt.h>

#include "jni.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "java_awt_Font.h"
#include "sun_awt_qt_QtFontPeer.h"
#include "awt.h"
#include "QpFontManager.h"

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtFontPeer_initIDs (JNIEnv *env, jclass cls)
{
    cls = env->FindClass ("java/awt/FontMetrics");
    
    if (cls == NULL)
        return;
    GET_FIELD_ID (java_awt_FontMetrics_fontFID, "font", "Ljava/awt/Font;");
    
    cls = env->FindClass ("java/awt/Font");
    if (cls == NULL)
        return;
       
    QtCachedIDs.java_awt_FontClass = (jclass) env->NewGlobalRef (cls);
    GET_METHOD_ID (java_awt_Font_contructorWithPeerMID, "<init>", 
                   "(Ljava/lang/String;IILsun/awt/peer/FontPeer;)V");

    cls = env->FindClass ("sun/awt/qt/QtFontPeer");
    if (cls == NULL)
	return;
    QtCachedIDs.QtFontPeerClass = (jclass) env->NewGlobalRef (cls);
    GET_FIELD_ID (QtFontPeer_ascentFID, "ascent", "I");
    GET_FIELD_ID (QtFontPeer_descentFID, "descent", "I");
    GET_FIELD_ID (QtFontPeer_leadingFID, "leading", "I");
    GET_FIELD_ID (QtFontPeer_dataFID, "data", "I");
    GET_STATIC_METHOD_ID (QtFontPeer_getFontMID, "getFont",
			  "(I)Ljava/awt/Font;");
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtFontPeer_init (JNIEnv *env, jobject thisObj, 
                                 jstring family, jboolean italic,
                                 jboolean bold, jint size)
{
    const char* familyName = (const char*) env->GetStringUTFChars(family, 
                                                                  NULL);
    QFont *qtfont;
    QString familyString(familyName);
    int ascent;
    int descent;
    int leading;

    QpFontManager *fmgr = QpFontManager::instance();
    qtfont = fmgr->createFont(familyString, 
                         (int)size,
                         (int)((bold == JNI_TRUE)?QFont::Bold:QFont::Normal),
                              italic,
                              &ascent,
                              &descent,
                              &leading);
    if (qtfont == NULL) {
        env->ThrowNew(QtCachedIDs.java_awt_AWTErrorClass, 
                      "Could not load default font");
    } else {
        env->SetIntField (thisObj, QtCachedIDs.QtFontPeer_dataFID, 
                          (jint)qtfont);
        env->SetIntField (thisObj, QtCachedIDs.QtFontPeer_ascentFID, 
                          (jint)ascent);
        env->SetIntField (thisObj, QtCachedIDs.QtFontPeer_descentFID, 
                          (jint)descent);
        env->SetIntField (thisObj, QtCachedIDs.QtFontPeer_leadingFID, 
                          (jint)leading);
    }
    env->ReleaseStringUTFChars(family, familyName);
}

JNIEXPORT jboolean JNICALL
Java_sun_awt_qt_QtFontPeer_areFontsTheSame (JNIEnv *env, jclass cls, 
                                            jint font1, jint font2)
{
    QpFontManager *fmgr = QpFontManager::instance();
    return (jboolean)fmgr->isEquals((QFont *)font1, (QFont *)font2);
}

JNIEXPORT jobject JNICALL
Java_sun_awt_qt_QtFontPeer_createFont (JNIEnv *env, jclass cls, jstring name, 
                                       jobject peer)
{
    jobject font;
    /* TODO: Set these appropriately. */
    jint style = java_awt_Font_PLAIN, size = 14;  

    QFont *qtfont = (QFont *)
        env->GetIntField (peer, QtCachedIDs.QtFontPeer_dataFID);
    
    if (qtfont == NULL) {
        env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
        return NULL;
    }
    
    font = env->NewObject (QtCachedIDs.java_awt_FontClass,
                           QtCachedIDs.java_awt_Font_contructorWithPeerMID,
                           name,
                           style,
                           size,
                           peer);
    if (font == NULL)
        return NULL;

    int ascent, descent, leading;
    QpFontManager *fmgr = QpFontManager::instance();
    fmgr->getMetrics(qtfont, &ascent, &descent, &leading);
    
    QFontMetrics fm(*qtfont);
    env->SetIntField (peer, QtCachedIDs.QtFontPeer_ascentFID, (jint)ascent);
    env->SetIntField (peer, QtCachedIDs.QtFontPeer_descentFID, (jint)descent);
    env->SetIntField (peer, QtCachedIDs.QtFontPeer_leadingFID, (jint)leading);
    env->SetObjectField (peer, 
                         QtCachedIDs.java_awt_FontMetrics_fontFID, font);
                         
    return font;
}

//6228133
JNIEXPORT void JNICALL
Java_sun_awt_qt_QtFontPeer_deleteFonts (JNIEnv *env, jclass cls, jint len, jintArray fonts)
{
    jint * jfontArray;
    jint i;

    jfontArray = env->GetIntArrayElements(fonts, JNI_FALSE);
    for (i = 0; i < len; i++) {
        delete (QFont *)jfontArray[i];
    }
    env->ReleaseIntArrayElements(fonts, jfontArray, 0);
}

JNIEXPORT jint JNICALL
Java_sun_awt_qt_QtFontPeer_asciiCharWidth (JNIEnv* env, jobject thisObj, 
                                           jchar c)
{
    jobject font;
    QFont* qtfont;
    jint width;
	
    font = env->GetObjectField (thisObj, 
                                QtCachedIDs.java_awt_FontMetrics_fontFID);
    
    if (font == NULL) {
        env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
        return 0;
    }
    
    qtfont = (QFont* )env->GetIntField (thisObj, 
                                        QtCachedIDs.QtFontPeer_dataFID);
    if (qtfont == NULL) {
        env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
        return 0;
    }

    QpFontManager *fmgr = QpFontManager::instance();
    width = fmgr->charWidth(qtfont, (char)c);

    return width;
}

JNIEXPORT jint JNICALL
Java_sun_awt_qt_QtFontPeer_stringWidthNative (JNIEnv* env, jobject thisObj, 
                                              jstring srcstring)
{
    QString* qtstring;
    jint qtstringLen;
    jobject font;
    QFont* qtfont;
    jint width;
    
    if (srcstring == NULL) {
        return 0;
    }
    font = env->GetObjectField (thisObj, 
                                QtCachedIDs.java_awt_FontMetrics_fontFID);
    
    if (font == NULL) {
        env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, "Null font");
        return 0;
    }
    
    qtfont = (QFont* )env->GetIntField (thisObj, 
                                        QtCachedIDs.QtFontPeer_dataFID);
    
    if (qtfont == NULL) {
        env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, 
		       "Null Qt font");
        return 0;
    }

    qtstring = awt_convertToQString(env, srcstring);
    if (qtstring == NULL) 
        return 0;
    qtstringLen = (jint) env->GetStringLength(srcstring);
    QpFontManager *fmgr = QpFontManager::instance();
    width = fmgr->stringWidth(qtfont, qtstring, qtstringLen);
    delete qtstring;
    return width; 
}
